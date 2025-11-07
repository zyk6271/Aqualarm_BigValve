/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-04-23     Rick       the first version
 */
#include "rtthread.h"
#include "rtdevice.h"
#include "pin_config.h"
#include "valve.h"
#include "flashwork.h"
#include "button.h"

#define DBG_TAG "VALVE"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

typedef enum
{
    VALVE_WORK_REVERSE = -1,
    VALVE_WORK_STOP,
    VALVE_WORK_FORWARD,
    VALVE_WORK_MANUALLY
} Valve_Work;

Valve_Work valve_break_status = VALVE_WORK_STOP;//-1 is back,0 is stop,1 is forward
Valve_Work valve_run_status = VALVE_WORK_STOP;//-1 is back,0 is stop,1 is forward

rt_bool_t valve_manually_status = 0;//0 is automotive,1 is manually
rt_bool_t valve_control_status = 0;//0 is close 1 is open

rt_timer_t valve_break_timer  = RT_NULL;

Button_t valve_open_switch_button;
Button_t valve_close_switch_button;

uint8_t valve_open_switch_level_read(void)
{
    return rt_pin_read(MOTO_OPEN_POSITION_PIN);
}

uint8_t valve_close_switch_level_read(void)
{
    return rt_pin_read(MOTO_CLOSE_POSITION_PIN);
}

void valve_open_swtich_down_callback(void *parameter)
{
    valve_open_done();
}

void valve_close_swtich_down_down_callback(void *parameter)
{
    valve_close_done();
}

void valve_open_swtich_button_pause(void)
{
    Button_Attach(&valve_open_switch_button, BUTTON_DOWN, RT_NULL);
}

void valve_open_swtich_button_resume(void)
{
    Button_Attach(&valve_open_switch_button, BUTTON_DOWN, valve_open_swtich_down_callback);
}

void valve_close_swtich_button_pause(void)
{
    Button_Attach(&valve_close_switch_button, BUTTON_DOWN, RT_NULL);
}

void valve_close_swtich_button_resume(void)
{
    Button_Attach(&valve_close_switch_button, BUTTON_DOWN, valve_close_swtich_down_down_callback);
}

rt_err_t valve_reverse_protection(int dir)
{
    if(dir != valve_run_status && valve_run_status != VALVE_WORK_STOP)//存在换向
    {
        LOG_E("reverse_proetction\r\n");
        return RT_ERROR;
    }

    return RT_EOK;
}

static void valve_break_timer_callback(void *parameter)
{
    valve_run(valve_break_status);
}

void valve_init(void)
{
    rt_pin_mode(KEY_HAND_PIN, PIN_MODE_INPUT);
    rt_pin_mode(KEY_WIRE_PIN, PIN_MODE_INPUT);
    rt_pin_mode(MOTO_OPEN_POSITION_PIN, PIN_MODE_INPUT);
    rt_pin_mode(MOTO_CLOSE_POSITION_PIN, PIN_MODE_INPUT);

    rt_pin_mode(MOTO_CLOSE_STATUS_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MOTO_OPEN_STATUS_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MOTO_OUTPUT1_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MOTO_OUTPUT2_PIN, PIN_MODE_OUTPUT);

    rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
    rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
    rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
    rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);

    Button_Create("valve_open_switch_button", &valve_open_switch_button, valve_open_switch_level_read, 0);
    Button_Create("valve_close_switch_button", &valve_close_switch_button, valve_close_switch_level_read, 0);

    valve_break_timer = rt_timer_create("valve_break", valve_break_timer_callback, RT_NULL, 500, RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
    if(wire_button_level_read() == 0)
    {
        valve_open();
    }
    else
    {
        valve_close();
    }
}

void valve_run(Valve_Work dir)
{
    switch(dir)
    {
    case VALVE_WORK_REVERSE:
        valve_run_status = dir;
        rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
        break;
    case VALVE_WORK_STOP:
        valve_run_status = dir;
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
        break;
    case VALVE_WORK_FORWARD:
        valve_run_status = dir;
        rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
        break;
    case VALVE_WORK_MANUALLY:
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
        break;
    default:
        break;
    }
}

void valve_stop(void)
{
    valve_break_status = VALVE_WORK_STOP;
    rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
    rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
    rt_timer_start(valve_break_timer);
    valve_close_swtich_button_pause();
    valve_open_swtich_button_pause();
}

void valve_manually(uint8_t state)
{
    if(state)
    {
        valve_manually_status = 1;
        valve_break_status = VALVE_WORK_MANUALLY;
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
        rt_timer_start(valve_break_timer);
    }
    else
    {
        valve_manually_status = 0;
        rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
        if(wire_button_level_read() == 0)
        {
            valve_open();
        }
        else
        {
            valve_close();
        }
    }
}

void valve_open(void)
{
    int dir = 0;

    valve_control_status = 1;

    if(valve_manually_status == 1)
    {
        LOG_E("valve_open fail,because valve is manually\r\n");
        return;
    }

    if(rt_pin_read(MOTO_OPEN_POSITION_PIN) == 0)
    {
        LOG_E("valve_open fail,because valve is opened\r\n");
        return;
    }

    rt_timer_stop(valve_break_timer);

    dir = VALVE_WORK_REVERSE;

    valve_close_swtich_button_pause();
    valve_open_swtich_button_resume();

    if(valve_reverse_protection(dir) == RT_EOK)
    {
        valve_run(dir);
    }
    else
    {
        valve_break_status = dir;
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
        rt_timer_start(valve_break_timer);
    }
}

void valve_close(void)
{
    int dir = 0;

    valve_control_status = 0;

    if(valve_manually_status == 1)
    {
        LOG_E("valve_close fail,because valve is manually\r\n");
        return;
    }

    if(rt_pin_read(MOTO_CLOSE_POSITION_PIN) == 0)
    {
        LOG_E("valve_close fail,because valve is closed\r\n");
        return;
    }

    rt_timer_stop(valve_break_timer);

    dir = VALVE_WORK_FORWARD;

    valve_open_swtich_button_pause();
    valve_close_swtich_button_resume();

    if(valve_reverse_protection(dir) == RT_EOK)
    {
        valve_run(dir);
    }
    else
    {
        valve_break_status = dir;
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
        rt_timer_start(valve_break_timer);
    }
}

void valve_open_done(void)
{
    valve_stop();
    rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
    rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_HIGH);
    LOG_I("valve_open_done\r\n");
}

void valve_close_done(void)
{
    valve_stop();
    rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_HIGH);
    rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
    LOG_I("valve_close_done\r\n");
}
