/*
 * Copyright (c) 2022 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * 修复版本：禁用 PWM 和按键中断，恢复简单 GPIO 控制
 * Kolton 2026-06-11
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "E53_SC1.h"
#include "E53_Common.h"
#include "osal_time.h"

#define E53_SC1_Light_GPIO  E53_IO_5

/* 全局变量 */
static uint8_t g_brightness = 0;
static uint8_t g_lightStatus = 0;

/* BH1750 相关函数 */
static int InitBH1750(void)
{
    int ret;
    uint8_t send_data[1] = { 0x01 };
    ret = E53_IICWrite((BH1750_ADDR << 1), send_data, 1);
    if (ret != 0) {
        return -1;
    }
    return 0;
}

static int StartBH1750(void)
{
    int ret;
    uint8_t send_data[1] = { 0x20 };
    ret = E53_IICWrite((BH1750_ADDR << 1), send_data, 1);
    if (ret != 0) {
        return -1;
    }
    return 0;
}

/* 初始化 - 暂时不启用 PWM 和按键 */
int E53_SC1Init(void)
{
    int ret;

    E53_GPIOInit(E53_SC1_Light_GPIO, E53_GPIO_Out_PullNone);
    E53_IICOpen();

    ret = InitBH1750();
    if (ret != 0) {
        return -1;
    }

    g_lightStatus = 0;
    g_brightness = 0;
    printf("[E53_SC1] initialized (GPIO mode)\r\n");
    return 0;
}

/* 反初始化 */
int E53_SC1DeInit(void)
{
    E53_GPIODeinit(E53_SC1_Light_GPIO);
    E53_IICClose();
    
    g_lightStatus = 0;
    g_brightness = 0;
    printf("[E53_SC1] deinitialized\r\n");
    return 0;
}

/* 读取光照数据 */
int E53_SC1ReadData(float *data)
{
    int ret;
    ret = StartBH1750();
    if (ret != 0) {
        return -1;
    }
    uint8_t recv_data[BH1750_RECV_DATA_LEN] = { 0 };
    ret = E53_IICRead((BH1750_ADDR << 1), recv_data, sizeof(recv_data));
    if (ret != 0) {
        return -1;
    }
    *data = (float)(((recv_data[0] << DATA_WIDTH_8_BIT) + recv_data[1]) / BH1750_COEFFICIENT_LUX);
    return 0;
}

/* 灯光控制 - 简单 GPIO 开关 */
void E53_SC1LightStatusSet(E53SC1Status status)
{
    if (status == ON) {
        E53_GPIOWrite(E53_SC1_Light_GPIO, 1);
        g_lightStatus = 1;
        g_brightness = 100;
        printf("[E53_SC1] Light ON\r\n");
    }
    if (status == OFF) {
        E53_GPIOWrite(E53_SC1_Light_GPIO, 0);
        g_lightStatus = 0;
        g_brightness = 0;
        printf("[E53_SC1] Light OFF\r\n");
    }
}

/* 设置亮度 - 暂时只支持开/关 */
int E53_SC1SetBrightness(uint8_t brightness)
{
    g_brightness = brightness;
    if (brightness > 0) {
        E53_GPIOWrite(E53_SC1_Light_GPIO, 1);
        g_lightStatus = 1;
    } else {
        E53_GPIOWrite(E53_SC1_Light_GPIO, 0);
        g_lightStatus = 0;
    }
    printf("[E53_SC1] Brightness set to %d%%\r\n", brightness);
    return 0;
}

/* 获取亮度 */
uint8_t E53_SC1GetBrightness(void)
{
    return g_brightness;
}

/* 按键初始化 - 暂时禁用 */
int E53_SC1ButtonInit(void)
{
    printf("[E53_SC1] Button disabled (debug mode)\r\n");
    return 0;
}
