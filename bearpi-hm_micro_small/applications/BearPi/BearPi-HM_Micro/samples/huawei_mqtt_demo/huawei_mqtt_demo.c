#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "hdf_io_service_if.h"
#include "hdf_sbuf.h"

extern struct hostent *gethostbyname(const char *name);

#define MQTT_HOST "bb4de50857.iotda-device.cn-south-4.myhuaweicloud.com"
#define MQTT_PORT 1883

#define DEVICE_ID "6a2fab136b6c4d5f8d63cf9e_bearpi_sc1_001"
#define MQTT_CLIENT_ID "6a2fab136b6c4d5f8d63cf9e_bearpi_sc1_001_0_0_2026061508"
#define MQTT_USERNAME DEVICE_ID
#define MQTT_PASSWORD ""

#define SERVICE_ID "LightService"
#define E53_SC1_SERVICE "hdf_e53_sc1"

#define MQTT_KEEPALIVE_SECONDS 60
#define MQTT_BUFFER_SIZE 1024
#define E53_REPLY_SIZE 256
#define REPORT_INTERVAL_SECONDS 10

enum {
    E53_SC1_CMD_START = 0,
    E53_SC1_CMD_STOP,
    E53_SC1_CMD_READ,
    E53_SC1_CMD_SET_LIGHT,
    E53_SC1_CMD_SET_BRIGHTNESS,
};

typedef struct {
    float lux;
    int ledOn;
    int brightness;
} LampData;

static int g_packetId = 1;
static int g_ledOn = 0;
static int g_brightness = 0;
static const char *g_mqttPassword = MQTT_PASSWORD;

static int E53Sc1SetBrightness(int brightness);

static int WriteAll(int fd, const uint8_t *buf, size_t len)
{
    size_t sent = 0;
    while (sent < len) {
        int ret = send(fd, buf + sent, len - sent, 0);
        if (ret <= 0) {
            printf("[MQTT] send failed, errno=%d\n", errno);
            return -1;
        }
        sent += (size_t)ret;
    }
    return 0;
}

static int ReadAll(int fd, uint8_t *buf, size_t len)
{
    size_t received = 0;
    while (received < len) {
        int ret = recv(fd, buf + received, len - received, 0);
        if (ret <= 0) {
            return -1;
        }
        received += (size_t)ret;
    }
    return 0;
}

static int AppendString(uint8_t *buf, size_t bufSize, size_t *offset, const char *str)
{
    size_t len = strlen(str);
    if (len > 0xFFFF || *offset + 2 + len > bufSize) {
        return -1;
    }
    buf[(*offset)++] = (uint8_t)(len >> 8);
    buf[(*offset)++] = (uint8_t)(len & 0xFF);
    memcpy(buf + *offset, str, len);
    *offset += len;
    return 0;
}

static int EncodeRemainingLength(uint8_t *buf, size_t bufSize, size_t *offset, size_t len)
{
    do {
        if (*offset >= bufSize) {
            return -1;
        }
        uint8_t encoded = (uint8_t)(len % 128);
        len /= 128;
        if (len > 0) {
            encoded |= 0x80;
        }
        buf[(*offset)++] = encoded;
    } while (len > 0);
    return 0;
}

static int ReadRemainingLength(int fd, size_t *remainingLength)
{
    uint8_t encoded = 0;
    int multiplier = 1;
    *remainingLength = 0;

    do {
        if (ReadAll(fd, &encoded, 1) != 0) {
            return -1;
        }
        *remainingLength += (size_t)(encoded & 0x7F) * multiplier;
        multiplier *= 128;
        if (multiplier > 128 * 128 * 128) {
            return -1;
        }
    } while ((encoded & 0x80) != 0);
    return 0;
}

static int ConnectTcp(const char *host, uint16_t port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_aton(host, &addr.sin_addr) == 0) {
        printf("[MQTT] resolving %s\n", host);
        struct hostent *he = gethostbyname(host);
        if (he == NULL || he->h_addr_list == NULL || he->h_addr_list[0] == NULL) {
            printf("[MQTT] gethostbyname failed\n");
            return -1;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], sizeof(addr.sin_addr));
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        printf("[MQTT] socket failed, errno=%d\n", errno);
        return -1;
    }

    printf("[MQTT] connecting to %s:%u\n", inet_ntoa(addr.sin_addr), port);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        printf("[MQTT] connect failed, errno=%d\n", errno);
        close(fd);
        return -1;
    }
    return fd;
}

