#include "app_evse.h"
#include "app_pwm.h"
#include "app_relay.h"
#include "app_led.h"
#include "app_main.h"
#include "app_adc.h"
#include "app_comm.h"

extern IWDG_HandleTypeDef hiwdg;

#define ADC_9V_THRESHOLD 2600
#define ADC_6V_THRESHOLD 1900
#define ADC_3V_THRESHOLD 1000
#define ADC_PE_FAULT_THRESHOLD 3200

EVSE_State_t current_evse_state = EVSE_STATE_A_DISCONNECTED;
float actual_temp_c = 0.0f;
uint32_t average_sensor_pa4 = 0;

static uint8_t is_authorized = 0; // 0 = Waiting for App, 1 = Allowed to charge
static float user_max_current = 16.0f; // Default 16 Amps

void App_EVSE_Init(void)
{
    // FIXED: Updated to use the correct global variable name
    current_evse_state = EVSE_STATE_A_DISCONNECTED;
}


// Called by ESP32 UART Command
void App_EVSE_SetAuthorization(uint8_t enable) {
    is_authorized = enable;
}

// Called by ESP32 UART Command
void App_EVSE_SetMaxCurrent(float amps) {
    // Safety clamp (6A to 32A standard)
    if (amps < 6.0f) amps = 6.0f;
    if (amps > 32.0f) amps = 32.0f;
    user_max_current = amps;
}


EVSE_State_t App_EVSE_Process(uint32_t cp_voltage, uint32_t cable_value, uint32_t pe_adc_value)
{
    // Static variables for Universal Debounce
    static EVSE_State_t pending_state = EVSE_STATE_UNKNOWN;
    static uint16_t transition_timer = 0;

    EVSE_State_t target_state = EVSE_STATE_UNKNOWN;
    uint32_t max_duty = 1000;

        // --- Cable Capacity & Current Calculation ---
        // Standard IEC61851: Duty Cycle (%) = Amps / 0.6
        // Since your timer period is 1000, Duty Ticks = (Amps / 0.6) * 10
        uint32_t user_pwm_ticks = (uint32_t)((user_max_current / 0.6f) * 10.0f);
    // --- Cable Capacity Calculation ---
        if (cable_value == 0) {
                max_duty = 1000; // Unplugged
            } else if (cable_value < 2100) {
                max_duty = 530;  // 32A Cable (53% duty)
            } else {
                max_duty = 266;  // 16A Cable (26.6% duty)
            }

    // Apply whichever is smaller: Cable limit OR App requested limit
        uint32_t active_duty = (user_pwm_ticks < max_duty) ? user_pwm_ticks : max_duty;

    // -------------------------------------------------------------------
    // STEP 1: Determine what the state *should* be based on raw sensors
    // -------------------------------------------------------------------
    if (pe_adc_value > ADC_PE_FAULT_THRESHOLD)
    {
        target_state = EVSE_STATE_FAULT_PE;
    }
    else if (cable_value == 0) // 🚫 Block everything if ADC is zero
    {
        target_state = EVSE_STATE_pCable_DISCONNECTED;
    }
    else if (cp_voltage > ADC_9V_THRESHOLD)
    {
        target_state = EVSE_STATE_A_DISCONNECTED;
    }
    else if (cp_voltage > ADC_6V_THRESHOLD)
    {
        target_state = EVSE_STATE_B_CONNECTED;
    }
    else if (cp_voltage > ADC_3V_THRESHOLD)
    {
    	if (is_authorized) {
    	    target_state = EVSE_STATE_C_CHARGING;
    	} else {
    	    // Force it to stay in State B (PWM active, Relay open) until authorized
    		target_state = EVSE_STATE_READY;
    	}
    }
    else
    {
        target_state = EVSE_STATE_FAULT_PE;
    }

    // -------------------------------------------------------------------
    // STEP 2: Universal Debounce Logic (The 200ms Filter)
    // -------------------------------------------------------------------
    if (target_state != current_evse_state)
    {
        if (target_state == pending_state)
        {
            if (transition_timer < 1000)
                transition_timer++;

            if (transition_timer >= 200) // 200ms of stable signal required
            {
                current_evse_state = target_state;
                transition_timer = 0;
            }
        }
        else
        {
            pending_state = target_state;
            transition_timer = 1;
        }
    }
    else
    {
        transition_timer = 0;
        pending_state = EVSE_STATE_UNKNOWN;
    }

    // -------------------------------------------------------------------
    // STEP 3: Execute Hardware Controls based on the APPROVED state
    // -------------------------------------------------------------------
    uint32_t target_pwm = 1000;

    switch (current_evse_state)
    {
    case EVSE_STATE_pCable_DISCONNECTED:
    case EVSE_STATE_A_DISCONNECTED:
        App_Relay_TurnOff();
        target_pwm = 1000;
        break;

    case EVSE_STATE_B_CONNECTED:
        App_Relay_TurnOff();
        target_pwm = active_duty;
        break;

    case EVSE_STATE_READY:
         App_Relay_TurnOff();
         target_pwm = active_duty;
         break;

    case EVSE_STATE_C_CHARGING:
        App_Relay_TurnOn();
        target_pwm = active_duty;
        break;

    case EVSE_STATE_FAULT_PE:
    default:
        App_Relay_TurnOff();
        target_pwm = 1000;
        break;
    }

    App_PWM_SetDutyCycle(target_pwm);

    return current_evse_state;
}

