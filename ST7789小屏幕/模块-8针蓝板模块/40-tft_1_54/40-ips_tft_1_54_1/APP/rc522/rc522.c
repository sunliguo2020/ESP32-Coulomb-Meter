#include "rc522.h"

#define MAXRLEN 32         
u8 Pcb = 0x00;					//CPU卡APDU指令分组号		


void MFRC_GPIO_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  /* Enable the GPIO Clock */
  RCC_APB2PeriphClockCmd(MF522_RST_CLK, ENABLE);

  /* Configure the GPIO pin */
  GPIO_InitStructure.GPIO_Pin = MF522_RST_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

  GPIO_Init(MF522_RST_PORT, &GPIO_InitStructure);
	
  /* Enable the GPIO Clock */
  RCC_APB2PeriphClockCmd(MF522_MISO_CLK, ENABLE);

  /* Configure the GPIO pin */
  GPIO_InitStructure.GPIO_Pin = MF522_MISO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

  GPIO_Init(MF522_MISO_PORT, &GPIO_InitStructure);
	
  /* Enable the GPIO Clock */
  RCC_APB2PeriphClockCmd(MF522_MOSI_CLK, ENABLE);

  /* Configure the GPIO pin */
  GPIO_InitStructure.GPIO_Pin = MF522_MOSI_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

  GPIO_Init(MF522_MOSI_PORT, &GPIO_InitStructure);
	
  /* Enable the GPIO Clock */
  RCC_APB2PeriphClockCmd(MF522_SCK_CLK, ENABLE);

  /* Configure the GPIO pin */
  GPIO_InitStructure.GPIO_Pin = MF522_SCK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

  GPIO_Init(MF522_SCK_PORT, &GPIO_InitStructure);
	
  /* Enable the GPIO Clock */
  RCC_APB2PeriphClockCmd(MF522_NSS_CLK, ENABLE);

  /* Configure the GPIO pin */
  GPIO_InitStructure.GPIO_Pin = MF522_NSS_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

  GPIO_Init(MF522_NSS_PORT, &GPIO_InitStructure);
	
}


/////////////////////////////////////////////////////////////////////
//功    能：寻卡
//参数说明: req_code[IN]:寻卡方式
//                0x52 = 寻感应区内所有符合14443A标准的卡
//                0x26 = 寻未进入休眠状态的卡
//          	  pTagType[OUT]：卡片类型代码
//                0x4400 = Mifare_UltraLight
//                0x0400 = Mifare_One(S50)
//                0x0200 = Mifare_One(S70)
//                0x0800 = Mifare_Pro(X)
//                0x4403 = Mifare_DESFire
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdRequest(unsigned char req_code,unsigned char *pTagType)
{
   char status;  
   unsigned int  unLen;
   unsigned char ucComMF522Buf[MAXRLEN]; 

   ClearBitMask(Status2Reg,0x08);
   WriteRawRC(BitFramingReg,0x07);
   SetBitMask(TxControlReg,0x03);
	
	 memset(ucComMF522Buf, 0x00, MAXRLEN);
   ucComMF522Buf[0] = req_code;
   status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);
   if ((status == MI_OK) && (unLen == 0x10))
   {    
       *pTagType     = ucComMF522Buf[0];
       *(pTagType+1) = ucComMF522Buf[1];
   }
   else
   {   status = MI_ERR;   }
   
   return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：防冲撞
//参数说明: pSnr[OUT]:卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////  
char PcdAnticoll(unsigned char *pSnr)
{
    char status;
    unsigned char i,snr_check=0;
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 
    

    ClearBitMask(Status2Reg,0x08);
    WriteRawRC(BitFramingReg,0x00);
    ClearBitMask(CollReg,0x80);
 
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x20;

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);


    if (status == MI_OK)
    {
    	 for (i=0; i<4; i++)
         {   
             *(pSnr+i)  = ucComMF522Buf[i];
             snr_check ^= ucComMF522Buf[i];
         }
         if (snr_check != ucComMF522Buf[i])
         {   status = MI_ERR;    }
    }
    
    SetBitMask(CollReg,0x80);

    return status;
}


