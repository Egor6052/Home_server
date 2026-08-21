#include "stm32f1xx_hal.h"
#include "headers/main_decl.h"
#include <string.h>
#include <stdbool.h>

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

static volatile uint8_t  uart1_rx_buff[RX_BUFF_SIZE];
static volatile uint16_t uart1_rx_head = 0;
static volatile uint16_t uart1_rx_tail = 0;
static uint8_t            uart1_rx_it_byte;

static volatile uint8_t  uart2_rx_buff[RX_BUFF_SIZE];
static volatile uint16_t uart2_rx_head = 0;
static volatile uint16_t uart2_rx_tail = 0;
static uint8_t            uart2_rx_it_byte;

// UART1: PA9=TX, PA10=RX
void UART1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitStruct.Pin   = GPIO_PIN_9; // TX
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_10; // RX
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 9600;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) { Error_Handler(); }

    HAL_NVIC_SetPriority(USART1_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    HAL_UART_Receive_IT(&huart1, &uart1_rx_it_byte, 1);
}

// UART2: PA2=TX, PA3=RX
void UART2_Init(void) {
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_2; // TX
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_3; // RX
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 9600;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) { Error_Handler(); }

    HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    HAL_UART_Receive_IT(&huart2, &uart2_rx_it_byte, 1);
}

void UART1_SendString(const char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), 100);
}

void UART2_SendString(const char *str) {
    HAL_UART_Transmit(&huart2, (uint8_t *)str, strlen(str), 100);
}

void uart1_flush(void) {
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    uart1_rx_head = uart1_rx_tail;
}

void uart2_flush(void) {
    __HAL_UART_CLEAR_OREFLAG(&huart2);
    uart2_rx_head = uart2_rx_tail;
}

// Через кільцевий буфер, який наповнює HAL_UART_RxCpltCallback нижче
bool UART1_ReadByte(uint8_t *out) {
    if (uart1_rx_tail == uart1_rx_head) return false;
    *out = uart1_rx_buff[uart1_rx_tail];
    uart1_rx_tail = (uint16_t)((uart1_rx_tail + 1) % RX_BUFF_SIZE);
    return true;
}

bool UART2_ReadByte(uint8_t *out) {
    if (uart2_rx_tail == uart2_rx_head) return false;
    *out = uart2_rx_buff[uart2_rx_tail];
    uart2_rx_tail = (uint16_t)((uart2_rx_tail + 1) % RX_BUFF_SIZE);
    return true;
}

void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}

void USART2_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart2);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        uint16_t next_head = (uint16_t)((uart1_rx_head + 1) % RX_BUFF_SIZE);
        if (next_head != uart1_rx_tail) {
            uart1_rx_buff[uart1_rx_head] = uart1_rx_it_byte;
            uart1_rx_head = next_head;
        }
        HAL_UART_Receive_IT(&huart1, &uart1_rx_it_byte, 1);
    } else if (huart->Instance == USART2) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        // HAL_GPIO_TogglePin(POWER_LED_GPIO_Port, POWER_LED);
        uint16_t next_head = (uint16_t)((uart2_rx_head + 1) % RX_BUFF_SIZE);
        if (next_head != uart2_rx_tail) {
            uart2_rx_buff[uart2_rx_head] = uart2_rx_it_byte;
            uart2_rx_head = next_head;
        }
        HAL_UART_Receive_IT(&huart2, &uart2_rx_it_byte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        HAL_UART_Receive_IT(&huart1, &uart1_rx_it_byte, 1);
    } else if (huart->Instance == USART2) {
        HAL_GPIO_TogglePin(HDD_LED_GPIO_Port, HDD_1_LED);
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        HAL_UART_Receive_IT(&huart2, &uart2_rx_it_byte, 1);
    }
}