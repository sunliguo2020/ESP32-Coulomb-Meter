#ifndef _RC522_H_
#define _RC522_H_

#include <string.h> 
#include <stdio.h>
#include "stm32f10x.h"
//////////////////////////////////
//端口定义
/////////////////////////////////////////////////////////////////////
//MFRC522
#define MF522_RST_PIN                    GPIO_Pin_8
#define MF522_RST_PORT                   GPIOA
#define MF522_RST_CLK                    RCC_APB2Periph_GPIOA  
  
#define MF522_MISO_PIN                   GPIO_Pin_14
#define MF522_MISO_PORT                  GPIOB
#define MF522_MISO_CLK                   RCC_APB2Periph_GPIOB  

#define MF522_MOSI_PIN                   GPIO_Pin_15
#define MF522_MOSI_PORT                  GPIOB
#define MF522_MOSI_CLK                   RCC_APB2Periph_GPIOB

#define MF522_SCK_PIN                    GPIO_Pin_13  
#define MF522_SCK_PORT                   GPIOB
#define MF522_SCK_CLK                    RCC_APB2Periph_GPIOB

#define MF522_NSS_PIN                    GPIO_Pin_12  
#define MF522_NSS_PORT                   GPIOB
#define MF522_NSS_CLK                    RCC_APB2Periph_GPIOB 

//指示灯
//#define LED_PIN                          GPIO_Pin_13  
//#define LED_PORT                         GPIOC
//#define LED_CLK                          RCC_APB2Periph_GPIOC   

#define RST_H                            GPIO_SetBits(MF522_RST_PORT, MF522_RST_PIN)
#define RST_L                            GPIO_ResetBits(MF522_RST_PORT, MF522_RST_PIN)
#define MOSI_H                           GPIO_SetBits(MF522_MOSI_PORT, MF522_MOSI_PIN)
#define MOSI_L                           GPIO_ResetBits(MF522_MOSI_PORT, MF522_MOSI_PIN)
#define SCK_H                            GPIO_SetBits(MF522_SCK_PORT, MF522_SCK_PIN)
#define SCK_L                            GPIO_ResetBits(MF522_SCK_PORT, MF522_SCK_PIN)
#define NSS_H                            GPIO_SetBits(MF522_NSS_PORT, MF522_NSS_PIN)
#define NSS_L                            GPIO_ResetBits(MF522_NSS_PORT, MF522_NSS_PIN)
#define READ_MISO                        GPIO_ReadInputDataBit(MF522_MISO_PORT, MF522_MISO_PIN)
//#define LED_ON                           GPIO_SetBits(LED_PORT, LED_PIN)
//#define LED_OFF                          GPIO_ResetBits(LED_PORT, LED_PIN)
/////////////////////////////////////////////////////////////////////
//函数原型
/////////////////////////////////////////////////////////////////////
 

#define TRUE 1
#define FALSE 0

#define s8  char
#define u8 unsigned char  
#define u16 unsigned int
/////////////////////////////////////////////////////////////////////
//函数原型
/////////////////////////////////////////////////////////////////////
void MFRC_GPIO_Init(void);
char PcdReset(void);
void PcdAntennaOn(void);
void PcdAntennaOff(void);
char PcdRequest(unsigned char req_code,unsigned char *pTagType);   
char PcdAnticoll(unsigned char *pSnr);

