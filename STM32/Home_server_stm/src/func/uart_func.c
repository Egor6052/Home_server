
#include "headers/main_decl.h"

#define UART2_RX_BUFFER_SIZE 128

uint8_t uart2_rx_buffer[UART2_RX_BUFFER_SIZE]; 
uint8_t rx_index = 0;
uint8_t rx_byte;

uint16_t get_UART2_data(void) {
    uint16_t received_data = 0;
    if (HAL_UART_Receive(&huart2, uart2_rx_buffer, 2, HAL_MAX_DELAY) == HAL_OK) {
        received_data = (uart2_rx_buffer[0] << 8) | uart2_rx_buffer[1];
    }
    return received_data;
}

void send_to_UART2(char *msg) {
    // Просто відправляємо рядок без
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

uint8_t UART2_ReceiveByte_NonBlocking(uint8_t *byte) {
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == SET) {
        *byte = (uint8_t)(huart2.Instance->DR & 0xFF);
        return 1;
    }
    return 0;
}

// Якщо хочеш тримати окрему функцію для рядків
void UART2_SendString(char *str) {
    HAL_UART_Transmit(&huart2, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}