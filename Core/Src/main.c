/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "key.h"
#include "esp8266.h"
#include "onenet.h"
#include <string.h>
#define TEMP_SAMPLE_MS 250U
#define TEMP_STALE_MS 1000U
#define ONENET_RECONNECT_MS 5000U
#define ONENET_RX_TIMEOUT_MS 15000U
#define PWM_MAX_COMPARE 95U
#include "MAX6675.h"
#include "pid.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t keynum;

uint8_t mode = 0;
float WaterTemperature = -1.0f;
static uint32_t WaterTemperature_tick = 0;
static volatile uint32_t WaterTemperature_valid_tick = 0;
static volatile uint8_t WaterTemperature_valid = 0;

PID_t WaterTemperaturePID={
	.Target=40,

	.Kp=5.0f,
	.Ki=0.125f,
	.Kd=0.0f,

	.Error0=0,
	.Error1=0,
	.ErrorInt=0,

	.OutMax=PWM_MAX_COMPARE,
	.OutMin=0,
};

	uint32_t upload_tick = 0;
	static uint32_t oled_log_tick = 0;
	static uint32_t oled_value_tick = 0;
	static uint32_t reconnect_tick = 0;
	uint16_t val=0;
	uint8_t previous_mode = 0;
	float previous_target = 40.0f;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  unsigned char *mqtt_data;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
	/* CS is high after GPIO init; allow the first conversion to finish. */
	WaterTemperature_tick = HAL_GetTick();
	WaterTemperature_valid_tick = WaterTemperature_tick;
	OLED_Init();
