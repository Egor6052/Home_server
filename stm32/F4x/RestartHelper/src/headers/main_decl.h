#ifndef MAIN_DECL_H
#define MAIN_DECL_H

#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdbool.h>

#define LED_Pin             GPIO_PIN_13
#define LED_GPIO_Port       GPIOC
#define RESTART_PIN         GPIO_PIN_10
#define RESTART_GPIO_Port   GPIOB

/* Размер кольцевого буфера приёма UART2 (вынесен сюда, т.к. используется
   и в main_gpio.c для заполнения буфера, и потенциально снаружи) */
#define RX_BUFF_SIZE        256

extern volatile uint32_t systick_10us_ticks;
extern volatile uint32_t milliseconds;
extern volatile uint32_t seconds;

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef  hi2c1;

/* Кольцевой буфер приёма UART2 (определён в main_gpio.c) */
extern volatile uint8_t  rx_buff[RX_BUFF_SIZE];
extern volatile uint16_t rx_head;
extern volatile uint16_t rx_tail;

/* --- Системные/периферийные функции (main_gpio.c) --- */
void SystemClock_Config(void);
void SysTick_Init_100kHz(void);
void GPIO_Init(void);

void UART1_Init(void);
void UART1_SendString(const char *str);
void uart1_flush(void);

void UART2_Init(void);
void UART2_SendString(const char *str);
void uart2_flush(void);

void i2c1_init(void);

void Error_Handler(void);
void blink_led(int value_time);

/* --- Модуль опроса/перезапуска (restart_helper.c) --- */
void restart_helper_init(void);
void restart_helper(void);

#endif /* MAIN_DECL_H */