static int E53Sc1SendCommand(int cmd, const char *arg, char *replyData, size_t replySize)
{
    int ret = -1;
    struct HdfIoService *serv = HdfIoServiceBind(E53_SC1_SERVICE);
    if (serv == NULL) {
        printf("[E53_SC1] fail to bind service %s\n", E53_SC1_SERVICE);
        return -1;
    }

    struct HdfSBuf *data = HdfSBufObtainDefaultSize();
    struct HdfSBuf *reply = HdfSBufObtainDefaultSize();
    if (data == NULL || reply == NULL) {
        printf("[E53_SC1] fail to obtain sbuf\n");
        goto out;
    }

    if (!HdfSbufWriteString(data, arg == NULL ? "" : arg)) {
        printf("[E53_SC1] fail to write sbuf\n");
        goto out;
    }

    ret = serv->dispatcher->Dispatch(&serv->object, cmd, data, reply);
    if (ret != 0) {
        printf("[E53_SC1] dispatch cmd=%d failed, ret=%d\n", cmd, ret);
        goto out;
    }

    if (replyData != NULL && replySize > 0) {
        const char *replyStr = HdfSbufReadString(reply);
        if (replyStr == NULL) {
            printf("[E53_SC1] empty reply\n");
            ret = -1;
            goto out;
        }
        snprintf(replyData, replySize, "%s", replyStr);
    }

out:
    if (data != NULL) {
        HdfSBufRecycle(data);
    }
    if (reply != NULL) {
        HdfSBufRecycle(reply);
    }
    HdfIoServiceRecycle(serv);
    return ret;
}

static int E53Sc1Init(void)
{
    printf("[E53_SC1] init\n");
    return E53Sc1SendCommand(E53_SC1_CMD_START, "", NULL, 0);
}

static int E53Sc1SetLight(int on)
{
    int ret = E53Sc1SendCommand(E53_SC1_CMD_SET_LIGHT, on ? "ON" : "OFF", NULL, 0);
    if (ret == 0) {
        g_ledOn = on ? 1 : 0;
        if (!g_ledOn) {
            g_brightness = 0;
        } else if (g_brightness <= 0) {
            g_brightness = 10;
            E53Sc1SetBrightness(g_brightness);
        }
    }
    return ret;
}

static int E53Sc1SetBrightness(int brightness)
{
    char value[16];
    if (brightness < 0) {
        brightness = 0;
    } else if (brightness > 100) {
        brightness = 100;
    }
    snprintf(value, sizeof(value), "%d", brightness);
    int ret = E53Sc1SendCommand(E53_SC1_CMD_SET_BRIGHTNESS, value, NULL, 0);
    if (ret == 0) {
        g_brightness = brightness;
        g_ledOn = brightness > 0 ? 1 : 0;
        E53Sc1SendCommand(E53_SC1_CMD_SET_LIGHT, g_ledOn ? "ON" : "OFF", NULL, 0);
    }
    return ret;
}

static int E53Sc1Read(LampData *lamp)
{
    char reply[E53_REPLY_SIZE] = {0};
    char led[8] = {0};

    if (E53Sc1SendCommand(E53_SC1_CMD_READ, "", reply, sizeof(reply)) != 0) {
        return -1;
    }

    printf("[E53_SC1] raw: %s\n", reply);
    int matched = sscanf(reply, "{\"Lux\":%f,\"LED\":\"%7[^\"]\",\"Brightness\":%d}",
        &lamp->lux, led, &lamp->brightness);
    if (matched != 3) {
        printf("[E53_SC1] parse failed, matched=%d\n", matched);
        return -1;
    }

    lamp->ledOn = strcmp(led, "ON") == 0 ? 1 : 0;
    if (lamp->brightness > 0) {
        lamp->ledOn = 1;
    }
    g_ledOn = lamp->ledOn;
    g_brightness = lamp->brightness;
    return 0;
}

