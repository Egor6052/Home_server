#include "stm32f4xx_hal.h"
#include "headers/main_decl.h"
#include "headers/weather_station.h"
#include "headers/inside_server.h"
#include <string.h>

uint32_t restart_timeout_sec = RESTART_TIMEOUT_DEFAULT_SEC;

#define RESTART_TIMEOUT_MIN_SEC       5

typedef enum {
    RESTART_STATE_NORMAL = 0,
    RESTART_STATE_HOLDING      /* пін RESTART піднятий, чекаємо RESTART_HOLD_SEC */
} restart_state_t;

uint8_t rx_packet[PACKET_SIZE];
uint8_t tx_packet[PACKET_SIZE];
int     byte_count = 0;

/* Внутрішній стан модуля */
static uint32_t        last_rx_sec       = 0;  /* seconds останнього ВАЛІДНОГО (свого) пакета */
static restart_state_t restart_state     = RESTART_STATE_NORMAL;
static uint32_t        restart_start_sec = 0;  /* seconds початку HOLDING                      */

static volatile uint8_t  u2_rx_ring[UART_RX_RING_SIZE];
static volatile uint16_t u2_rx_head = 0;
static volatile uint16_t u2_rx_tail = 0;
static uint8_t            rx_it_byte;

static bool uart_ring_read(uint8_t *out);
static void poll_incoming_packets(void);
static void process_packet(const ProtocolPacket_t *req);
static void send_response(const ProtocolPacket_t *req);

void restart_helper_init(void) {
    last_rx_sec   = seconds;
    byte_count    = 0;
    restart_state = RESTART_STATE_NORMAL;
    HAL_UART_Receive_IT(&huart2, &rx_it_byte, 1);
}

void restart_helper(void) {

    if (restart_state == RESTART_STATE_HOLDING) {
        if ((seconds - restart_start_sec) >= (uint32_t)RESTART_HOLD_SEC) {
            HAL_GPIO_WritePin(RESTART_GPIO_Port, RESTART_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

            restart_state = RESTART_STATE_NORMAL;

            byte_count  = 0;
            u2_rx_tail  = u2_rx_head;
            last_rx_sec = seconds;
        }
        return;
    }

    poll_incoming_packets();

    if ((seconds - last_rx_sec) >= restart_timeout_sec) {
        trigger_restart();
        blink_power_led(50);
    }
}

static void poll_incoming_packets(void) {
    uint8_t byte;

    while (restart_state == RESTART_STATE_NORMAL && uart_ring_read(&byte)) {
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
        case 0x02: /* скинути таймаут до дефолтного */
            restart_timeout_sec = RESTART_TIMEOUT_DEFAULT_SEC;
            break;
        case 0x01: /* негайний рестарт - не чекаємо на watchdog */
            trigger_restart();
            restart_triggered = true;
            break;
        default:
            break; /* 0x00 або невідомий код - нічого не робимо */
    }

    if (req->field.rest_time != 0xFF) {
        uint8_t new_timeout = req->field.rest_time;
        if (new_timeout < RESTART_TIMEOUT_MIN_SEC) {
            new_timeout = RESTART_TIMEOUT_MIN_SEC;
        }
        restart_timeout_sec = new_timeout;
    }

    if (!restart_triggered) {
        HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED, GPIO_PIN_RESET);
    }
}

static void send_response(const ProtocolPacket_t *req) {
    ProtocolPacket_t res;
    memset(&res, 0, sizeof(res));

    res.field.id              = MY_ID;
    res.field.rest_time       = (restart_timeout_sec > 254) ? 254 : (uint8_t)restart_timeout_sec;
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

    res.field.crc = CalculateCRC16(res.bytes, PACKET_SIZE - sizeof(uint16_t));

    memcpy(tx_packet, res.bytes, PACKET_SIZE);
    HAL_UART_Transmit(&huart2, tx_packet, PACKET_SIZE, 100);
}

void trigger_restart(void) {
    HAL_GPIO_WritePin(RESTART_GPIO_Port, RESTART_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED, GPIO_PIN_SET);
    restart_start_sec = seconds;
    restart_state      = RESTART_STATE_HOLDING;
    blink_power_led(100);
}

//    void USART2_IRQHandler(void) { HAL_UART_IRQHandler(&huart2); }
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        // ДІАГНОСТИКА: блимає на кожен прийнятий байт UART2
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

        uint16_t next_head = (u2_rx_head + 1) % UART_RX_RING_SIZE;
        if (next_head != u2_rx_tail) {
            u2_rx_ring[u2_rx_head] = rx_it_byte;
            u2_rx_head = next_head;
        }
        HAL_UART_Receive_IT(&huart2, &rx_it_byte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        HAL_UART_Receive_IT(&huart2, &rx_it_byte, 1);
    }
}

void USART2_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart2);
}

static bool uart_ring_read(uint8_t *out) {
    if (u2_rx_tail == u2_rx_head) return false;
    *out = u2_rx_ring[u2_rx_tail];
    u2_rx_tail = (u2_rx_tail + 1) % UART_RX_RING_SIZE;
    return true;
}