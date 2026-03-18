#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "main.h" // Gives us access to the HAL and hardware definitions
#include "FreeRTOS.h"
#include "queue.h"

// Data structure to pass ADC readings safely between the DMA interrupt and the EVSE Task
typedef struct {
    uint32_t cp_adc;
    uint32_t cable_adc;
    uint32_t temp_adc;
    uint32_t pe_adc;
} ADC_Data_t;

// Global Queue Handle
extern QueueHandle_t xAdcDataQueue;

void App_Init(void);

#endif /* APP_MAIN_H */