/*
 * Copyright (c) 2022 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * 智慧台灯驱动：HDF PWM 调光
 * Kolton 2026-06-14
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "E53_SC1.h"
#include "E53_Common.h"
#include "osal_time.h"

/* 全局变量 */
static uint8_t g_brightness = 0;
static uint8_t g_lightStatus = 0;
static uint8_t g_pwmReady = 0;

static int E53_SC1PwmInit(void)
{
    int ret;

    ret = E53_PWMOpen(E53_SC1_PWM_NUM);
    if (ret != 0) {
        printf("[E53_SC1] PWM%d open failed\r\n", E53_SC1_PWM_NUM);
        return -1;
    }

    E53_PWMSet(E53_SC1_PWM_PERIOD, 1);
    E53_PWMStart();
    g_pwmReady = 1;
    printf("[E53_SC1] use PWM%d TIM3_CH4 PB1 period=%d\r\n", E53_SC1_PWM_NUM, E53_SC1_PWM_PERIOD);
    return 0;
}

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

/* 初始化 */
int E53_SC1Init(void)
{
    int ret;

    E53_IICOpen();

    ret = InitBH1750();
    if (ret != 0) {
        return -1;
    }

    E53_SC1PwmInit();

    g_lightStatus = 0;
    g_brightness = 0;
    printf("[E53_SC1] Smart Light initialized\r\n");
    return 0;
}

/* 反初始化 */
int E53_SC1DeInit(void)
{
    if (g_pwmReady) {
        E53_PWMStop();
        E53_PWMClose();
        g_pwmReady = 0;
    }
    E53_IICClose();
    
    g_lightStatus = 0;
    g_brightness = 0;
    printf("[E53_SC1] Smart Light deinitialized\r\n");
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

/* 灯光控制 */
void E53_SC1LightStatusSet(E53SC1Status status)
{
    if (status == ON) {
        g_lightStatus = 1;
        if (g_brightness == 0) {
            g_brightness = 80;
        }
        E53_SC1SetBrightness(g_brightness);
        printf("[E53_SC1] Light ON, Brightness=%d%%\r\n", g_brightness);
    }
    if (status == OFF) {
        g_lightStatus = 0;
        E53_SC1SetBrightness(0);
        printf("[E53_SC1] Light OFF\r\n");
    }
}

/* 设置亮度 - GPIO 软件 PWM */
int E53_SC1SetBrightness(uint8_t brightness)
{
    if (brightness > E53_SC1_BRIGHTNESS_MAX) {
        brightness = E53_SC1_BRIGHTNESS_MAX;
    }

    g_brightness = brightness;
    if (g_pwmReady) {
        unsigned int duty = (brightness == 0) ? 1 : (E53_SC1_PWM_PERIOD * brightness / E53_SC1_BRIGHTNESS_MAX);
        E53_PWMSetDuty(duty);
    }

    if (brightness > 0) {
        g_lightStatus = 1;
    } else {
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
    printf("[E53_SC1] Button disabled\r\n");
    return 0;
}
