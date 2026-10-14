//#include "main.h"
///*全局变量，用于存储按键键码*/
//uint8_t Key_Num;

///**
//  * 函    数：获取全局变量定义的按键键码
//  * 参    数：无
//  * 返 回 值：按键键码
//  */
//uint8_t Key_GetNum(void)
//{
//	uint8_t Temp;			//定义一个临时变量用于中转
//	if (Key_Num)			//如果全局变量的键码不为0
//	{
//		/*这3句的目的是，实现读取键码并读后清零的效果*/
//		Temp = Key_Num;		//先把键码存入临时变量
//		Key_Num = 0;		//键码清零
//		return Temp;		//返回临时变量，return语句执行后，函数直接结束
//	}
//	return 0;				//如果if不成立，键码为0，则默认返回0
//}

///**
//  * 函    数：获取按键状态
//  * 参    数：无
//  * 返 回 值：有按键按下，直接返回键码（非阻塞），没有按键按下，返回0
//  */
//uint8_t Key_GetState(void)
//{
//	if (HAL_GPIO_ReadPin(GPIOB, KEY1_Pin) == 0)		//如果PB1引脚电平为0
//	{
//		return 1;		//直接返回键码1
//	}
//	if (HAL_GPIO_ReadPin(GPIOB, KEY2_Pin) == 0)		//如果PB0引脚电平为0
//	{
//		return 2;		//直接返回键码2
//	}
//	return 0;			//没有if成立，表示没有按键按下，默认返回0
//}

///**
//  * 函    数：用于驱动按键模块运行的自定义按键定时中断函数
//  * 参    数：无
//  * 返 回 值：无
//  * 注意事项：此函数必须在主程序中每隔1ms自动执行一次
//  */
//void Key_Tick(void)
//{
//	/*定义静态变量（默认初值为0，函数退出后保留值和存储空间）*/
//	static uint8_t Count;					//用于计次分频
//	static uint8_t CurrState, PrevState;	//保存按键本次状态和上次状态
//	
//	Count ++;			//计次自增
//	if (Count >= 20)	//如果计次20次，则if成立，即if每隔20ms进一次
//	{
//		Count = 0;		//计次清零，便于下次计次
//		
//		/*获取按键的本次状态和上次状态*/
//		PrevState = CurrState;			//获取上次状态
//		CurrState = Key_GetState();		//获取本次状态
//		
//		/*如果本次状态的键码为0，且上次键码不为0，即检测到按键松手瞬间*/
//		if (CurrState == 0 && PrevState != 0)
//		{
//			/*将上次状态的键码复制给全局变量，后续读取此变量，即可得知哪个按键按下了*/
//			Key_Num = PrevState;
//		}
//	}
//}

#include "main.h"    
#include "key.h"
/**
  * 函    数：按键获取键码
  * 参    数：无
  * 返 回 值：按下按键的键码值，范围：0~2，返回0代表没有按键按下
  * 注意事项：此函数是阻塞式操作，当按键按住不放时，函数会卡住，直到按键松手
  */
uint8_t Key_GetNum(void)
{
	uint8_t KeyNum = 0;		//定义变量，默认键码值为0
	
	if (HAL_GPIO_ReadPin(GPIOB, KEY1_Pin) == 0)			//读PB1输入寄存器的状态，如果为0，则代表按键1按下
	{
		HAL_Delay(20);											//延时消抖
		while (HAL_GPIO_ReadPin(GPIOB, KEY1_Pin) == 0);	//等待按键松手
		HAL_Delay(20);											//延时消抖
		KeyNum = 1;												//置键码为1
	}
	
	if (HAL_GPIO_ReadPin(GPIOB, KEY2_Pin) == 0)			//读PB11输入寄存器的状态，如果为0，则代表按键2按下
	{
		HAL_Delay(20);											//延时消抖
		while (HAL_GPIO_ReadPin(GPIOB, KEY2_Pin) == 0);	//等待按键松手
		HAL_Delay(20);											//延时消抖
		KeyNum = 2;												//置键码为2
	}
	
	return KeyNum;			//返回键码值，如果没有按键按下，所有if都不成立，则键码为默认值0
}