static int SendMqttConnect(int fd)
{
    uint8_t packet[MQTT_BUFFER_SIZE];
    uint8_t payload[MQTT_BUFFER_SIZE];
    size_t payloadLen = 0;
    size_t offset = 0;

    if (AppendString(payload, sizeof(payload), &payloadLen, "MQTT") != 0) {
        return -1;
    }
    payload[payloadLen++] = 0x04;
    payload[payloadLen++] = 0xC2;
    payload[payloadLen++] = (uint8_t)(MQTT_KEEPALIVE_SECONDS >> 8);
    payload[payloadLen++] = (uint8_t)(MQTT_KEEPALIVE_SECONDS & 0xFF);

    if (AppendString(payload, sizeof(payload), &payloadLen, MQTT_CLIENT_ID) != 0 ||
        AppendString(payload, sizeof(payload), &payloadLen, MQTT_USERNAME) != 0 ||
        AppendString(payload, sizeof(payload), &payloadLen, g_mqttPassword) != 0) {
        return -1;
    }

    packet[offset++] = 0x10;
    if (EncodeRemainingLength(packet, sizeof(packet), &offset, payloadLen) != 0 ||
        offset + payloadLen > sizeof(packet)) {
        return -1;
    }
    memcpy(packet + offset, payload, payloadLen);
    offset += payloadLen;

    printf("[MQTT] sending CONNECT, clientId=%s, username=%s\n", MQTT_CLIENT_ID, MQTT_USERNAME);
    if (WriteAll(fd, packet, offset) != 0) {
        return -1;
    }

    uint8_t ack[4] = {0};
    if (ReadAll(fd, ack, sizeof(ack)) != 0) {
        printf("[MQTT] CONNACK recv failed\n");
        return -1;
    }
    printf("[MQTT] CONNACK: %02X %02X %02X %02X\n", ack[0], ack[1], ack[2], ack[3]);
    return (ack[0] == 0x20 && ack[1] == 0x02 && ack[3] == 0x00) ? 0 : -1;
}

static int SendMqttSubscribe(int fd)
{
    uint8_t packet[MQTT_BUFFER_SIZE];
    uint8_t payload[MQTT_BUFFER_SIZE];
    size_t payloadLen = 0;
    size_t offset = 0;
    char topic[256];

    snprintf(topic, sizeof(topic), "$oc/devices/%s/sys/properties/set/#", DEVICE_ID);
    uint16_t packetId = (uint16_t)g_packetId++;
    payload[payloadLen++] = (uint8_t)(packetId >> 8);
    payload[payloadLen++] = (uint8_t)(packetId & 0xFF);
    if (AppendString(payload, sizeof(payload), &payloadLen, topic) != 0) {
        return -1;
    }
    payload[payloadLen++] = 0x00;

    packet[offset++] = 0x82;
    if (EncodeRemainingLength(packet, sizeof(packet), &offset, payloadLen) != 0 ||
        offset + payloadLen > sizeof(packet)) {
        return -1;
    }
    memcpy(packet + offset, payload, payloadLen);
    offset += payloadLen;

    printf("[MQTT] subscribe topic: %s\n", topic);
    if (WriteAll(fd, packet, offset) != 0) {
        return -1;
    }

    uint8_t fixedHeader = 0;
    size_t remainLen = 0;
    uint8_t ack[8];
    if (ReadAll(fd, &fixedHeader, 1) != 0 || ReadRemainingLength(fd, &remainLen) != 0 ||
        remainLen > sizeof(ack) || ReadAll(fd, ack, remainLen) != 0) {
        printf("[MQTT] SUBACK recv failed\n");
        return -1;
    }
    printf("[MQTT] SUBACK type=0x%02X result=0x%02X\n", fixedHeader, ack[remainLen - 1]);
    return fixedHeader == 0x90 && ack[remainLen - 1] != 0x80 ? 0 : -1;
}

