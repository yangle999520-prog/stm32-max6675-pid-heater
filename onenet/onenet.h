#ifndef __ONENET_H
#define __ONENET_H

#include "stm32f0xx_hal.h"
#include <stdbool.h>

extern char devid[16];
extern char key[48];
extern volatile uint8_t onenet_link_state;
extern volatile uint16_t onenet_rx_count;
extern volatile uint8_t onenet_last_rx_type;
extern volatile uint32_t onenet_last_rx_tick;

bool OneNET_RegisterDevice(void);
bool OneNet_DevLink(void);
void OneNet_SendData(void);
void OneNET_Publish(const char *topic, const char *msg);
void OneNET_Subscribe(void);
void OneNet_RevPro(unsigned char *cmd);

#endif
