#ifndef APP_LED_H
#define APP_LED_H

#include "main.h"
#include "app_evse.h"

void App_LED_Init(void);
void App_LED_SetState(EVSE_State_t state);
void App_LED_ToggleHeartbeat(EVSE_State_t state);

// The FreeRTOS Task
void vTask_LED_Heartbeat(void *pvParameters);

#endif