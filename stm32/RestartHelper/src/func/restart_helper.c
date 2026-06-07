#include "stm32f1xx_hal.h"
#include "headers/main_decl.h"
#include "../lib/ssd1306/ssd1306.h"
#include "../lib/ssd1306/ssd1306_fonts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define UART_RX_BUF_SIZE       256   /* розмір кільцевого буфера       */
#define JSON_MAX_LEN           200   /* максимальна довжина одного JSON */
#define POLL_INTERVAL_SEC        5   /* інтервал між запитами, секунди */
#define RESTART_TIMEOUT_SEC   1200   /* 20 хвилин = 1200 секунд        */
 
// Внутрішній стан модуля
static uint32_t heartbeat_tx   = 0;   /* наш лічильник запитів          */
static uint32_t last_rx_sec    = 0;   /* seconds у момент останньої відповіді */
static uint32_t last_poll_sec  = 0;   /* seconds у момент останнього запиту   */
static bool     first_run      = true;
 
/* кільцевий DMA/IRQ буфер */
static uint8_t  rx_dma_buf[UART_RX_BUF_SIZE];  /* HAL_UART_Receive_IT кладе сюди по 1 байту */
static uint8_t  rx_ring[UART_RX_BUF_SIZE];
static uint16_t rx_head = 0;   /* індекс запису  */
static uint16_t rx_tail = 0;   /* індекс читання */
 
static char     json_line[JSON_MAX_LEN];
static uint16_t json_len = 0;

 
void restart_helper_init(void) {
    last_rx_sec   = seconds;
    last_poll_sec = seconds;
    first_run     = true;
    uart_rx_start();
}

// Відправити запит на UART для отримання даних:
    // Запит що повинен надсилатися: 
    // {
    //     "heartbeat": 0, // heartbeat буде на одиницю прибавлятися пожного разу як отримає запит, та надсилати обратно, щоб клієнт знав що пристрій живий, і на клієнті також буде додавання одиниці, щоб відслідковувати кількість отриманих запитів
    //     "command": "get_data",
    // }


    // Запит що повинен прийти у відповідь:
    // {
    //     "heartbeat": 12, // heartbeat буде на одиницю прибавлятися пожного разу як отримає запит, та надсилати обратно, щоб клієнт знав що пристрій живий, і на клієнті також буде додавання одиниці, щоб відслідковувати кількість отриманих запитів
    //     "timestamp": 1234567890, // отримаємо від клієнта час
    //     "temperature": 25.5,  
    //     "humidity": 60.0
    // }
    
    // неблокуючий UART почтійно читає буфер, приймає дату і скидає таймери
    // якщо не відповідає на протязі 20 хв, тригерити пін для перезавантаження
    // Ось пін для перезавантаження клієнта:
    // #define RESTART_PIN         GPIO_PIN_0

    // Якщо все гаразд, малюємо інформацію на OLED дисплей:

    // Вивести дані тут: 
    // show_display_info(temperature, humidity, timestamp);

void restart_helper(void) {
    
    if (first_run || (seconds - last_poll_sec) >= (uint32_t)POLL_INTERVAL_SEC) {
        first_run     = false;
        last_poll_sec = seconds;
        send_request();
    }

    uint8_t byte;
    uint16_t bytes_received = 0;

    while (uart_ring_read(&byte)) {
        bytes_received++;
        if (byte == '\n' || byte == '\r') {
            /* кінець рядка — пробуємо розпарсити */
            if (json_len > 0) {
                json_line[json_len] = '\0';
 
                uint32_t hb   = 0;
                uint32_t ts   = 0;
                float    temp = 0.0f;
                float    hum  = 0.0f;
 
                if (parse_response(json_line, &hb, &ts, &temp, &hum)) {
                    /* --- успішна відповідь --- */
                    heartbeat_tx = hb;   /* синхронізуємо свій heartbeat з відповіддю */
                    last_rx_sec  = seconds;
 
                    /* малюємо на дисплеї */
                    show_display_info(temp, hum, ts);
                }
                json_len = 0;
            }
        } else {
            /* накопичуємо байт */
            if (json_len < JSON_MAX_LEN - 1) {
                json_line[json_len++] = (char)byte;
            } else {
                /* переповнення — скидаємо буфер */
                json_len = 0;
            }
        }
    }
 
    /* --- 3. Перевірка таймауту --- */
    if ((seconds - last_rx_sec) >= (uint32_t)RESTART_TIMEOUT_SEC) {
        trigger_restart();
    }


}

