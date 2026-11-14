#include "main.h"
#include "esp8266.h"
#include "device_credentials.h"

#include "usart.h"
#include "OLED.h"

#include <string.h>
#include <stdio.h>

unsigned char esp8266_buf[512];
volatile unsigned short esp8266_cnt = 0;
volatile uint8_t esp8266_last_send_status = 0;
volatile uint16_t esp8266_send_count = 0;
static uint8_t esp8266_rx_byte;
static volatile uint32_t esp8266_rx_tick;

void ESP8266_Clear(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    memset(esp8266_buf, 0, sizeof(esp8266_buf));
    esp8266_cnt = 0;
    __set_PRIMASK(primask);
}


_Bool ESP8266_WaitRecive(void)
{
    if(esp8266_cnt == 0 || HAL_GetTick() - esp8266_rx_tick < 20)
        return REV_WAIT;
    return REV_OK;
}


_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	uint32_t timeOut = 200;
	if(strstr(cmd, "AT+CWJAP"))
		timeOut = 1500;
	else if(strstr(cmd, "AT+CIPSTART"))
		timeOut = 1000;
    ESP8266_Clear();
	
	HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen((const char *)cmd), 100);
	
	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		
			{
				ESP8266_Clear();									
				return 0;
			}
		}
		HAL_Delay(10);
	}
	return 1;
}

void ESP8266_SendData(unsigned char *data, unsigned short len)
{
	char cmdBuf[32];
	esp8266_last_send_status = 0;
	ESP8266_Clear();								
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		
	if(!ESP8266_SendCmd(cmdBuf, ">"))				
	{
		if(HAL_UART_Transmit(&huart1, data, len, 100) == HAL_OK)
		{
			esp8266_last_send_status = 1;
			esp8266_send_count++;
		}
		else
		{
			esp8266_last_send_status = 2;
		}
	}
}

unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{
	char *ptrIPD = NULL;
	
	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								
		{
			ptrIPD = strstr((char *)esp8266_buf, "IPD,");				
			if(ptrIPD == NULL)											
			{
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							
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
	
	return NULL;														
}

_Bool ESP8266_ConnectWiFi(void)
{
	return ESP8266_SendCmd(ESP8266_WIFI_INFO, "OK") == 0;
}

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
	while(!ESP8266_ConnectWiFi())
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
