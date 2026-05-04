#include "stm32f10x.h"
#include "apds9960.h"
#include "Delay.h"
#include "apds9960_IIC.h"

extern int abs(int __x); 

void Wire_begin_(void);

/* Container for gesture data */
typedef struct gesture_data_type {
    uint8_t u_data[32];
    uint8_t d_data[32];
    uint8_t l_data[32];
    uint8_t r_data[32];
    uint8_t index;
    uint8_t total_gestures;
    uint8_t in_threshold;
    uint8_t out_threshold;
} gesture_data_type;

/* Members */
gesture_data_type gesture_data_;

int gesture_ud_delta_;
int gesture_lr_delta_;
int gesture_ud_count_;
int gesture_lr_count_;
int gesture_near_count_;
int gesture_far_count_;
int gesture_state_;
int gesture_motion_;

void SparkFun_APDS9960(void)
{
    gesture_ud_delta_ = 0;
    gesture_lr_delta_ = 0;
    gesture_ud_count_ = 0;
    gesture_lr_count_ = 0;
    gesture_near_count_ = 0;
    gesture_far_count_ = 0;
    gesture_state_ = 0;
    gesture_motion_ = DIR_NONE;
}

bool SparkFun_APDS9960_init(void)
{
    uint8_t id = 0;

    /* Initialize I2C */
    Wire_begin_();
    Delay_ms(50);  // 等待I2C稳定
    
    /* 读取器件ID - 检测硬件连接 */
    if( !wireReadDataByte(APDS9960_ID, &id) ) 
    {
        return false;
    }    
    
    if( !((id == APDS9960_ID_1 || id == APDS9960_ID_2)) ) 
    {
        return false;  // ID不对，硬件有问题
    }
    
    /* 失能所有模式 */
    if( !setMode(ALL, OFF) )
    {
        return false;
    }
    Delay_ms(10);
    
    /* 设置手势传感器寄存器默认值 */
    if( !setGestureEnterThresh(DEFAULT_GPENTH) ) 
    {
        return false;
    } 

    if( !setGestureExitThresh(DEFAULT_GEXTH) ) 
    {
        return false;
    }
    
    if( !wireWriteDataByte(APDS9960_GCONF1, DEFAULT_GCONF1) ) 
    { 
        return false;
    }
    
    if( !setGestureGain(DEFAULT_GGAIN) ) 
    {
        return false;
    }   
    
    if( !setGestureLEDDrive(DEFAULT_GLDRIVE) ) 
    {
        return false;
    }     
    
    if( !setGestureWaitTime(DEFAULT_GWTIME) ) 
    {
        return false;
    } 
    
    if( !wireWriteDataByte(APDS9960_GOFFSET_U, DEFAULT_GOFFSET) ) 
    {
        return false;
    }   

    if( !wireWriteDataByte(APDS9960_GOFFSET_D, DEFAULT_GOFFSET) ) 
    {
        return false;
    }
    
    if( !wireWriteDataByte(APDS9960_GOFFSET_L, DEFAULT_GOFFSET) ) 
    {
        return false;
    }         
    
    if( !wireWriteDataByte(APDS9960_GOFFSET_R, DEFAULT_GOFFSET) ) 
    {
        return false;
    }
    
    if( !wireWriteDataByte(APDS9960_GPULSE, DEFAULT_GPULSE) ) 
    {
        return false;
    }
    
    if( !wireWriteDataByte(APDS9960_GCONF3, DEFAULT_GCONF3) ) 
    {
        return false;
    }
    
    if( !setGestureIntEnable(DEFAULT_GIEN) ) 
    {
        return false;
    }
    
    return true;  
}

uint8_t getMode(void)
{
    uint8_t enable_value = 0;
    
    if( !wireReadDataByte(APDS9960_ENABLE, &enable_value) )     
    {
        return 0;  // 读取失败返回0
    }
    
    return enable_value;
}

