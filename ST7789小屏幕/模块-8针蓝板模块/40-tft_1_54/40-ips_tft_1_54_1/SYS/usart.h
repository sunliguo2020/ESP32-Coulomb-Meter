#ifndef __USART_H
#define __USART_H

#include "sys.h" 
#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "stdio.h"
#include "stdarg.h"
#include "string.h"


#define USART_REC_LEN  			200  	//定义最大接收字节数 200
#define USART2_MAX_RECV_LEN		400					//最大接收缓存字节数
#define USART2_MAX_SEND_LEN		400					//最大发送缓存字节数
#define USART2_RX_EN 			1								//0,不接收;1,接收


void Uart1_Init(u32 bound);
void Uart2_Init(u32 bound);
void Uart3_Init(u32 bound);
void USART_SendString(USART_TypeDef* USARTx, char *DataString);

void u2_printf(char* fmt,...);

#endif



