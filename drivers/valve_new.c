//#include "rtthread.h"
//#include "rtdevice.h"
//#include "pin_config.h"
//#include "valve.h"
//#include "flashwork.h"
//
//#define DBG_TAG "VALVE_NEW"
//#define DBG_LVL DBG_LOG
//#include <rtdbg.h>
//
//#define DETECT_SAMPLE_CNT   32     // 死区检测采样次数
//#define DETECT_SAMPLE_TIME  5000   // 死区检测采样时间
//
//typedef enum
//{
//    VALVE_CONTROL_OFF,        // 阀门关闭
//    VALVE_CONTROL_ON,         // 阀门开启
//} ValveControl;
//
//typedef enum
//{
//    VALVE_STATE_IDLE,        // 空闲状态
//    VALVE_STATE_OPENING,     // 正在开启
//    VALVE_STATE_CLOSING,     // 正在关闭
//    VALVE_STATE_BRAKING,      // 刹车状态
//} ValveState;
//
//typedef enum
//{
//    DIR_STOP,        // 停止
//    DIR_FORWARD,     // 正转(开阀方向)
//    DIR_REVERSE      // 反转(关阀方向)
//} ValveDirection;
//
//static struct {
//    ValveState state;            // 当前状态
//    ValveDirection dir;          // 运动方向
//    ValveControl control;        // 开启状态
//    uint32_t target_pos;         // 目标位置
//    uint32_t adc_samples[DETECT_SAMPLE_CNT];    // ADC采样缓存
//    uint8_t sample_cnt;          // 采样计数
//    uint8_t sample_time;         // 采样计时
//    rt_bool_t manual_mode;       // 手动模式标志
//    rt_bool_t dead_detect_mode;  // 死区检测模式标志
//} valve_ctrl;
//
//rt_timer_t valve_brake_timer;      // 刹车定时器
//rt_timer_t valve_detect_timer;     // 死区检测定时器
//rt_timer_t valve_watch_timer;      // 位置检测定时器
//
//void valve_init(void)
//{
//    rt_pin_mode(MOTO_OUTPUT1_PIN, PIN_MODE_OUTPUT);
//    rt_pin_mode(MOTO_OUTPUT2_PIN, PIN_MODE_OUTPUT);
//    rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
//    rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
//    rt_pin_mode(MOTO_CLOSE_STATUS_PIN, PIN_MODE_OUTPUT);
//    rt_pin_mode(MOTO_OPEN_STATUS_PIN, PIN_MODE_OUTPUT);
//    rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
//    rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
//}
//
//static void set_motor_output(ValveDirection dir)
//{
//    switch(dir)
//    {
//    case DIR_STOP:
//        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
//        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
//        break;
//    case DIR_FORWARD:
//        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
//        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
//        break;
//    case DIR_REVERSE:
//        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
//        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
//        break;
//    default:
//        break;
//    }
//    valve_ctrl.dir = dir;
//}
//
//static void brake_timeout_cb(void *param)
//{
//    if(valve_ctrl.state == VALVE_STATE_BRAKING)
//    {
//        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
//        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
//        valve_ctrl.state = VALVE_STATE_IDLE;
//        LOG_I("Brake released");
//    }
//}
//
//static void deadzone_detect_timer_cb(void *param)
//{
//    if(valve_ctrl.sample_time++ >= DETECT_SAMPLE_TIME)
//    {
////        rt_timer_stop(detect_timer);
////        if(wire_button_level_read())
////        {
////            valve_close();
////        }
////        else
////        {
////            valve_open();
////        }
//    }
//
//    // 采集ADC值
//    if(valve_ctrl.sample_cnt < DETECT_SAMPLE_CNT)
//    {
//        valve_ctrl.adc_samples[valve_ctrl.sample_cnt++] = ADC_GetValue(0);
//        return;
//    }
//
//    // 计算方差判断死区
//    if(valve_deadzone_check(valve_ctrl.adc_samples, DETECT_SAMPLE_CNT))
//    {
//        LOG_W("Dead zone detected!");
//        valve_ctrl.sample_time = 0;
//    }
//    valve_ctrl.sample_cnt = 0;
//}
//
//void valve_stop(void)
//{
//    rt_timer_stop(valve_watch_timer);
//    rt_timer_stop(valve_detect_timer);
//    rt_timer_start(valve_brake_timer);
//    rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
//    rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
//    valve_ctrl.state = VALVE_STATE_BRAKING;
//}
//
//static void valve_position_handle_timer_cb(void *param)
//{
//    uint32_t valve_current_position = ADC_GetValue(0);
//    if(valve_ctrl.control == VALVE_CONTROL_ON)
//    {
//        if(valve_ctrl.dir == DIR_FORWARD)
//        {
//            if(valve_current_position < open_forward_target_position - deviation_value)
//            {
//                valve_stop();
//                rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
//                rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_HIGH);
//                LOG_I("valve_position_watch valve stop,code 1,open_forward_target_position %d\r\n",open_forward_target_position);
//            }
//        }
//        if(valve_ctrl.dir == DIR_REVERSE)
//        {
//            if(valve_current_position > open_backward_target_position + deviation_value)
//            {
//                valve_stop();
//                rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
//                rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_HIGH);
//                LOG_I("valve_position_watch valve stop,code 2,open_backward_target_position %d\r\n",open_backward_target_position);
//            }
//        }
//    }
//    else
//    {
//        if(valve_ctrl.dir == DIR_FORWARD)
//        {
//            if(valve_current_position < close_forward_target_position - deviation_value)
//            {
//                valve_stop();
//                rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_HIGH);
//                rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
//                LOG_I("valve_position_watch valve stop,code 3,close_forward_target_position %d\r\n",close_forward_target_position);
//            }
//        }
//        if(valve_ctrl.dir == DIR_REVERSE)
//        {
//            if(valve_current_position > close_backward_target_position + deviation_value)
//            {
//                valve_stop();
//                rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_HIGH);
//                rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
//                LOG_I("valve_position_watch valve stop,code 4,close_backward_target_position %d\r\n",close_backward_target_position);
//            }
//        }
//    }
//}
//
//rt_err_t valve_reverse_proetction(ValveDirection new_dir)
//{
//    if(new_dir != valve_ctrl.dir && valve_ctrl.dir != DIR_STOP)//存在换向
//    {
//        valve_ctrl.state = VALVE_STATE_BRAKING;
//        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
//        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
//        rt_thread_mdelay(500);
//        valve_ctrl.state = VALVE_STATE_IDLE;
//        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
//        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_LOW);
//        LOG_W("Direction change blocked!");
//        return RT_ERROR;
//    }
//    else
//    {
//        return RE_EOK;
//    }
//
//}
//
//void valve_open(void)
//{
//    if(valve_ctrl.manual_mode == 1 || valve_ctrl.dead_detect_mode == 1)
//    {
//        return;
//    }
//
//    uint32_t position = ADC_GetValue(0);
//    if (position >= 0 && position < open_forward_target_position)
//    {
//        dir = DIR_REVERSE;
//    }
//    else if (position > open_backward_target_position && position < 4096)
//    {
//        dir = DIR_FORWARD;
//    }
//
//    if(valve_reverse_proetction(dir) == RT_EOK)
//    {
//        valve_ctrl.state = VALVE_STATE_OPENING;
//        rt_pin_write(MOTO_CLOSE_STATUS_PIN, PIN_LOW);
//        rt_pin_write(MOTO_OPEN_STATUS_PIN, PIN_LOW);
//        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_LOW);
//        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
//    }
//    else
//    {
//
//    }
//
////    if(valve_reverse_proetction(dir) == RT_EOK)
////    {
////        valve_run(dir);
////    }
////    else
////    {
////        pre_run_status = dir;
////        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
////        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
////        rt_timer_start(delay_timer);
////    }
//}
//
//void valve_manually_control(uint8_t value)
//{
//    valve_ctrl.manual_mode = value;
//    if(valve_ctrl.manual_mode)
//    {
//        pre_run_status = 3;
//        rt_pin_write(MOTO_OUTPUT1_PIN, PIN_HIGH);
//        rt_pin_write(MOTO_OUTPUT2_PIN, PIN_HIGH);
//        rt_timer_stop(run_timer);
//        rt_timer_stop(detect_timer);
//        rt_timer_start(delay_timer);
//    }
//}
