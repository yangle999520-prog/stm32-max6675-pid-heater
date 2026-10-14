#ifndef __LED_H
#define __LED_H

#include "stm32f0xx.h"
#include <stdbool.h>

#define LED_ON     1
#define LED_OFF    0

typedef struct
{
    uint8_t LED_Status;
}LED_INFO;

extern LED_INFO led_info;

void LED_Set(_Bool status);

#endif