/////////////////////////////////////////////////////////////////////
//功    能：选定卡片
//参数说明: pSnr[IN]:卡片物理号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdSelect(unsigned char *pSnr)
{
    char status;
    unsigned char i;
    unsigned int  unLen = 0;
    unsigned char ucComMF522Buf[MAXRLEN] = {0}; 
	
    ClearBitMask(Status2Reg,0x08);
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;
    for (i=0; i<4; i++)
    {
    	ucComMF522Buf[i+2] = *(pSnr+i);
    	ucComMF522Buf[6]  ^= *(pSnr+i);
    }
    CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
// for(i=0; i < unLen;i++){
//    printf("%x ",ucComMF522Buf[i]);
// }					
// printf("\n");		
	//=0 的bit6表示是否支持14443-A协议
		
    if ((status == MI_OK) && (unLen == 0x18))
    {   status = MI_OK;  }
    else
    {   status = MI_ERR;    }

    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：验证卡片密码
//参数说明: auth_mode[IN]: 密码验证模式
//                 0x60 = 验证A密钥
//                 0x61 = 验证B密钥 
//          addr[IN]：块地址
//          pKey[IN]：密码
//          pSnr[IN]：卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////               
char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr)
{
    char status;
    unsigned int  unLen;
    unsigned char i,ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
    for (i=0; i<6; i++)
    {    ucComMF522Buf[i+2] = *(pKey+i);   }
    for (i=0; i<6; i++)
    {    ucComMF522Buf[i+8] = *(pSnr+i);   }
 //   memcpy(&ucComMF522Buf[2], pKey, 6); 
 //   memcpy(&ucComMF522Buf[8], pSnr, 4); 
    
    status = PcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
    if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
    {   status = MI_ERR;   }
    
    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：读取M1卡一块数据
//参数说明: addr[IN]：块地址
//          pData[OUT]：读出的数据，16字节
//返    回: 成功返回MI_OK
///////////////////////////////////////////////////////////////////// 
char PcdRead(unsigned char addr,unsigned char *pData)
{
    char status;
    unsigned int  unLen;
    unsigned char i,ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
   
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
    if ((status == MI_OK) && (unLen == 0x90))
 //   {   memcpy(pData, ucComMF522Buf, 16);   }
    {
        for (i=0; i<16; i++)
        {    *(pData+i) = ucComMF522Buf[i];   }
    }
    else
    {   status = MI_ERR;   }
    
    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：写数据到M1卡一块
//参数说明: addr[IN]：块地址
//          pData[IN]：写入的数据，16字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////                  
char PcdWrite(unsigned char addr,unsigned char *pData)
{
    char status;
    unsigned int  unLen;
    unsigned char i,ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = PICC_WRITE;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
        
    if (status == MI_OK)
    {
        //memcpy(ucComMF522Buf, pData, 16);

        for (i=0; i<16; i++)
        {    ucComMF522Buf[i] = *(pData+i);   }
        CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {   status = MI_ERR;   }
    }
    
    return status;
}



/////////////////////////////////////////////////////////////////////
//功    能：命令卡片进入休眠状态
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdHalt(void)
{
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = PICC_HALT;
    ucComMF522Buf[1] = 0;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    return MI_OK;
}

/////////////////////////////////////////////////////////////////////
//用MF522计算CRC16函数
/////////////////////////////////////////////////////////////////////
void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData)
{
    unsigned char i,n;
    ClearBitMask(DivIrqReg,0x04);
    WriteRawRC(CommandReg,PCD_IDLE);
    SetBitMask(FIFOLevelReg,0x80);
    for (i=0; i<len; i++)
    {   WriteRawRC(FIFODataReg, *(pIndata+i));   }
    WriteRawRC(CommandReg, PCD_CALCCRC);
    i = 0xFF;
    do 
    {
        n = ReadRawRC(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04));
    pOutData[0] = ReadRawRC(CRCResultRegL);
    pOutData[1] = ReadRawRC(CRCResultRegM);
}

/////////////////////////////////////////////////////////////////////
//功    能：复位RC522
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdReset(void)
{
    RST_H;
    delay_10ms(1);
    RST_L;
    delay_10ms(1);
    RST_H;
	  delay_10ms(10);
	
// 		if(ReadRawRC(0x02) == 0x80)
// 		{
// 			delay_10ms(10);	
// 			delay_10ms(10);	
// 			delay_10ms(10);	
// 			delay_10ms(10);	
// 		}
    WriteRawRC(CommandReg,PCD_RESETPHASE);
    delay_10ms(1);
	
    WriteRawRC(ModeReg,0x3D);            //和Mifare卡通讯，CRC初始值0x6363
    WriteRawRC(TModeReg,0x8D);   
  	WriteRawRC(TReloadRegL,0x30);           
    WriteRawRC(TReloadRegH,0xFF);
    WriteRawRC(TPrescalerReg,0x3E);
    WriteRawRC(TxAutoReg,0x40);  
	  Pcb=0x00;		
    return MI_OK;
}
//////////////////////////////////////////////////////////////////////
//设置RC632的工作方式 
//////////////////////////////////////////////////////////////////////
char M500PcdConfigISOType(unsigned char type)
{
   if (type == 'A')                     //ISO14443_A
   { 
       ClearBitMask(Status2Reg,0x08);
       WriteRawRC(ModeReg,0x3D);//3F
       WriteRawRC(RxSelReg,0x86);//84
       WriteRawRC(RFCfgReg,0x7F);   //4F

   	   WriteRawRC(TReloadRegL,0x30);//tmoLength);// TReloadVal = 'h6a =tmoLength(dec) 
	     WriteRawRC(TReloadRegH,0xFF);
       WriteRawRC(TModeReg,0x8D);
	     WriteRawRC(TPrescalerReg,0x3E);
	  
	     delay_10ms(1);
       PcdAntennaOn();
   }
   else{ return (char)-1; }
   
   return MI_OK;
}
/////////////////////////////////////////////////////////////////////
//功    能：读RC632寄存器
//参数说明：Address[IN]:寄存器地址
//返    回：读出的值
/////////////////////////////////////////////////////////////////////
unsigned char ReadRawRC(unsigned char Address)
{
     unsigned char i, ucAddr;
     unsigned char ucResult=0;

     NSS_L;
     ucAddr = ((Address<<1)&0x7E)|0x80;

     for(i=8;i>0;i--)
     {
         SCK_L;
	 	 if(ucAddr&0x80)
         	MOSI_H;
		 else
				 MOSI_L;
         SCK_H;
         ucAddr <<= 1;
     }

     for(i=8;i>0;i--)
     {
         SCK_L;
         ucResult <<= 1;
         SCK_H;
		 if(READ_MISO == 1)
         	ucResult |= 1;
     }

     NSS_H;
     SCK_H;
     return ucResult;
}

/////////////////////////////////////////////////////////////////////
//功    能：写RC632寄存器
//参数说明：Address[IN]:寄存器地址
//          value[IN]:写入的值
/////////////////////////////////////////////////////////////////////
void WriteRawRC(unsigned char Address, unsigned char value)
{  
    unsigned char i, ucAddr;

    SCK_L;
    NSS_L;
    ucAddr = ((Address<<1)&0x7E);

    for(i=8;i>0;i--)
    {
		if(ucAddr&0x80)
        	MOSI_H;
		else
			MOSI_L;
        SCK_H;
        ucAddr <<= 1;
        SCK_L;
    }

    for(i=8;i>0;i--)
    {
		if(value&0x80)
        	MOSI_H;
		else
			MOSI_L;
        SCK_H;
        value <<= 1;
        SCK_L;
    }
    NSS_H;
    SCK_H;
}

/////////////////////////////////////////////////////////////////////
//功    能：置RC522寄存器位
//参数说明：reg[IN]:寄存器地址
//          mask[IN]:置位值
/////////////////////////////////////////////////////////////////////
void SetBitMask(unsigned char reg,unsigned char mask)  
{
    char tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg,tmp | mask);  // set bit mask
}

