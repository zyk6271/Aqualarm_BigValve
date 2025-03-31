#include "rtthread.h"
#include "rtdevice.h"
#include "pin_config.h"
#include "valve.h"
#include "flashwork.h"

#define DBG_TAG "CALIB"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

extern int open_forward_target_position;
extern int open_backward_target_position;
extern int close_forward_target_position;
extern int close_backward_target_position;

void valve_calibration_load(void)
{
    open_forward_target_position = flash_get_key("open_forward_target_position");
    if(open_forward_target_position == 0)
    {
        open_forward_target_position = 1450;
    }

    open_backward_target_position = flash_get_key("open_backward_target_position");
    if(open_backward_target_position == 0)
    {
        open_backward_target_position = 1510;
    }

    close_forward_target_position = flash_get_key("close_forward_target_position");
    if(close_forward_target_position == 0)
    {
        close_forward_target_position = 2500;
    }

    close_backward_target_position = flash_get_key("close_backward_target_position");
    if(close_backward_target_position == 0)
    {
        close_backward_target_position = 2580;
    }

    LOG_D("valve_calibration_load success\r\n");
}

void ofsave(void)
{
    open_forward_target_position = ADC_GetValue(0);
    flash_set_key("open_forward_target_position",open_forward_target_position);
    LOG_I("open_forward_target_position set to %d\r\n",open_forward_target_position);
}
MSH_CMD_EXPORT(ofsave,ofsave);

void obsave(void)
{
    open_backward_target_position = ADC_GetValue(0);
    flash_set_key("open_backward_target_position",open_backward_target_position);
    LOG_I("open_backward_target_position set to %d\r\n",open_backward_target_position);
}
MSH_CMD_EXPORT(obsave,obsave);

void cfsave(void)
{
    close_forward_target_position = ADC_GetValue(0);
    flash_set_key("close_forward_target_position",close_forward_target_position);
    LOG_I("close_forward_target_position set to %d\r\n",close_forward_target_position);
}
MSH_CMD_EXPORT(cfsave,cfsave);

void cbsave(void)
{
    close_backward_target_position = ADC_GetValue(0);
    flash_set_key("close_backward_target_position",close_backward_target_position);
    LOG_I("close_backward_target_position set to %d\r\n",close_backward_target_position);
}
MSH_CMD_EXPORT(cbsave,cbsave);
