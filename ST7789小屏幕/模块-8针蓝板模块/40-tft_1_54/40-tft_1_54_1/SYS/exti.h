#ifndef __EXTI_H
#define __EXTI_H	 
#include "stm32f10x.h"  
  
//外部中断0服务函数
void EXTI15_10_IRQHandler(void);  
  
//初始化
void EXTI_GPIO_Config(void);  
void EXTI_INTERRUPT_Config(void);  
void EXTI_NVIC_Config(void);


void EXTI_EVENT_Config(void);

#endif
