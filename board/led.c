#include "led.h"
#include "main.h"                  // Device header

LED_INFO led_info = {0};

/**
 * @brief 设置LED状态
 * @param status: LED_ON点亮 / LED_OFF熄灭
 * @note 低电平点亮LED(PAx引脚)
 */
void LED_Set(_Bool status)
{
	// status=LED_ON(1) → 输出低Bit_RESET，点亮
	// status=LED_OFF(0) → 输出高Bit_SET，熄灭
	HAL_GPIO_WritePin(GPIOB, KEY1_Pin, (status == LED_ON) ? GPIO_PIN_RESET : GPIO_PIN_SET);
	
	led_info.LED_Status = status;
}