/////////////////////////////////////////////////////////////////////
//功    能：清RC522寄存器位
//参数说明：reg[IN]:寄存器地址
//          mask[IN]:清位值
/////////////////////////////////////////////////////////////////////
void ClearBitMask(unsigned char reg,unsigned char mask)  
{
    char tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg, tmp & ~mask);  // clear bit mask
} 

//*************************************************************************
// 函数名	：PcdComMF522  
// 描述		：通过RC522和ISO14443卡通讯
// 入口		：Command[IN]:RC522命令字
// 			  pDataIn[IN]:通过RC522发送到卡片的数据
// 			  InLenByte[IN]:发送数据的字节长度
// 			  *pOutLenBit[OUT]:返回数据的位长度
// 出口		：pDataOut[OUT]:接收到的卡片返回数据
// 返回		：无
//*************************************************************************

char PcdComMF522(u8 Command, u8 *pDataIn, u8 InLenByte, u8 *pDataOut, u16  *pOutLenBit)
{
    char status = MI_ERR;
    u8 irqEn   = 0x00;
    u8 waitFor = 0x00;
    u8 lastBits;
    u8 n = 0;
	
    u16 i;

    switch(Command)
    {
       case PCD_AUTHENT:
          irqEn   = 0x12;
          waitFor = 0x10;
          break;
       case PCD_TRANSCEIVE:
          irqEn   = 0x77;
          waitFor = 0x30;							// 接受到数据及命令执行完毕	
          break;
       default:
         break;
    }
    WriteRawRC(ComIEnReg,irqEn|0x80);			    // 容许除定时器中断请求以为得所有中断请求
    ClearBitMask(ComIrqReg,0x80);					// 屏蔽位清除
    WriteRawRC(CommandReg,PCD_IDLE);				// 取消当前命令
    SetBitMask(FIFOLevelReg,0x80);					// 清除FIFO中的读写指针

    for (i=0; i<InLenByte; i++)
    {   
		WriteRawRC(FIFODataReg, pDataIn[i]);    		//数据写入FIFO
	}
    WriteRawRC(CommandReg, Command);					//写入命令,将缓冲区中的数据发送到天线,并激活自动接收器
   
    if (Command == PCD_TRANSCEIVE)						//如果命令为0C
    {    
		SetBitMask(BitFramingReg,0x80);  				//相当于启动发送STARTSENG
	}
    i = 3000;											//根据时钟频率调整，操作M1卡最大等待时间=600,操作CPU卡最大等待时间=1200
	do 
    {
         n = ReadRawRC(ComIrqReg);						//读取中断标志,检查数据返回
         i--;
    }
    while ((i!=0) && !(n&0x01) && !(n&waitFor));		// 定时器未超时,没有错误,0x01,0x30	
    ClearBitMask(BitFramingReg,0x80);					// 相当于清除发送STARTSENG

    if (i!=0)											// 定时时间到，i，没有递减到0
    {    
         if(!(ReadRawRC(ErrorReg)&0x1B))				// 判断有无出现错误标志	 Buffer溢出,位冲突,接收CRC错误,奇偶校验错误,
         {	
             status = MI_OK;							// 初始化状态 					 
			 
		 	 if (n & irqEn & 0x01)						// 若是PCD_TRANSCEIVE, irq = 0x77,  定时器为0中断产生,定时器为0时为错误
			 {   
				status = MI_NOTAGERR;   				// 搜索不到卡
			 }
             if (Command == PCD_TRANSCEIVE)				// 如果是发送接收指令
             { 
               	n = ReadRawRC(FIFOLevelReg);			// 读取接收到的数据的字节数
              	lastBits = ReadRawRC(ControlReg) & 0x07;// 2-0:RxLastBits,显示接收到最后一个字节的位数
                if (lastBits)							// 若该位为0，最后整个字节有效
                {   
					*pOutLenBit = (n-1)*8 + lastBits;   //pOutLenBit记录总共收到的位数
				}
                else
                {   
					*pOutLenBit = n*8;   				//接收完整位数
				}
                if (n == 0)								//假如没有中断产生
                {   
					n = 1;   							//n置1
				}
                if (n > MAXRLEN)						// 一次最大能接受到的字节数
                {   
					n = MAXRLEN;   						//超出最大长度,只接受最大长度的值
				}
                for (i=0; i<n; i++)
                {   
					pDataOut[i] = ReadRawRC(FIFODataReg); //从FIFO读取数据   
				}
            }
         }
         else
         {   
		 	status = MI_ERR;   							//有错误
		 }
   }
   SetBitMask(ControlReg,0x80);           	//停止定时器
   WriteRawRC(CommandReg,PCD_IDLE); 		//清空指令
   return status;								//返回状态
}



