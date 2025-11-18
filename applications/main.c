/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-14     RT-Thread    first version
 */

#include <rtthread.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

int main(void)
{
    adc_init();
    button_init();
    storage_init();
    valve_init();
    uart_init();
    syswatch_init();
    while(1)
    {
        syswatch_feed();
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}
