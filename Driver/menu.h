#ifndef __MENU_H
#define __MENU_H

#include "stm32f10x.h"

// 声明外部变量
extern uint8_t func_index;

// 函数声明
void Menu_key_set(void);
void SS(void);
void LED_KS(uint8_t id);
void LED_JS(void);
void show_menu(uint8_t index);   // 添加这行

#endif