#include "app_comm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define RX_BUFFER_SIZE 128
static uint8_t rx_byte;
static char rx_buffer[RX_BUFFER_SIZE];
static uint16_t rx_index = 0;
static uint8_t is_receiving = 0;

// Helper to convert STM32 state to ESP32 String
static const char* GetStateString(EVSE_State_t state) {
    switch(state) {
        case EVSE_STATE_A_DISCONNECTED:
        	return "GUN_NOT_CONNECTED";
        case EVSE_STATE_pCable_DISCONNECTED:
        	return "POWER_CABLE_FAULT";
        case EVSE_STATE_B_CONNECTED:
            return "GUN_CONNECTED";
        case EVSE_STATE_C_CHARGING:
            return "CHARGING";
        case EVSE_STATE_FAULT_PE:
            return "FAULT";
        default:
            return "UNKNOWN";
    }
}

void App_Comm_Init(void) {
    // Start listening for the first byte via Interrupt
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

// 1. SENDING DATA TO ESP32
void App_Comm_SendTelemetry(float voltage, float current, float power, float energy, EVSE_State_t state) {
    char tx_buffer[256];

    // Format exactly as the ESP32 protocol_parser.c expects
    int len = snprintf(tx_buffer, sizeof(tx_buffer),
                       "$DATA,V,%.1f,I,%.1f,P,%.1f,E,%.3f,STATUS,%s#",
                       voltage, current, power, energy, GetStateString(state));

    if (len > 0 && len < sizeof(tx_buffer)) {
        // Send over UART (Using polling for simplicity, but can be swapped to DMA/IT)
        HAL_UART_Transmit(&huart1, (uint8_t*)tx_buffer, len, HAL_MAX_DELAY);
    }
}

// 2. RECEIVING COMMANDS FROM ESP32
// This callback fires every time 1 byte is received from the ESP32
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) { // Change to your USART instance

        if (rx_byte == '$') {
            // Start of packet
            rx_index = 0;
            is_receiving = 1;
        }
        else if (rx_byte == '#' && is_receiving) {
            // End of packet
            rx_buffer[rx_index] = '\0';
            is_receiving = 0;

            // Process the complete command
            App_Comm_ProcessCommand(rx_buffer);
        }
        else if (is_receiving && rx_index < (RX_BUFFER_SIZE - 1)) {
            // Accumulate payload
            rx_buffer[rx_index++] = rx_byte;
        }

        // Re-arm the interrupt to catch the next byte
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

void App_Comm_ProcessCommand(const char* payload) {
    // payload looks like "CMD,START_CHARGING" or "CMD,SET_CURRENT:16.0"

    if (strncmp(payload, "CMD,", 4) == 0) {
        const char* cmd_body = payload + 4;

        if (strncmp(cmd_body, "START_CHARGING", 14) == 0) {
            // Authorize the EVSE to close the relay
            App_EVSE_SetAuthorization(1);
        }
        else if (strncmp(cmd_body, "STOP_CHARGING", 13) == 0) {
            // Revoke authorization (forces EVSE back to State B, opens relay)
            App_EVSE_SetAuthorization(0);
        }
        else if (strncmp(cmd_body, "SET_CURRENT:", 12) == 0) {
            // Extract the float value and update the PWM
            float target_current = atof(cmd_body + 12);
            App_EVSE_SetMaxCurrent(target_current);
        }
    }
}
