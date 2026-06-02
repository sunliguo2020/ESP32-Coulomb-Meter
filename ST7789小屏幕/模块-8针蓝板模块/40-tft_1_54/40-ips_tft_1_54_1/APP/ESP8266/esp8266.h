#ifndef __ESP8266_H
#define __ESP8266_H
#include "sys.h"
#include "stdio.h"	

#define REV_OK		0	//接收完成标志
#define REV_WAIT	1	//接收未完成标志

const u8* wifista_ssid="cxxh";			//路由器SSID号
const u8* wifista_password="123456789"; 	//连接密码

#define WEATHER_PORTNUM 	"80"		//天气连接端口号:80	
#define WEATHER_SERVERIP 	"api.seniverse.com"		//天气服务器IP
////苏宁易购连接端口号:80
//#define TIMER_PORTNUM 	"80"			
//#define TIMER_SERVERIP 	"quan.suning.com"		//苏宁易购服务器IP

//拼多多连接端口号:80
#define TIMER_PORTNUM 	"80"			
#define TIMER_SERVERIP 	"qapi.pinduoduo.com"		 //拼多多服务器IP

//函数声明
unsigned char *ESP8266_GetIPD_GET(unsigned short timeOut,u8 *buff);

void ESP8266_Clear(void);
_Bool ESP8266_WaitRecive(void);

_Bool ESP8266_SendCmd(char *cmd, char *res);
void ESP8266_SendData(unsigned char *data, unsigned short len);

void ESP8266_Init(void);
void Get_current_time(void);

//解析苏宁返回数据
void cJSON_Time_Parse(void);
int Get_Year(char *y);
int Get_Moonth(char *m);
int Get_Day(char *d);
int Get_Times(char *h, char *m, char *s);

		
#endif
