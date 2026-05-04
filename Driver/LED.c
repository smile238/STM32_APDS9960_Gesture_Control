#include "stm32f10x.h"                  // Device header

void LED_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//开启时钟
	
	GPIO_InitTypeDef LED;//结构体变量
	LED.GPIO_Mode= GPIO_Mode_Out_PP;//推挽输出
	LED.GPIO_Pin= GPIO_Pin_1| GPIO_Pin_2|GPIO_Pin_3;//设置端口
	LED.GPIO_Speed= GPIO_Speed_50MHz;//输出速度
	GPIO_Init(GPIOA,&LED);//初始化
	
	GPIO_SetBits(GPIOA,GPIO_Pin_1| GPIO_Pin_2|GPIO_Pin_3);//我是LED接的是电源，GPIO输出高电平，LED灭（无压差）
}

void LED1_ON(void)//低电平亮
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_1);
}

void LED1_OFF(void)//高电平灭
{
	GPIO_SetBits(GPIOA,GPIO_Pin_1);
}


void LED2_ON(void)//低电平亮
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_2);
}

void LED2_OFF(void)//高电平灭
{
	GPIO_SetBits(GPIOA,GPIO_Pin_2);
}


void LED3_ON(void)//低电平亮
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_3);
}

void LED3_OFF(void)//高电平灭
{
	GPIO_SetBits(GPIOA,GPIO_Pin_3);
}