char PcdSelect(unsigned char *pSnr);         
char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr);     
char PcdRead(unsigned char addr,unsigned char *pData);     
char PcdWrite(unsigned char addr,unsigned char *pData);    
char PcdValue(unsigned char dd_mode,unsigned char addr,unsigned char *pValue);   
char PcdBakValue(unsigned char sourceaddr, unsigned char goaladdr);                                 
char PcdHalt(void);
char PcdComMF522(u8 Command, u8 *pDataIn, u8 InLenByte, u8 *pDataOut, u16  *pOutLenBit);
void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData);
void WriteRawRC(unsigned char Address,unsigned char value);
unsigned char ReadRawRC(unsigned char Address); 
void SetBitMask(unsigned char reg,unsigned char mask); 
void ClearBitMask(unsigned char reg,unsigned char mask); 
char M500PcdConfigISOType(unsigned char type);
void delay_10ms(unsigned int _10ms);
void WaitCardOff(void);
char PcdRats(u8 * DataOut,u8 * Len);
void PcdSelectFile(u8 *pDataIn);
void PcdReadBinary(u8 offset, u16 Len, u8* pDataOut);
void PcdSwitchPCB(void);
u16 CardReset(u8 * Data_Out,u8 *  Len);
u8 Pcd_Cmd(u8* pDataIn, u8  In_Len, u8* pDataOut,u8 * Out_Len);
/////////////////////////////////////////////////////////////////////
//MF522命令字
/////////////////////////////////////////////////////////////////////
#define PCD_IDLE              0x00               //取消当前命令
#define PCD_AUTHENT           0x0E               //验证密钥
#define PCD_RECEIVE           0x08               //接收数据
#define PCD_TRANSMIT          0x04               //发送数据
#define PCD_TRANSCEIVE        0x0C               //发送并接收数据
#define PCD_RESETPHASE        0x0F               //复位
#define PCD_CALCCRC           0x03               //CRC计算

/////////////////////////////////////////////////////////////////////
//Mifare_One卡片命令字
/////////////////////////////////////////////////////////////////////
#define PICC_REQIDL           0x26               //寻天线区内未进入休眠状态
#define PICC_REQALL           0x52               //寻天线区内全部卡
#define PICC_ANTICOLL1        0x93               //防冲撞
#define PICC_ANTICOLL2        0x95               //防冲撞
#define PICC_AUTHENT1A        0x60               //验证A密钥
#define PICC_AUTHENT1B        0x61               //验证B密钥
#define PICC_READ             0x30               //读块
#define PICC_WRITE            0xA0               //写块
#define PICC_DECREMENT        0xC0               //扣款
#define PICC_INCREMENT        0xC1               //充值
#define PICC_RESTORE          0xC2               //调块数据到缓冲区
#define PICC_TRANSFER         0xB0               //保存缓冲区中数据
#define PICC_HALT             0x50               //休眠


#define		ST_OK					0x9000	//正确执行
#define		ST_ERR					0xFFFF	//返回错误
#define		ST_RETDATWRG			0x6281	//返回数据错误
#define		ST_VERIFY				0x6283	//选择文件无效,文件或密钥校验错误
#define		ST_STNCHG				0x6400	//状态标志未改变
#define		ST_EEPWRG				0x6581	//写EEPROM不成功
#define		ST_LENWRG				0x6700	//错误长度
#define		ST_CLAPTWRG				0x6900	//CLA与线路保护要求不匹配
#define		ST_INVALID				0x6901	//无效的状态
#define		ST_COMMAND				0x6981	//命令与文件结构不相容
#define		ST_SECURE				0x6982	//不满足安全状态
#define		ST_KEYLOCK				0x6983	//密钥锁死
#define		ST_USECON				0x6985	//使用条件不满足
#define		ST_NSPACK				0x6987	//无安全报文
#define		ST_SPDATWRG				0x6988	//安全报文数据项不正确
#define		ST_DATPRWRG				0x6A80	//数据域参数错误
#define		ST_FUNWRG				0x6A81	//功能不支持或卡中无MF或卡片锁定
#define		ST_NOFILE				0x6A82	//文件未找到
#define		ST_NORECORD				0x6A83	//记录未找到
#define		ST_NOSPACE				0x6A84	//文件无足够空间
#define		ST_P1P2WRG				0x6A86	//参数P1,P2错误,文件已存在
#define		ST_NOKEY				0x6A88	//密钥未找到
#define		ST_OFFSETWRG			0x6B00	//在达到Le/Lc字节之前文件结束,偏移量错误
#define		ST_CLAWRG				0x6E00	//无效的CLA
#define		ST_DATWRG				0x6F00	//数据无效
#define		ST_MACWRG				0x9302	//MAC错误
#define		ST_APPLOCK				0x9303	//应用已被锁定
#define		ST_SUMLACK				0x9401	//金额不足
#define		ST_NOKEY2				0x9403	//密钥未找到
#define		ST_MACINVAL			    0x9406	//所需的MAC不可用
/////////////////////////////////////////////////////////////////////
//MF522 FIFO长度定义
/////////////////////////////////////////////////////////////////////
#define DEF_FIFO_LENGTH       64                 //FIFO size=64byte

