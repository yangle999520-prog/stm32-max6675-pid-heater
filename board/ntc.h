#ifndef __NTC_H
#define __NTC_H

#define R0      10000.0f    // NTC 25℃电阻
#define B_VALUE 3950.0f     // B值
#define R_PULL  10000.0f    // 分压电阻
#define ADC_MAX 4095.0f     // 12bit ADC
#define V_REF   3.3f        // 参考电压

float NTC_GetTemp(void);
	
#endif