/////////////////////////////////////////////////////////////////////
//开启天线  
//每次启动或关闭天险发射之间应至少有1ms的间隔
/////////////////////////////////////////////////////////////////////
void PcdAntennaOn()
{
    unsigned char i;
    i = ReadRawRC(TxControlReg);
    if (!(i & 0x03))
    {
        SetBitMask(TxControlReg, 0x03);
    }
}


/////////////////////////////////////////////////////////////////////
//关闭天线
/////////////////////////////////////////////////////////////////////
void PcdAntennaOff()
{
    ClearBitMask(TxControlReg, 0x03);
}

//等待卡离开
void WaitCardOff(void)
{
	char          status;
  unsigned char	TagType[2];

	while(1)
	{
		status = PcdRequest(REQ_ALL, TagType);
		if(status)
		{
			status = PcdRequest(REQ_ALL, TagType);
			if(status)
			{
				status = PcdRequest(REQ_ALL, TagType);
				if(status)
				{
					break;
				}
			}
		}
		delay_10ms(10);
	}
}

///////////////////////////////////////////////////////////////////////
// Delay 10ms
///////////////////////////////////////////////////////////////////////
void delay_10ms(unsigned int _10ms)
{
	unsigned int i, j;

	for(i=0; i<_10ms; i++)
	{
		for(j=0; j<60000; j++);
	}
}

