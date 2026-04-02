#include "app_comm.h"
#include "app_main.h" // For access to the RTOS Queues
#include "FreeRTOS.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_MSG_SIZE 128

static uint8_t rx_byte;
static char rx_buffer[MAX_MSG_SIZE];
static uint16_t rx_index = 0;
static uint8_t is_receiving = 0;

static const char* GetStateString(EVSE_State_t state) {
    switch(state) {
        case EVSE_STATE_A_DISCONNECTED:      return "GUN_NOT_CONNECTED";
        case EVSE_STATE_pCable_DISCONNECTED: return "POWER_CABLE_FAULT";
        case EVSE_STATE_B_CONNECTED:         return "GUN_CONNECTED";
        case EVSE_STATE_READY:         		 return "READY_TO_CHARGE";
        case EVSE_STATE_C_CHARGING:          return "CHARGING";
        case EVSE_STATE_FAULT_PE:            return "FAULT";
        default:                             return "UNKNOWN";
    }
}

void App_Comm_Init(void) {
    // Start listening for the first byte via Interrupt
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

// 1. QUEUEING DATA TO SEND
void App_Comm_SendTelemetry(float voltage, float current, float power, float energy, EVSE_State_t state) {
    char tx_buffer[MAX_MSG_SIZE];

    int len = snprintf(tx_buffer, sizeof(tx_buffer),
                       "$DATA,V,%.1f,I,%.1f,P,%.1f,E,%.3f,STATUS,%s#",
                       voltage, current, power, energy, GetStateString(state));

    if (len > 0 && len < sizeof(tx_buffer)) {
        // Drop it into the TX Queue instantly (Don't wait if full, just drop it to keep EVSE loop fast)
        xQueueSend(xUartTxQueue, tx_buffer, 0); 
    }
}

// 2. INTERRUPT RECEIVER
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) { 
        if (rx_byte == '$') {
            rx_index = 0;
            is_receiving = 1;
        }
        else if (rx_byte == '#' && is_receiving) {
            rx_buffer[rx_index] = '\0';
            is_receiving = 0;

            // Packet complete! Send it to the UART task queue
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xQueueSendFromISR(xUartRxQueue, rx_buffer, &xHigherPriorityTaskWoken);
            
            // ---> NEW: Wipe the global buffer clean for the debugger!
            memset(rx_buffer, 0, MAX_MSG_SIZE);

            // Yield to the UART task if it was waiting for this queue
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        else if (is_receiving && rx_index < (MAX_MSG_SIZE - 1)) {
            rx_buffer[rx_index++] = rx_byte;
        }

        // Re-arm the interrupt listener
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

// 3. ROBUST ERROR RECOVERY
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // Clear Overrun/Noise/Framing flags to prevent the STM32 from going deaf
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1); // Restart listener
    }
}

// 4. COMMAND PARSER
void App_Comm_ProcessCommand(const char* payload) {
    if (strncmp(payload, "CMD,", 4) == 0) {
        const char* cmd_body = payload + 4;

        if (strncmp(cmd_body, "START_CHARGING", 14) == 0) {
            App_EVSE_SetAuthorization(1);
        }
        else if (strncmp(cmd_body, "STOP_CHARGING", 13) == 0) {
            App_EVSE_SetAuthorization(0);
        }
        else if (strncmp(cmd_body, "SET_CURRENT:", 12) == 0) {
            float target_current = atof(cmd_body + 12);
            App_EVSE_SetMaxCurrent(target_current);
        }
    }
}

// 5. THE DEDICATED UART RTOS TASK
void vTask_UART_Comm(void *pvParameters) {
    char rx_msg[MAX_MSG_SIZE];
    char tx_msg[MAX_MSG_SIZE];
    
    App_Comm_Init(); // Start the hardware interrupt listener

    while(1) {
        // 1. Check for incoming commands (Wait up to 10ms for a command)
        if (xQueueReceive(xUartRxQueue, rx_msg, pdMS_TO_TICKS(10)) == pdPASS) {
            // A command arrived from the ISR! Parse it here safely.
            App_Comm_ProcessCommand(rx_msg);

            // ---> NEW: Wipe the local task buffer after executing
            memset(rx_msg, 0, MAX_MSG_SIZE);

        }
        
        // 2. Check for outgoing telemetry (Do not wait, just check if the EVSE task queued something)
        if (xQueueReceive(xUartTxQueue, tx_msg, 0) == pdPASS) {
            // Safe, blocking transmission that ONLY blocks this task, not the EVSE task
            HAL_UART_Transmit(&huart1, (uint8_t*)tx_msg, strlen(tx_msg), HAL_MAX_DELAY);
        }
    }
}
