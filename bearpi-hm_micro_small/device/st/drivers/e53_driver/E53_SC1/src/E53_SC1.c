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
#include "gpio_if.h"
#include "osal_time.h"
#include "osal_thread.h"

#define E53_SC1_BUTTON_THREAD_STACK 0x1000
#define E53_SC1_BUTTON_SCAN_MS 20
#define E53_SC1_BUTTON_DEBOUNCE_MS 300
#define E53_SC1_DEFAULT_BRIGHTNESS 80

/* 全局变量 */
static uint8_t g_brightness = 0;
static uint8_t g_lastBrightness = E53_SC1_DEFAULT_BRIGHTNESS;
static uint8_t g_lightStatus = 0;
static uint8_t g_initialized = 0;
static uint8_t g_pwmReady = 0;
static uint8_t g_buttonThreadRunning = 0;
static struct OsalThread g_buttonThread;

static int E53_SC1ButtonThread(void *arg)
{
    (void)arg;
    uint8_t lastPressed = 0;

    while (g_buttonThreadRunning) {
        uint16_t level = E53_Level_High;
        if (GpioRead(E53_SC1_BUTTON_GPIO, &level) == 0) {
            uint8_t pressed = (level == E53_Level_Down) ? 1 : 0;
            if (pressed && !lastPressed) {
                printf("[E53_SC1] USER_KEY1/S2 pressed\r\n");
                if (g_lightStatus) {
                    E53_SC1LightStatusSet(OFF);
                } else {
                    E53_SC1LightStatusSet(ON);
                }
                printf("[E53_SC1] Button action: LED=%s, Brightness=%d%%\r\n",
                    g_lightStatus ? "ON" : "OFF", g_brightness);
                OsalMSleep(E53_SC1_BUTTON_DEBOUNCE_MS);
            }
            lastPressed = pressed;
        } else {
            printf("[E53_SC1] USER_KEY1/S2 read failed\r\n");
            OsalMSleep(E53_SC1_BUTTON_DEBOUNCE_MS);
        }
        OsalMSleep(E53_SC1_BUTTON_SCAN_MS);
    }
    return 0;
}

static int E53_SC1PwmInit(void)
{
    int ret;

    if (g_pwmReady) {
        return 0;
    }

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

    if (g_initialized) {
        printf("[E53_SC1] Smart Light already initialized\r\n");
        return 0;
    }

    E53_IICOpen();

    ret = InitBH1750();
    if (ret != 0) {
        return -1;
    }

    E53_SC1PwmInit();
    E53_SC1ButtonInit();

    g_lightStatus = 0;
    g_brightness = 0;
    g_initialized = 1;
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
    if (g_buttonThreadRunning) {
        g_buttonThreadRunning = 0;
    }
    E53_IICClose();
    
    g_lightStatus = 0;
    g_brightness = 0;
    g_initialized = 0;
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
            g_brightness = g_lastBrightness;
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

/* 设置亮度 - HDF PWM */
int E53_SC1SetBrightness(uint8_t brightness)
{
    if (brightness > E53_SC1_BRIGHTNESS_MAX) {
        brightness = E53_SC1_BRIGHTNESS_MAX;
    }

    g_brightness = brightness;
    if (brightness > 0) {
        g_lastBrightness = brightness;
    }
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

int E53_SC1ButtonInit(void)
{
    int ret;
    struct OsalThreadParam threadCfg;

    if (g_buttonThreadRunning) {
        return 0;
    }

    ret = GpioSetDir(E53_SC1_BUTTON_GPIO, GPIO_DIR_IN);
    if (ret != 0) {
        printf("[E53_SC1] USER_KEY1/S2 GPIO init failed\r\n");
        return -1;
    }

    memset(&threadCfg, 0, sizeof(threadCfg));
    threadCfg.name = "E53_SC1_USER_KEY1";
    threadCfg.priority = OSAL_THREAD_PRI_DEFAULT;
    threadCfg.stackSize = E53_SC1_BUTTON_THREAD_STACK;
    g_buttonThreadRunning = 1;
    ret = OsalThreadCreate(&g_buttonThread, E53_SC1ButtonThread, NULL);
    if (ret != 0) {
        g_buttonThreadRunning = 0;
        printf("[E53_SC1] USER_KEY1/S2 thread create failed\r\n");
        return -1;
    }
    ret = OsalThreadStart(&g_buttonThread, &threadCfg);
    if (ret != 0) {
        g_buttonThreadRunning = 0;
        printf("[E53_SC1] USER_KEY1/S2 thread start failed\r\n");
        return -1;
    }

    printf("[E53_SC1] USER_KEY1/S2 button polling ready, GPIO=%d active low\r\n", E53_SC1_BUTTON_GPIO);
    return 0;
}
