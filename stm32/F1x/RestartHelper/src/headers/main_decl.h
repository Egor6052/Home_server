#ifndef MAIN_DECL_H
#define MAIN_DECL_H

#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdbool.h>

#define LED_Pin             GPIO_PIN_13
#define LED_GPIO_Port       GPIOC
#define RESTART_PIN         GPIO_PIN_10
#define RESTART_GPIO_Port   GPIOB

#define POWER_ON_BTN          GPIO_PIN_8
#define POWER_ON_GPIO_Port    GPIOA

#define RESET_BTN_1          GPIO_PIN_15
#define RESET_BTN_1_GPIO_Port    GPIOB

#define RESET_BTN_2          GPIO_PIN_14
#define RESET_BTN_2_GPIO_Port    GPIOB

#define POWER_LED          GPIO_PIN_13
#define POWER_LED_GPIO_Port    GPIOB

#define HDD_1_LED         GPIO_PIN_12
#define HDD_2_LED         GPIO_PIN_9
#define HDD_LED_GPIO_Port GPIOB

#define RX_BUFF_SIZE        256
extern char rx_buffer[RX_BUFF_SIZE];

#define UART_RX_RING_SIZE           256  /* байтовий кільцевий буфер прийому UART2 (ISR)        */
#define RESTART_HOLD_SEC             10  /* тримати пін RESTART активним стільки секунд         */
#define RESTART_TIMEOUT_DEFAULT_SEC  1500  /* дефолтний watchdog-таймаут, поки RPi не пришле свій */

#define CULLER_PIN GPIO_PIN_0
#define CULLER_GPIO_Port GPIOB

#define BUUZER_PIN GPIO_PIN_15
#define BUZZER_PORT GPIOA

#define CULLER_TEMP_MIN_C        20
#define CULLER_TEMP_MAX_C        90
#define CULLER_HYSTERESIS_C       5

extern uint16_t restart_timeout_sec;
extern uint8_t MY_ID;

#define PACKET_SIZE 16
extern uint8_t rx_packet[PACKET_SIZE];
extern uint8_t tx_packet[PACKET_SIZE];
extern int byte_count;

typedef union {
    struct __attribute__((packed)) {
        uint8_t  id;
        uint16_t rest_time;        // число = змінити watchdog-таймаут (сек), 0xFF = не змінювати
        int16_t  temperature;      // ЗАПИТ: 0=не звітувати/1=звітувати. ВІДПОВІДЬ: sensor_data.temperature, *100
        int16_t  street_temp;      // ЗАПИТ: 0=не звітувати/1=звітувати. ВІДПОВІДЬ: weather_station.temp, *100
        uint8_t  street_humidity;  // ЗАПИТ: 0=не звітувати/1=звітувати. ВІДПОВІДЬ: weather_station.humidity, % (0..100, БЕЗ *100)
        uint8_t  restart_command;  // 0x00 = нічого, 0x01 = негайний рестарт, 0x02 = скинути таймаут до дефолтного
        uint8_t  status;           // ВІДПОВІДЬ: 0x00 = OK, 0x01 = Error V, 0x02 = Error T (поки завжди 0x00 - TODO)
        uint8_t  culler_temp;      // значение целочисленное, на пример 60, пороговое для включение куллера.
        uint8_t  culler_status;    // от Raspberry: 0xFF = ничего не делать, 0x01 - включить насильно, 0x00 - выключить насильно. от stm32 0x00 - не работает, 0x01 - работает.
        uint8_t  reserved[2];      // запас: майбутні команди керування тощо
        uint16_t crc;
    } field;
    uint8_t bytes[PACKET_SIZE];
} ProtocolPacket_t;

extern volatile uint32_t systick_10us_ticks;
extern volatile uint32_t milliseconds;
extern volatile uint32_t seconds;

extern UART_HandleTypeDef huart2; // connections to RPi
extern UART_HandleTypeDef huart1; // extern weather station

// extern I2C_HandleTypeDef  hi2c1;
extern TIM_HandleTypeDef htim1;

extern volatile uint8_t  rx_buff[RX_BUFF_SIZE];
extern volatile uint16_t rx_head;
extern volatile uint16_t rx_tail;

void SystemClock_Config(void);
void SysTick_Init_100kHz(void);
void GPIO_Init(void);
void MX_TIM1_Init(void);

void beep(int frequency, int duration_ms);
// void PlayStalkerClick(void);
void PlayStartupSong(void);

uint16_t CalculateCRC16(uint8_t *buffer, uint16_t length);
void UART1_Init(void);
void UART1_SendString(const char *str);
void uart1_flush(void);

void UART2_Init(void);
void UART2_SendString(const char *str);
void uart2_flush(void);
bool UART2_ReadByte(uint8_t *out);

// void i2c1_init(void);

void Error_Handler(void);
void blink_led(int value_time);

void restart_helper_init(void);
void restart_helper(void);
bool PorewButton(void);
bool RestartButton1(void);
bool RestartButton2(void);

void trigger_restart(void);

void    culler_init(void);
void    culler_apply_command(uint8_t cmd);
void    culler_set_threshold(uint8_t new_threshold_c);
uint8_t culler_get_status(void);
void    culler_autonomous_tick(int16_t temperature_c100);

void blink_power_led(int value_time);
void blink_hdd_led(uint8_t hdd_num, int value_time);

#endif /* MAIN_DECL_H */