static void send_request(void) {
    heartbeat_tx++;
    char buf[64];
    snprintf(buf, sizeof(buf),
             "{\"heartbeat\":%lu,\"command\":\"get_data\"}\r\n",
             (unsigned long)heartbeat_tx);
    UART1_SendString(buf);
}

static void trigger_restart(void) {
    /* Показуємо повідомлення на дисплеї */
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 10);
    ssd1306_WriteString("NO RESPONSE!", Font_11x18, White);
    ssd1306_SetCursor(0, 35);
    ssd1306_WriteString("Restarting...", Font_7x10, White);
    ssd1306_UpdateScreen();
    HAL_Delay(2000);
 
    /* Тригеримо пін скидання (активний HIGH — утримуємо 500 мс) */
    HAL_GPIO_WritePin(RESTART_GPIO_Port, RESTART_PIN, GPIO_PIN_SET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(RESTART_GPIO_Port, RESTART_PIN, GPIO_PIN_RESET);
 
    /* Скидаємо таймер щоб не тригерити одразу знову */
    last_rx_sec = seconds;
}


void show_display_info(float temperature, float humidity, uint32_t timestamp) {
    char line[24];
 
    ssd1306_Fill(Black);
 
    /* Рядок 1: температура */
    snprintf(line, sizeof(line), "Temp: %.1f C", (double)temperature);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(line, Font_7x10, White);
 
    /* Рядок 2: вологість */
    snprintf(line, sizeof(line), "Hum:  %.1f %%", (double)humidity);
    ssd1306_SetCursor(0, 14);
    ssd1306_WriteString(line, Font_7x10, White);
 
    /* Рядок 3: Unix-timestamp (останні 6 цифр для економії місця) */
    snprintf(line, sizeof(line), "TS: %06lu", (unsigned long)(timestamp % 1000000UL));
    ssd1306_SetCursor(0, 28);
    ssd1306_WriteString(line, Font_7x10, White);
 
    /* Рядок 4: heartbeat — лічильник живих відповідей */
    snprintf(line, sizeof(line), "HB: %lu", (unsigned long)heartbeat_tx);
    ssd1306_SetCursor(0, 42);
    ssd1306_WriteString(line, Font_7x10, White);
 
    ssd1306_UpdateScreen();
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
 
static float parse_float_field(const char *json, const char *key) {
    char search[32];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char *p = strstr(json, search);
    if (!p) return 0.0f;
    p += strlen(search);
    while (*p == ' ') p++;
    return (float)strtod(p, NULL);
}
 
static bool parse_response(const char *json,
                            uint32_t *hb,
                            uint32_t *ts,
                            float    *temp,
                            float    *hum) {
    /* Перевіряємо що рядок схожий на JSON-об'єкт */
    if (json[0] != '{') return false;
 
    /* Перевіряємо наявність обов'язкових полів */
    if (!strstr(json, "\"heartbeat\""))   return false;
    if (!strstr(json, "\"temperature\"")) return false;
    if (!strstr(json, "\"humidity\""))    return false;
 
    *hb   = (uint32_t)parse_int_field  (json, "heartbeat");
    *ts   = (uint32_t)parse_int_field  (json, "timestamp");
    *temp = parse_float_field(json, "temperature");
    *hum  = parse_float_field(json, "humidity");
 
    return true;
}



void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        /* Кладемо отриманий байт у кільцевий буфер */
        uint8_t byte = rx_dma_buf[0];
        uint16_t next_head = (rx_head + 1) % UART_RX_BUF_SIZE;
        if (next_head != rx_tail) {          /* є місце */
            rx_ring[rx_head] = byte;
            rx_head = next_head;
        }
        /* Перезапускаємо прийом наступного байту */
        HAL_UART_Receive_IT(&huart1, rx_dma_buf, 1);
    }
}

static void uart_rx_start(void) {
    HAL_UART_Receive_IT(&huart1, rx_dma_buf, 1);
}

// Читання одного байту з кільцевого буфера
// Повертає true якщо байт є, false якщо буфер порожній
static bool uart_ring_read(uint8_t *out) {
    if (rx_tail == rx_head) return false;   /* порожній */
    *out    = rx_ring[rx_tail];
    rx_tail = (rx_tail + 1) % UART_RX_BUF_SIZE;
    return true;
}