//*************************************************************************
// 函数名	：PcdSwitchPCB(void)
// 描述		：切换分组号
// 入口		：
// 出口		：
// 返回		：成功返回MI_OK
//*************************************************************************
void PcdSwitchPCB(void)
{
	switch(Pcb)
	{
		case 0x00:
			Pcb=0x0A;
			break;
		case 0x0A:
			Pcb=0x0B;
			break;
		case 0x0B:
			Pcb=0x0A;
			break;
	}
}


//*************************************************************************
// 函数名	：PcdRats
// 描述		：转入APDU命令格式
// 入口		： 无
// 出口		： DataOut 输出的数据，	Len 输出数据的长度
// 返回		：成功返回MI_OK
//*************************************************************************

char PcdRats(u8 * DataOut,u8 * Len)
{
	char status =MI_ERR;  
	unsigned int unLen;
	u8 ucComMF522Buf[MAXRLEN]; 
//int i;
	ClearBitMask(Status2Reg,0x08);	// 清空校验成功标志,清除MFCrypto1On位

	memset(ucComMF522Buf, 0x00, MAXRLEN);

	ucComMF522Buf[0] = 0xE0;		
	ucComMF522Buf[1] = 0x51;				

    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);	// 生成发送内容的CRC校验,保存到最后两个字节
// for(i=0; i < 4;i++){
//    printf("%x ",ucComMF522Buf[i]);
// }					
// printf("\n");	

   status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,DataOut,&unLen);// 将收到的卡片类型号保存

// for(i=0; i < unLen;i++){
//    printf("%x ",DataOut[i]);
// }					
// printf("\n");


	if (status == MI_OK)
	{
		Len[0]= unLen/8-2;
		status = MI_OK;
	}	 
	else
		status = MI_ERR;	 
	return status;								//返回结果
}


//*************************************************************************
// 函数名	：PcdReadBinary
// 描述		：读取二进制文件
// 入口		：mode : 线路保护
//					soffset:最高位为1,P1 P2为欲读文件的偏移量,P1为偏移量高字节,P2为低字节,所读的文件为当前文件
//					doffset:最高位不为1,低5位为短的文件标识符,P2为读的偏移量
//					Lc : 长度
//					pDataIn : 输入数据
//					Le : 返回长度
// 出口		：
// 返回		：成功返回MI_OK
//*************************************************************************
void PcdReadBinary(u8 offset, u16 Len, u8* pDataOut)
{
	char status =MI_ERR;  
	unsigned int unLen;
	u8 i,ucComMF522Buf[MAXRLEN]; 
  u8 Err[8];
	ClearBitMask(Status2Reg,0x08);							//清空校验成功标志,清除MFCrypto1On位
	
 //	if(Len > GetItemSum(ucComMF522Buf) - 6) return;
	
//	MemSet(ucComMF522Buf, 0x00, MAXRLEN);
	
 	PcdSwitchPCB();

	ucComMF522Buf[0] = Pcb;
	ucComMF522Buf[1] = 0x01;
	ucComMF522Buf[2] = 0x00;
	ucComMF522Buf[3] = 0xB0;
	ucComMF522Buf[4] = 0x95;				
	ucComMF522Buf[5] = 0x0C;								//当前文件
	ucComMF522Buf[6] = 0x08;								//起始位置
	//ucComMF522Buf[6] = 08;									//返回数据的长度
	CalulateCRC(ucComMF522Buf, 7, &ucComMF522Buf[7]);	// 生成发送内容的CRC校验,保存到最后两个字节	

//for(i=0;i < MAXRLEN;i++){
//			printf("%c",ucComMF522Buf[i]);		
//}
//printf("\n\n\n\n\n");		
	status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 9, ucComMF522Buf, &unLen);// 将收到的卡片类型号保存