//	HAL_TIM_Base_Start_IT(&htim3);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	__HAL_TIM_ENABLE_IT(&htim3, TIM_IT_UPDATE);
	PID_Init(&WaterTemperaturePID);
	WaterTemperaturePID.Target = 40.0f;
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);

	ESP8266_Init();
	while (OneNet_DevLink() != 0)
	{
		HAL_Delay(1000);
	}
	OneNET_Subscribe();
	upload_tick = HAL_GetTick();
	OLED_Clear();
	OLED_ShowString(1, 1, "TEMP:");
	OLED_ShowString(1, 9, " C");
	OLED_ShowString(2, 1, "SET:");
	OLED_ShowString(2, 8, " OUT:");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		keynum = Key_GetNum();

        /* Manual keys adjust PWM duty in 10-count steps. */
        if (mode == 1U) {
            if (keynum == 1U && val < PWM_MAX_COMPARE) {
                val += 10U;
                if (val > PWM_MAX_COMPARE) val = PWM_MAX_COMPARE;
            }
            if (keynum == 2U) val = val >= 10U ? val - 10U : 0U;
        }
        if (mode != previous_mode || WaterTemperaturePID.Target != previous_target) {
            PID_Reset(&WaterTemperaturePID);
            previous_mode = mode;
            previous_target = WaterTemperaturePID.Target;
        }
        /* Wait for conversion without blocking the main loop. */
        if ((uint32_t)(HAL_GetTick() - WaterTemperature_tick) >= TEMP_SAMPLE_MS) {
            WaterTemperature = MAX6675_ReadWaterTemperature();
            WaterTemperature_tick = HAL_GetTick();
            if (WaterTemperature >= 0.0f) {
                WaterTemperature_valid = 1U;
                WaterTemperature_valid_tick = WaterTemperature_tick;
                if (mode == 0U && WaterTemperaturePID.Target > 0.0f) {
                    WaterTemperaturePID.Actual = WaterTemperature;
                    PID_Update(&WaterTemperaturePID);
                }
            } else {
                WaterTemperature_valid = 0U;
                PID_Reset(&WaterTemperaturePID);
            }
        }
        /* Fault, stale sample, or zero setpoint disables heating in both modes. */
        if (WaterTemperature_valid == 0U ||
            (uint32_t)(HAL_GetTick() - WaterTemperature_valid_tick) > TEMP_STALE_MS ||
            (mode == 0U && WaterTemperaturePID.Target <= 0.0f)) {
            PID_Reset(&WaterTemperaturePID);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);
        } else {
//					            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1,
                mode == 1U ? val : (uint16_t)WaterTemperaturePID.Out);
        }
		if ((uint32_t)(HAL_GetTick() - oled_value_tick) >= TEMP_SAMPLE_MS)
		{
			oled_value_tick = HAL_GetTick();
			if (WaterTemperature < 0.0f)
				OLED_ShowString(1, 6, "ERR");
			else
				OLED_ShowNum(1, 6, (uint32_t)WaterTemperature, 3);
			OLED_ShowNum(2, 5, (uint32_t)WaterTemperaturePID.Target, 3);
			OLED_ShowNum(2, 13,
				mode == 1U ? val : (uint16_t)WaterTemperaturePID.Out, 3);
		}
		if (ESP8266_WaitRecive() == REV_OK)
		{
			if (strstr((char *)esp8266_buf, "WIFI DISCONNECT") != NULL ||
			    strstr((char *)esp8266_buf, "CLOSED") != NULL)
				onenet_link_state = 0;
			mqtt_data = ESP8266_GetIPD(0);
			if (mqtt_data != NULL)
			{
				OneNet_RevPro(mqtt_data);
			}
			ESP8266_Clear();
		}
		if (onenet_link_state == 1U && esp8266_send_count != 0U &&
		    esp8266_last_send_status != 1U)
		{
			onenet_link_state = 0U;
		}
		if (onenet_link_state == 1U &&
		    (uint32_t)(HAL_GetTick() - onenet_last_rx_tick) > ONENET_RX_TIMEOUT_MS)
		{
			onenet_link_state = 0U;
		}
		if (onenet_link_state == 0U &&
		    (uint32_t)(HAL_GetTick() - reconnect_tick) >= ONENET_RECONNECT_MS)
		{
			reconnect_tick = HAL_GetTick();
			onenet_link_state = 2U;
			if (ESP8266_ConnectWiFi() &&
			    OneNet_DevLink() == 0)
			{
				OneNET_Subscribe();
				if (esp8266_last_send_status == 1U)
				{
					onenet_link_state = 1U;
					upload_tick = HAL_GetTick();
					OLED_ShowString(2, 1, "SET:");
					OLED_ShowString(2, 8, " OUT:");
				}
				else
				{
					onenet_link_state = 0U;
				}
			}
			else
			{
				onenet_link_state = 0U;
			}
		}
		if ((uint32_t)(HAL_GetTick() - oled_log_tick) >= 500U)
		{
			oled_log_tick = HAL_GetTick();
			OLED_ShowString(3, 1, onenet_link_state == 1U ? "N:UP    " :
			                (onenet_link_state == 2U ? "N:RETRY " : "N:DOWN  "));
			OLED_ShowString(3, 8, "TX:");
			OLED_ShowNum(3, 11, esp8266_send_count, 3);
			OLED_ShowString(4, 1, "AT:");
			OLED_ShowString(4, 4, esp8266_last_send_status == 1U ? "OK   " :
			                 (esp8266_last_send_status == 2U ? "UART " : "PROM "));
			OLED_ShowString(4, 9, "R:");
			OLED_ShowNum(4, 11, onenet_rx_count, 2);
			OLED_ShowString(4, 13, "T:");
			OLED_ShowHexNum(4, 15, onenet_last_rx_type, 1);
		}

		if (onenet_link_state == 1U &&
		    (uint32_t)(HAL_GetTick() - upload_tick) >= 2000U)
		{
			upload_tick = HAL_GetTick();
			OneNet_SendData();
		}


	HAL_Delay(20);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM3) //判断是不是TIM3的更新中断
  {
			Key_Tick();
            /* Interrupt watchdog also runs during blocking OneNET calls. */
            if (WaterTemperature_valid == 0U ||
                (uint32_t)(HAL_GetTick() - WaterTemperature_valid_tick) > TEMP_STALE_MS)
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (uint16_t)WaterTemperaturePID.Out);
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
