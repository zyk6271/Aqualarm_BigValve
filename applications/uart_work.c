/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-18     Rick       the first version
 */
#include <rtthread.h>

rt_thread_t uart_thread_t = RT_NULL;

static struct rt_semaphore rx_sem;
rt_device_t console;

void free_print(void)
{
    rt_size_t total = 0, used = 0, max_used = 0;

    rt_memory_info(&total, &used, &max_used);
    rt_kprintf("total    : %d\n", total);
    rt_kprintf("used     : %d\n", used);
    rt_kprintf("maximum  : %d\n", max_used);
    rt_kprintf("available: %d\n", total - used);
}

static rt_err_t custon_rx_ind(rt_device_t dev, rt_size_t size)
{
    if (size > 0)
    {
        rt_sem_release(&rx_sem);
    }

    return RT_EOK;
}

static char uart_sample_get_char(void)
{
    char ch;

    while (rt_device_read(console, 0, &ch, 1) == 0)
    {
        rt_sem_control(&rx_sem, RT_IPC_CMD_RESET, RT_NULL);
        rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
    }

    return ch;
}

void uart_recv(void)
{
    char ch;
    static uint8_t recv_buf[2] = {0};
    ch = uart_sample_get_char();
    recv_buf[1] = recv_buf[0];
    recv_buf[0] = ch;
    if(recv_buf[0] == 'F' && recv_buf[1] == 'O')
    {
        ofsave();
    }
    else if(recv_buf[0] == 'F' && recv_buf[1] == 'C')
    {
        cfsave();
    }
    else if(recv_buf[0] == 'B' && recv_buf[1] == 'O')
    {
        obsave();
    }
    else if(recv_buf[0] == 'B' && recv_buf[1] == 'C')
    {
        cbsave();
    }
    else if(recv_buf[0] == 'P' && recv_buf[1] == 'C')
    {
        calib_print();
    }
    else if(recv_buf[0] == 'R' && recv_buf[1] == 'M')
    {
        free_print();
    }
}

void uart_thread_entry(void *parameter)
{
    console = rt_console_get_device();
    rt_device_open(console, RT_DEVICE_FLAG_INT_RX);
    rt_device_set_rx_indicate(console, custon_rx_ind);
    while(1)
    {
        uart_recv();
    }
}

void uart_init(void)
{
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    uart_thread_t = rt_thread_create("uart", uart_thread_entry, RT_NULL, 256, 12, 10);
    rt_thread_startup(uart_thread_t);
}

