#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "main.h" 
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

// Data structure to pass ADC readings safely
typedef struct {
    uint32_t cp_adc;
    uint32_t cable_adc;
    uint32_t temp_adc;
    uint32_t pe_adc;
} ADC_Data_t;

// Global RTOS Handles
extern QueueHandle_t xAdcDataQueue;
extern QueueHandle_t xUartTxQueue;
extern QueueHandle_t xUartRxQueue;

void App_Init(void);

#endif /* APP_MAIN_H */