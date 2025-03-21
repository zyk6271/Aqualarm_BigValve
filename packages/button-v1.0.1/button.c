/************************************************************
  * @brief   button_drive
  * @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0.1
  * @note    button.c
  ***********************************************************/

#include "button.h"

#ifdef PKG_USING_BUTTON

#define DBG_TAG "BUTTON"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/*******************************************************************
 *                          Variable Declaration                              
 *******************************************************************/

static struct button* Head_Button = RT_NULL;


/*******************************************************************
 *                         Function Declaration   
 *******************************************************************/
static void Print_Btn_Info(Button_t* btn);
static void Add_Button(Button_t* btn);


/************************************************************
  * @brief   Create a Button 
  * @param   name:button name 
  * @param   btn:button structure
  * @param   read_btn_level:Button trigger level reading function,
  *                 Return the level of the rt_uint8_t type by yourself
  * @param   btn_trigger_level:Button trigger level
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    RT_NULL
  ***********************************************************/
void Button_Create(const char *name,
                  Button_t *btn, 
                  rt_uint8_t(*read_btn_level)(void),
                  rt_uint8_t btn_trigger_level)
{
  if( btn == RT_NULL)
  {
    LOG_D("struct button is RT_NULL!");
  }
  
  memset(btn, 0, sizeof(struct button));      //Clear structure information
 
  rt_memcpy(btn->Name, name, BTN_NAME_MAX);    //button name
  
  btn->Button_State = NONE_TRIGGER;                     //Button status
  btn->Button_Last_State = NONE_TRIGGER;                //Button last status
  btn->Button_Trigger_Event = NONE_TRIGGER;             //Button trigger event
  btn->Read_Button_Level = read_btn_level;              //Button trigger level reading function
  btn->Button_Trigger_Level = btn_trigger_level;        //Button trigger level
  btn->Button_Last_Level = btn->Read_Button_Level();    //Button current level
  btn->Debounce_Time = 0;
  
  LOG_D("button create success!");
  
  Add_Button(btn);          //Added to the singly linked list when button created
 
}

/************************************************************
  * @brief   burron trigger events are attach to callback function
  * @param   btn:button structure
  * @param   btn_event:button events
  * @param   btn_callback : Callback handler after the button is triggered.Need user implementation
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  ***********************************************************/
void Button_Attach(Button_t *btn,Button_Event btn_event,Button_CallBack btn_callback)
{
  if( btn == RT_NULL)
  {
    LOG_D("struct button is RT_NULL!");
  }
  
  if(BUTTON_ALL_RIGGER == btn_event)
  {
    for(rt_uint8_t i = 0 ; i < number_of_event-1 ; i++)
      /*A callback function triggered by a button event ,Used to handle button events */
      btn->CallBack_Function[i] = btn_callback;   
  }
  else
  {
    btn->CallBack_Function[btn_event] = btn_callback; 
  }
}

/************************************************************
  * @brief   Delete an already created button
  * @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    RT_NULL
  ***********************************************************/
void Button_Delete(Button_t *btn)
{
  struct button** curr;
  for(curr = &Head_Button; *curr;) 
  {
    struct button* entry = *curr;
    if (entry == btn) 
    {
      *curr = entry->Next;
    } 
    else
    {
      curr = &entry->Next;
    }
  }
}

/************************************************************
  * @brief   Get Button Event Info
  * @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  ***********************************************************/
void Get_Button_EventInfo(Button_t *btn)
{
  for(rt_uint8_t i = 0 ; i < number_of_event-1 ; i++)
  {
    if(btn->CallBack_Function[i] != 0)
    {
      /* print */
      LOG_D("Button_Event:%d",i);
    }      
  } 
}

/************************************************************
  * @brief   Get Button Event
  * @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  ***********************************************************/
rt_uint8_t Get_Button_Event(Button_t *btn)
{
  return (rt_uint8_t)(btn->Button_Trigger_Event);
}

/************************************************************
  * @brief   Get Button State
  * @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  ***********************************************************/
rt_uint8_t Get_Button_State(Button_t *btn)
{
  return (rt_uint8_t)(btn->Button_State);
}

void Button_Resume(Button_t *btn)
{
    btn->Button_Pause = 0;
}

void Button_Pause(Button_t *btn)
{
    btn->Button_Pause = 1;
}