/////////////////////////////////////////////////////////////////////
//MF522寄存器定义
/////////////////////////////////////////////////////////////////////
// PAGE 0
#define     RFU00                 0x00    
#define     CommandReg            0x01    
#define     ComIEnReg             0x02    
#define     DivlEnReg             0x03    
#define     ComIrqReg             0x04    
#define     DivIrqReg             0x05
#define     ErrorReg              0x06    
#define     Status1Reg            0x07    
#define     Status2Reg            0x08    
#define     FIFODataReg           0x09
#define     FIFOLevelReg          0x0A
#define     WaterLevelReg         0x0B
#define     ControlReg            0x0C
#define     BitFramingReg         0x0D
#define     CollReg               0x0E
#define     RFU0F                 0x0F
// PAGE 1     
#define     RFU10                 0x10
#define     ModeReg               0x11
#define     TxModeReg             0x12
#define     RxModeReg             0x13
#define     TxControlReg          0x14
#define     TxAutoReg             0x15
#define     TxSelReg              0x16
#define     RxSelReg              0x17
#define     RxThresholdReg        0x18
#define     DemodReg              0x19
#define     RFU1A                 0x1A
#define     RFU1B                 0x1B
#define     MifareReg             0x1C
#define     RFU1D                 0x1D
#define     RFU1E                 0x1E
#define     SerialSpeedReg        0x1F
// PAGE 2    
#define     RFU20                 0x20  
#define     CRCResultRegM         0x21
#define     CRCResultRegL         0x22
#define     RFU23                 0x23
#define     ModWidthReg           0x24
#define     RFU25                 0x25
#define     RFCfgReg              0x26
#define     GsNReg                0x27
#define     CWGsCfgReg            0x28
#define     ModGsCfgReg           0x29
#define     TModeReg              0x2A
#define     TPrescalerReg         0x2B
#define     TReloadRegH           0x2C
#define     TReloadRegL           0x2D
#define     TCounterValueRegH     0x2E
#define     TCounterValueRegL     0x2F
// PAGE 3      
#define     RFU30                 0x30
#define     TestSel1Reg           0x31
#define     TestSel2Reg           0x32
#define     TestPinEnReg          0x33
#define     TestPinValueReg       0x34
#define     TestBusReg            0x35
#define     AutoTestReg           0x36
#define     VersionReg            0x37
#define     AnalogTestReg         0x38
#define     TestDAC1Reg           0x39  
#define     TestDAC2Reg           0x3A   
#define     TestADCReg            0x3B   
#define     RFU3C                 0x3C   
#define     RFU3D                 0x3D   
#define     RFU3E                 0x3E   
#define     RFU3F		          0x3F


#define     REQ_ALL               0x26   //读卡完成还会再次读取
#define     REQ_IDL               0x52   //读卡完成还等待卡离开磁场范围
#define     KEYA                  0x60
#define     FSDI 4
/////////////////////////////////////////////////////////////////////
//和MF522通讯时返回的错误代码
/////////////////////////////////////////////////////////////////////
#define MI_OK                          (char)0
#define MI_NOTAGERR                    (char)(-1)
#define MI_ERR                         (char)(-2)
	

#endif


