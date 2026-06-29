#ifndef MAIN_DECL_H
#define MAIN_DECL_H

#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdbool.h>

#define LED_Pin             GPIO_PIN_13
#define LED_GPIO_Port       GPIOC

#define RESTART_PIN         GPIO_PIN_0
#define RESTART_GPIO_Port   GPIOA

extern volatile uint32_t systick_10us_ticks;
extern volatile uint32_t milliseconds;
extern volatile uint32_t seconds;

extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef  hi2c1;

void SystemClock_Config(void);
void SysTick_Init_100kHz(void);
void GPIO_Init(void);
void UART1_Init(void);
void i2c1_init(void);

void UART1_SendString(const char *str);
void uart_flush(void);
void Error_Handler(void);
void blink_led(int value_time);


static void uart_rx_start(void);
static bool uart_ring_read(uint8_t *out);
static bool parse_response(const char *json, uint32_t *hb, uint32_t *ts, float *temp, float *hum);
static float  parse_float_field (const char *json, const char *key);
static int32_t parse_int_field  (const char *json, const char *key);
static void   send_request(void);
static void   trigger_restart(void);


void restart_helper_init(void);
void restart_helper(void);
void show_display_info(float temperature, float humidity, uint32_t timestamp);
void screen_logo(void);

#endif /* MAIN_DECL_H */