bool setMode(int8_t mode, uint8_t enable)
{
    uint8_t reg_val;
    uint8_t verify_val;

    reg_val = getMode();
    if( reg_val == 0xFF )  // 检查错误
    {
        return false;
    }
    
    enable = enable & 0x01;
    if((mode >= 0) && (mode <= 6))
    {
        if(enable)
        {
            reg_val |= (1 << mode);
        } 
        else
        {
            reg_val &= ~(1 << mode);
        }
    } 
    else if( mode == ALL )
    {
        if (enable) 
        {
            reg_val = 0x7F;
        } 
        else
        {
            reg_val = 0x00;
        }
    }
    
    if( !wireWriteDataByte(APDS9960_ENABLE, reg_val) )     
    {
        return false;
    }
    
    // 验证写入
    Delay_ms(5);
    if( !wireReadDataByte(APDS9960_ENABLE, &verify_val) )
    {
        return false;
    }
    
    if(verify_val != reg_val)
    {
        return false;
    }
    
    return true;
}
/*
 * - 删除了冗余的setMode分步使能，改为一次性写入0x4D
 * - 增加了GPULSE手势脉冲配置（原版缺失）
 * - 每步增加延时，确保寄存器写入稳定
 * - 配置顺序更合理：先关→配置→使能
 * 
 * 修改者：
 * 修改日期：2026-05-04
 */

bool enableGestureSensor(bool interrupts)
{
    resetGestureParameters();
    
    // 关闭所有模式
    wireWriteDataByte(APDS9960_ENABLE, 0x00);
    Delay_ms(100);
    
    // 设置等待时间
    wireWriteDataByte(APDS9960_WTIME, 0xFF);
    Delay_ms(10);
    
    // 设置接近脉冲（适中）
    wireWriteDataByte(APDS9960_PPULSE, 0x89);
    Delay_ms(10);
    
    // 设置手势脉冲（适中）
    wireWriteDataByte(APDS9960_GPULSE, 0xC9);
    Delay_ms(10);
    
    // 设置手势进入阈值
    wireWriteDataByte(APDS9960_GPENTH, 0x28);  // 40
    Delay_ms(10);
    
    // 设置手势退出阈值
    wireWriteDataByte(APDS9960_GEXTH, 0x1E);   // 30
    Delay_ms(10);
    
    // 设置GCONF1
    wireWriteDataByte(APDS9960_GCONF1, 0x40);
    Delay_ms(10);
    
    // 设置GCONF2（增益4x，LED电流100mA，等待2.8ms）
    wireWriteDataByte(APDS9960_GCONF2, 0x41);
    Delay_ms(10);
    
    // 设置手势模式
    wireWriteDataByte(APDS9960_GCONF4, 0x01);
    Delay_ms(10);
    
    // 使能电源、等待、接近、手势
    wireWriteDataByte(APDS9960_ENABLE, 0x4D);
    Delay_ms(100);
    
    return true;
}


bool isGestureAvailable(void)
{
    uint8_t val;
    
    if( !wireReadDataByte(APDS9960_GSTATUS, &val) ) 
    {
        return false;
    }
    
    val &= APDS9960_GVALID;
    
    if( val == 1) 
    {
        return true;
    } 
    else 
    {
        return false;
    }
}

int readGesture(void)
{
    uint8_t fifo_level = 0;
    int8_t bytes_read = 0;
    uint8_t fifo_data[128];
    uint8_t gstatus;
    int motion;
    int i;
    
    if(!isGestureAvailable() || !(getMode() & 0x41) ) 
    {
        return DIR_NONE;
    }
    
    while(1) 
    {
        Delay_ms(FIFO_PAUSE_TIME);
        
        if( !wireReadDataByte(APDS9960_GSTATUS, &gstatus) ) 
        {
            return ERROR;
        }
        
        if((gstatus & APDS9960_GVALID) == APDS9960_GVALID) 
        {
            if( !wireReadDataByte(APDS9960_GFLVL, &fifo_level) ) 
            {
                return ERROR;
            }
            
            if( fifo_level > 0) 
            {
                bytes_read = wireReadDataBlock(APDS9960_GFIFO_U, 
                                               (uint8_t*)fifo_data, 
                                               (fifo_level * 4) );
            
                if(bytes_read == -1) 
                {
                    return ERROR;
                }
                
                if( bytes_read >= 4 ) 
                {
                    for( i = 0; i < bytes_read; i += 4 ) 
                    {
                        gesture_data_.u_data[gesture_data_.index] = fifo_data[i + 0];
                        gesture_data_.d_data[gesture_data_.index] = fifo_data[i + 1];
                        gesture_data_.l_data[gesture_data_.index] = fifo_data[i + 2];
                        gesture_data_.r_data[gesture_data_.index] = fifo_data[i + 3];
                        gesture_data_.index++;
                        gesture_data_.total_gestures++;
                    }
                    
                    if( processGestureData() ) 
                    {
                        if( decodeGesture() ) 
                        {
                            // U-Turn Gestures
                        }
                    }
                    
                    gesture_data_.index = 0;
                    gesture_data_.total_gestures = 0;
                }
            }
        } 
        else 
        {
            Delay_ms(FIFO_PAUSE_TIME);
            decodeGesture();
            motion = gesture_motion_;
            resetGestureParameters();
            return motion;
        }
    }
}

