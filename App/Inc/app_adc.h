#ifndef APP_ADC_H
#define APP_ADC_H

#include "main.h" // Needed for standard integer types and HAL definitions

// Function prototype: Returns a 32-bit integer containing the ADC value
void App_ADC_Read_All(uint32_t* ch1_val, uint32_t* ch2_val, uint32_t* ch3_val, uint32_t* ch4_val);

#endif /* APP_ADC_H */
