/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-04-23     RT-Thread    first version
 */
#include <rtthread.h>
#include "valve.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

int main(void)
{
    adc_init();
    storage_init();
    valve_init();
    button_init();
    while (1)
    {
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}
