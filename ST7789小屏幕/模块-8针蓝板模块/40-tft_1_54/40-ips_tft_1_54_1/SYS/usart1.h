#ifndef __USART1_H
#define __USART1_H


#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include <stdio.h>

void Uart1_Init(u32 bound);
void USART_SendString(USART_TypeDef* USARTx, char *DataString);

#endif



