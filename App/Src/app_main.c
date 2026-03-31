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
    // 1. Initialize Hardware peripherals
    // App_PWM_Init();
    App_Relay_Init();
    App_LED_Init();
    App_EVSE_Init();


    // 2. Create RTOS Primitives
    xAdcDataQueue = xQueueCreate(5, sizeof(ADC_Data_t));

    // 3. Create Tasks
    xTaskCreate(vTask_EVSE_Logic, "EVSE_Task", 256, NULL, configMAX_PRIORITIES - 1, &xEvseTaskHandle);
    xTaskCreate(vTask_LED_Heartbeat, "LED_Task", 128, NULL, 1, &xLedTaskHandle);

    // 4. Start the ADC via DMA
    // App_ADC_Start_DMA();
}
