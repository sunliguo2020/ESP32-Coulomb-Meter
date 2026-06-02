#include "lcd12864.h"

void LCD12864_UserConfig(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	
	GPIO_InitStructure.GPIO_Pin	= CS|SID|CLK;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(CMD_PROT,&GPIO_InitStructure);
}

/*LCD12864*/
void LCD12864_Write_Byte(uint8_t data)
{
	uint8_t WBi;
	CS_HIGH;	
	for(WBi=0;WBi<8;WBi++)
	{
		(data<<WBi&0x80)?SID_HIGH:SID_LOW;	
	
		CLK_LOW;	//
		CLK_HIGH;	//
	}
	CS_LOW;		
}
	
/*LCD12864*/
void LCD12864_Write_CMD_Data(uint8_t cmd,uint8_t data)
{
	LCD12864_Write_Byte(cmd);
	LCD12864_Write_Byte(data&0xf0);			//0x38 0011 1000 & 1111 0000
	LCD12864_Write_Byte((data<<4)&0xf0);	//0x38<<4 1000 & 1111 0000
	delay_ms(2);	
}

/*LCD12864*/
void LCD12864_Write_Init(void)
{
	LCD12864_Write_CMD_Data(CMD,0x30);		//
	LCD12864_Write_CMD_Data(CMD,0x06);		//
	LCD12864_Write_CMD_Data(CMD,0x0C);		//
	LCD12864_Write_CMD_Data(CMD,0x01);		//
	LCD12864_Write_CMD_Data(CMD,0x80);		//
}

void LCD12864_Open(void)
{
	LCD12864_Write_CMD_Data(CMD,0x0C);	
}

void LCD12864_Shut(void)
{
	LCD12864_Write_CMD_Data(CMD,0x08);		//
}

/*LCD12864*/
void LCD12864_Write_Word(LCD_InitTypeDef LCD_InitStructure)
{
	uint8_t LWWi;
	LCD12864_Write_CMD_Data(CMD,LCD_InitStructure.com);		//
	
	for(LWWi=0;LWWi<LCD_InitStructure.num;LWWi++)
	{
		LCD12864_Write_CMD_Data(DAT,*LCD_InitStructure.dat++);	//
		delay_ms(200);
	}
}
	
/*LCD12864*/
void LCD12864_Write_Number(uint8_t com,uint8_t num)
{
	const uint8_t number[] = {"0123456789"};
	LCD12864_Write_CMD_Data(CMD,com);					
	LCD12864_Write_CMD_Data(DAT,number[num/1000]);		//1234 1
	LCD12864_Write_CMD_Data(DAT,number[num%1000/100]);	//1234 2
	LCD12864_Write_CMD_Data(DAT,number[num%100/10]);	//1234 3
	LCD12864_Write_CMD_Data(DAT,number[num%100%10]);	//1234 4
}

