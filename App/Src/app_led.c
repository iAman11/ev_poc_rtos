#include "main.h"
#include "app_led.h"
#include "FreeRTOS.h"
#include "task.h"

/* LED Hardware Definitions */
#define LED_PORT        GPIOC
#define LED_RED_PIN     GPIO_PIN_14
#define LED_BLUE_PIN    GPIO_PIN_13

/* Helper Macros */
#define LED_RED_ON()    HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN, GPIO_PIN_SET)
#define LED_RED_OFF()   HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN, GPIO_PIN_RESET)

#define LED_BLUE_ON()   HAL_GPIO_WritePin(LED_PORT, LED_BLUE_PIN, GPIO_PIN_SET)
#define LED_BLUE_OFF()  HAL_GPIO_WritePin(LED_PORT, LED_BLUE_PIN, GPIO_PIN_RESET)

#define LED_RED_TOGGLE()    HAL_GPIO_TogglePin(LED_PORT, LED_RED_PIN)
#define LED_BLUE_TOGGLE()   HAL_GPIO_TogglePin(LED_PORT, LED_BLUE_PIN)

extern EVSE_State_t current_evse_state;

void App_LED_Init(void)
{
    LED_RED_ON();      // Default: Disconnected state
    LED_BLUE_OFF();
}

void App_LED_SetState(EVSE_State_t state)
{
    static EVSE_State_t last_state = EVSE_STATE_UNKNOWN;

    if (state != last_state)
    {
        LED_RED_OFF();
        LED_BLUE_OFF();

        switch (state)
        {
            case EVSE_STATE_A_DISCONNECTED: LED_RED_ON(); break;
            case EVSE_STATE_pCable_DISCONNECTED: LED_RED_ON(); LED_BLUE_ON(); break;
            case EVSE_STATE_B_CONNECTED:    LED_BLUE_ON(); break;
            case EVSE_STATE_C_CHARGING:     LED_BLUE_ON(); break;
            case EVSE_STATE_FAULT_PE:       LED_RED_ON(); break;
            default: break;
        }
        last_state = state;
    }
}

void App_LED_ToggleHeartbeat(EVSE_State_t current_state)
{
    switch (current_state)
    {
        case EVSE_STATE_C_CHARGING: LED_BLUE_TOGGLE(); break;
        case EVSE_STATE_FAULT_PE:   LED_RED_TOGGLE(); break;
        default: break;
    }
}

// RTOS UI Task Wrapper
void vTask_LED_Heartbeat(void *pvParameters)
{
    while(1)
    {
        App_LED_ToggleHeartbeat(current_evse_state);
        
        // Non-blocking RTOS delay. Yields CPU for 500ms.
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}
