#include "main.h"
#include "ntc.h"
#include "adc.h"
#include "math.h"

float NTC_GetTemp(void)
{
    uint16_t adc_val = 0;
    float Vadc, Rntc, temp;

    HAL_ADC_Start(&hadc);
    HAL_ADC_PollForConversion(&hadc, 10);
    adc_val = HAL_ADC_GetValue(&hadc);
    HAL_ADC_Stop(&hadc);

    // 算出ADC电压
    Vadc = adc_val / ADC_MAX * V_REF;
    // 计算NTC电阻
    Rntc = R_PULL * Vadc / (V_REF - Vadc);

    // B值公式
    temp = 1.0f / (1.0f/298.15f + log(Rntc/R0)/B_VALUE);
    temp = temp - 273.15f; // 转摄氏度

    return temp;
}
