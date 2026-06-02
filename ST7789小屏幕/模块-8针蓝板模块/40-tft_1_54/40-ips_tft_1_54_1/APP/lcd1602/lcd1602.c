#include "lcd1602.h"
#include "delay.h"

void GPIO_Configuration(void)
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOA,ENABLE);//
  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(LCD_DATA_PORT, &GPIO_InitStructure);				
 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOC, &GPIO_InitStructure);				
}

void LCD1602_Wait_Ready(void)
{
	int8_t sta;
	
	DATAOUT(0xff);    //
	LCD_RS_Clr();     //RS 0
	LCD_RW_Set();     //RW 1
	do
	{
		LCD_EN_Set();   //EN 1
		delay_ms(5);	//
 		sta = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7);//
		LCD_EN_Clr();  //EN  0
	}while(sta & 0x80);//
}
 

void LCD1602_Write_Cmd(u8 cmd)
{
	LCD1602_Wait_Ready();  //
	LCD_RS_Clr();   
	LCD_RW_Clr();
	DATAOUT(cmd);  //
	LCD_EN_Set();
	LCD_EN_Clr();
	
	DATAOUT(cmd<<4); //
	LCD_EN_Set();
	LCD_EN_Clr();
}
 
void LCD1602_Write_Dat(u8 dat)
{
	LCD1602_Wait_Ready(); //
	LCD_RS_Set();   //1
	LCD_RW_Clr();   //0
	
	DATAOUT(dat);   //
	LCD_EN_Set();  
	LCD_EN_Clr();
	
	DATAOUT(dat<<4); 
	LCD_EN_Set(); 
	LCD_EN_Clr();
	
}
 
void LCD1602_ClearScreen(void)
{
	LCD1602_Write_Cmd(0x01);
	
}
 
void LCD1602_Set_Cursor(u8 x, u8 y)
{
	u8 addr;
	
	if (y == 0)
		addr = 0x00 + x;
	else
		addr = 0x40 + x;
	LCD1602_Write_Cmd(addr | 0x80);
}
 

void LCD1602_Show_Str(u8 x, u8 y, u8 *str)
{
	LCD1602_Set_Cursor(x, y);
	while(*str != '\0')
	{
		LCD1602_Write_Dat(*str++);
	}
}
 

void LCD1602_Init(void)
{
	LCD1602_Write_Cmd(0x28);	//
	LCD1602_Write_Cmd(0x0C);	//
	LCD1602_Write_Cmd(0x06);	//
	LCD1602_Write_Cmd(0x01);	//
}

