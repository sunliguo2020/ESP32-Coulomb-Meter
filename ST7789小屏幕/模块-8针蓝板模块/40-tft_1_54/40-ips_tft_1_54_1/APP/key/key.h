#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h"

#define KEY0  GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_11)//读取按键0
#define KEY1  GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_12)//读取按键1
#define KEY2  GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)//读取按键2 
#define KEY3  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_3)//读取按键3

#define KEY4  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_4)//读取按键4
#define KEY5  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_5)//读取按键5
#define KEY6  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_6)//读取按键6
#define KEY7  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7)//读取按键7

void KEY_Init(int t);
void KEY_Init1(void);
int Get_KEY_Num(void);
int READ_KEY_Num(void);
#endif

