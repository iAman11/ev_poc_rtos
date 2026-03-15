#ifndef APP_PWM_H
#define APP_PWM_H

#include "main.h"

void App_PWM_Init(void);
void App_PWM_SetDutyCycle(uint32_t adc_value);

#endif /* APP_PWM_H */
