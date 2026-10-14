//单片机头文件
#include "main.h"
//网络设备驱动
#include "esp8266.h"
//硬件驱动
#include "usart.h"
#include "OLED.h"
//C库
#include <string.h>
#include <stdio.h>

#define ESP8266_WIFI_INFO		"AT+CWJAP=\"yangle\",\"y1234567\"\r\n"
unsigned char esp8266_buf[512];
volatile unsigned short esp8266_cnt = 0;
static uint8_t esp8266_rx_byte;
static volatile uint32_t esp8266_rx_tick;

//==========================================================
//	函数名称：	ESP8266_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_Clear(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    memset(esp8266_buf, 0, sizeof(esp8266_buf));
    esp8266_cnt = 0;
    __set_PRIMASK(primask);
}
//==========================================================
//	函数名称：	ESP8266_WaitRecive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool ESP8266_WaitRecive(void)
{
    if(esp8266_cnt == 0 || HAL_GetTick() - esp8266_rx_tick < 20)
        return REV_WAIT;
    return REV_OK;
}
//==========================================================
//	函数名称：	ESP8266_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	uint32_t timeOut = 200;
    if(strstr(cmd, "AT+CWJAP") || strstr(cmd, "AT+CIPSTART"))
        timeOut = 2000;
    ESP8266_Clear();
	// HAL库发送字符串，huart1是串口2句柄（CubeMX生成）
	HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen((const char *)cmd), 100);
	
	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//如果检索到关键词
			{
				ESP8266_Clear();									//清空缓存
				return 0;
			}
		}
		HAL_Delay(10);
	}
	return 1;
}
//==========================================================
//	函数名称：	ESP8266_SendData
//
//	函数功能：	发送数据
//
//	入口参数：	data：数据
//				len：长度
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{
	char cmdBuf[32];
	ESP8266_Clear();								//清空接收缓存
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//发送命令
	if(!ESP8266_SendCmd(cmdBuf, ">"))				//收到‘>’时可以发送数据
	{
		HAL_UART_Transmit(&huart1, data, len, 100);
	}
}
//==========================================================
//	函数名称：	ESP8266_GetIPD
//
//	函数功能：	获取平台返回的数据
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//				如ESP8266的返回格式为	"+IPD,x:yyy"	x代表数据长度，yyy是数据内容
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{
	char *ptrIPD = NULL;
	
	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								//如果接收完成
		{
			ptrIPD = strstr((char *)esp8266_buf, "IPD,");				//搜索“IPD”头
			if(ptrIPD == NULL)											//如果没找到，可能是IPD头的延迟，还是需要等待一会，但不会超过设定的时间
			{
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//找到':'
				if(ptrIPD != NULL)
				{
					ptrIPD++;
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;
			}
		}
		HAL_Delay(5);
	} while(timeOut--);
	
	return NULL;														//超时还未找到，返回空指针
}
//==========================================================
//	函数名称：	ESP8266_Init
//
//	函数功能：	初始化ESP8266
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_Init(void)
{
	ESP8266_Clear();
    HAL_UART_Receive_IT(&huart1, &esp8266_rx_byte, 1);
    OLED_ShowString(1, 1, "ESP AT CHECK    ");
	
	while(ESP8266_SendCmd("AT\r\n", "OK"))
		HAL_Delay(500);
	
	while(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
		HAL_Delay(500);
	
	while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
		HAL_Delay(500);
	
	while(ESP8266_SendCmd("ATE0\r\n", "OK"))
        HAL_Delay(500);
	while(ESP8266_SendCmd("AT+CIPMUX=0\r\n", "OK"))
        HAL_Delay(500);
	while(ESP8266_SendCmd("AT+CIPMODE=0\r\n", "OK"))
        HAL_Delay(500);
    OLED_ShowString(1, 1, "WIFI CONNECT    ");
	while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "OK"))
		HAL_Delay(500);
    OLED_ShowString(1, 1, "WIFI OK         ");
	
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        if(esp8266_cnt < sizeof(esp8266_buf) - 1)
        {
            esp8266_buf[esp8266_cnt++] = esp8266_rx_byte;
            esp8266_buf[esp8266_cnt] = 0;
        }
        esp8266_rx_tick = HAL_GetTick();
        HAL_UART_Receive_IT(huart, &esp8266_rx_byte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        HAL_UART_Receive_IT(huart, &esp8266_rx_byte, 1);
    }
}
