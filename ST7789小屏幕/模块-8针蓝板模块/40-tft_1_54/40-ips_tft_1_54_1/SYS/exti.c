#include "exti.h"
#include "led.h"
#include "delay.h"
extern int flg;
// GPIO初始化
void EXTI_GPIO_Config(void)  
{  
    GPIO_InitTypeDef GPIO_InitStructure;  
		//时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);  
  
    //PA0   
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;  
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //上拉输入  
    GPIO_Init(GPIOA, &GPIO_InitStructure);  
}  
  
// EXTI初始化
void EXTI_INTERRUPT_Config(void)  
{  
	EXTI_InitTypeDef EXTI_InitStructure;  

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource11);	//中断线0连接GPIOA.0
	// 外部中断EXTI 线0  
	//EXTI_Line0 对应脚pA0、pB0   初始化绑定
	EXTI_InitStructure.EXTI_Line = EXTI_Line11;  
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;  

	// 上升沿触发  EXTI_Trigger_Rising  下降沿触发  EXTI_Trigger_Falling   双沿触发  EXTI_Trigger_Rising_Falling   软件触发   
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //上升沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;  
	EXTI_Init(&EXTI_InitStructure);  

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource4);
	EXTI_InitStructure.EXTI_Line=EXTI_Line4;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //上升沿触发
	EXTI_Init(&EXTI_InitStructure);	  	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource4);
	EXTI_InitStructure.EXTI_Line=EXTI_Line4;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //上升沿触发
	EXTI_Init(&EXTI_InitStructure);	  	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器


}  
  
// NVIC初始化
void EXTI_NVIC_Config(void)  
{  
	NVIC_InitTypeDef NVIC_InitStructure;  

	//中断组为2
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  

	//配置EXTI0中断优先级  
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  
	NVIC_Init(&NVIC_InitStructure);  
}  
  
//外部0服务
void EXTI15_10_IRQHandler(void)  
{   
	if (EXTI_GetITStatus(EXTI_Line11) != RESET)  
	{  
		//    
		flg++;
		if(flg==2)flg=0;delay_ms(300);
		//清除中断标志位   
		EXTI_ClearITPendingBit(EXTI_Line11);  
	}  
}




// EXTI初始化
void EXTI_EVENT_Config(void)  
{  
    EXTI_InitTypeDef EXTI_InitStructure;  

		GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);	//中断线0连接GPIOA.0
    // 外部中断EXTI 线0  
    //EXTI_Line0 对应脚pA0、pB0   初始化绑定
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;  
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Event;  

    // 上升沿触发  EXTI_Trigger_Rising  下降沿触发  EXTI_Trigger_Falling   双沿触发  EXTI_Trigger_Rising_Falling   软件触发   
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //下降沿触发
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;  
    EXTI_Init(&EXTI_InitStructure);  

 

}

