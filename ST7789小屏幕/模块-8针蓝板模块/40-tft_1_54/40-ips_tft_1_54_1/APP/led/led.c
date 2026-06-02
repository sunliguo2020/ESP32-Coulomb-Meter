#include "led.h"

void LED_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); 
	GPIO_InitTypeDef GPIO_InitStructure;	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;				
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(GPIOB, &GPIO_InitStructure);  						
	GPIO_ResetBits(GPIOB,GPIO_Pin_9);
}


//输入：0 1 2 3 	
void LED(u8 led)
{
//	switch(led)
//	{
//		case 0://全亮
//			GPIO_SetBits(GPIOB,GPIO_Pin_9);
//			GPIO_SetBits(GPIOB,GPIO_Pin_5);
//			break;
//		case 1://1亮2灭
//			GPIO_SetBits(GPIOB,GPIO_Pin_9);
//			GPIO_ResetBits(GPIOB,GPIO_Pin_5);
//			break;
//		case 2://1灭2亮
//			GPIO_SetBits(GPIOB,GPIO_Pin_5);
//			GPIO_ResetBits(GPIOB,GPIO_Pin_9);
//			break;
//		case 3://全灭
//			GPIO_ResetBits(GPIOB,GPIO_Pin_9);
//			GPIO_ResetBits(GPIOB,GPIO_Pin_5);
//			break;
//		default:
//			break;
//	}
}


