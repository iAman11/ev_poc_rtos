#include "app_adc.h"
#include "app_main.h" // For ADC_Data_t and xAdcDataQueue

extern ADC_HandleTypeDef hadc;
extern QueueHandle_t xAdcDataQueue;

// Raw array for the DMA hardware to write into automatically
volatile uint32_t adc_dma_buffer[4]; 

void App_ADC_Start_DMA(void)
{
    // Start ADC conversion using DMA. The CPU does not wait.
    HAL_ADC_Start_DMA(&hadc, (uint32_t*)adc_dma_buffer, 4);
}

// Hardware Interrupt Callback: Fires automatically when the DMA has filled the buffer
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    ADC_Data_t fresh_data;

    // Package the DMA data
    fresh_data.cp_adc = adc_dma_buffer[0];
    fresh_data.cable_adc = adc_dma_buffer[1];
    fresh_data.temp_adc = adc_dma_buffer[2];
    fresh_data.pe_adc = adc_dma_buffer[3];

    // Send the data to the EVSE task queue.
    xQueueSendFromISR(xAdcDataQueue, &fresh_data, &xHigherPriorityTaskWoken);
    
    // Force a context switch if the EVSE task is higher priority than the currently running task
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}