#ifndef APP_EVSE_H
#define APP_EVSE_H

#include "main.h"
#include "app_main.h"

typedef enum
{
    EVSE_STATE_UNKNOWN        = 0,
    EVSE_STATE_A_DISCONNECTED = 1,
    EVSE_STATE_B_CONNECTED    = 2,
    EVSE_STATE_C_CHARGING     = 3,
    EVSE_STATE_FAULT_PE       = 4,  // Protective-Earth (ground) fault detected via PA4
    EVSE_STATE_FAULT_CP       = 5   // Control-Pilot short / State-E fault (CP <= ~3 V)
} EVSE_State_t;

// Global volatile variables – add these to STM32CubeIDE Live Expressions
extern volatile EVSE_State_t current_evse_state;
extern volatile float        actual_temp_c;
extern volatile ADC_Data_t   live_adc_values;

void App_EVSE_Init(void);
EVSE_State_t App_EVSE_Process(uint32_t cp_voltage, uint32_t cable_value, uint32_t pe_adc_value);

// The FreeRTOS Task
void vTask_EVSE_Logic(void *pvParameters);

#endif