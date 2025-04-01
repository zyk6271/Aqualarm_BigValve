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

Valve_Work break_resume_status = VALVE_WORK_STOP;//-1 is back,0 is stop,1 is forward
Valve_Work run_status = VALVE_WORK_STOP;//-1 is back,0 is stop,1 is forward

rt_bool_t valve_manually_status = 0;//0 is automotive,1 is manually
rt_bool_t valve_dead_detect_status = 0;//1 is detecting now
rt_bool_t valve_status = 0;//0 is close 1 is open

int open_forward_target_position = 0;
int open_backward_target_position = 0;
int close_forward_target_position = 0;
int close_backward_target_position = 0;

uint32_t valve_sample_value[10] = {0};
uint32_t valve_sample_cnt = 0;

rt_timer_t valve_break_timer  = RT_NULL;
rt_timer_t valve_watch_timer  = RT_NULL;
rt_timer_t valve_deadzone_detect_timer  = RT_NULL;

rt_err_t valve_reverse_protection(int dir);
rt_err_t valve_dead_calc(uint32_t *src,uint8_t blockSize);

static void valve_deadzone_detect_timer_callback(void *parameter)
{
    if(valve_sample_cnt < 8)
    {
        valve_sample_value[valve_sample_cnt] = ADC_GetValue(0);
        valve_sample_cnt++;
    }
    else
    {
        if(valve_dead_calc(valve_sample_value,8) == RT_ERROR)
        {
            valve_sample_cnt = 0;
            LOG_E("valve_dead_calc passing,pos %d",ADC_GetValue(0));
        }
        else
        {
            valve_dead_detect_status = 0;
            rt_timer_stop(valve_deadzone_detect_timer);
            if(wire_button_level_read() == 0)
            {
                valve_open();
            }
            else
            {
                valve_close();
            }
            LOG_I("valve_dead_calc ok,pos %d",ADC_GetValue(0));
        }
    }
}

static void valve_break_timer_callback(void *parameter)
{
    valve_run(break_resume_status);
}

static void valve_watch_timer_callback(void *parameter)
{
    valve_position_watch();
}

