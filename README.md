# STM32_APDS9960_Gesture_Control
基于STM32F103和APDS9960手势传感器的安全警示系统。手势滑动切换安全等级界面，LED实时反馈。

![演示](Images/demo.gif)

## 项目简介
基于STM32F103和APDS9960手势传感器，实现手势滑动切换安全等级界面，LED实时反馈状态。

## 功能演示
| 手势 | 功能 | LED反馈 |
|------|------|---------|
| 向左滑 | 下一个菜单 |
| 向右滑 | 上一个菜单 |
| 安全界面 | - | LED1常亮 |
| 注意界面 | - | LED2常亮 |
| 警告界面 | - | LED2闪烁 |
| 危险界面 | - | LED3常亮 |

## 硬件连接图
![接线图](Images/wiring.jpg)

### 引脚对照表
| APDS9960 | STM32 | 说明 |
|-----|-------|------|
| VCC | 3.3V| 电源 |
| GND | GND | 地 |
| SDA | PB7 | I2C数据 |
| SCL | PB6 | I2C时钟 |
| VL  | 3.3V| LED电源（必须接！） |
| INT | 不接| 中断引脚 |

### LED连接
| LED | STM32 | 说明 |
|-----|-------|------|
| LED1(绿) | PA1 | 安全 |
| LED2(黄) | PA2 | 注意/警告指示 |
| LED3(红) | PA3 | 危险指示 |

### OLED连接
| OLED | STM32 |
|------|-------|
| VCC  | 3.3V  |
| GND  | GND   |
| SDA  | PB9   |
| SCL  | PB8   |

## 核心代码

### 手势识别（最终稳定版）
```c
void Menu_key_set(void)
{
    static uint8_t last_l=0, last_r=0;
    static uint8_t gesture_lock = 0;
    static uint8_t lock_count = 0;
    static uint8_t init = 0;
    
    uint8_t l = read_reg(0xFE);
    uint8_t r = read_reg(0xFF);
    
    if(init == 0) {
        last_l = l; last_r = r; init = 1; return;
    }
    
    int16_t diff_l = l - last_l;
    int16_t diff_r = r - last_r;
    
    if(!gesture_lock) {
        if(diff_r > 82 && diff_r > diff_l) {
            func_index = (func_index + 1) % 4;
            show_menu(func_index);
            gesture_lock = 1;
        }
        else if(diff_l > 82 && diff_l > diff_r) {
            func_index = (func_index + 3) % 4;
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

> 📌 完整代码见：
