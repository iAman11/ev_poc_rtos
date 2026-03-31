#include "app_evse.h"
#include "app_pwm.h"
#include "app_relay.h"
#include "app_led.h"
#include "app_main.h"
#include "app_adc.h"

extern IWDG_HandleTypeDef hiwdg;

#define ADC_9V_THRESHOLD 2600
#define ADC_6V_THRESHOLD 1900
#define ADC_3V_THRESHOLD 1000
#define ADC_PE_FAULT_THRESHOLD 3200

EVSE_State_t current_evse_state = EVSE_STATE_A_DISCONNECTED;
float actual_temp_c = 0.0f;
uint32_t average_sensor_pa4 = 0;

void App_EVSE_Init(void)
{
    // FIXED: Updated to use the correct global variable name
    current_evse_state = EVSE_STATE_A_DISCONNECTED;
}

EVSE_State_t App_EVSE_Process(uint32_t cp_voltage, uint32_t cable_value, uint32_t pe_adc_value)
{
    // Static variables for Universal Debounce
    static EVSE_State_t pending_state = EVSE_STATE_UNKNOWN;
    static uint16_t transition_timer = 0;
    
    EVSE_State_t target_state = EVSE_STATE_UNKNOWN;
    uint32_t duty;

    // --- Cable Capacity Calculation ---
    if (cable_value == 0)       // Cable physically unplugged from EVSE
    {
        duty = 1000;            // No PWM (Solid +12V)
    }
    else if (cable_value < 2100) 
    {
        duty = 530;             // High capacity cable
    }
    else 
    {
        duty = 260;             // Standard capacity cable
    }

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
        target_state = EVSE_STATE_C_CHARGING;
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
            if (transition_timer < 1000) transition_timer++;
            
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
            target_pwm = duty; 
            break;
            
        case EVSE_STATE_C_CHARGING:
            App_Relay_TurnOn(); 
            target_pwm = duty;  
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
    uint8_t  pa4_sample_index = 0;
    uint8_t  is_first_run = 1;

    App_PWM_Init();       // Start the timer to trigger the ADC
	App_ADC_Start_DMA();
    
    while(1)
    {
        // Block indefinitely until new ADC data arrives from DMA
        if (xQueueReceive(xAdcDataQueue, &received_data, portMAX_DELAY) == pdPASS)
        {
            // --- FEED THE WATCHDOG ---
            HAL_IWDG_Refresh(&hiwdg);
            
            // 1. Fill the rolling buffer
            if (is_first_run)
            {
                for (int i = 0; i < PE_WINDOW_SIZE; i++) pa4_samples[i] = received_data.pe_adc;
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
				average_sensor_pa4
            );

            // 5. Instantly update UI Base State
            App_LED_SetState(current_evse_state);
        }
    }
}
