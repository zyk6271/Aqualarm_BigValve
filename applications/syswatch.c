/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-17     Rick       the first version
 */
#include "rtthread.h"
#include "stm32g0xx.h"

IWDG_HandleTypeDef iwdg_handle;

void syswatch_init(void)
{
    iwdg_handle.Instance = IWDG;
    iwdg_handle.Init.Prescaler = IWDG_PRESCALER_128;
    iwdg_handle.Init.Window = 4095;
    iwdg_handle.Init.Reload = 4095;
    if (HAL_IWDG_Init(&iwdg_handle) != HAL_OK)
    {
        Error_Handler();
    }
}

void syswatch_feed(void)
{
    HAL_IWDG_Refresh(&iwdg_handle);
}
