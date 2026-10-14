#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f0xx_hal.h"
#include <stdbool.h>

#define REV_OK			0
#define REV_WAIT		1

extern unsigned char esp8266_buf[512];
extern volatile unsigned short esp8266_cnt;

void ESP8266_Clear(void);
_Bool ESP8266_WaitRecive(void);
_Bool ESP8266_SendCmd(char *cmd, char *res);
void ESP8266_SendData(unsigned char *data, unsigned short len);
unsigned char *ESP8266_GetIPD(unsigned short timeOut);
void ESP8266_Init(void);

#endif
