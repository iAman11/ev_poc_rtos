#include "app_main.h"
#include "app_adc.h"
#include "app_led.h"
#include "app_evse.h"
#include "app_pwm.h"
#include "app_relay.h"


uint32_t peak_voltage_pa1 = 0;
uint32_t control_knob_pa2 = 0;
uint32_t last_heartbeat_time = 0;

EVSE_State_t current_evse_state;

void App_Init(void)
{
    App_PWM_Init();
    App_Relay_Init();
    App_LED_Init();
    App_EVSE_Init();
}

void App_main(void)
{
    uint32_t current_time = HAL_GetTick();

    App_ADC_Read_All(&peak_voltage_pa1, &control_knob_pa2);

    current_evse_state =
        App_EVSE_Process(peak_voltage_pa1, control_knob_pa2);

    App_LED_SetState(current_evse_state);

    if ((current_time - last_heartbeat_time) >= 500)
    {
        App_LED_ToggleHeartbeat(current_evse_state);
        last_heartbeat_time = current_time;
    }

    HAL_Delay(50);
}
