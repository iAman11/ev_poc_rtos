#include "app_pwm.h"

extern TIM_HandleTypeDef htim1;

void App_PWM_Init(void)
{
    // 1. Start the invisible trigger channel (Channel 1) to fire the ADC!
    HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_1);
    // Start generating the PWM signal on Timer 1, Channel 4 (PA11)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
}

void App_PWM_SetDutyCycle(uint32_t duty)
{
    // 3. Apply the calculated pulse width to the Timer
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, duty);
}
