/*
 * Copyright (c) 2022 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "E53_SC1.h"
#include "E53_Common.h"
#include "osal_time.h"

#define E53_SC1_Light_GPIO  E53_IO_5

/* 智慧台灯：全局变量 */
static uint8_t g_brightness = 0;       /* 当前亮度 0-100 */
static uint8_t g_lightStatus = 0;      /* 灯光状态 0=OFF 1=ON */

/* 智慧台灯：按键中断回调函数 */
static int32_t ButtonIrqCallback(uint16_t gpio, void *data)
{
    (void)gpio;
    (void)data;
    /* 切换灯光状态 */
    if (g_lightStatus == 0) {
        g_lightStatus = 1;
        g_brightness = 80; /* 默认亮度 80% */
        E53_SC1SetBrightness(g_brightness);
        printf("[E53_SC1] Button pressed: Light ON, Brightness=%d%%\r\n", g_brightness);
    } else {
        g_lightStatus = 0;
        g_brightness = 0;
        E53_SC1SetBrightness(0);
        printf("[E53_SC1] Button pressed: Light OFF\r\n");
    }
    return 0;
}

/***************************************************************
 * 函数名称: Init_BH1750
 * 说    明: 写命令初始化BH1750
 * 参    数: 无
 * 返 回 值: 0 成功; -1 失败
 ***************************************************************/
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

/***************************************************************
 * 函数名称: Start_BH1750
 * 说    明: 启动BH1750
 * 参    数: 无
 * 返 回 值: 0 成功; -1 失败
 ***************************************************************/
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
/***************************************************************
 * 函数名称: E53SC1Init
 * 说    明: 初始化E53_SC1
 * 参    数: 无
 * 返 回 值: 0 成功; -1 失败
 ***************************************************************/
int E53_SC1Init(void)
{
    int ret;

    E53_GPIOInit(E53_SC1_Light_GPIO, E53_GPIO_Out_PullNone);
    E53_IICOpen();

    ret = InitBH1750();
    if (ret != 0) {
        return -1;
    }

    /* 智慧台灯：初始化 PWM */
    ret = E53_PWMOpen(E53_SC1_PWM_NUM);
    if (ret != 0) {
        printf("[E53_SC1] PWM Open failed\r\n");
    } else {
        E53_PWMSetPeriod(E53_SC1_PWM_PERIOD);
        E53_PWMSetDuty(0);
        E53_PWMStart();
        printf("[E53_SC1] PWM initialized\r\n");
    }

    /* 智慧台灯：初始化按键 */
    E53_SC1ButtonInit();

    g_lightStatus = 0;
    g_brightness = 0;
    printf("[E53_SC1] Smart Light initialized\r\n");
    return 0;
}
/***************************************************************
 * 函数名称: E53SC1DeInit
 * 说    明: 初始化E53_SC1
 * 参    数: 无
 * 返 回 值: 0 成功; -1 失败
 ***************************************************************/
int E53_SC1DeInit(void)
{
    int ret;
    E53_GPIODeinit(E53_SC1_Light_GPIO);
    E53_IICClose();

    /* 智慧台灯：关闭 PWM 和按键 */
    E53_PWMStop();
    E53_PWMClose();
    E53_GPIOUnsetIRQ(E53_SC1_BUTTON_GPIO);
    E53_GPIODeinit(E53_SC1_BUTTON_GPIO);

    g_lightStatus = 0;
    g_brightness = 0;
    printf("[E53_SC1] Smart Light deinitialized\r\n");

    ret = InitBH1750();
    if (ret != 0) {
        return -1;
    }
    return 0;
}
/***************************************************************
 * 函数名称: E53_SC1_Read_Data
 * 说    明: 测量光照强度
 * 参    数: data,光照强度数据指针
 * 返 回 值: 0 成功; -1 失败
 ***************************************************************/
int E53_SC1ReadData(float *data)
{
    int ret;
    ret = StartBH1750(); // 启动传感器采集数据
    if (ret != 0) {
        return -1;
    }
    uint8_t recv_data[BH1750_RECV_DATA_LEN] = { 0 };
    ret = E53_IICRead((BH1750_ADDR << 1), recv_data, sizeof(recv_data)); // 读取传感器数据

    if (ret != 0) {
        return -1;
    }
    *data = (float)(((recv_data[0] << DATA_WIDTH_8_BIT) + recv_data[1]) / BH1750_COEFFICIENT_LUX); // 合成数据，即光照数据
    return 0;
}
/***************************************************************
 * 函数名称: LightStatusSet
 * 说    明: 灯状态设置
 * 参    数: status,ENUM枚举的数据
 *									OFF,光灯
 *									ON,开灯
 * 返 回 值: 无
 ***************************************************************/
void E53_SC1LightStatusSet(E53SC1Status status)
{
    if (status == ON) {
        g_lightStatus = 1;
        if (g_brightness == 0) {
            g_brightness = 80; /* 默认亮度 80% */
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

/***************************************************************
 * 函数名称: E53_SC1SetBrightness
 * 说    明: 设置灯光亮度（PWM调光）
 * 参    数: brightness, 亮度值 0-100
 * 返 回 值: 0 成功; -1 失败
 ***************************************************************/
int E53_SC1SetBrightness(uint8_t brightness)
{
    uint32_t duty;

    if (brightness > E53_SC1_BRIGHTNESS_MAX) {
        brightness = E53_SC1_BRIGHTNESS_MAX;
    }

    g_brightness = brightness;

    /* 将 0-100 映射到 PWM 占空比 */
    duty = (uint32_t)((float)brightness / 100.0f * E53_SC1_PWM_MAX_DUTY);
    E53_PWMSetDuty(duty);

    if (brightness > 0) {
        g_lightStatus = 1;
    } else {
        g_lightStatus = 0;
    }

    return 0;
}

/***************************************************************
 * 函数名称: E53_SC1GetBrightness
 * 说    明: 获取当前亮度值
 * 参    数: 无
 * 返 回 值: 当前亮度 0-100
 ***************************************************************/
uint8_t E53_SC1GetBrightness(void)
{
    return g_brightness;
}

/***************************************************************
 * 函数名称: E53_SC1ButtonInit
 * 说    明: 初始化按键 GPIO 中断
 * 参    数: 无
 * 返 回 值: 0 成功; -1 失败
 ***************************************************************/
int E53_SC1ButtonInit(void)
{
    E53_Status ret;

    /* 初始化按键 GPIO 为上拉输入模式 */
    ret = E53_GPIOInit(E53_SC1_BUTTON_GPIO, E53_GPIO_In_Up);
    if (ret != E53_Ok) {
        printf("[E53_SC1] Button GPIO init failed\r\n");
        return -1;
    }

    /* 设置下降沿触发中断（按下按键时触发） */
    ret = E53_GPIOSetIRQ(E53_SC1_BUTTON_GPIO, E53_GPIO_IRQ_TRIGGER_FALLING,
                          ButtonIrqCallback, NULL);
    if (ret != E53_Ok) {
        printf("[E53_SC1] Button IRQ setup failed\r\n");
        return -1;
    }

    /* 启动中断 */
    ret = E53_GPIOStartIRQ(E53_SC1_BUTTON_GPIO);
    if (ret != E53_Ok) {
        printf("[E53_SC1] Button IRQ start failed\r\n");
        return -1;
    }

    printf("[E53_SC1] Button initialized on E53_IO_6\r\n");
    return 0;
}
