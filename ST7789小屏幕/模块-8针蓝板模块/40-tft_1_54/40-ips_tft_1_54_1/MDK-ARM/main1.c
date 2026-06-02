#include "sys.h"
#include "delay.h"
#include "oled.h"
#include "key.h"

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
	u8  keys=99;

	u8  dat_buf[20];
	SystemInit();	
		
	OLED_Init();			//初始化OLED  
	OLED_Clear(); 
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级

	JTAG_Set(JTAG_SWD_DISABLE);     
	JTAG_Set(SWD_ENABLE);  
	delay_ms(500);	

	LED_Init();
	KEY_Init1();

	while(1)
	{	
		
		OLED_ShowString(0,0,"LOVE YOU",16);
		keys = READ_KEY_Num();
		dat_buf[0]=(char)(keys/10+'0');
		dat_buf[1]=(char)(keys%10+'0');
		OLED_ShowString(0,2,dat_buf,16);
		  


		
	}
//	return 0;
}

