#ifndef APP_COMM_H
#define APP_COMM_H

#include "main.h"
#include "app_evse.h"

// Expose the UART handle (defined in main.c after CubeMX generation)
extern UART_HandleTypeDef huart1;

void App_Comm_Init(void);
void App_Comm_SendTelemetry(float voltage, float current, float power, float energy, EVSE_State_t state);
void App_Comm_ProcessCommand(const char* cmd_str);

#endif /* APP_COMM_H */
