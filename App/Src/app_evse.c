#include "app_evse.h"
#include "app_pwm.h"
#include "app_relay.h"

#define ADC_9V_THRESHOLD 2500
#define ADC_6V_THRESHOLD 2000

static EVSE_State_t current_state = EVSE_STATE_A_DISCONNECTED;

void App_EVSE_Init(void)
{
    current_state = EVSE_STATE_A_DISCONNECTED;
}

EVSE_State_t App_EVSE_Process(uint32_t cp_voltage, uint32_t cable_value)
{
    EVSE_State_t new_state = current_state;

    uint32_t duty =
        (cable_value < 2048) ? 260 : 530;

    GPIO_PinState pe_status =
        HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);

    if (pe_status == GPIO_PIN_SET)
    {
        new_state = EVSE_STATE_FAULT_PE;

        App_PWM_SetDutyCycle(1000);
        App_Relay_TurnOff();
    }
    else
    {
        if (cp_voltage > ADC_9V_THRESHOLD)
        {
            new_state = EVSE_STATE_A_DISCONNECTED;

            App_PWM_SetDutyCycle(1000);
            App_Relay_TurnOff();
        }
        else if (cp_voltage > ADC_6V_THRESHOLD)
        {
            new_state = EVSE_STATE_B_CONNECTED;

            App_PWM_SetDutyCycle(duty);
            App_Relay_TurnOff();
        }
        else
        {
            new_state = EVSE_STATE_C_CHARGING;

            App_PWM_SetDutyCycle(duty);
            App_Relay_TurnOn();
        }
    }

    current_state = new_state;

    return current_state;
}
