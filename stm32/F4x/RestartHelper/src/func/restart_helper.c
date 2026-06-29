#include "stm32f4xx_hal.h"
#include "headers/main_decl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UART_RX_BUF_SIZE       256   /* розмір кільцевого буфера       */
#define JSON_MAX_LEN           200   /* максимальна довжина одного JSON */
#define POLL_INTERVAL_SEC        5   /* інтервал між запитами, секунди */
#define RESTART_TIMEOUT_SEC     60   /* немає відповіді 60 с — перезапуск (тестовий режим) */
#define RESTART_HOLD_SEC        15   /* тримати пін RESTART активним 15 с */

/* Стан модуля перезапуску */
typedef enum {
    RESTART_STATE_NORMAL = 0,  /* штатний опрос heartbeat */
    RESTART_STATE_HOLDING      /* пін RESTART піднятий, чекаємо RESTART_HOLD_SEC */
} restart_state_t;

/* --- Внутрішні (static) прототипи модуля --- */
static void    uart_rx_start(void);
static bool    uart_ring_read(uint8_t *out);
static bool    parse_response(const char *json, uint32_t *hb);
static int32_t parse_int_field(const char *json, const char *key);
static void    send_request(void);
static void    trigger_restart(void);

/* Внутрішній стан модуля */
static uint32_t        heartbeat_tx   = 0;   /* наш лічильник запитів          */
static uint32_t        last_rx_sec    = 0;   /* seconds у момент останньої відповіді */
static uint32_t        last_poll_sec  = 0;   /* seconds у момент останнього запиту   */
static bool            first_run      = true;
static restart_state_t restart_state  = RESTART_STATE_NORMAL;
static uint32_t        restart_start_sec = 0;  /* seconds у момент початку HOLDING */

/* кільцевий буфер прийому UART1, заповнюється з ISR USART1_IRQHandler.
   Імена з префіксом u1_, щоб не конфліктувати з rx_head/rx_tail
   буфера UART2, які оголошені як extern volatile у main_decl.h */
static uint8_t  rx_dma_buf[UART_RX_BUF_SIZE];  /* НЕ використовується для DMA, лишив назву для сумісності */
static uint8_t  u1_rx_ring[UART_RX_BUF_SIZE];
static uint16_t u1_rx_head = 0;   /* індекс запису  */
static uint16_t u1_rx_tail = 0;   /* індекс читання */

static char     json_line[JSON_MAX_LEN];
static uint16_t json_len = 0;


void restart_helper_init(void) {
    last_rx_sec   = seconds;
    last_poll_sec = seconds;
    first_run     = true;
    uart_rx_start();
}

void restart_helper(void) {

    /* У стані HOLDING пін RESTART і LED вже піднятi — просто чекаємо
       RESTART_HOLD_SEC і не займаємось опитуванням/парсингом */
    if (restart_state == RESTART_STATE_HOLDING) {
        if ((seconds - restart_start_sec) >= (uint32_t)RESTART_HOLD_SEC) {
            // HAL_GPIO_WritePin(RESTART_GPIO_Port, RESTART_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            restart_state = RESTART_STATE_NORMAL;
            /* Скидаємо таймери, щоб не тригернути одразу знову */
            last_rx_sec   = seconds;
            last_poll_sec = seconds;
            first_run     = true;
        }
        return;
    }

    if (first_run || (seconds - last_poll_sec) >= (uint32_t)POLL_INTERVAL_SEC) {
        first_run     = false;
        last_poll_sec = seconds;
        send_request();
    }

    uint8_t byte;

    while (uart_ring_read(&byte)) {
        /* finde last byte of line */
        if (byte == '\n' || byte == '\r') {
            if (json_len > 0) {
                json_line[json_len] = '\0';

                uint32_t hb = 0;

                if (parse_response(json_line, &hb)) {
                    heartbeat_tx = hb;
                    if (heartbeat_tx >= 255) heartbeat_tx = 0;
                    last_rx_sec = seconds;
                }
                json_len = 0;
            }
        } else {
            /* накопичуємо байт */
            if (json_len < JSON_MAX_LEN - 1) {
                json_line[json_len++] = (char)byte;
            } else {
                /* overflow, скидаємо */
                json_len = 0;
            }
        }
    }

    if ((seconds - last_rx_sec) >= (uint32_t)RESTART_TIMEOUT_SEC) {
        trigger_restart();
    }
}

static void send_request(void) {
    heartbeat_tx++;
    if (heartbeat_tx >= 255) heartbeat_tx = 0;
    char buf[48];
    snprintf(buf, sizeof(buf),
             "{\"heartbeat\":%lu}\r\n",
             (unsigned long)heartbeat_tx);
    UART1_SendString(buf);
}

static void trigger_restart(void) {
    /* Піднімаємо пін RESTART і LED, переходимо у стан HOLDING.
       Обидва опускаються неблокуюче у restart_helper(),
       через RESTART_HOLD_SEC секунд — щоб не зупиняти MCU на 15 с. */
    // HAL_GPIO_WritePin(RESTART_GPIO_Port, RESTART_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    restart_start_sec = seconds;
    restart_state     = RESTART_STATE_HOLDING;
}

static int32_t parse_int_field(const char *json, const char *key) {
    /* Шукаємо "key": у рядку */
    char search[32];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char *p = strstr(json, search);
    if (!p) return 0;
    p += strlen(search);
    /* Пропускаємо пробіли */
    while (*p == ' ') p++;
    return (int32_t)strtol(p, NULL, 10);
}

static bool parse_response(const char *json, uint32_t *hb) {
    /* Перевіряємо що рядок схожий на JSON-об'єкт */
    if (json[0] != '{') return false;

    /* Перевіряємо наявність обов'язкового поля heartbeat */
    if (!strstr(json, "\"heartbeat\"")) return false;

    *hb = (uint32_t)parse_int_field(json, "heartbeat");

    return true;
}

/* HAL-callback викликається з HAL_UART_IRQHandler (див. USART1_IRQHandler нижче) */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        /* Кладемо отриманий байт у кільцевий буфер */
        uint8_t byte = rx_dma_buf[0];
        uint16_t next_head = (u1_rx_head + 1) % UART_RX_BUF_SIZE;
        if (next_head != u1_rx_tail) {          /* є місце */
            u1_rx_ring[u1_rx_head] = byte;
            u1_rx_head = next_head;
        }
        /* Перезапускаємо прийом наступного байту */
        HAL_UART_Receive_IT(&huart1, rx_dma_buf, 1);
    }
}

/* ISR USART1: раніше був відсутній, через що HAL_UART_RxCpltCallback
   ніколи не викликався і прийом на UART1 фактично не працював.
   HAL_UART_IRQHandler сам розбирається з прапорцями і кличе callback. */
void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}

static void uart_rx_start(void) {
    HAL_UART_Receive_IT(&huart1, rx_dma_buf, 1);
}

/* Читання одного байту з кільцевого буфера
   Повертає true якщо байт є, false якщо буфер порожній */
static bool uart_ring_read(uint8_t *out) {
    if (u1_rx_tail == u1_rx_head) return false;   /* порожній */
    *out      = u1_rx_ring[u1_rx_tail];
    u1_rx_tail = (u1_rx_tail + 1) % UART_RX_BUF_SIZE;
    return true;
}