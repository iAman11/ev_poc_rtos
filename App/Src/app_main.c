#include "app_main.h"
#include "app_adc.h"
#include "app_led.h"
#include "app_evse.h"
#include "app_pwm.h"
#include "app_relay.h"


uint32_t cp_adc = 0;
uint32_t cable_adc = 0;
uint32_t temp_sen_adc = 0;
uint32_t last_heartbeat_time = 0;

float actual_temp_c = 0.0f;

// === NEW PA4 SENSOR VARIABLES ===
uint32_t raw_sensor_pa4 = 0;          // The raw single reading
uint32_t pa4_samples[20] = {0};       // Array to hold 20 samples
uint8_t  pa4_sample_index = 0;        // Keeps track of where we are in the array
uint32_t averaged_sensor_pa4 = 0;

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
    static uint8_t is_first_run = 1;

//    if (current_time < 2000)
//    {
//        // We still want the heartbeat LED to blink while we wait!
//        if ((current_time - last_heartbeat_time) >= 500)
//        {
//            App_LED_ToggleHeartbeat(current_evse_state);
//            last_heartbeat_time = current_time;
//        }
//        HAL_Delay(50);
//        return; // EXIT EARLY! Do not run the ADC or EVSE code below yet.
//    }

    App_ADC_Read_All(&cp_adc, &cable_adc, &temp_sen_adc, &raw_sensor_pa4);
    if (is_first_run)
    {
        for (int i = 0; i < 20; i++)
        {
            pa4_samples[i] = raw_sensor_pa4;
        }
        is_first_run = 0;
    }

    pa4_samples[pa4_sample_index] = raw_sensor_pa4;

    // Move to the next slot in the array (and wrap around to 0 if we hit 20)
    pa4_sample_index++;
    if (pa4_sample_index >= 20)
    {
        pa4_sample_index = 0;
    }

    // Add up all 20 numbers in the array
    uint32_t sum = 0;
    for (int i = 0; i < 20; i++)
    {
        sum += pa4_samples[i];
    }

    // Divide by 20 to get the perfectly smoothed average!
    averaged_sensor_pa4 = sum / 20;

    float temp_mv = ((float)temp_sen_adc * 3300.0f) / 4095.0f;

    // Step B: Apply the sensor's formula: (Voltage - 400mV) / 19.5mV per degree
    actual_temp_c = (temp_mv - 400.0f) / 19.5f;

    current_evse_state =
        App_EVSE_Process(cp_adc, cable_adc, averaged_sensor_pa4);

    App_LED_SetState(current_evse_state);

    if ((current_time - last_heartbeat_time) >= 500)
    {
        App_LED_ToggleHeartbeat(current_evse_state);
        last_heartbeat_time = current_time;
    }

    HAL_Delay(50);
}
