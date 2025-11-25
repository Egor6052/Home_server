#ifndef MAIN_DECL_H
#define MAIN_DECL_H

#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define LED_Pin        GPIO_PIN_13
#define LED_GPIO_Port  GPIOC

// 7-бітна адреса датчика AHT10 (0x38). Зсунута на 1 для HAL I2C.
#define AHT10_ADDR (0x38 << 1)
// Команда ініціалізації
#define AHT10_CMD_INIT 0xE1
// Команда вимірювання
#define AHT10_CMD_MEASURE 0xAC

extern float last_temp;
extern float last_hum;

extern I2C_HandleTypeDef hi2c1;
// Буфер для прийому
char rx_buffer[128];
extern uint8_t rx_index;
extern uint8_t rx_byte;

extern UART_HandleTypeDef huart2;
extern uint8_t uart2_rx_buffer[128];
extern uint8_t uart2_tx_buffer[256];

void SystemClock_Config(void);
void GPIO_Init(void);
void Blink_LED(void);
void Error_Handler(void);
void SysTick_Handler(void);
void I2C_Init(void);
uint8_t AHT10_Init(void);
uint8_t AHT10_Read_Data(void);

void UART2_Init(void);
uint16_t get_UART2_data(void);
void send_to_UART2(char *msg);
uint8_t UART2_ReceiveByte_NonBlocking(uint8_t *byte);
void UART2_SendString(char *str);
void float_to_str(char* buffer, float f, int precision);

const char* getID_stm(void);
char* getTemperature(void);
char* getHumidity(void);
char* getLatitude(void);
char* getLongitude(void);

// void Generate_UUID_from_UID(uuid_t* uuid);
// void Format_UUID_String(char* buffer, const uuid_t* uuid);
// void Get_Unique_ID_String(char* buffer);

#endif // MAIN_DECL_H