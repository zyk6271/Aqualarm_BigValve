/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-07-13     Rick       the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>
#include "pin_config.h"
#include "fal.h"
#include "flashwork.h"

#define DBG_TAG "FLASH"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

const struct fal_partition *partition = RT_NULL;

typedef struct
{
    uint8_t magic_word;
    uint32_t open_forward_target_position;
    uint32_t open_backward_target_position;
    uint32_t close_forward_target_position;
    uint32_t close_backward_target_position;
}calib_data_format;

calib_data_format valve_calib_data;

uint32_t flash_get_key(uint8_t id)
{
    uint32_t value_temp = 0;
    switch(id)
    {
    case 0:
        value_temp = valve_calib_data.open_forward_target_position;
        break;
    case 1:
        value_temp = valve_calib_data.open_backward_target_position;
        break;
    case 2:
        value_temp = valve_calib_data.close_forward_target_position;
        break;
    case 3:
        value_temp = valve_calib_data.close_backward_target_position;
        break;
    default:
        break;
    }

    return value_temp;
}

void flash_set_key(uint8_t id, uint32_t value)
{
    switch(id)
    {
    case 0:
        valve_calib_data.open_forward_target_position = value;
        break;
    case 1:
        valve_calib_data.open_backward_target_position = value;
        break;
    case 2:
        valve_calib_data.close_forward_target_position = value;
        break;
    case 3:
        valve_calib_data.close_backward_target_position = value;
        break;
    default:
        break;
    }

    if(fal_partition_erase(partition, 0, sizeof(calib_data_format)) < 0)
    {
        rt_kprintf("fal_partition_erase failed\r\n");
    }
    if(fal_partition_write(partition, 0, (uint8_t *)&valve_calib_data, sizeof(calib_data_format)) < 0)
    {
        rt_kprintf("fal_partition_write failed\r\n");
    }
}

void calib_print(void)
{
    rt_kprintf("open_forward value [%d],open_back value [%d]\r\n",valve_calib_data.open_forward_target_position,valve_calib_data.open_backward_target_position);
    rt_kprintf("close_forward value [%d],close_back value [%d]\r\n",valve_calib_data.close_forward_target_position,valve_calib_data.close_backward_target_position);
}

int storage_init(void)
{
    rt_err_t status;
    status = fal_init();
    if (status == 0)
    {
        rt_kprintf("fal_init fail\r\n");
        return RT_ERROR;
    };
    partition = fal_partition_find("config");
    if (partition == RT_NULL)
    {
        rt_kprintf("partition find failed!\n");
        return RT_ERROR;
    }
    if(fal_partition_read(partition, 0, (uint8_t *)&valve_calib_data, sizeof(calib_data_format)) < 0)
    {
        rt_kprintf("partition read failed!\n");
        return RT_ERROR;
    }
    if(valve_calib_data.magic_word != 0x55)
    {
        valve_calib_data.magic_word = 0x55;
        valve_calib_data.open_forward_target_position = 0;
        valve_calib_data.open_backward_target_position = 0;
        valve_calib_data.close_forward_target_position = 0;
        valve_calib_data.close_backward_target_position = 0;
        if(fal_partition_erase(partition, 0, sizeof(calib_data_format)) < 0)
        {
            rt_kprintf("fal_partition_erase failed\r\n");
        }
        if(fal_partition_write(partition, 0, (uint8_t *)&valve_calib_data, sizeof(calib_data_format)) < 0)
        {
            rt_kprintf("fal_partition_write failed\r\n");
        }
    }
    calib_print();

    return RT_EOK;
}

