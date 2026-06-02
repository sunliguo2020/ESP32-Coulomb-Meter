#include "timer.h"

extern u8 sec,min,hour;
extern u16 count;

extern u16 USART2_RX_STA;
//对于72MHZ的频率，500ms中断一次，这两个参数设置如下：
//TIM_Prescaler=7199;   //预分频值  7200
//TIM_Period=4999;    //下一个更新事件装入活动的自动重装载寄存器周期的值
//分频7200，用72000000/7200=10000Hz
//此时的周期就是1/10000=0.0001s
//500ms=0.0001s*5000次  1s=0.0001s*10000次
//5000-1=4999次         10000-1=9999次
//即通过4999次系统的运行，就是500ms  通过9999次系统的运行，就是1s

//定时0.5s 4999 7199
void Timer3_Init(u16 arr,u16 psc)
{

  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能
	
	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器


	TIM_Cmd(TIM3, ENABLE);  //使能TIMx					 

}

void Timer2_Init(u16 arr,u16 pse)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	
	TIM_TimeBaseInitStruct.TIM_Period =  arr;
	TIM_TimeBaseInitStruct.TIM_Prescaler =  pse;
	TIM_TimeBaseInitStruct.TIM_CounterMode =  TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStruct);
	
	NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority =2;
	NVIC_Init(&NVIC_InitStruct);
	
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
	
	TIM_Cmd(TIM2,DISABLE);
}

//时钟定时
void TIM3_IRQHandler(void)
{	
	sec++;
	Timers();
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
}


//定时器3中断服务程序
//void TIM3_IRQHandler(void)   //TIM3中断
//{
//	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //检查TIM3更新中断发生与否
//	{
//		USART2_RX_STA|=1<<15;	//标记接收完成
//		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //清除TIMx更新中断标志 
//		TIM_Cmd(TIM3, DISABLE);  //关闭TIM3
//	}
//}

void TIM2_IRQHandler(void)
{	
	count++;
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
}

void Timers(void)
{
	if(sec >= 60)
	{
		sec=0;
		min++;
		if(min >= 60) 
		{
			min=0;
			hour++;
			if(hour >= 24) 
			{
				hour = 0;
			}
		}	
	}
}

int CountTime(u8 h,u8 m)
{
 if(h > hour)
	 return (h - hour - 1) * 3600 + (60 - min + m) * 60;
 else if(h < hour)
	 return (23 - hour + h) * 3600 + (60 - min + m) * 60;
 else
	 if(m >= min)
		 return (m - min) * 60;
	 else
		 return (23 - hour + h) * 3600 + (60 - min + m) * 60;
}