static int SendMqttPublishRaw(int fd, const char *topic, const char *payload)
{
    uint8_t packet[MQTT_BUFFER_SIZE];
    size_t offset = 0;
    size_t topicLen = strlen(topic);
    size_t payloadLen = strlen(payload);
    size_t remainLen = 2 + topicLen + payloadLen;

    packet[offset++] = 0x30;
    if (EncodeRemainingLength(packet, sizeof(packet), &offset, remainLen) != 0 ||
        offset + remainLen > sizeof(packet)) {
        printf("[MQTT] publish packet too large\n");
        return -1;
    }
    packet[offset++] = (uint8_t)(topicLen >> 8);
    packet[offset++] = (uint8_t)(topicLen & 0xFF);
    memcpy(packet + offset, topic, topicLen);
    offset += topicLen;
    memcpy(packet + offset, payload, payloadLen);
    offset += payloadLen;

    printf("[MQTT] report topic: %s\n", topic);
    printf("[MQTT] report payload: %s\n", payload);
    return WriteAll(fd, packet, offset);
}

static int SendPropertyReport(int fd, const LampData *lamp)
{
    char topic[256];
    char payload[512];
    int reportLedOn = lamp->ledOn || lamp->brightness > 0;

    snprintf(topic, sizeof(topic), "$oc/devices/%s/sys/properties/report", DEVICE_ID);
    snprintf(payload, sizeof(payload),
        "{\"services\":[{\"service_id\":\"%s\",\"properties\":{\"light\":%.2f,\"led_status\":%s,\"led_brightness\":%d}}]}",
        SERVICE_ID, lamp->lux, reportLedOn ? "true" : "false", lamp->brightness);
    return SendMqttPublishRaw(fd, topic, payload);
}

static int SendSetResponse(int fd, const char *requestId, int resultCode)
{
    char topic[320];
    char payload[128];

    snprintf(topic, sizeof(topic), "$oc/devices/%s/sys/properties/set/response/request_id=%s", DEVICE_ID, requestId);
    snprintf(payload, sizeof(payload), "{\"result_code\":%d,\"result_desc\":\"%s\"}",
        resultCode, resultCode == 0 ? "success" : "failed");
    return SendMqttPublishRaw(fd, topic, payload);
}

static int ExtractRequestId(const char *topic, char *requestId, size_t requestIdSize)
{
    const char *key = "request_id=";
    const char *pos = strstr(topic, key);
    if (pos == NULL || requestIdSize == 0) {
        return -1;
    }
    pos += strlen(key);
    snprintf(requestId, requestIdSize, "%s", pos);
    return 0;
}

static int JsonBoolValue(const char *json, const char *key, int *value)
{
    const char *pos = strstr(json, key);
    if (pos == NULL) {
        return -1;
    }
    pos = strchr(pos, ':');
    if (pos == NULL) {
        return -1;
    }
    pos++;
    while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') {
        pos++;
    }
    if (strncmp(pos, "true", 4) == 0) {
        *value = 1;
        return 0;
    }
    if (strncmp(pos, "false", 5) == 0) {
        *value = 0;
        return 0;
    }
    return -1;
}

static int JsonIntValue(const char *json, const char *key, int *value)
{
    const char *pos = strstr(json, key);
    if (pos == NULL) {
        return -1;
    }
    pos = strchr(pos, ':');
    if (pos == NULL) {
        return -1;
    }
    *value = atoi(pos + 1);
    return 0;
}

static int HandlePropertySet(int fd, const char *topic, const char *payload)
{
    char requestId[128] = {0};
    int ret = 0;
    int ledStatus = 0;
    int brightness = 0;

    printf("[MQTT] set topic: %s\n", topic);
    printf("[MQTT] set payload: %s\n", payload);
    if (ExtractRequestId(topic, requestId, sizeof(requestId)) != 0) {
        printf("[MQTT] set request_id missing\n");
        return -1;
    }

    int hasBrightness = JsonIntValue(payload, "led_brightness", &brightness) == 0;
    int hasLedStatus = JsonBoolValue(payload, "led_status", &ledStatus) == 0;

    if (hasBrightness) {
        if (E53Sc1SetBrightness(brightness) != 0) {
            ret = -1;
        }
    }
    if (hasLedStatus) {
        if (E53Sc1SetLight(ledStatus) != 0) {
            ret = -1;
        }
    }

    SendSetResponse(fd, requestId, ret == 0 ? 0 : 1);
    return ret;
}

