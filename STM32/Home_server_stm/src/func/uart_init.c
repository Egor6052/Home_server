#include "headers/main_decl.h"
#include <stdio.h>
#include <string.h>

#if defined(STM32F0)
#include "stm32f0xx_hal.h"

#elif defined(STM32F1)
#include "stm32f1xx_hal.h"

// // UART1 (PA9 = TX, PA10 = RX)
// void UART1_Init(void) {
//     GPIO_InitTypeDef GPIO_InitStruct = {0};
    
//     // Увімкнення тактування GPIOA та UART1
//     __HAL_RCC_GPIOA_CLK_ENABLE();
//     __HAL_RCC_USART1_CLK_ENABLE();
    
//     // PA9 - TX
//     GPIO_InitStruct.Pin = GPIO_PIN_9;
//     GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//     HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
//     // PA10 - RX
//     GPIO_InitStruct.Pin = GPIO_PIN_10;
//     GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//     GPIO_InitStruct.Pull = GPIO_NOPULL;
//     HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
//     huart1.Instance = USART1;
//     huart1.Init.BaudRate = 9600;
//     huart1.Init.WordLength = UART_WORDLENGTH_8B;
//     huart1.Init.StopBits = UART_STOPBITS_1;
//     huart1.Init.Parity = UART_PARITY_NONE;
//     huart1.Init.Mode = UART_MODE_TX_RX;
//     huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//     huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    
//     if (HAL_UART_Init(&huart1) != HAL_OK)
//     {
//         Error_Handler();
//     }
// }

UART_HandleTypeDef huart2;
// UART2 (PA2 = TX, PA3 = RX)
void UART2_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Увімкнення тактування GPIOA та UART2
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    
    // PA2 - TX
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // PA3 - RX
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

// // UART3 (PB11 = RX, PB10 = TX)
// void UART3_Init(void) {
//     GPIO_InitTypeDef GPIO_InitStruct = {0};

//     // Увімкнення тактування GPIOB та UART3
//     __HAL_RCC_GPIOB_CLK_ENABLE();
//     __HAL_RCC_USART3_CLK_ENABLE();

//     // PB10 - TX
//     GPIO_InitStruct.Pin = GPIO_PIN_10;
//     GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//     // PB11 - RX
//     GPIO_InitStruct.Pin = GPIO_PIN_11;
//     GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//     GPIO_InitStruct.Pull = GPIO_NOPULL;
//     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

//     huart3.Instance = USART3;
//     huart3.Init.BaudRate = 57600;
//     huart3.Init.WordLength = UART_WORDLENGTH_8B;
//     huart3.Init.StopBits = UART_STOPBITS_1;
//     huart3.Init.Parity = UART_PARITY_NONE;
//     huart3.Init.Mode = UART_MODE_TX_RX;
//     huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//     huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    
//     if (HAL_UART_Init(&huart3) != HAL_OK)
//     {
//         Error_Handler();
//     }
// }

#elif defined(STM32F3)
#include "stm32f3xx_hal.h"
#include "../headers/main_decl.h"

// UART1 (PA9 = TX, PA10 = RX)
void UART1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Увімкнення тактування GPIOA та USART1
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    // PA9 - TX
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF_7;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA10 - RX
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF_7;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600; // SR04M
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

// UART2 (PA2 = TX, PA3 = RX)
void UART2_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Увімкнення тактування GPIOA та USART2
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    // PA2 - TX
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF_7;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA3 - RX
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF_7;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 4800; // NMEA 0183
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

// UART3 (PB10 = TX, PB11 = RX)
void UART3_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Увімкнення тактування GPIOB та USART3
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();

    // PB10 - TX
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF_7;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // PB11 - RX
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF_7;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    huart3.Instance = USART3;
    huart3.Init.BaudRate = 57600; // MAVLINK
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart3) != HAL_OK) {
        Error_Handler();
    }
}

#elif defined(STM32F4)
#include "stm32f4xx_hal.h"

// UART1 (PA9 = TX, PA10 = RX)
void UART1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Увімкнення тактування
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    
    // Налаштування PA9 (TX) та PA10 (RX)
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; // <--- Обидва піни в режимі AF
    GPIO_InitStruct.Pull = GPIO_PULLUP;     // <--- Використовуємо підтяжку
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; // <--- Висока швидкість
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1; // <--- ГОЛОВНА ЗМІНА!
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Налаштування UART
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

// UART2 (PA2 = TX, PA3 = RX)
void UART2_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Увімкнення тактування
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    
    // Налаштування PA2 (TX) та PA3 (RX)
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2; // <--- ГОЛОВНА ЗМІНА!
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Налаштування UART
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}


// UART3 (PB11 = RX, PB10 = TX)
// void UART3_Init(void) {
//     GPIO_InitTypeDef GPIO_InitStruct = {0};

//     // Увімкнення тактування
//     __HAL_RCC_GPIOB_CLK_ENABLE();
//     __HAL_RCC_USART3_CLK_ENABLE();

//     // Налаштування PB10 (TX) та PB11 (RX)
//     GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
//     GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//     GPIO_InitStruct.Pull = GPIO_PULLUP;
//     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//     GPIO_InitStruct.Alternate = GPIO_AF7_USART3; // <--- ГОЛОВНА ЗМІНА!
//     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

//     // Налаштування UART
//     huart3.Instance = USART3;
//     huart3.Init.BaudRate = 57600; // Ваша швидкість
//     huart3.Init.WordLength = UART_WORDLENGTH_8B;
//     huart3.Init.StopBits = UART_STOPBITS_1;
//     huart3.Init.Parity = UART_PARITY_NONE;
//     huart3.Init.Mode = UART_MODE_TX_RX;
//     huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//     huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    
//     if (HAL_UART_Init(&huart3) != HAL_OK)
//     {
//         Error_Handler();
//     }
// }

#elif defined(STM32L0)
#include "stm32l0xx_hal.h"
#elif defined(STM32L1)
#include "stm32l1xx_hal.h"
#elif defined(STM32L4)
#include "stm32l4xx_hal.h"
#elif defined(STM32L5)
#include "stm32l5xx_hal.h"
#elif defined(STM32H7)
#include "stm32h7xx_hal.h"
#elif defined(STM32F7)
#include "stm32f7xx_hal.h"
#elif defined(STM32G0)
#include "stm32g0xx_hal.h"
#elif defined(STM32G4)
#include "stm32g4xx_hal.h"
#elif defined(STM32C0)
#include "stm32c0xx_hal.h"
#elif defined(STM32U5)
#include "stm32u5xx_hal.h"
#endif
