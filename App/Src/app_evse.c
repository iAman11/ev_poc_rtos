#include "app_evse.h"
#include "app_pwm.h"
#include "app_relay.h"
#include "app_led.h"
#include "app_main.h"

#define ADC_9V_THRESHOLD 2600
#define ADC_6V_THRESHOLD 1900
#define ADC_3V_THRESHOLD 1000
#define ADC_PE_FAULT_THRESHOLD 3900

EVSE_State_t current_evse_state = EVSE_STATE_A_DISCONNECTED;
float actual_temp_c = 0.0f;

void App_EVSE_Init(void)
{
	current_evse_state = EVSE_STATE_A_DISCONNECTED;
}

// The core logic function remains completely unchanged
EVSE_State_t App_EVSE_Process(uint32_t cp_voltage, uint32_t cable_value, uint32_t pe_adc_value)
{
    EVSE_State_t new_state = current_evse_state;
    uint32_t target_pwm = 1000;

    uint32_t duty = (cable_value < 2100) ? 530 : 260;

    if (pe_adc_value > ADC_PE_FAULT_THRESHOLD)
    {
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
    current_evse_state = new_state;

    return current_evse_state;
}

// RTOS Task Wrapper
void vTask_EVSE_Logic(void *pvParameters)
{
    ADC_Data_t received_data;
    
    // State variables for moving average
    uint32_t pa4_samples[20] = {0};
    uint8_t  pa4_sample_index = 0;
    uint8_t  is_first_run = 1;
    uint32_t averaged_sensor_pa4 = 0;
    
    while(1)
    {
        // Block indefinitely until new ADC data arrives from DMA
        if (xQueueReceive(xAdcDataQueue, &received_data, portMAX_DELAY) == pdPASS)
        {
            // 1. Process PA4 (PE) Moving Average
            if (is_first_run)
            {
                for (int i = 0; i < 20; i++) pa4_samples[i] = received_data.pe_adc;
                is_first_run = 0;
            }
            
            pa4_samples[pa4_sample_index] = received_data.pe_adc;
            pa4_sample_index = (pa4_sample_index + 1) % 20;
            
            uint32_t sum = 0;
            for (int i = 0; i < 20; i++) sum += pa4_samples[i];
            averaged_sensor_pa4 = sum / 20;

            // 2. Process Temperature
            float temp_mv = ((float)received_data.temp_adc * 3300.0f) / 4095.0f;
            actual_temp_c = (temp_mv - 400.0f) / 19.5f;

            // 3. Execute Core EVSE State Machine
            current_evse_state = App_EVSE_Process(
                received_data.cp_adc, 
                received_data.cable_adc, 
                averaged_sensor_pa4
            );

            // 4. Instantly update UI Base State
            App_LED_SetState(current_evse_state);
        }
    }
}
