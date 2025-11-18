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

int pre_run_status = 0;//-1 is back,0 is stop,1 is forward
int run_status = 0;//-1 is back,0 is stop,1 is forward
int manually_status = 0;//0 is automotive,1 is manually
int detect_flag = 0;
int valve_status = 0;//0 is close 1 is open

int open_forward_target_position = 0;
int open_backward_target_position = 0;
int close_forward_target_position = 0;
int close_backward_target_position = 0;

uint32_t pos_value[10];
uint32_t pos_senq_cnt = 0;
uint32_t pos_total_cnt = 0;

rt_timer_t delay_timer  = RT_NULL;
rt_timer_t run_timer  = RT_NULL;
rt_timer_t detect_timer  = RT_NULL;

rt_err_t valve_dead_calc(uint32_t *src,uint8_t blockSize);
rt_err_t valve_reverse_proetction(int dir);

static void detect_timer_callback(void *parameter)
{
    pos_value[pos_senq_cnt++] = ADC_GetValue(1);
    pos_total_cnt ++;
    if(pos_senq_cnt > 10)
    {
        pos_senq_cnt = 0;
        if(valve_dead_calc(pos_value,10) == RT_ERROR)
        {
            pos_total_cnt = 0;
        }
    }
    if(pos_total_cnt > 500)
    {
        detect_flag = 0;
        rt_timer_stop(detect_timer);
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

static void delay_timer_callback(void *parameter)
{
    valve_run(pre_run_status);
}

static void run_timer_callback(void *parameter)
{
    valve_position_watch();
}

void valve_init(void)
{
    run_status = 0;
    rt_pin_mode(MOTO_CLOSE_STATUS_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MOTO_OPEN_STATUS_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
    rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
    rt_pin_mode(MOTO_OUTPUT1_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MOTO_OUTPUT2_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
    rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
    delay_timer = rt_timer_create("valve_delay", delay_timer_callback, RT_NULL, 500, RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
    run_timer = rt_timer_create("valve_run", run_timer_callback, RT_NULL, 3, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    detect_timer = rt_timer_create("valve_detect", detect_timer_callback, RT_NULL, 10, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);

    open_forward_target_position = flash_get_key(0);
    if(open_forward_target_position == 0)
    {
        open_forward_target_position = 1460;
    }

    open_backward_target_position = flash_get_key(1);
    if(open_backward_target_position == 0)
    {
        open_backward_target_position = 1540;
    }

    close_forward_target_position = flash_get_key(2);
    if(close_forward_target_position == 0)
    {
        close_forward_target_position = 2590;
    }

    close_backward_target_position = flash_get_key(3);
    if(close_backward_target_position == 0)
    {
        close_backward_target_position = 2690;
    }

    valve_position_reset();
}

void valve_run(int dir)
{
    switch(dir)
    {
    case -1:
        run_status = dir;
        rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
        rt_timer_start(run_timer);
        break;
    case 0:
        run_status = dir;
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
        break;
    case 1:
        run_status = dir;
        rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
        rt_timer_start(run_timer);
        break;
    default:
        break;
    }
}

void valve_stop(void)
{
    pre_run_status = 0;
    rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
    rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
    rt_timer_stop(run_timer);
    rt_timer_stop(detect_timer);
    rt_timer_start(delay_timer);
    rt_kprintf("valve_stop\r\n");
}

void valve_manually(uint8_t state)
{
    manually_status = state;
    if(state)
    {
        valve_stop();
    }
}

void valve_open(void)
{
    int dir = 0;
    uint32_t position = ADC_GetValue(1);

    valve_status = 1;

    if(manually_status == 1 || detect_flag == 1)
    {
        return;
    }

    if (position >= 0 && position < open_forward_target_position)
    {
        dir = 1;
    }
    else if (position > open_backward_target_position && position < 4096)
    {
        dir = -1;
    }
    else
    {
        return;
    }

    rt_timer_stop(delay_timer);
    rt_timer_stop(run_timer);

    if(valve_reverse_proetction(dir) == RT_EOK)
    {
        valve_run(dir);
    }
    else
    {
        pre_run_status = dir;
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
        rt_timer_start(delay_timer);
    }
}

void valve_close(void)
{
    int dir = 0;
    uint32_t position = ADC_GetValue(1);
    valve_status = 0;

    if(manually_status == 1 || detect_flag == 1)
    {
        return;
    }

    if (position >= 0 && position < close_forward_target_position)
    {
        dir = 1;
    }
    else if (position > close_backward_target_position && position < 4096)
    {
        dir = -1;
    }
    else
    {
        return;
    }

    rt_timer_stop(delay_timer);
    rt_timer_stop(run_timer);

    if(valve_reverse_proetction(dir) == RT_EOK)
    {
        valve_run(dir);
    }
    else
    {
        pre_run_status = dir;
        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
        rt_timer_start(delay_timer);
    }
}

void valve_position_watch(void)
{
    if(valve_status)
    {
        if(run_status > 0)
        {
            if(ADC_GetValue(1) > open_forward_target_position)
            {
                valve_stop();
                rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
                rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_HIGH);
            }
        }
        else if(run_status < 0)
        {
            if(ADC_GetValue(1) < open_backward_target_position)
            {
                valve_stop();
                rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
                rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_HIGH);
            }
        }
    }
    else
    {
        if(run_status > 0)
        {
            if(ADC_GetValue(1) > close_forward_target_position)
            {
                valve_stop();
                rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_HIGH);
                rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
            }
        }
        else if(run_status < 0)
        {
            if(ADC_GetValue(1) < close_backward_target_position)
            {
                valve_stop();
                rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_HIGH);
                rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
            }
        }
    }
}

rt_err_t valve_reverse_proetction(int dir)
{
    if(dir != run_status && run_status != 0)//存在换向
    {
        rt_kprintf("reverse_proetction\r\n");
        return RT_ERROR;
    }

    return RT_EOK;
}

void valve_position_reset(void)
{
    rt_memset(pos_value, 0, 10);
    pos_senq_cnt = 0;
    pos_total_cnt = 0;
    run_status = -1;
    detect_flag = 1;
    rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
    rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
    rt_timer_start(detect_timer);
}

rt_err_t valve_dead_calc(uint32_t *src,uint8_t blockSize)
{
    uint32_t blkCnt; /* Loop counter */
    int sum = 0; /* Temporary result storage */
    int meanOfSquares, squareOfMean; /* Square of mean and mean of square */
    int sumOfSquares = 0; /* Sum of squares */
    int in; /* Temporary variable to store input value */

    if (blockSize <= 1U)
    {
        return RT_EOK;
    }

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
    if(meanOfSquares - squareOfMean > 1000)
    {
        rt_kprintf("dead_calc RT_ERROR,meanOfSquares is %d,squareOfMean is %d\r\n",meanOfSquares,squareOfMean);
        return RT_ERROR;
    }

    return RT_EOK;
}

void ofsave(void)
{
    open_forward_target_position = ADC_GetValue(1);
    flash_set_key(0,open_forward_target_position);
    rt_kprintf("open_forward_target_position set to %d\r\n",open_forward_target_position);
}

void obsave(void)
{
    open_backward_target_position = ADC_GetValue(1);
    flash_set_key(1,open_backward_target_position);
    rt_kprintf("open_backward_target_position set to %d\r\n",open_backward_target_position);
}

void cfsave(void)
{
    close_forward_target_position = ADC_GetValue(1);
    flash_set_key(2,close_forward_target_position);
    rt_kprintf("close_forward_target_position set to %d\r\n",close_forward_target_position);
}

void cbsave(void)
{
    close_backward_target_position = ADC_GetValue(1);
    flash_set_key(3,close_backward_target_position);
    rt_kprintf("close_backward_target_position set to %d\r\n",close_backward_target_position);
}
