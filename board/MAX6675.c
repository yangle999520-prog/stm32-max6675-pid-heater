#include "main.h"  
#include "MAX6675.h"  
#include "spi.h" 

float MAX6675_ReadTemp(void)
{
    uint16_t tx = 0xFFFF;
    uint16_t data = 0;

    HAL_Delay(250); 	
    HAL_GPIO_WritePin(MAX6675_CS_GPIO_Port, MAX6675_CS_Pin, GPIO_PIN_RESET);

    if (HAL_SPI_TransmitReceive(&hspi1,
                                (uint8_t *)&tx,
                                (uint8_t *)&data,
                                1, 100) != HAL_OK) 
		{
        HAL_GPIO_WritePin(MAX6675_CS_GPIO_Port, MAX6675_CS_Pin, GPIO_PIN_SET);
        return -1.0f;
    }

    HAL_GPIO_WritePin(MAX6675_CS_GPIO_Port, MAX6675_CS_Pin, GPIO_PIN_SET);

    if (data & 0x0004) 
		{
        return -1.0f;  // ÈÈµçÅ¼¶ÏÏß
    }

    return ((data >> 3) & 0x0FFF) * 0.25f;
}
