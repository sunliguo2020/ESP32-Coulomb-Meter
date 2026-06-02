#ifndef __LCD12864_H
#define __LCD12864_H

#include "stm32f10x.h"
#include "delay.h"

#define CS 	GPIO_Pin_15	//
#define SID GPIO_Pin_14	//
#define CLK GPIO_Pin_13	//
#define PSB GPIO_Pin_3	//
#define CMD_PROT GPIOC	//

#define CS_HIGH GPIO_SetBits(CMD_PROT,CS)
#define CS_LOW GPIO_ResetBits(CMD_PROT,CS)
#define SID_HIGH GPIO_SetBits(CMD_PROT,SID)
#define SID_LOW GPIO_ResetBits(CMD_PROT,SID)
#define CLK_HIGH GPIO_SetBits(CMD_PROT,CLK)
#define CLK_LOW GPIO_ResetBits(CMD_PROT,CLK)
#define PSB_HIGH GPIO_SetBits(CMD_PROT,PSB)
#define PSB_LOW GPIO_ResetBits(CMD_PROT,PSB)

#define CMD 0xF8	//
#define DAT 0xFA	//

typedef struct{
	uint8_t com;	//
	uint8_t num;	//
	uint8_t *dat;	//
}LCD_InitTypeDef;	//

void LCD12864_UserConfig(void);
void LCD12864_Write_Init(void);
void LCD12864_Write_CMD_Data(uint8_t cmd,uint8_t data);
void LCD12864_Write_Word(LCD_InitTypeDef LCD_InitStructure);/*LCD12864*/
void LCD12864_Write_Number(uint8_t com,uint8_t num);/*LCD12864*/
void LCD12864_Open(void);
void LCD12864_Shut(void);

#endif

