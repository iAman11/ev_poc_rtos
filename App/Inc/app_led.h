#ifndef APP_LED_H
#define APP_LED_H

/* EVSE State Definitions */
typedef enum
{
    EVSE_STATE_UNKNOWN = 0,
    EVSE_STATE_A_DISCONNECTED,
    EVSE_STATE_B_CONNECTED,
    EVSE_STATE_C_CHARGING,
    EVSE_STATE_FAULT_PE
} EVSE_State_t;

void App_LED_Init(void);
void App_LED_SetState(EVSE_State_t state);
void App_LED_ToggleHeartbeat(EVSE_State_t current_state);

#endif