bool enablePower(void)
{
    if( !setMode(POWER, 1) ) 
    {
        return false;
    }
    return true;
}

bool disablePower(void)
{
    if( !setMode(POWER, 0) ) 
    {
        return false;
    }
    return true;
}

void resetGestureParameters(void)
{
    gesture_data_.index = 0;
    gesture_data_.total_gestures = 0;
    
    gesture_ud_delta_ = 0;
    gesture_lr_delta_ = 0;
    
    gesture_ud_count_ = 0;
    gesture_lr_count_ = 0;
    
    gesture_near_count_ = 0;
    gesture_far_count_ = 0;
    
    gesture_state_ = 0;
    gesture_motion_ = DIR_NONE;
}

bool processGestureData(void)
{
    uint8_t u_first = 0;
    uint8_t d_first = 0;
    uint8_t l_first = 0;
    uint8_t r_first = 0;
    uint8_t u_last = 0;
    uint8_t d_last = 0;
    uint8_t l_last = 0;
    uint8_t r_last = 0;
    int ud_ratio_first;
    int lr_ratio_first;
    int ud_ratio_last;
    int lr_ratio_last;
    int ud_delta;
    int lr_delta;
    int i;

    if( gesture_data_.total_gestures <= 4 ) 
    {
        return false;
    }
    
    if( (gesture_data_.total_gestures <= 32) && (gesture_data_.total_gestures > 0) ) 
    {
        for( i = 0; i < gesture_data_.total_gestures; i++ ) 
        {        
            if( (gesture_data_.u_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.d_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.l_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.r_data[i] > GESTURE_THRESHOLD_OUT) ) 
            {
                u_first = gesture_data_.u_data[i];
                d_first = gesture_data_.d_data[i];
                l_first = gesture_data_.l_data[i];
                r_first = gesture_data_.r_data[i];
                break;
            }
        }
        
        if( (u_first == 0) || (d_first == 0) || (l_first == 0) || (r_first == 0) ) 
        {
            return false;
        }
        
        for( i = gesture_data_.total_gestures - 1; i >= 0; i-- ) 
        {
            if( (gesture_data_.u_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.d_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.l_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.r_data[i] > GESTURE_THRESHOLD_OUT) ) 
            {
                u_last = gesture_data_.u_data[i];
                d_last = gesture_data_.d_data[i];
                l_last = gesture_data_.l_data[i];
                r_last = gesture_data_.r_data[i];
                break;
            }
        }
    }
    
    ud_ratio_first = ((u_first - d_first) * 100) / (u_first + d_first);
    lr_ratio_first = ((l_first - r_first) * 100) / (l_first + r_first);
    ud_ratio_last = ((u_last - d_last) * 100) / (u_last + d_last);
    lr_ratio_last = ((l_last - r_last) * 100) / (l_last + r_last);
    
    ud_delta = ud_ratio_last - ud_ratio_first;
    lr_delta = lr_ratio_last - lr_ratio_first;
    
    gesture_ud_delta_ += ud_delta;
    gesture_lr_delta_ += lr_delta;
    
    if( gesture_ud_delta_ >= GESTURE_SENSITIVITY_1 )
    {
        gesture_ud_count_ = 1;
    } 
    else if( gesture_ud_delta_ <= -GESTURE_SENSITIVITY_1 ) 
    {
        gesture_ud_count_ = -1;
    } 
    else 
    {
        gesture_ud_count_ = 0;
    }
    
    if( gesture_lr_delta_ >= GESTURE_SENSITIVITY_1 ) 
    {
        gesture_lr_count_ = 1;
    } 
    else if( gesture_lr_delta_ <= -GESTURE_SENSITIVITY_1 ) 
    {
        gesture_lr_count_ = -1;
    }
    else 
    {
        gesture_lr_count_ = 0;
    }
    
    if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == 0) ) 
    {
        if( (abs(ud_delta) < GESTURE_SENSITIVITY_2) && (abs(lr_delta) < GESTURE_SENSITIVITY_2) )
        {    
            if( (ud_delta == 0) && (lr_delta == 0) ) 
            {
                gesture_near_count_++;
            }
            else if( (ud_delta != 0) || (lr_delta != 0) ) 
            {
                gesture_far_count_++;
            }
            
            if( (gesture_near_count_ >= 10) && (gesture_far_count_ >= 2) ) 
            {
                if( (ud_delta == 0) && (lr_delta == 0) ) 
                {
                    gesture_state_ = NEAR_STATE;
                }
                else if( (ud_delta != 0) && (lr_delta != 0) ) 
                {
                    gesture_state_ = FAR_STATE;
                }
                return true;
            }
        }
    } 
    else 
    {
        if( (abs((int)ud_delta) < GESTURE_SENSITIVITY_2) && (abs((int)lr_delta) < GESTURE_SENSITIVITY_2) ) 
        {
            if( (ud_delta == 0) && (lr_delta == 0) ) 
            {
                gesture_near_count_++;
            }
            
            if( gesture_near_count_ >= 10 ) 
            {
                gesture_ud_count_ = 0;
                gesture_lr_count_ = 0;
                gesture_ud_delta_ = 0;
                gesture_lr_delta_ = 0;
            }
        }
    }
    
    return false;
}

