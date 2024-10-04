#include "stm32f4xx_hal.h"
#include <stdio.h>

#define BUFFER_SIZE 10240  // Maximum size for your buffer
uint8_t TxData[10240];     // Transmission buffer
uint8_t RxData[128];       // Temporary buffer for DMA reception (smaller chunk)
uint8_t dataBuffer[BUFFER_SIZE];  // Circular buffer for incoming data

int isSent = 1;            // Flag for transmission complete
int isReceived = 1;        // Flag for reception complete
int hasError = 0;          // Flag for UART errors
int countinterrupt = 0;    // Debugging: number of interrupts
int countloop = 0;         // Debugging: number of loop executions

int head = 0;              // Circular buffer head pointer
int tail = 0;              // Circular buffer tail pointer

UART_HandleTypeDef huart2;  // Assuming UART2 is used

// Callback function for transmission completion
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        isSent = 1;   // Flag to indicate data has been transmitted
        countinterrupt++; // Increment the interrupt counter for debugging
    }
}

// Callback function for reception completion
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        isReceived = 1;  // Flag to indicate data has been received
        countinterrupt++; // Increment the interrupt counter for debugging

        // Append received data to the circular buffer
        for (int i = 0; i < sizeof(RxData); i++)
        {
            // Insert data into buffer and update the head position
            dataBuffer[head] = RxData[i];
            head = (head + 1) % BUFFER_SIZE;  // Wrap head position if it exceeds buffer size
        }

        // Restart DMA reception for the next chunk of data
        HAL_UART_Receive_DMA(&huart2, RxData, sizeof(RxData));
    }
}

// Callback function for UART error handling
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        hasError = 1;  // Flag to indicate a UART error occurred

        // Check the specific error type
        if (huart->ErrorCode & HAL_UART_ERROR_ORE)
        {
            // Overrun Error (ORE): Data is overwritten before being read
            printf("Overrun Error (ORE) occurred.\n");
        }
        if (huart->ErrorCode & HAL_UART_ERROR_FE)
        {
            // Frame Error (FE): Incorrect stop bits or frame format
            printf("Frame Error (FE) occurred.\n");
        }
        if (huart->ErrorCode & HAL_UART_ERROR_NE)
        {
            // Noise Error (NE): Noise during data reception
            printf("Noise Error (NE) occurred.\n");
        }
        if (huart->ErrorCode & HAL_UART_ERROR_PE)
        {
            // Parity Error (PE): Mismatch in expected parity
            printf("Parity Error (PE) occurred.\n");
        }

        // Abort the ongoing UART operation due to the error
        HAL_UART_Abort(&huart2);

        // Reset UART and restart communication
        HAL_UART_DeInit(&huart2);
        HAL_UART_Init(&huart2);

        // Reinitialize DMA reception after error handling
        HAL_UART_Receive_DMA(&huart2, RxData, sizeof(RxData));

        // Reset the error flag
        hasError = 0;
    }
}

// Function to process the received data from the circular buffer
void processBufferData()
{
    // Process the data in the circular buffer if new data is available
    while (tail != head)
    {
        uint8_t dataByte = dataBuffer[tail];  // Get the byte to process
        tail = (tail + 1) % BUFFER_SIZE;      // Move tail forward

        // Example: process or print received data (custom logic here)
        printf("Processed byte: %x\n", dataByte);
    }
}

int main(void)
{
    // UART and DMA initialization
    HAL_UART_Receive_DMA(&huart2, RxData, sizeof(RxData));

    while (1)
    {
        // Check if transmission is complete
        if (isSent == 1)
        {
            // Start DMA transmission (non-blocking)
            HAL_UART_Transmit_DMA(&huart2, TxData, sizeof(TxData));
            isSent = 0;  // Reset the flag to indicate ongoing transmission
        }

        // Check if data has been received and process it
        if (isReceived == 1)
        {
            processBufferData();  // Process incoming data from the circular buffer
            isReceived = 0;  // Reset the flag to indicate new reception
        }

        // Check if there was an error (error handling is done in the callback)
        if (hasError == 1)
        {
            // Error handling logic is in the error callback, nothing to do here
        }

        // Perform other non-blocking tasks (GPIO toggle example)
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(500);  // Delay to simulate non-blocking task execution
        countloop++;  // Increment loop counter for debugging
    }
}
