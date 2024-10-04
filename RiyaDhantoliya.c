#include "stm32f4xx_hal.h"
#include <stdio.h>

int isSent = 1;
int isReceived = 1;
int hasError = 0;
uint8_t TxData[10240];
uint8_t RxData[128];
uint8_t dataBuffer[10240];
int head = 0;
int tail = 0;
int countinterrupt = 0;
int countloop = 0;
UART_HandleTypeDef huart2;

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        isSent = 1;
        countinterrupt++;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        isReceived = 1;
        countinterrupt++;

        for (int i = 0; i < sizeof(RxData); i++)
        {
            dataBuffer[head] = RxData[i];
            head = (head + 1) % sizeof(dataBuffer);
        }

        HAL_UART_Receive_DMA(&huart2, RxData, sizeof(RxData));
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        hasError = 1;

        if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
            printf("Overrun Error (ORE) occurred.\n");
        }
        if (huart->ErrorCode & HAL_UART_ERROR_FE) {
            printf("Frame Error (FE) occurred.\n");
        }
        if (huart->ErrorCode & HAL_UART_ERROR_NE) {
            printf("Noise Error (NE) occurred.\n");
        }
        if (huart->ErrorCode & HAL_UART_ERROR_PE) {
            printf("Parity Error (PE) occurred.\n");
        }

        HAL_UART_Abort(&huart2);
        HAL_UART_DeInit(&huart2);
        HAL_UART_Init(&huart2);

        HAL_UART_Receive_DMA(&huart2, RxData, sizeof(RxData));
        hasError = 0;
    }
}

void processBufferData()
{
    while (tail != head)
    {
        uint8_t dataByte = dataBuffer[tail];
        tail = (tail + 1) % sizeof(dataBuffer);
        printf("Processed byte: %c\n", dataByte);
    }
}

int main()
{
    HAL_UART_Receive_DMA(&huart2, RxData, sizeof(RxData));

    while (1)
    {
        if (isSent == 1)
        {
            HAL_UART_Transmit_DMA(&huart2, TxData, sizeof(TxData));
            isSent = 0;
        }

        if (isReceived == 1)
        {
            processBufferData();
            isReceived = 0;
        }

        if (hasError == 1)
        {
        }

        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(500);
        countloop++;
    }
}
