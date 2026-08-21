#include "stm32f1xx_hal.h"
#include "headers/main_decl.h"
#include "headers/weather_station.h"
#include "headers/inside_server.h"
#include <string.h>

extern bool UART1_ReadByte(uint8_t *out);                          /* uart.c */
extern uint16_t CalculateCRC16(uint8_t *buffer, uint16_t length);  /* func.c */
extern UART_HandleTypeDef huart1;                                  /* uart.c - потрібна для send_response() */

extern void    culler_apply_command(uint8_t cmd);
extern void    culler_set_threshold(uint8_t new_threshold_c);
extern uint8_t culler_get_status(void);
extern void    culler_autonomous_tick(int16_t temperature_c100);

uint16_t restart_timeout_sec = RESTART_TIMEOUT_DEFAULT_SEC;
#define RESTART_TIMEOUT_MIN_SEC       5

typedef enum { RESTART_STATE_NORMAL = 0, RESTART_STATE_HOLDING } restart_state_t;

uint8_t rx_packet[PACKET_SIZE];
uint8_t tx_packet[PACKET_SIZE];
int     byte_count = 0;

static uint32_t        last_rx_sec       = 0;
static restart_state_t restart_state     = RESTART_STATE_NORMAL;
static uint32_t        restart_start_sec = 0;

static void poll_incoming_packets(void);
static void process_packet(const ProtocolPacket_t *req);
static void send_response(const ProtocolPacket_t *req);

void restart_helper_init(void) {
    last_rx_sec   = seconds;
    byte_count    = 0;
    restart_state = RESTART_STATE_NORMAL;
}

void restart_helper(void) {
    if (restart_state == RESTART_STATE_HOLDING) {
        if ((seconds - restart_start_sec) >= (uint32_t)RESTART_HOLD_SEC) {
            HAL_GPIO_WritePin(RESTART_GPIO_Port, RESTART_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

            restart_state = RESTART_STATE_NORMAL;
            byte_count    = 0;
            last_rx_sec   = seconds;
        }

        culler_autonomous_tick(sensor_data.temperature);
        return;
    }

    poll_incoming_packets();

    if ((seconds - last_rx_sec) >= restart_timeout_sec) {
        blink_power_led(50);
        culler_autonomous_tick(sensor_data.temperature);
        trigger_restart();
    }
}

static void poll_incoming_packets(void) {
    uint8_t byte;

    while (UART1_ReadByte(&byte)) {
        if (byte_count < PACKET_SIZE) {
            rx_packet[byte_count++] = byte;
        }

        if (byte_count == PACKET_SIZE) {
            ProtocolPacket_t *req = (ProtocolPacket_t *)rx_packet;
            uint16_t crc_calc = CalculateCRC16(rx_packet, PACKET_SIZE - sizeof(uint16_t));

            if (crc_calc == req->field.crc) {
                if (req->field.id == MY_ID) {
                    last_rx_sec = seconds;
                    process_packet(req);
                    send_response(req);
                }
                /* валідний кадр (свій чи чужий) розібрано - готові до наступного "з нуля" */
                byte_count = 0;
            } else {
                /* CRC не зійшовся: шум або втрачена синхронізація.
                   Зсуваємо вікно на 1 байт замість повного скидання -
                   ресинхронізація відбудеться максимум за PACKET_SIZE байт. */
                memmove(rx_packet, rx_packet + 1, PACKET_SIZE - 1);
                byte_count = PACKET_SIZE - 1;
            }
        }
    }
}

static void process_packet(const ProtocolPacket_t *req) {
    bool restart_triggered = false;
    switch (req->field.restart_command) {
        case 0x02: restart_timeout_sec = RESTART_TIMEOUT_DEFAULT_SEC; break;
        case 0x01: trigger_restart(); restart_triggered = true; break;
        default: break;
    }
    if (req->field.rest_time != 0xFFFF) {
        uint16_t new_timeout = req->field.rest_time;
        if (new_timeout < RESTART_TIMEOUT_MIN_SEC) new_timeout = RESTART_TIMEOUT_MIN_SEC;
        restart_timeout_sec = new_timeout;
    }

    /* Кулер: поки Raspberry жива й шле валідні пакети (а ми тут саме
       в такому пакеті), просто застосовуємо її волю напряму - "тримати
       стан, поки Raspberry не пропаде" виходить само собою, бо між
       пакетами process_packet() взагалі не викликається. */
    culler_set_threshold(req->field.culler_temp);
    culler_apply_command(req->field.culler_status);

    if (!restart_triggered) {
        HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED, GPIO_PIN_RESET);
    }
}

static void send_response(const ProtocolPacket_t *req) {
    ProtocolPacket_t res;
    memset(&res, 0, sizeof(res));

    res.field.id              = MY_ID;
    res.field.rest_time = restart_timeout_sec;
    res.field.restart_command = req->field.restart_command; /* ack: яку команду виконали */
    res.field.status          = 0x00;

    /* Кожне поле звітується лише за флагом із запиту; 0 у відповіді означає
       "не запитувалось" - RPi сам знає, що він запитав, і не має
       перевіряти це значення, якщо не виставляв відповідний флаг. */
    if (req->field.temperature != 0) {
        res.field.temperature = sensor_data.temperature;
    }
    if (req->field.street_temp != 0) {
        res.field.street_temp = weather_station.temp;
    }
    if (req->field.street_humidity != 0) {
        res.field.street_humidity = (uint8_t)weather_station.humidity; /* % 0..100, без масштабування */
    }

    /* Завжди реальний поточний стан кулера, а не відлуння команди з
       запиту - Raspberry має бачити правду, навіть коли зараз керує
       автономна (не її) логіка. */
    res.field.culler_status = culler_get_status();

    res.field.crc = CalculateCRC16(res.bytes, PACKET_SIZE - sizeof(uint16_t));

    memcpy(tx_packet, res.bytes, PACKET_SIZE);
    HAL_UART_Transmit(&huart1, tx_packet, PACKET_SIZE, 100);
}

void trigger_restart(void) {
    HAL_GPIO_WritePin(RESTART_GPIO_Port, RESTART_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED, GPIO_PIN_SET);
    restart_start_sec = seconds;
    restart_state      = RESTART_STATE_HOLDING;
    blink_power_led(100);
}