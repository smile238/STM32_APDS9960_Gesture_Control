#include "stm32f10x.h"
#include "menu.h"
#include "LED.h"
#include "OLED.h"
#include "Delay.h"
#include "apds9960.h"
#include "apds9960_IIC.h"

int main(void)
{	
    OLED_Init();
    LED_Init();
    IIC_Init();
    
    SparkFun_APDS9960();
    SparkFun_APDS9960_init();
    enableGestureSensor(true);
    
    OLED_Clear();
    show_menu(0);

    
  while(1)
  {
    Menu_key_set();		
    SS();           // ← 必须有这行，LED才会闪烁
    Delay_ms(30);
  }
}