//for(i=0;i < MAXRLEN;i++){
//			printf("%c",ucComMF522Buf[i]);		
//}
//printf("\n\n\n\n\n");

	if (status == MI_OK)
	{
		Err[0] = ucComMF522Buf[unLen+2];
		Err[1] = ucComMF522Buf[unLen+3];
	//	printf("22222");
	//	printf("%d",unLen);
	//  printf("%c",Err[0]);	
	//	printf("%c",Err[0]);	
		
		if(Err[0] == 0x90 &&Err[1] == 0x00)
			for (i=0; i< Len; i++){
        		*(pDataOut+i) = ucComMF522Buf[i+2];				//保存接收到数据
	//			printf(" %x ",ucComMF522Buf[i+2]);
			}

	}
	else
	{
		Err[0] = 0xFF;
		Err[1] = 0xFF; 
	}

}



 //发送命令MIFAREPRO卡
 /**********************************************************
 原型: unsigned int pro_send(unsigned char  *comm, unsigned char  slen,unsigned char  *resp,unsigned char  *rlen)
功能:给PRO卡片发APDU指令.
参数:comm.: 输入APDU指令; slen 输入发送APDU长度; resp: 输出返回数据; rlen:返回卡数据的长度
结果:卡片状态SW1SW2
 ***************************************************************/
unsigned int pro_send(unsigned char  *comm, unsigned char  slen,unsigned char  *resp,unsigned char  *rlen)
{
	//07 07 0a 00 00 84 00 00 08
	//07 07 0a 00 00 84 00 00 04
	//例如：取随机数命令0084000008
	//COMM=0a000084000008
	//slen=7+2;其中前面7个数是APDU后面二个数是校验
	char Status;
	unsigned int ulen;
	/*
	 buffer[0]=0x02;
	 buffer[1]=0x00;
	 buffer[2]=0xc0;
	 buffer[3]=0x00;
	 buffer[4]=0x00;
	 buffer[5]=0x10;  
	 CalulateCRC(buffer,6,&buffer[6]);
	 */
	CalulateCRC(comm,slen,&comm[slen]);	  //crc
	 Status = PcdComMF522(PCD_TRANSCEIVE,comm,slen+2,&resp[0], &ulen);
	 if (Status == 0)
	  {
	  	 if(ulen!=0)
		 {
		 	
		//	rlen[0]=ulen/8-2;
		//	rlen[0]=ulen/8-2;
		rlen[0]=ulen/8-1;
		}
		else
		{
			return 1;
		}
	}

	
	 return   Status;		
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//*************************************************************************
// 函数名	：Pcd_Cmd
// 描述		：执行上位机发来的指令
// 入口		：pDataIn : 要执行的指令 In_Len  指令数据长度					
// 出口		：pDataOut：输出执行后的返回数据   Out_Len输出的数据长度
// 返回		：MI_OK
//*************************************************************************
 
u8 Pcd_Cmd(u8* pDataIn, u8  In_Len, u8* pDataOut,u8 * Out_Len)
{
	char status =MI_ERR;  
	unsigned int  unLen;
	u8 ucComMF522Buf[MAXRLEN]; 
 	u8 i;

	ClearBitMask(Status2Reg,0x08);					// 清空校验成功标志,清除MFCrypto1On位
	memset(ucComMF522Buf, 0x00, MAXRLEN);

 	PcdSwitchPCB();
	
	ucComMF522Buf[0] = Pcb;
	ucComMF522Buf[1] = 0x01;



	ucComMF522Buf[2] = pDataIn[0];				// CLA
	ucComMF522Buf[3] = pDataIn[1];				// INS 			
	ucComMF522Buf[4] = pDataIn[2];				// P1						 
	ucComMF522Buf[5] = pDataIn[3];				// P2					
	ucComMF522Buf[6] = pDataIn[4];				// LEN	
	


	for(i=0;i<ucComMF522Buf[6];i++)				//DATA
	{
		ucComMF522Buf[7+i] = pDataIn[5+i];	  

	}
		
								
    CalulateCRC(ucComMF522Buf,In_Len+2,&ucComMF522Buf[In_Len+2]);	// 生成发送内容的CRC校验,保存到最后两个字节
	for(i=0; i <In_Len+4;i++){
			printf("%x ",ucComMF522Buf[i]);
	}	
	printf("\n");


	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,In_Len+4,pDataOut,&unLen); 		
// for(i=0; i < 16;i++){
//     printf("%x ",pDataOut[i]);
// }	
// printf("\n");
	if (status == MI_OK)
	{
	
		Out_Len[0] = unLen/8-4;	 //接收到数据的长度,不要前面和后面的各两个字节，才是返回的有用数据
						
		return MI_OK;

	}
	else
	{
	    Out_Len[0] = unLen/8-4;
		if((pDataOut[2]==0x90)&&(pDataOut[3]==0x00))
		return MI_OK;
		else
		return MI_ERR;

	}

}

