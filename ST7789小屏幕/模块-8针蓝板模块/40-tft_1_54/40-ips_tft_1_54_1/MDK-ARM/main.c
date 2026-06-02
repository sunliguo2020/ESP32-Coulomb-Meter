#include "sys.h"
#include "delay.h"
//#include "led.h"
//#include "oled.h"
#include "timer.h"
#include "usart.h"
#include "tftlcd.h"
#include "pic.h"
#include "exti.h"

int flg=1;
int sec=55,min=59,hour=23;
u16 count;
#define JTAG_SWD_DISABLE   0X02
#define SWD_ENABLE         0X01
#define JTAG_SWD_ENABLE    0X00	

void JTAG_Set(u8 mode)
{
	u32 temp;
	temp=mode;
	temp<<=25;
	RCC->APB2ENR|=1<<0;     //开启辅助时钟	   
	AFIO->MAPR&=0XF8FFFFFF; //清除MAPR的[26:24]
	AFIO->MAPR|=temp;       //设置jtag模式
} 

int main(void)
{
	SystemInit();	
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级

	JTAG_Set(JTAG_SWD_DISABLE);     
	JTAG_Set(SWD_ENABLE);  
	delay_ms(10);	
	
//	u8 strdats[8]="";

//	Uart2_Init(9600);
//	u2_printf("send:AT\r\n");	
//	Uart3_Init(115200);
//	USART_SendString(USART3,"AT\r\n");
	Timer3_Init(9999,7199);
//	
	EXTI_GPIO_Config();  
	EXTI_INTERRUPT_Config();
	EXTI_NVIC_Config();


//	OLED_Init();
//	OLED_Clear(); 
//	OLED_ShowString(2,2,"Hello MY World!!!",16);

	TFTLCD_Init();
	TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,WHITE);			
	delay_ms(1000);	
	TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,BLACK);
	delay_ms(1000);	
	TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,BLUE);		
	delay_ms(1000);
	TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,RED);		
	delay_ms(1000);	
	TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GREEN);		
	delay_ms(1000);

	while(1)
	{
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,WHITE);			//
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,BLACK);//
		delay_ms(1000);	
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,BLUE);		//
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,RED);		//
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GREEN);		//
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_DrawLine(0,0,60,60,BLUE);
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GREEN);		//
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_DrawRectangle(10,20,50,50,RED);
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GREEN);		//
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_DrawCircle(30,30,20,BLACK);
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GREEN);		//
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_ShowChar(0,0,'a',BLACK,RED,24,0);
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GREEN);		//
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_ShowString(0,0,"i love you",BLACK,RED,16,0);
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GREEN);		//
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_ShowIntNum(0,0,13,2,RED,GREEN,24);
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GREEN);		//
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_ShowFloatNum1(3,3,12.34,4,BLUE,BLACK,24);
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GREEN);		//
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GREEN);		//
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Image(10,10,40,40,image);
		delay_ms(1000);
		while(flg%2==0);
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GREEN);		//
		delay_ms(1000);
		while(flg%2==0);
		while(1)
		{
			TFTLCD_ShowString(0,0,"time:",BLACK,GREEN,12,0);
			TFTLCD_ShowIntNum(0,15,hour/10,1,RED,GREEN,12);
			TFTLCD_ShowIntNum(6,15,hour%10,1,RED,GREEN,12);
			TFTLCD_ShowChar(12,15,':',RED,GREEN,12,0);
			TFTLCD_ShowIntNum(18,15,min/10,1,RED,GREEN,12);
			TFTLCD_ShowIntNum(24,15,min%10,1,RED,GREEN,12);
			TFTLCD_ShowChar(30,15,':',RED,GREEN,12,0);
			TFTLCD_ShowIntNum(36,15,sec/10,1,RED,GREEN,12);
			TFTLCD_ShowIntNum(42,15,sec%10,1,RED,GREEN,12);
			if(sec==20)
			{
				sec=55;min=59;hour=23;
				break;
			}
		}
	}
}

