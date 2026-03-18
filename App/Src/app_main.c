#include "app_main.h"
#include "app_adc.h"
#include "app_led.h"
#include "app_evse.h"
#include "app_pwm.h"
#include "app_relay.h"
#include "FreeRTOS.h"
#include "task.h"

// OS Primitives
QueueHandle_t xAdcDataQueue;
TaskHandle_t xEvseTaskHandle;
TaskHandle_t xLedTaskHandle;

void App_Init(void)
{
    // 1. Initialize Hardware periperhals
    App_PWM_Init();
    App_Relay_Init();
    App_LED_Init();
    App_EVSE_Init();

    // 2. Create RTOS Primitives
    // Queue depth of 5 is plenty, as the task should process it instantly
    xAdcDataQueue = xQueueCreate(5, sizeof(ADC_Data_t));

    // 3. Create Tasks
    // Highest Priority: Process EVSE Safety Logic
    xTaskCreate(vTask_EVSE_Logic, "EVSE_Task", 256, NULL, configMAX_PRIORITIES - 1, &xEvseTaskHandle);
    
    // Lower Priority: Handle LED Blinking UI
    xTaskCreate(vTask_LED_Heartbeat, "LED_Task", 128, NULL, 1, &xLedTaskHandle);

    // 4. Start the ADC via DMA (Non-blocking)
    App_ADC_Start_DMA();

    // NOTE: The CubeMX generated main.c will call osKernelStart() / vTaskStartScheduler() after this returns.
}