void valve_init(void)
{
    rt_pin_mode(MOTO_CLOSE_STATUS_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MOTO_OPEN_STATUS_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MOTO_OUTPUT1_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MOTO_OUTPUT2_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
    rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
    rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
    rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);

    valve_break_timer = rt_timer_create("valve_break", valve_break_timer_callback, RT_NULL, 500, RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
    valve_watch_timer = rt_timer_create("valve_run", valve_watch_timer_callback, RT_NULL, 3, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    valve_deadzone_detect_timer = rt_timer_create("valve_detect", valve_deadzone_detect_timer_callback, RT_NULL, 600, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);

    valve_position_reset();
}

void valve_run(Valve_Work dir)
{
    switch(dir)
    {
    case VALVE_WORK_REVERSE:
        run_status = dir;
        rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
        rt_timer_start(valve_watch_timer);
        break;
    case VALVE_WORK_STOP:
        run_status = dir;
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
        break;
    case VALVE_WORK_FORWARD:
        run_status = dir;
        rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
        rt_timer_start(valve_watch_timer);
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
    break_resume_status = VALVE_WORK_STOP;
    rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
    rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
    rt_timer_stop(valve_watch_timer);
    rt_timer_stop(valve_deadzone_detect_timer);
    rt_timer_start(valve_break_timer);
}

void valve_manually(uint8_t state)
{
    if(state)
    {
        valve_manually_status = 1;
        break_resume_status = VALVE_WORK_MANUALLY;
        rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
        rt_timer_stop(valve_watch_timer);
        rt_timer_stop(valve_deadzone_detect_timer);
        rt_timer_start(valve_break_timer);
    }
    else
    {
        valve_manually_status = 0;
        valve_position_reset();
    }
}

void valve_open(void)
{
    int dir,position = 0;

    valve_status = 1;

    if(valve_manually_status == 1 || valve_dead_detect_status == 1)
    {
        return;
    }

    position = ADC_GetValue(0);

    if (position >= 0 && position < open_forward_target_position)
    {
        dir = VALVE_WORK_REVERSE;
    }
    else if (position > open_backward_target_position && position <= 4096)
    {
        dir = VALVE_WORK_FORWARD;
    }
    else
    {
        dir = VALVE_WORK_STOP;
    }

    rt_timer_stop(valve_break_timer);
    rt_timer_stop(valve_watch_timer);
    rt_timer_stop(valve_deadzone_detect_timer);

    if(valve_reverse_protection(dir) == RT_EOK)
    {
        valve_run(dir);
    }
    else
    {
        break_resume_status = dir;
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
        rt_timer_start(valve_break_timer);
    }
}

void valve_close(void)
{
    int dir,position = 0;

    valve_status = 0;

    if(valve_manually_status == 1 || valve_dead_detect_status == 1)
    {
        return;
    }

    position = ADC_GetValue(0);

    if (position >= 0 && position < close_forward_target_position)
    {
        dir = VALVE_WORK_REVERSE;
    }
    else if (position > close_backward_target_position && position <= 4096)
    {
        dir = VALVE_WORK_FORWARD;
    }
    else
    {
        dir = VALVE_WORK_STOP;
    }

    rt_timer_stop(valve_break_timer);
    rt_timer_stop(valve_watch_timer);
    rt_timer_stop(valve_deadzone_detect_timer);

    if(valve_reverse_protection(dir) == RT_EOK)
    {
        valve_run(dir);
    }
    else
    {
        break_resume_status = dir;
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
        rt_timer_start(valve_break_timer);
    }
}

void valve_position_watch(void)
{
    uint16_t position = ADC_GetValue(0);
    if(valve_status)
    {
        if(run_status == VALVE_WORK_FORWARD)
        {
            if(position <= open_forward_target_position)
            {
                valve_stop();
                rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
                rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_HIGH);
                LOG_I("valve_position_watch valve stop,code 1,open_forward_target_position %d\r\n",open_forward_target_position);
            }
        }
        else if(run_status == VALVE_WORK_REVERSE)
        {
            if(position >= open_backward_target_position)
            {
                valve_stop();
                rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
                rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_HIGH);
                LOG_I("valve_position_watch valve stop,code 2,open_backward_target_position %d\r\n",open_backward_target_position);
            }
        }
    }
    else
    {
        if(run_status == VALVE_WORK_FORWARD)
        {
            if(position <= close_forward_target_position)
            {
                valve_stop();
                rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_HIGH);
                rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
                LOG_I("valve_position_watch valve stop,code 3,close_forward_target_position %d\r\n",close_forward_target_position);
            }
        }
        else if(run_status == VALVE_WORK_REVERSE)
        {
            if(position >= close_backward_target_position)
            {
                valve_stop();
                rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_HIGH);
                rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
                LOG_I("valve_position_watch valve stop,code 4,close_backward_target_position %d\r\n",close_backward_target_position);
            }
        }
    }
}

rt_err_t valve_reverse_protection(int dir)
{
    if(dir != run_status && run_status != VALVE_WORK_STOP)//存在换向
    {
        LOG_E("reverse_proetction\r\n");
        return RT_ERROR;
    }

    return RT_EOK;
}

void valve_position_reset(void)
{
    valve_sample_cnt = 0;
    valve_dead_detect_status = 1;

    rt_timer_stop(valve_watch_timer);
    rt_timer_stop(valve_break_timer);
    rt_timer_start(valve_deadzone_detect_timer);

    run_status = VALVE_WORK_REVERSE;
    rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
    rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
    rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
    rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
}

rt_err_t valve_dead_calc(uint32_t *src,uint8_t blockSize)
{
    uint8_t blkCnt; /* Loop counter */
    int sum = 0; /* Temporary result storage */
    int meanOfSquares, squareOfMean; /* Square of mean and mean of square */
    int sumOfSquares = 0; /* Sum of squares */
    int in; /* Temporary variable to store input value */

    /* Initialize blkCnt with number of samples */
    blkCnt = blockSize;

    while (blkCnt > 0U)
    {
        in = *src++;
        /* Compute sum of squares and store result in a temporary variable, sumOfSquares. */
        sumOfSquares += ((int) (in) * (in));
        /* Compute sum and store result in a temporary variable, sum. */
        sum += in;

        /* Decrement loop counter */
        blkCnt--;
    }

    /* Compute Mean of squares and store result in a temporary variable, meanOfSquares. */
    meanOfSquares = (sumOfSquares / (int) (blockSize - 1U));

    /* Compute square of mean */
    squareOfMean = (sum * sum / (int) (blockSize * (blockSize - 1U)));

    /* Compute variance and store result in destination */
    if(meanOfSquares - squareOfMean > 100000)
    {
        LOG_E("valve in dead zone,meanOfSquares is %d,squareOfMean is %d\r\n",meanOfSquares,squareOfMean);
        return RT_ERROR;
    }

    return RT_EOK;
}
