#include "app_main.h"
#include "app_adc.h"
#include "app_led.h"
#include "app_evse.h"
#include "app_pwm.h"
#include "app_relay.h"
#include "app_comm.h"

// OS Primitives
QueueHandle_t xAdcDataQueue;
QueueHandle_t xUartTxQueue;
QueueHandle_t xUartRxQueue;

TaskHandle_t xEvseTaskHandle;
TaskHandle_t xLedTaskHandle;
TaskHandle_t xUartTaskHandle;

void App_Init(void)
{
    // 1. Initialize Hardware peripherals
    App_Relay_Init();
    App_LED_Init();
    App_EVSE_Init();
    
    // (Note: App_Comm_Init() is now called from inside the UART task)

    // 2. Create RTOS Queues
    xAdcDataQueue = xQueueCreate(5, sizeof(ADC_Data_t));
    xUartTxQueue = xQueueCreate(2, 80); // 2 slots of 80 bytes
    xUartRxQueue = xQueueCreate(2, 80); // 2 slots of 80 bytes

    // 3. Create Tasks
    // FIXED: Increased EVSE task stack to 512 to prevent stack overflow
    xTaskCreate(vTask_EVSE_Logic, "EVSE_Task", 256, NULL, configMAX_PRIORITIES - 1, &xEvseTaskHandle);
    
    // NEW: Dedicated UART Task (Lower priority than EVSE safety loop)
    xTaskCreate(vTask_UART_Comm, "UART_Task", 192, NULL, configMAX_PRIORITIES - 1, &xUartTaskHandle);
    
    // UI Task
    xTaskCreate(vTask_LED_Heartbeat, "LED_Task", 128, NULL, 1, &xLedTaskHandle);
}