static int HandleMqttPacket(int fd)
{
    uint8_t fixedHeader = 0;
    size_t remainLen = 0;
    uint8_t body[MQTT_BUFFER_SIZE];

    if (ReadAll(fd, &fixedHeader, 1) != 0 || ReadRemainingLength(fd, &remainLen) != 0) {
        return -1;
    }
    if (remainLen > sizeof(body) || ReadAll(fd, body, remainLen) != 0) {
        return -1;
    }

    if ((fixedHeader & 0xF0) == 0x30) {
        if (remainLen < 2) {
            return -1;
        }
        size_t topicLen = ((size_t)body[0] << 8) | body[1];
        if (2 + topicLen > remainLen || topicLen >= 256) {
            return -1;
        }
        char topic[256] = {0};
        char payload[MQTT_BUFFER_SIZE] = {0};
        memcpy(topic, body + 2, topicLen);
        size_t payloadLen = remainLen - 2 - topicLen;
        if (payloadLen >= sizeof(payload)) {
            payloadLen = sizeof(payload) - 1;
        }
        memcpy(payload, body + 2 + topicLen, payloadLen);
        printf("[MQTT] PUBLISH topic=%s\n", topic);
        if (strstr(topic, "/sys/properties/set/request_id=") != NULL) {
            HandlePropertySet(fd, topic, payload);
        }
    } else if (fixedHeader == 0xD0) {
        printf("[MQTT] PINGRESP\n");
    } else {
        printf("[MQTT] packet type=0x%02X len=%u\n", fixedHeader, (unsigned int)remainLen);
    }
    return 0;
}

static int SendPingReq(int fd)
{
    uint8_t pingReq[2] = {0xC0, 0x00};
    printf("[MQTT] PINGREQ\n");
    return WriteAll(fd, pingReq, sizeof(pingReq));
}

int main(int argc, char **argv)
{
    const char *host = MQTT_HOST;
    if (argc > 1 && argv[1] != NULL) {
        host = argv[1];
    }
    if (argc > 2 && argv[2] != NULL) {
        g_mqttPassword = argv[2];
    }

    printf("\n[MQTT] Huawei IoTDA E53_SC1 smart lamp demo start\n");
    printf("[MQTT] Please connect Wi-Fi first, this program only handles MQTT.\n");
    if (g_mqttPassword[0] == '\0') {
        printf("[MQTT] missing password, usage: ./huawei_mqtt_demo <host-or-ip> <password>\n");
        return -1;
    }

    int fd = ConnectTcp(host, MQTT_PORT);
    if (fd < 0) {
        return -1;
    }
    if (SendMqttConnect(fd) != 0 || SendMqttSubscribe(fd) != 0) {
        close(fd);
        return -1;
    }
    if (E53Sc1Init() != 0) {
        close(fd);
        return -1;
    }

    time_t lastReport = 0;
    time_t lastPing = time(NULL);
    while (1) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(fd, &readSet);
        struct timeval timeout = {1, 0};
        int ret = select(fd + 1, &readSet, NULL, NULL, &timeout);
        if (ret < 0) {
            printf("[MQTT] select failed, errno=%d\n", errno);
            break;
        }
        if (ret > 0 && FD_ISSET(fd, &readSet)) {
            if (HandleMqttPacket(fd) != 0) {
                printf("[MQTT] connection closed\n");
                break;
            }
        }

        time_t now = time(NULL);
        if (now - lastReport >= REPORT_INTERVAL_SECONDS) {
            LampData lamp = {0};
            if (E53Sc1Read(&lamp) == 0) {
                if (SendPropertyReport(fd, &lamp) != 0) {
                    break;
                }
            }
            lastReport = now;
        }
        if (now - lastPing >= MQTT_KEEPALIVE_SECONDS / 2) {
            if (SendPingReq(fd) != 0) {
                break;
            }
            lastPing = now;
        }
    }

    close(fd);
    return -1;
}
