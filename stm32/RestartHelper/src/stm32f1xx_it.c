#include "stm32f1xx_hal.h"
#include "headers/main_decl.h"

void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}