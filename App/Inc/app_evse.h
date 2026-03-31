#ifndef APP_EVSE_H
#define APP_EVSE_H

#include "main.h"

typedef enum
{
    EVSE_STATE_UNKNOWN = 0,
    EVSE_STATE_pCable_DISCONNECTED,
    EVSE_STATE_A_DISCONNECTED,
    EVSE_STATE_B_CONNECTED,
    EVSE_STATE_C_CHARGING,
    EVSE_STATE_FAULT_PE
} EVSE_State_t;

void App_EVSE_Init(void);
EVSE_State_t App_EVSE_Process(uint32_t cp_voltage, uint32_t cable_value, uint32_t pe_adc_value);

// The FreeRTOS Task
void vTask_EVSE_Logic(void *pvParameters);

#endif