#include "app_evse.h"
#include "app_pwm.h"
#include "app_relay.h"

#define ADC_9V_THRESHOLD 2600
#define ADC_6V_THRESHOLD 1900
#define ADC_3V_THRESHOLD 1000
//#define ADC_9V_THRESHOLD 4000
//#define ADC_6V_THRESHOLD 4000
//#define ADC_3V_THRESHOLD 1000
#define ADC_PE_FAULT_THRESHOLD 3900

static EVSE_State_t current_state = EVSE_STATE_A_DISCONNECTED;

void App_EVSE_Init(void)
{
    current_state = EVSE_STATE_A_DISCONNECTED;
}

EVSE_State_t App_EVSE_Process(uint32_t cp_voltage, uint32_t cable_value, uint32_t pe_adc_value)
{
    EVSE_State_t new_state = current_state;
    uint32_t target_pwm = 1000;

    uint32_t duty =
        (cable_value < 2100) ? 530 : 260;

    if (pe_adc_value > ADC_PE_FAULT_THRESHOLD)
    {
        // FAULT: The voltage has dropped below our safety threshold
        new_state = EVSE_STATE_FAULT_PE;
        App_Relay_TurnOff();
    }
    else
    {
        if (cp_voltage > ADC_9V_THRESHOLD)
        {
            new_state = EVSE_STATE_A_DISCONNECTED;
            App_Relay_TurnOff();
        }
        else if (cp_voltage > ADC_6V_THRESHOLD)
        {
            new_state = EVSE_STATE_B_CONNECTED;
            target_pwm = duty;
            App_Relay_TurnOff();
        }
        else if (cp_voltage > ADC_3V_THRESHOLD)
        {
            new_state = EVSE_STATE_C_CHARGING;
            target_pwm = duty;
            App_Relay_TurnOn();
        }
        else
        {
        	new_state = EVSE_STATE_FAULT_PE;
        	App_Relay_TurnOff();
        }
    }

    App_PWM_SetDutyCycle(target_pwm);
    current_state = new_state;

    return current_state;
}
