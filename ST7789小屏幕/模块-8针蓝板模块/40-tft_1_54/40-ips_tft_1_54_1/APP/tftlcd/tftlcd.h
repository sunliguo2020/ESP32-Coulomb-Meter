#ifndef __TFTLCD_H
#define __TFTLCD_H	 
#include "sys.h"
#include "stdlib.h"	
 
#define TFT  154
 
#define TFTLCD_SCL_GPIO_PORT				GPIOB
#define TFTLCD_SCL_GPIO_PIN				GPIO_Pin_9
				
#define TFTLCD_SDA_GPIO_PORT				GPIOB
#define TFTLCD_SDA_GPIO_PIN				GPIO_Pin_8
				
#define TFTLCD_RST_GPIO_PORT				GPIOB
#define TFTLCD_RST_GPIO_PIN				GPIO_Pin_7
				
#define TFTLCD_DC_GPIO_PORT				GPIOB
#define TFTLCD_DC_GPIO_PIN				GPIO_Pin_6
				
#define TFTLCD_CS_GPIO_PORT				GPIOB
#define TFTLCD_CS_GPIO_PIN				GPIO_Pin_5
 
#define TFTLCD_BLK_GPIO_PORT				GPIOB
#define TFTLCD_BLK_GPIO_PIN				GPIO_Pin_4

//BRR BSRR ODR 都是用来控制16位针脚的  如 GPIOx->BRR=0x01和GPIOx->BSRR=0x01<<16相同 GPIOx->BSRR=0x10000 

#define TFTLCD_SDA_SET    GPIO_SetBits(TFTLCD_SDA_GPIO_PORT,TFTLCD_SDA_GPIO_PIN) //BSRR--Bit Set Reset Register  设置 GPIOx  GPIOx->BSRR= GPIO_Pin11;
#define TFTLCD_SDA_RESET  GPIO_ResetBits(TFTLCD_SDA_GPIO_PORT,TFTLCD_SDA_GPIO_PIN)//BRR--Bit Reset Register  清空 GPIOx   GPIOx->BRR= GPIO_Pin11;

#define TFTLCD_SCL_SET    GPIO_SetBits(TFTLCD_SCL_GPIO_PORT,TFTLCD_SCL_GPIO_PIN)
#define TFTLCD_SCL_RESET  GPIO_ResetBits(TFTLCD_SCL_GPIO_PORT,TFTLCD_SCL_GPIO_PIN)

#define TFTLCD_CS_SET     GPIO_SetBits(TFTLCD_CS_GPIO_PORT,TFTLCD_CS_GPIO_PIN)
#define TFTLCD_CS_RESET   GPIO_ResetBits(TFTLCD_CS_GPIO_PORT,TFTLCD_CS_GPIO_PIN)

#define TFTLCD_DC_SET     GPIO_SetBits(TFTLCD_DC_GPIO_PORT,TFTLCD_DC_GPIO_PIN)
#define TFTLCD_DC_RESET   GPIO_ResetBits(TFTLCD_DC_GPIO_PORT,TFTLCD_DC_GPIO_PIN)

#define TFTLCD_RST_SET    GPIO_SetBits(TFTLCD_RST_GPIO_PORT,TFTLCD_RST_GPIO_PIN)
#define TFTLCD_RST_RESET  GPIO_ResetBits(TFTLCD_RST_GPIO_PORT,TFTLCD_RST_GPIO_PIN)

#define TFTLCD_BLK_SET    GPIO_SetBits(TFTLCD_BLK_GPIO_PORT,TFTLCD_BLK_GPIO_PIN)
#define TFTLCD_BLK_RESET  GPIO_ResetBits(TFTLCD_BLK_GPIO_PORT,TFTLCD_BLK_GPIO_PIN)

#define USE_HORIZONTAL 0
//0: C 34 L 0
//1: C 34 L 0
//2: C 0  L 34
//3: C 0  L 34



//0: C 82 L 18
//1: C 82 L 18
//2: C 18  L 82
//3: C 18  L 82
#define TFT_COLUMN_OFFSET 1	
#define TFT_LINE_OFFSET   0


#if TFT==18
	#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
		#define TFTLCD_W 128
		#define TFTLCD_H 160
	#else
		#define TFTLCD_W 160
		#define TFTLCD_H 128
	#endif
#elif TFT==13	 
	#define TFTLCD_W 240
	#define TFTLCD_H 240
#elif TFT==145	
	#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
		#define TFTLCD_W 60
		#define TFTLCD_H 160
	#else
		#define TFTLCD_W 160
		#define TFTLCD_H 60
	#endif

#elif TFT==154	
	#define TFTLCD_W 240
	#define TFTLCD_H 240


#elif TFT==225	
	#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
		#define TFTLCD_W 76
		#define TFTLCD_H 284
	#else
		#define TFTLCD_W 284
		#define TFTLCD_H 76
	#endif

#endif
 
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE           	 0x001F  
#define BRED             0XF81F
#define GRED		 0XFFE0
#define GBLUE		 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN  		 0XBC40 
#define BRRED 		 0XFC07 
#define GRAY  		 0X8430 
#define DARKBLUE      	 0X01CF	
#define LIGHTBLUE      	 0X7D7C	
#define GRAYBLUE       	 0X5458 
#define LIGHTGREEN     	 0X841F
#define LGRAY 		 0XC618 
#define LGRAYBLUE        0XA651 
#define LBBLUE           0X2B12 


void TFTLCD_GPIO_Init(void);
void TFTLCD_Writ_Bus(u8 dat);
void TFTLCD_WR_DATA8(u8 dat);
void TFTLCD_WR_DATA(u16 dat);
void TFTLCD_WR_REG(u8 dat);
void TFTLCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2);
void TFTLCD_Init(void);//LCD
 
 
void TFTLCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color);
void TFTLCD_DrawPoint(u16 x,u16 y,u16 color);
void TFTLCD_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color);
void TFTLCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color);
void TFTLCD_DrawCircle(u16 x0,u16 y0,u8 r,u16 color);
 
void TFTLCD_ShowChinese(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);
 
void TFTLCD_ShowChar(u16 x,u16 y,u8 num,u16 fc,u16 bc,u8 sizey,u8 mode);
void TFTLCD_ShowString(u16 x,u16 y,const u8 *p,u16 fc,u16 bc,u8 sizey,u8 mode);
u32 mypow(u8 m,u8 n);
void TFTLCD_ShowIntNum(u16 x,u16 y,u16 num,u8 len,u16 fc,u16 bc,u8 sizey);
void TFTLCD_ShowFloatNum1(u16 x,u16 y,float num,u8 len,u16 fc,u16 bc,u8 sizey);
 
void TFTLCD_ShowPicture(u16 x,u16 y,u16 length,u16 width,const u8 pic[]);
void TFTLCD_Image(u16 x, u16 y, u16 width, u16 height, const u8 *image);

#endif


