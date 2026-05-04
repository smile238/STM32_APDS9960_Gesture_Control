#include "stm32f10x.h"
#include "menu.h"
#include "LED.h"
#include "OLED.h"
#include "Delay.h"
#include "apds9960.h"
#include "apds9960_IIC.h"

uint8_t func_index = 0;  
uint8_t oled_first_init = 0;


// ========== 闪烁控制变量 ==========
static uint8_t SS_EN = 0;
static uint8_t LED_Num = 0;
static uint16_t JS = 0;
static uint8_t ZT = 0;

void LED_KS(uint8_t led_id)
{
    SS_EN = 1;
    LED_Num = led_id;
    JS = 0;
    ZT = 0;
    
    // 先关闭所有LED
    LED1_OFF();
    LED2_OFF();
    LED3_OFF();
    
    // 根据ID点亮对应LED（闪烁由SS函数控制）
    if(led_id == 1) LED1_ON();
    else if(led_id == 2) LED2_ON();
    else if(led_id == 3) LED3_ON();
}

void LED_JS(void)
{
    SS_EN = 0;
    LED1_OFF();
    LED2_OFF();
    LED3_OFF();
}

uint8_t read_reg(uint8_t reg)
{
    uint8_t val = 0;
    IIC_Start();
    IIC_Send_Byte(0x39 << 1 | 0);
    IIC_Wait_Ack();
    IIC_Send_Byte(reg);
    IIC_Wait_Ack();
    IIC_Start();
    IIC_Send_Byte(0x39 << 1 | 1);
    IIC_Wait_Ack();
    val = IIC_Read_Byte(0);
    IIC_Stop();
    return val;
}

// 菜单显示
// 菜单显示
// 菜单显示
void show_menu(uint8_t index)
{
    OLED_Clear();
    
    switch(index) {
        case 0:  // 安全 - LED1常亮
            OLED_ShowCHINESE(2,2,0);
            OLED_ShowCHINESE(2,5,1);
            // 停止所有闪烁，LED1常亮
            SS_EN = 0;
            LED1_ON();
            LED2_OFF();
            LED3_OFF();
            break;
            
        case 1:  // 注意 - LED2常亮
            OLED_ShowCHINESE(1,2,2);
            OLED_ShowCHINESE(1,5,3);
            OLED_ShowCHINESE(3,2,4);
            OLED_ShowCHINESE(3,3,5);
            OLED_ShowCHINESE(3,4,6);
            OLED_ShowCHINESE(3,5,7);
            // 停止所有闪烁，LED2常亮
            SS_EN = 0;
            LED1_OFF();
            LED2_ON();
            LED3_OFF();
            break;
            
        case 2:  // 警告 - LED2闪烁
            OLED_ShowCHINESE(1,2,8);
            OLED_ShowCHINESE(1,5,9);
            OLED_ShowCHINESE(3,2,5);
            OLED_ShowCHINESE(3,3,10);
            OLED_ShowCHINESE(3,4,6);
            OLED_ShowCHINESE(3,5,7);
            // LED2闪烁
            SS_EN = 1;
            LED_Num = 2;
            ZT = 0;
            LED1_OFF();
            LED3_OFF();
            break;
            
        case 3:  // 危险 - LED3常亮
            OLED_ShowCHINESE(1,2,11);
            OLED_ShowCHINESE(1,5,12);
            OLED_ShowCHINESE(3,1,5);
            OLED_ShowCHINESE(3,2,10);
            OLED_ShowCHINESE(3,3,13);
            OLED_ShowCHINESE(3,4,14);
            OLED_ShowCHINESE(3,5,6);
            OLED_ShowCHINESE(3,6,7);
            // 停止所有闪烁，LED3常亮
            SS_EN = 0;
            LED1_OFF();
            LED2_OFF();
            LED3_ON();
            break;
    }
}

void SS(void)
{
    static uint16_t tick = 0;
    
    if(!SS_EN) return;  // SS_EN=0时不闪烁
    
    tick++;
    if(tick>=15)  // 约100ms翻转一次
    {
        tick = 0;
        ZT = !ZT;  
        
        if(ZT)
        {
            if(LED_Num == 1) LED1_ON();
            else if(LED_Num == 2) LED2_ON();
            else if(LED_Num == 3) LED3_ON();
        }
        else
        {
            if(LED_Num == 1) LED1_OFF();
            else if(LED_Num == 2) LED2_OFF();
            else if(LED_Num == 3) LED3_OFF();
        }
    }
}

/* 2026-05-04 
 * 修改：Menu_key_set 手势识别最终优化版
 * - 使用原始差值直接判断，无滤波
 * - 向左滑条件：diff_r > 82 && diff_r > diff_l（R值突变）
 * - 向右滑条件：diff_l > 82 && diff_l > diff_r（L值突变）
 * - 手势锁防抖：lock_count > 20 解锁
 * 
 * 测试结果：误触发率从35%降到25%，响应速度快
 * 修改者：胡庭艳
 */

// 菜单切换函数
void Menu_key_set(void)
{
    static uint8_t last_l=0, last_r=0;
    static uint8_t gesture_lock = 0;
    static uint8_t lock_count = 0;
    static uint8_t init = 0;
    
    uint8_t l = read_reg(0xFE);
    uint8_t r = read_reg(0xFF);
    
    if(init == 0) {
        last_l = l;
        last_r = r;
        init = 1;
        return;
    }
    
    int16_t diff_l = l - last_l;
    int16_t diff_r = r - last_r;
    
    if(!gesture_lock) {
        // 向左滑：R值变大（因为手从右向左，右边传感器先感应到）
        if(diff_r > 82 && diff_r > diff_l) {
            func_index++;
            if(func_index > 3) func_index = 0;
            show_menu(func_index);
            gesture_lock = 1;
        }
        // 向右滑：L值变大（因为手从左向右，左边传感器先感应到）
        else if(diff_l > 82 && diff_l > diff_r) {
            if(func_index > 0) func_index--;
            else func_index = 3;
            show_menu(func_index);
            gesture_lock = 1;
        }
    }
    
    if(gesture_lock) {
        lock_count++;
        if(lock_count > 20) {
            gesture_lock = 0;
            lock_count = 0;
        }
    }
    
    last_l = l;
    last_r = r;
}