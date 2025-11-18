#include "rtthread.h"
#include "rtdevice.h"
#include "pin_config.h"
#include <agile_button.h>

#define DBG_TAG "sign"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

agile_btn_t *switch_button = RT_NULL;
agile_btn_t *wire_button = RT_NULL;

uint8_t switch_button_level_read(void)
{
    return rt_pin_read(KEY_HAND_PIN);
}

uint8_t wire_button_level_read(void)
{
    return rt_pin_read(KEY_WIRE_PIN);
}

void switch_button_up_callback(agile_btn_t *btn)
{
    rt_kprintf("switch_button_up_callback\r\n");
    valve_manually(1);
}

void switch_button_down_callback(agile_btn_t *btn)
{
    rt_kprintf("switch_button_down_callback\r\n");
    valve_manually(0);
    valve_position_reset();
}

void wire_button_up_callback(agile_btn_t *btn)
{
    rt_kprintf("wire_button_up_callback\r\n");
    valve_open();
}

void wire_button_down_callback(agile_btn_t *btn)
{
    rt_kprintf("wire_button_down_callback\r\n");
    valve_close();
}

void button_init(void)
{
    switch_button = agile_btn_create(KEY_HAND_PIN, PIN_HIGH, PIN_MODE_INPUT);
    wire_button = agile_btn_create(KEY_WIRE_PIN, PIN_LOW, PIN_MODE_INPUT);

    agile_btn_set_event_cb(switch_button, BTN_PRESS_DOWN_EVENT, switch_button_up_callback);
    agile_btn_set_event_cb(switch_button, BTN_PRESS_UP_EVENT, switch_button_down_callback);

    agile_btn_set_event_cb(wire_button, BTN_PRESS_DOWN_EVENT, wire_button_up_callback);
    agile_btn_set_event_cb(wire_button, BTN_PRESS_UP_EVENT, wire_button_down_callback);

    agile_btn_start(switch_button);
    agile_btn_start(wire_button);
}
