#ifndef APP_LED_H
#define APP_LED_H

#include "main.h"
#include "app_evse.h"  // Include this so LED functions know EVSE_State_t

void App_LED_Init(void);
void App_LED_SetState(EVSE_State_t state);
void App_LED_ToggleHeartbeat(EVSE_State_t state);

#endif