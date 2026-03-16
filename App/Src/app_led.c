#include "main.h"
#include "app_led.h"

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


/* Initialize LED states */
void App_LED_Init(void)
{
    LED_RED_ON();      // Default: Disconnected state
    LED_BLUE_OFF();
}


/* Set base LED state based on EVSE state */
void App_LED_SetState(EVSE_State_t state)
{
    static EVSE_State_t last_state = EVSE_STATE_UNKNOWN;

    /* Only update LEDs if state has changed */
    if (state != last_state)
    {
        /* Clear LEDs before applying new state */
        LED_RED_OFF();
        LED_BLUE_OFF();

        switch (state)
        {
            case EVSE_STATE_A_DISCONNECTED:
                LED_RED_ON();
                break;

            case EVSE_STATE_B_CONNECTED:
                LED_BLUE_ON();
                break;

            case EVSE_STATE_C_CHARGING:
                /* Start Blue ON so blink starts clean */
                LED_BLUE_ON();
                break;

            case EVSE_STATE_FAULT_PE:
                /* Start Red ON so blink starts clean */
                LED_RED_ON();
                break;

            default:
                /* Unknown state → both OFF */
                break;
        }

        last_state = state;
    }
}


/* Called periodically (ex: 500ms timer) for blinking */
void App_LED_ToggleHeartbeat(EVSE_State_t current_state)
{
    switch (current_state)
    {
        case EVSE_STATE_C_CHARGING:
            LED_BLUE_TOGGLE();
            break;

        case EVSE_STATE_FAULT_PE:
            LED_RED_TOGGLE();
            break;

        default:
            /* No blinking in other states */
            break;
    }
}
