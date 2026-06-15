# Huawei IoTDA E53_SC1 智慧台灯 MQTT Demo

这个 demo 用于 BearPi-HM Micro + E53_SC1 智慧台灯接入华为云 IoTDA。

## 功能

- 连接华为云 IoTDA MQTT `1883` 端口。
- 周期上报 `LightService` 属性：`light`、`led_status`、`led_brightness`。
- 订阅 `$oc/devices/{device_id}/sys/properties/set/#`，处理云端下发。
- 收到设备影子配置后，发布 `properties/set/response/request_id=...` 响应，避免云端等待超时。
- 下发亮度时同步本地灯开关状态：亮度大于 `0` 上报 `led_status=true`，亮度为 `0` 上报 `false`。

## 同步到工程

把本目录覆盖到 BearPi 工程：

```sh
cp -r applications/BearPi/BearPi-HM_Micro/samples/huawei_mqtt_demo \
  /home/bearpi/project/bearpi-hm_micro_small/applications/BearPi/BearPi-HM_Micro/samples/
```

确保 `build/lite/components/applications.json` 已包含 `huawei_mqtt_demo` 组件。

## 构建

```sh
cd /home/bearpi/project/bearpi-hm_micro_small
export PATH=/home/bearpi/.local/bin:$PATH
hb build -t notest --tee -f
```

## 运行

先连接 WiFi，再运行：

```sh
cd /bin
./huawei_mqtt_demo 123.60.224.23
```

如果板端 DNS 可用，也可以使用域名：

```sh
./huawei_mqtt_demo bb4de50857.iotda-device.cn-south-4.myhuaweicloud.com
```

## 验证

- 正常上报时，云端设备影子会更新 `light`、`led_status`、`led_brightness`。
- 云端下发时，串口应出现 `[MQTT] set topic`、`[MQTT] set payload` 和 `properties/set/response`。
- 如果云端提示“上一条配置未响应”，等待 5 分钟或清除影子期望值后再测试。