bool decodeGesture(void)
{
    if( gesture_state_ == NEAR_STATE )
    {
        gesture_motion_ = DIR_NEAR;
        return true;
    } 
    else if ( gesture_state_ == FAR_STATE )
    {
        gesture_motion_ = DIR_FAR;
        return true;
    }
    
    if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == 0) )
    {
        gesture_motion_ = DIR_UP;
    } 
    else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == 0) ) 
    {
        gesture_motion_ = DIR_DOWN;
    } 
    else if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == 1) ) 
    {
        gesture_motion_ = DIR_RIGHT;
    } 
    else if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == -1) ) 
    {
        gesture_motion_ = DIR_LEFT;
    }
    else if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == 1) ) 
    {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) 
        {
            gesture_motion_ = DIR_UP;
        } 
        else 
        {
            gesture_motion_ = DIR_RIGHT;
        }
    } 
    else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == -1) ) 
    {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) 
        {
            gesture_motion_ = DIR_DOWN;
        } 
        else 
        {
            gesture_motion_ = DIR_LEFT;
        }
    } 
    else if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == -1) ) 
    {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) 
        {
            gesture_motion_ = DIR_UP;
        } 
        else 
        {
            gesture_motion_ = DIR_LEFT;
        }
    } 
    else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == 1) ) 
    {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) 
        {
            gesture_motion_ = DIR_DOWN;
        }
        else 
        {
            gesture_motion_ = DIR_RIGHT;
        }
    } 
    else 
    {
        return false;
    }
    return true;
}

bool setLEDBoost(uint8_t boost)
{
    uint8_t val;
    
    if( !wireReadDataByte(APDS9960_CONFIG2, &val) ) {
        return false;
    }
    
    boost &= 0x03;
    boost = boost << 4;
    val &= 0xCF;
    val |= boost;
    
    if( !wireWriteDataByte(APDS9960_CONFIG2, val) ) {
        return false;
    }
    
    return true;
}    

bool setGestureEnterThresh(uint8_t threshold)
{
    if( !wireWriteDataByte(APDS9960_GPENTH, threshold) )        
    {
        return false;
    }
    return true;
}

bool setGestureExitThresh(uint8_t threshold)
{
    if( !wireWriteDataByte(APDS9960_GEXTH, threshold) ) {
        return false;
    }
    return true;
}

