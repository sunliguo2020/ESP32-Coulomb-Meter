#include "i2c.h"
#include "delay.h"

//初始化IIC
void Init_I2C(void)
{					     
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA,GPIO_Pin_6|GPIO_Pin_7); 	//PB12,PB13 输出高
}

//SDA脚设置为OUT
void I2C_SDA_OUT(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;        
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
}
//SDA脚设置为IN
void I2C_SDA_IN(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
}

//产生IIC起始信号
void I2C_Start(void)
{
	I2C_SDA_OUT();     //sda线输出
	I2C_SDA_SET;	  	  
	I2C_SCL_SET;
	delay_us(4);
 	I2C_SDA_RESET;//START:when CLK is high,DATA change form high to low 
	delay_us(4);
	I2C_SCL_RESET;//
}	  
//产生IIC停止信号
void I2C_Stop(void)
{
	I2C_SDA_OUT();//sda线输出
//	I2C_SCL_RESET;
	I2C_SDA_RESET;//STOP:when CLK is high DATA change form low to high
 	delay_us(4);
	I2C_SCL_SET;
	delay_us(4);			 
	I2C_SDA_SET;//发送I2C总线结束信号
					   	
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
u8 I2C_Wait_Ack(void)
{
	u8 Time=0;
	I2C_SDA_OUT();//sda线输出
	I2C_SDA_SET;
	delay_us(1);	   
	I2C_SCL_SET;
	delay_us(1);	 
	I2C_SDA_IN();      //SDA设置为输入  
	delay_us(1);
	while(I2C_READ_SDA)
	{
		Time++;
		if(Time>250)
		{
			I2C_SCL_RESET;
			I2C_Stop();
			return 1;
		}
	}
	I2C_SCL_RESET;//时钟输出0 	   
	return 0;  
} 
//产生ACK应答
void I2C_Ack(void)
{
//	I2C_SCL_RESET;
	I2C_SDA_OUT();
	I2C_SDA_RESET;
	delay_us(5);
	I2C_SCL_SET;
	delay_us(5);
	I2C_SCL_RESET;
	delay_us(5);
	I2C_SDA_SET;
}
//不产生ACK应答		    
void I2C_NAck(void)
{
//	I2C_SCL_RESET;
	I2C_SDA_OUT();
	I2C_SDA_SET;
	delay_us(5);
	I2C_SCL_SET;
	delay_us(5);
	I2C_SCL_RESET;
	delay_us(5);
	I2C_SDA_SET;
	delay_us(5);
}					 				     
//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
void I2C_Send_Byte(u8 txd)
{                        
	u8 t;   
	I2C_SDA_OUT(); 	    
//	I2C_SCL_RESET;//拉低时钟开始数据传输
	for(t=0;t<8;t++)
	{              
		//IIC_SDA=(txd&0x80)>>7;
		if(txd&0x80)
			I2C_SDA_SET;
		else
			I2C_SDA_RESET;	  
		delay_us(5);   //对TEA5767这三个延时都是必须的
		I2C_SCL_SET;
		txd<<=1; 
		delay_us(5); 
		I2C_SCL_RESET;	
		delay_us(5);
	}	 
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
u8 I2C_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	I2C_SCL_RESET;	
	delay_us(5);
	I2C_SDA_SET;
	delay_us(5);
	I2C_SDA_IN();//SDA设置为输入
	
	for(i=0;i<8;i++ )
	{
		I2C_SCL_RESET; 
		delay_us(5);
		I2C_SCL_SET;
		delay_us(5);
		receive<<=1;
		if(I2C_READ_SDA)
			receive++;   
		delay_us(5); 
	}					 
//	if(!ack)
//		I2C_NAck();//发送nACK
//	else
//		I2C_Ack(); //发送ACK   
	return receive;
}


/******************************************
*函数名：Write_Byte_E2PROM
*输入：	addr  写入的地址
		dat	  写入的数据
*返回：无
*作用：把dat写入eeprom的addr里
******************************************/
void Write_Byte_E2PROM(unsigned char addr, unsigned char dat)
{
	I2C_Start();
	I2C_Send_Byte(eeprom_wr_addr);
	I2C_Ack();
	I2C_Send_Byte(addr);
	I2C_Ack();
	I2C_Send_Byte(dat);
	I2C_Ack();
	I2C_Stop();
}

/******************************************
*函数名：Read_Byte_E2PROM
*输入：addr
*返回：dat
*作用：从eeprom的addr地址读取数据
******************************************/
unsigned char Read_Byte_E2PROM(unsigned char addr)
{
	unsigned char dat;
	
	I2C_Start();
	I2C_Send_Byte(eeprom_wr_addr);
	I2C_Ack();
	I2C_Send_Byte(addr);
	I2C_Ack();
	I2C_Start();
	I2C_Send_Byte(eeprom_rd_addr);
	I2C_Ack();
	dat = I2C_Read_Byte(0);
	I2C_NAck();
	I2C_Stop();
	
	return dat;	
}