/************************************************************
  * @brief   button cycle processing function
  * @param   btn:button structure
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    This function must be called in a certain period. The recommended period is 20~50ms.
  ***********************************************************/
void Button_Cycle_Process(Button_t *btn)
{
    /* Get the current button level */
    rt_uint8_t current_level = (rt_uint8_t)btn->Read_Button_Level();

    /* Button level changes, debounce */
    if((current_level != btn->Button_Last_Level)&&(++(btn->Debounce_Time) >= BUTTON_DEBOUNCE_TIME))
    {
        /* Update current button level */
        btn->Button_Last_Level = current_level;

        /* button is pressed */
        btn->Debounce_Time = 0;

//        switch(btn->Button_State)
//        {
//        case NONE_TRIGGER:
//            if(current_level == btn->Button_Trigger_Level)
//            {
//                btn->Button_State = BUTTON_DOWN;
//                TRIGGER_CB(BUTTON_DOWN);
//            }
//            break;
//        case BUTTON_DOWN:
//            btn->Button_State = NONE_TRIGGER;
//            TRIGGER_CB(BUTTON_UP);
//            break;
//        }

        /* If the button is not pressed, change the button state to press (first press / double trigger) */
//        if(btn->Button_State == NONE_TRIGGER)
//        {
//            btn->Button_State = BUTTON_DOWM;
//            TRIGGER_CB(BUTTON_DOWM);
//        }
//        else if(btn->Button_State == BUTTON_DOWM)
//        {
//            btn->Button_State = NONE_TRIGGER;
//            TRIGGER_CB(BUTTON_UP);
//        }
        if(current_level == btn->Button_Trigger_Level)
        {
            //btn->Button_State = BUTTON_DOWM;
            TRIGGER_CB(BUTTON_DOWN);
        }
        else
        {
            //btn->Button_State = NONE_TRIGGER;
            TRIGGER_CB(BUTTON_UP);
        }
    }

}

/************************************************************
  * @brief   Traversing the way to scan the button without losing each button
  * @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    This function is called periodically, it is recommended to call 20-50ms once.
  ***********************************************************/
void Button_Process(void)
{
  struct button* pass_btn;
  for(pass_btn = Head_Button; pass_btn != RT_NULL; pass_btn = pass_btn->Next)
  {
      Button_Cycle_Process(pass_btn);
  }
}

/************************************************************
  * @brief   Search Button
  * @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    RT_NULL
  ***********************************************************/
void Search_Button(void)
{
  struct button* pass_btn;
  for(pass_btn = Head_Button; pass_btn != RT_NULL; pass_btn = pass_btn->Next)
  {
    LOG_D("button node have %s",pass_btn->Name);
  }
}

/************************************************************
  * @brief   Handle all button callback functions
  * @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    Not implemented yet
  ***********************************************************/
void Button_Process_CallBack(void *btn)
{
  rt_uint8_t btn_event = Get_Button_Event(btn);

  switch(btn_event)
  {
    case BUTTON_DOWN:
    {
      LOG_D("Add processing logic for your press trigger");
      break;
    }
    
    case BUTTON_UP:
    {
      LOG_D("Add processing logic for your trigger release");
      break;
    }
    
    case BUTTON_DOUBLE:
    {
      LOG_D("Add processing logic for your double-click trigger");
      break;
    }
    
    case BUTTON_LONG:
    {
      LOG_D("Add processing logic for your long press trigger");
      break;
    }
    
    case BUTTON_LONG_FREE:
    {
      LOG_D("Add processing logic for your long press trigger free");
      break;
    }
    
    case BUTTON_CONTINUOS:
    {
      LOG_D("Add your continuous trigger processing logic");
      break;
    }
    
    case BUTTON_CONTINUOS_FREE:
    {
      LOG_D("Add processing logic for your continuous trigger release");
      break;
    }
      
  } 
}


/**************************** The following is the internal call function ********************/

/************************************************************
  * @brief   Connect buttons with a single linked list
  * @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    RT_NULL
  ***********************************************************/
static void Add_Button(Button_t* btn)
{
  struct button *pass_btn = Head_Button;
  
  while(pass_btn)
  {
    pass_btn = pass_btn->Next;
  }
  
  btn->Next = Head_Button;
  Head_Button = btn;
}

#endif



