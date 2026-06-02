#ifndef _I2C_H_
#define _I2C_H_
#include "sys.h"

//IO操作函数	 
#define I2C_SCL_SET    GPIO_SetBits(GPIOA,GPIO_Pin_6) //SCL
#define I2C_SCL_RESET  GPIO_ResetBits(GPIOA,GPIO_Pin_6) //SCL
#define I2C_SDA_SET    GPIO_SetBits(GPIOA,GPIO_Pin_7) //SDA
#define I2C_SDA_RESET  GPIO_ResetBits(GPIOA,GPIO_Pin_7) //SDA	 
#define I2C_READ_SDA   GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_7)  //输入SDA 

#define eeprom_wr_addr 0xa0
#define eeprom_rd_addr 0xa1
//IIC所有操作函数
void Init_I2C(void);                //初始化IIC的IO口	
void I2C_SDA_OUT(void);
void I2C_SDA_IN(void);
void I2C_Start(void);				//发送IIC开始信号
void I2C_Stop(void);	  			//发送IIC停止信号
void I2C_Send_Byte(u8 txd);			//IIC发送一个字节
u8 I2C_Read_Byte(unsigned char ack);//IIC读取一个字节
u8 I2C_Wait_Ack(void); 				//IIC等待ACK信号
void I2C_Ack(void);					//IIC发送ACK信号
void I2C_NAck(void);				//IIC不发送ACK信号

void I2C_Write_One_Byte(u8 daddr,u8 addr,u8 data);
u8 I2C_Read_One_Byte(u8 daddr,u8 addr);	  

void Write_Byte_E2PROM(unsigned char addr, unsigned char dat);
unsigned char Read_Byte_E2PROM(unsigned char addr);
#endif

