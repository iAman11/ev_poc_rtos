#include "app_main.h"
#include "app_adc.h"
#include "app_pwm.h"
#include "app_relay.h"
#include "app_led.h"

uint32_t peak_voltage_pa1 = 0;  
uint32_t control_knob_pa2 = 0;  
uint32_t last_heartbeat_time = 0;

#define ADC_9V_THRESHOLD 2500
#define ADC_6V_THRESHOLD 2000

EVSE_State_t current_evse_state = EVSE_STATE_A_DISCONNECTED;

void App_Init(void)
{
	App_PWM_Init();   
	App_Relay_Init(); 
	App_LED_Init();   
}

void App_main(void)
{
	uint32_t current_time = HAL_GetTick();

	App_ADC_Read_All(&peak_voltage_pa1, &control_knob_pa2);
	uint32_t active_duty_cycle = (control_knob_pa2 < 2048) ? 260 : 530;

	// Read the PE Detection Circuit (HIGH = Missing/Fault, LOW = Safe)
	// Make sure to match the GPIO Port and Pin to whatever you picked in CubeMX!
	GPIO_PinState pe_status = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);

	// --------------------------------------------------
	// EVSE PROTOCOL STATE MACHINE
	// --------------------------------------------------
	
	if (pe_status == GPIO_PIN_RESET)
	{
	    // CRITICAL FAULT: Protective Earth is Missing!
	    current_evse_state = EVSE_STATE_FAULT_PE;
	    
	    App_PWM_SetDutyCycle(1000); // Force flat +12V (disables charging)
	    App_Relay_TurnOff();        // Force Relay OFF
	}
	else 
	{
	    // NORMAL OPERATION: PE is connected, proceed with J1772 logic
	    if (peak_voltage_pa1 > ADC_9V_THRESHOLD)
	    {
	        current_evse_state = EVSE_STATE_A_DISCONNECTED;
	        App_PWM_SetDutyCycle(1000); 
	        App_Relay_TurnOff();        
	    }
	    else if (peak_voltage_pa1 > ADC_6V_THRESHOLD)
	    {
	        current_evse_state = EVSE_STATE_B_CONNECTED;
	        App_PWM_SetDutyCycle(active_duty_cycle);
	        App_Relay_TurnOff();                     
	    }
	    else 
	    {
	        current_evse_state = EVSE_STATE_C_CHARGING;
	        App_PWM_SetDutyCycle(active_duty_cycle);
	        App_Relay_TurnOn();                      
	    }
	}

	// Tell the LED module to update the colors based on the state
	App_LED_SetState(current_evse_state);

	// --------------------------------------------------
	// TIMED BLINKING LOGIC (Every 500ms)
	// --------------------------------------------------
	if ((current_time - last_heartbeat_time) >= 500)
	{
	    App_LED_ToggleHeartbeat(current_evse_state);
	    last_heartbeat_time = current_time;
	}

	HAL_Delay(50);
}
