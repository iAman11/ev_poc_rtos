#include "app_relay.h"

void App_Relay_Init(void)
{
    // Ensure the relay is turned off when the system boots
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
}

void App_Relay_Toggle(void)
{
    // This HAL macro flips the pin from High to Low, or Low to High
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_12);
}

void App_Relay_TurnOn(void)
{
    // Close the relay (Send power to EV)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
}

void App_Relay_TurnOff(void)
{
    // Open the relay (Stop power to EV)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
}