// RTOS Task Wrapper
void vTask_EVSE_Logic(void *pvParameters)
{
    ADC_Data_t received_data;

// INCREASED BUFFER: 50 samples @ 1kHz = 50ms window
// This guarantees we capture at least two full cycles of 50Hz/60Hz fluctuations
#define PE_WINDOW_SIZE 50
    uint32_t pa4_samples[PE_WINDOW_SIZE] = {0};
    uint8_t pa4_sample_index = 0;
    uint8_t is_first_run = 1;

    App_PWM_Init(); // Start the timer to trigger the ADC
    App_ADC_Start_DMA();

    while (1)
    {
        // Block indefinitely until new ADC data arrives from DMA
        if (xQueueReceive(xAdcDataQueue, &received_data, portMAX_DELAY) == pdPASS)
        {
            // --- FEED THE WATCHDOG ---
            HAL_IWDG_Refresh(&hiwdg);

            // 1. Fill the rolling buffer
            if (is_first_run)
            {
                for (int i = 0; i < PE_WINDOW_SIZE; i++)
                    pa4_samples[i] = received_data.pe_adc;
                is_first_run = 0;
            }

            pa4_samples[pa4_sample_index] = received_data.pe_adc;
            pa4_sample_index = (pa4_sample_index + 1) % PE_WINDOW_SIZE;

            // 2. Find the AVERAGE value in the window to filter out noise spikes
            uint32_t pe_sum = 0;
            for (int i = 0; i < PE_WINDOW_SIZE; i++)
            {
                pe_sum += pa4_samples[i];
            }
            average_sensor_pa4 = pe_sum / PE_WINDOW_SIZE;

            // 3. Process Temperature
            float temp_mv = ((float)received_data.temp_adc * 3300.0f) / 4095.0f;
            actual_temp_c = (temp_mv - 400.0f) / 19.5f;

            // 4. Execute Core EVSE State Machine
            // We now pass the MINIMUM value to the state machine instead of the average
            current_evse_state = App_EVSE_Process(
                received_data.cp_adc,
                received_data.cable_adc,
                average_sensor_pa4);

            // 5. Instantly update UI Base State
            App_LED_SetState(current_evse_state);
            // Send Data to ESP32 every ~1 second <---
            static uint32_t telemetry_timer = 0;
            telemetry_timer++;
            if (telemetry_timer >= 1000) // Assuming 1kHz loop rate
            {
                telemetry_timer = 0;

                // Replace 230.0, 16.0 etc., with your actual calculated variables later
                float mock_voltage = 230.0f;
                float mock_current = 16.0f;
                float mock_power = mock_voltage * mock_current;
                float mock_energy = 2.5f;

                App_Comm_SendTelemetry(mock_voltage, mock_current, mock_power, mock_energy, current_evse_state);
            }
        }
    }
}
