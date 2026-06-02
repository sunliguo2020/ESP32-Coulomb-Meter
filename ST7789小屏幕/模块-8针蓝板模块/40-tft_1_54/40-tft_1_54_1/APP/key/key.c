#include "key.h"
#include "delay.h"

void KEY_Init(int t)
{	
	if(t==1)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	
		GPIO_InitTypeDef GPIO_InitStructure;		
		//推挽输出
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_10|GPIO_Pin_11;	
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //设置成推挽输出
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_Init(GPIOB, &GPIO_InitStructure);  						
		GPIO_SetBits(GPIOB,GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_10|GPIO_Pin_11);//输出高电平
		
		//下拉输入
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; //PA0设置成输入，默认下拉	
		GPIO_Init(GPIOB, &GPIO_InitStructure);  						
	}
	else
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		GPIO_InitTypeDef GPIO_InitStructure;		
		//推挽输出
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;	
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //设置成推挽输出
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_Init(GPIOB, &GPIO_InitStructure);  						
		GPIO_SetBits(GPIOB,GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);//输出高电平
		
		//下拉输入
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_10|GPIO_Pin_11;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; //PA0设置成输入，默认下拉	
		GPIO_Init(GPIOB, &GPIO_InitStructure); 
			
	}
}

void KEY_Init1(void)
{	
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE);	
		GPIO_InitTypeDef GPIO_InitStructure;		
		//推挽输出
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_15;	
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //设置成推挽输出
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_Init(GPIOA, &GPIO_InitStructure);  						
		GPIO_SetBits(GPIOA,GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_15);//输出高电平		
	
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;	
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //设置成推挽输出
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_Init(GPIOB, &GPIO_InitStructure);  						
		GPIO_SetBits(GPIOB,GPIO_Pin_3);//输出高电平	
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; //PA0设置成输入，默认下拉	
		GPIO_Init(GPIOB, &GPIO_InitStructure); 
}


int Get_KEY_Num(void)
{
	int key_num=80;
	KEY_Init(1);//4~7输入
	if(KEY4==1)key_num=0;
	else if(KEY5==1)key_num=1;
	else if(KEY6==1)key_num=2;
	else if(KEY7==1)key_num=3;
	
	KEY_Init(0);//0~3输入
	if(KEY0==1)key_num+=0;
	else if(KEY1==1)key_num+=4;
	else if(KEY2==1)key_num+=8;
	else if(KEY3==1)key_num+=12;


	return key_num;
}

int READ_KEY_Num(void)
{
	int keys=80;
	
	GPIO_SetBits(GPIOA,GPIO_Pin_11);//输出高电平		
	GPIO_ResetBits(GPIOB,GPIO_Pin_3);//输出高电平
	GPIO_ResetBits(GPIOA,GPIO_Pin_12|GPIO_Pin_15);//输出高电平	
	if(KEY4==1)keys=0;
	else if(KEY5==1)keys=1;
	else if(KEY6==1)keys=2;
	else if(KEY7==1)keys=3;
	
	delay_ms(10);
	GPIO_SetBits(GPIOA,GPIO_Pin_12);//输出高电平		
	GPIO_ResetBits(GPIOB,GPIO_Pin_3);//输出高电平
	GPIO_ResetBits(GPIOA,GPIO_Pin_11|GPIO_Pin_15);//输出高电平	
	if(KEY4==1)keys=4;
	else if(KEY5==1)keys=5;
	else if(KEY6==1)keys=6;
	else if(KEY7==1)keys=7;
	
	delay_ms(10);
	GPIO_SetBits(GPIOA,GPIO_Pin_15);//输出高电平		
	GPIO_ResetBits(GPIOB,GPIO_Pin_3);//输出高电平
	GPIO_ResetBits(GPIOA,GPIO_Pin_11|GPIO_Pin_12);//输出高电平	
	if(KEY4==1)keys=8;
	else if(KEY5==1)keys=9;
	else if(KEY6==1)keys=10;
	else if(KEY7==1)keys=11;
			
	delay_ms(10);
	GPIO_ResetBits(GPIOA,GPIO_Pin_15);//输出高电平		
	GPIO_SetBits(GPIOB,GPIO_Pin_3);//输出高电平
	GPIO_ResetBits(GPIOA,GPIO_Pin_11|GPIO_Pin_12);//输出高电平	
	if(KEY4==1)keys=12;
	else if(KEY5==1)keys=13;
	else if(KEY6==1)keys=14;
	else if(KEY7==1)keys=15;

	return keys;
}