bool setGestureGain(uint8_t gain)
{
    uint8_t val;
    
    if( !wireReadDataByte(APDS9960_GCONF2, &val) ) 
    {
        return false;
    }
    
    gain &= 0x03;
    gain = gain << 5;  
    val &= 0x9F;
    val |= gain;
    
    if( !wireWriteDataByte(APDS9960_GCONF2, val) ) 
    {
        return false;
    }
    
    return true;
}

bool setGestureLEDDrive(uint8_t drive)
{
    uint8_t val;
    
    if( !wireReadDataByte(APDS9960_GCONF2, &val) ) 
    {
        return false;
    }
    
    drive &= 0x03;
    drive = drive << 3;
    val &= 0xE7;
    val |= drive;
    
    if( !wireWriteDataByte(APDS9960_GCONF2, val) ) 
    {
        return false;
    }
    
    return true;
}

bool setGestureWaitTime(uint8_t time)
{
    uint8_t val;
    
    if( !wireReadDataByte(APDS9960_GCONF2, &val))
    {
        return false;
    }
        
    time &= 0x07;
    val &= 0xF8;
    val |= time;
    
    if( !wireWriteDataByte(APDS9960_GCONF2, val) ) 
    {
        return false;
    }
    
    return true;
}

bool setGestureIntEnable(uint8_t enable)
{
    uint8_t val;
    
    if( !wireReadDataByte(APDS9960_GCONF4, &val) )
    {
        return false;
    }
    
    enable &= 0x01;
    enable = enable << 1;
    val &= 0xFD;
    val |= enable;
    
    if( !wireWriteDataByte(APDS9960_GCONF4, val) ) 
    {
        return false;
    }
    
    return true;
}

bool setGestureMode(uint8_t mode)
{
    uint8_t val;
    
    if( !wireReadDataByte(APDS9960_GCONF4, &val) )    
    {
        return false;
    }
    
    mode &= 0x01;
    val &= 0xFE;
    val |= mode;
    
    if( !wireWriteDataByte(APDS9960_GCONF4, val) ) {
        return false;
    }
    
    return true;
}

bool wireWriteDataByte(uint8_t reg, uint8_t val)
{
    IIC_Start(); 
    IIC_Send_Byte((APDS9960_I2C_ADDR<<1)|0);
    if(IIC_Wait_Ack())
    {
        IIC_Stop();         
        return false;        
    }
    IIC_Send_Byte(reg);
    IIC_Wait_Ack();
    IIC_Send_Byte(val);
    if(IIC_Wait_Ack())
    {
        IIC_Stop();     
        return false;         
    }         
    IIC_Stop();     
    return true;
}

bool wireReadDataByte(uint8_t reg, uint8_t *val)
{
    IIC_Start(); 
    IIC_Send_Byte((APDS9960_I2C_ADDR<<1)|0x00);
    if(IIC_Wait_Ack())
    {
        IIC_Stop();
        return false;
    }
    IIC_Send_Byte(reg);
    if(IIC_Wait_Ack())
    {
        IIC_Stop();
        return false;
    }
    IIC_Start();
    IIC_Send_Byte((APDS9960_I2C_ADDR<<1)|0x01);
    if(IIC_Wait_Ack())
    {
        IIC_Stop();
        return false;
    }
    *val = IIC_Read_Byte(0);
    IIC_Stop();
    return true;
}

int wireReadDataBlock(uint8_t reg, uint8_t *val, unsigned int len)
{
    unsigned char i = 0;
    IIC_Start(); 
    IIC_Send_Byte((APDS9960_I2C_ADDR<<1)|0);
    if(IIC_Wait_Ack())
    {
        IIC_Stop();         
        return -1;        
    }
    IIC_Send_Byte(reg);
    IIC_Wait_Ack();
    IIC_Start();
    IIC_Send_Byte((APDS9960_I2C_ADDR<<1)|1);
    IIC_Wait_Ack();
    while(len)
    {
        if(len == 1)
        {
            val[i] = IIC_Read_Byte(0);
        }
        else
        {
            val[i] = IIC_Read_Byte(1);
        }
        i++; 
        len--;
    }  
    IIC_Stop();
    return i;
}

void Wire_begin_(void)
{
    IIC_Init();
}
