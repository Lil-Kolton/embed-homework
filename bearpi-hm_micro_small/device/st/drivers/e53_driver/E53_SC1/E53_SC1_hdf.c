/*
 * Copyright (c) 2022 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * HDF 驱动层 - 修复版本
 * Kolton 2026-06-11
 */

#include <stdint.h>
#include <string.h>

#include "E53_SC1.h"
#include "E53_Common.h"

#include "hdf_device_desc.h" 
#include "hdf_log.h"         
#include "device_resource_if.h"
#include "osal_io.h"
#include "osal_mem.h"
#include "gpio_if.h"
#include "osal_time.h"


static uint8_t LightStatus;
static float lux_data;

typedef enum {
    E53_SC1_Start = 0,
    E53_SC1_Stop,
    E53_SC1_Read,
    E53_SC1_SetLight,
    E53_SC1_SetBrightness,
}E53_SC1Ctrl;

int32_t E53_SC1_DriverDispatch(struct HdfDeviceIoClient *client, int cmdCode, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int ret = -1;
    char *replay_buf;

    HDF_LOGE("E53 driver dispatch");
    if (client == NULL || client->device == NULL) {
        HDF_LOGE("E53 driver device is NULL");
        return HDF_ERR_INVALID_OBJECT;
    }
    switch (cmdCode) {
        case E53_SC1_Start:
            ret = E53_SC1Init();
            if (ret != 0) {
                HDF_LOGE("E53 SC1 Init err");
                return HDF_FAILURE;
            }
            ret = HdfSbufWriteString(reply, "E53 SC1 Init successful");
            if (ret == 0) {
                HDF_LOGE("reply failed");
                return HDF_FAILURE;
            }
            break;
        case E53_SC1_Stop:
            ret = E53_SC1DeInit();
            if (ret != 0) {
                HDF_LOGE("E53 SC1 DeInit err");
                return HDF_FAILURE;
            }
            ret = HdfSbufWriteString(reply, "E53 SC1 DeInit successful");
            if (ret == 0) {
                HDF_LOGE("reply failed");
                return HDF_FAILURE;
            }
            break;
        case E53_SC1_Read:
            ret = E53_SC1ReadData(&lux_data);
            if (ret != 0) {
                HDF_LOGE("E53 SC1 Read Data err");
                return HDF_FAILURE;
            }
            replay_buf = OsalMemAlloc(150);
            (void)memset_s(replay_buf, 150, 0, 150);
            sprintf(replay_buf, "{\"Lux\":%.2f,\"LED\":\"%s\",\"Brightness\":%d}", 
                    lux_data, LightStatus ? "ON" : "OFF", E53_SC1GetBrightness());
            if (!HdfSbufWriteString(reply, replay_buf)) {
                HDF_LOGE("replay is fail");
                return HDF_FAILURE;
            }
            OsalMemFree(replay_buf);
            break;
        case E53_SC1_SetLight:
            const char* readdata = HdfSbufReadString(data);
            if (strcmp(readdata, "ON") == 0) {
                E53_SC1LightStatusSet(ON);
                LightStatus = 1;
            } else if (strcmp(readdata, "OFF") == 0) {
                E53_SC1LightStatusSet(OFF);
                LightStatus = 0;
            } else {
                HDF_LOGE("Command wrong!");
                return HDF_FAILURE;
            }
            replay_buf = OsalMemAlloc(150);
            (void)memset_s(replay_buf, 150, 0, 150);
            sprintf(replay_buf, "{\"Lux\":%.2f,\"LED\":\"%s\",\"Brightness\":%d}", 
                    lux_data, LightStatus ? "ON" : "OFF", E53_SC1GetBrightness());
            if (!HdfSbufWriteString(reply, replay_buf)) {
                HDF_LOGE("replay is fail");
                return HDF_FAILURE;
            }
            OsalMemFree(replay_buf);
            break;
        case E53_SC1_SetBrightness:
            const char* brightnessStr = HdfSbufReadString(data);
            if (brightnessStr == NULL) {
                HDF_LOGE("Brightness data is NULL");
                return HDF_FAILURE;
            }
            int brightnessVal = atoi(brightnessStr);
            if (brightnessVal < 0 || brightnessVal > 100) {
                HDF_LOGE("Brightness value out of range: %d", brightnessVal);
                return HDF_FAILURE;
            }
            E53_SC1SetBrightness((uint8_t)brightnessVal);
            LightStatus = (brightnessVal > 0) ? 1 : 0;
            replay_buf = OsalMemAlloc(150);
            (void)memset_s(replay_buf, 150, 0, 150);
            sprintf(replay_buf, "{\"Lux\":%.2f,\"LED\":\"%s\",\"Brightness\":%d}", 
                    lux_data, LightStatus ? "ON" : "OFF", E53_SC1GetBrightness());
            if (!HdfSbufWriteString(reply, replay_buf)) {
                HDF_LOGE("replay is fail");
                return HDF_FAILURE;
            }
            OsalMemFree(replay_buf);
            break;

        default:
            return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}


static int32_t Hdf_E53_SC1_DriverBind(struct HdfDeviceObject *deviceObject)
{
    if (deviceObject == NULL) {
        HDF_LOGE("e53 driver bind failed!");
        return HDF_ERR_INVALID_OBJECT;
    }
    static struct IDeviceIoService e53Driver = {
        .Dispatch = E53_SC1_DriverDispatch,
    };
    deviceObject->service = (struct IDeviceIoService *)(&e53Driver);
    HDF_LOGD("E53 driver bind success");
    return HDF_SUCCESS;
}

static int32_t Hdf_E53_SC1_DriverInit(struct HdfDeviceObject *device)
{
    (void)device;
    if (E53_SC1Init() != 0) {
        HDF_LOGE("E53 SC1 auto init failed");
        return HDF_FAILURE;
    }
    HDF_LOGI("E53 SC1 auto init success");
    return HDF_SUCCESS;
}

void Hdf_E53_SC1_DriverRelease(struct HdfDeviceObject *deviceObject)
{
    if (deviceObject == NULL) {
        HDF_LOGE("Led driver release failed!");
        return;
    }
    HDF_LOGD("Led driver release success");
    return;
}

static struct HdfDriverEntry g_E53DriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_E53_SC1",
    .Bind = Hdf_E53_SC1_DriverBind,
    .Init = Hdf_E53_SC1_DriverInit,
    .Release = Hdf_E53_SC1_DriverRelease,
};

HDF_INIT(g_E53DriverEntry);
