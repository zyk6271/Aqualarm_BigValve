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
#include <spi_flash.h>
#include <drv_spi.h>
#include <string.h>
#include <stdlib.h>
#include "pin_config.h"
#include "fal.h"
#include "easyflash.h"
#include "flashwork.h"

#define DBG_TAG "FLASH"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

rt_spi_flash_device_t w25q16;

uint32_t total_slot = 0;

ALIGN(RT_ALIGN_SIZE)
static struct rt_mutex flash_mutex;

int storage_init(void)
{
    rt_err_t status;
    rt_mutex_init(&flash_mutex, "flash_mutex", RT_IPC_FLAG_FIFO);
    extern rt_spi_flash_device_t rt_sfud_flash_probe(const char *spi_flash_dev_name, const char *spi_dev_name);
    rt_hw_spi_device_attach("spi1", "spi10", GPIOA, GPIO_PIN_15);
    w25q16 = rt_sfud_flash_probe("norflash0", "spi10");
    if (RT_NULL == w25q16)
    {
        LOG_E("sfud fail\r\n");
        return RT_ERROR;
    };
    status = fal_init();
    if (status == 0)
    {
        LOG_E("fal_init fail\r\n");
        return RT_ERROR;
    };
    status = easyflash_init();
    if (status != EF_NO_ERR)
    {
        LOG_E("easyflash_init fail\r\n");
        return RT_ERROR;
    };
    LOG_I("Storage Init Success\r\n");
    valve_calibration_load();
    return RT_EOK;
}

uint32_t flash_get_key(char *key_name)
{
    uint8_t read_len = 0;
    uint32_t read_value = 0;
    char read_value_temp[32] = {0};
    read_len = ef_get_env_blob(key_name, read_value_temp, 32, NULL);
    if(read_len>0)
    {
        read_value = atol(read_value_temp);
    }
    else
    {
        read_value = 0;
    }

    return read_value;
}

void flash_set_key(char *key_name,uint32_t value)
{
    char *value_buf = rt_malloc(64);//申请临时buffer空间
    rt_sprintf(value_buf, "%d", value);
    ef_set_env_blob(key_name, value_buf,rt_strlen(value_buf));
    rt_free(value_buf);
}
