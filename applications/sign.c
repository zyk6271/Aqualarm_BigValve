/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-07-03     Tobby       the first version
 */
#include "rtthread.h"
#include "rtdevice.h"
#include "pin_config.h"
#include "button.h"
#include "flashwork.h"

#define DBG_TAG "sign"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

Button_t switch_button;
Button_t wire_button;

rt_thread_t button_watch_t = RT_NULL;

uint8_t switch_button_level_read(void)
{
    return rt_pin_read(KEY_HAND_PIN);
}

uint8_t wire_button_level_read(void)
{
    return rt_pin_read(KEY_WIRE_PIN);
}

void switch_button_up_callback(void *parameter)
{
    valve_manually(1);
}

void switch_button_down_callback(void *parameter)
{
    valve_manually(0);
}

void wire_button_up_callback(void *parameter)
{
    valve_open();
}

void wire_button_down_callback(void *parameter)
{
    valve_close();
}

void button_watch_thread_entry(void *parameter)
{
    while (1)
    {
        Button_Process();
        rt_thread_mdelay(10);
    }
}

void button_init(void)
{
    rt_pin_mode(KEY_HAND_PIN, PIN_MODE_INPUT);
    rt_pin_mode(KEY_WIRE_PIN, PIN_MODE_INPUT);

    Button_Create("switch_button", &switch_button, switch_button_level_read, 1);
    Button_Attach(&switch_button, BUTTON_DOWN, switch_button_up_callback);
    Button_Attach(&switch_button, BUTTON_UP, switch_button_down_callback);

    Button_Create("wire_button", &wire_button, wire_button_level_read, 0);
    Button_Attach(&wire_button, BUTTON_DOWN, wire_button_up_callback);
    Button_Attach(&wire_button, BUTTON_UP, wire_button_down_callback);

    button_watch_t = rt_thread_create("button_watch", button_watch_thread_entry, RT_NULL, 1024, 5, 10);
    rt_thread_startup(button_watch_t);
}
