#include "../headers/weather_station.h"
#include "../headers/main_decl.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

WeatherStation weather_station = {0};

#define WS_REQUEST_STR          "{\"mk\":\"1\",\"data\":\"true\"}"
#define WS_RX_BUF_SIZE          128     /* достатньо для {"id":"1","lat":50.45,...}         */
#define WS_BYTE_TIMEOUT_MS      20      /* тайм-аут очікування одного байта                  */
#define WS_MAX_TIMEOUTS         100     /* макс. к-сть послідовних тайм-аутів (~2с загалом)   */

#define WS_LATLON_SCALE         100.0
#define WS_TEMP_SCALE           100.0


/**
 * @brief Приймає з UART1 відповідь-JSON байт за байтом, доки не зустріне '}'
 *        або не вичерпає лічильник тайм-аутів.
 * @return true, якщо в буфері є хоча б один символ '}' (ймовірний кінець об'єкта)
 */

static bool uart1_read_json_response(char *out_buf, size_t out_size) {
    size_t len = 0;
    uint16_t timeouts = 0;

    out_buf[0] = '\0';

    while (len < out_size - 1 && timeouts < WS_MAX_TIMEOUTS) {
        uint8_t byte;
        if (HAL_UART_Receive(&huart1, &byte, 1, WS_BYTE_TIMEOUT_MS) == HAL_OK) {
            out_buf[len++] = (char)byte;
            out_buf[len] = '\0';
            if (byte == '}') {
                break;
            }
        } else {
            timeouts++;
        }
    }

    return (len > 0) && (memchr(out_buf, '}', len) != NULL);
}

/**
 * @brief Виймає з "плоского" (без вкладеності) JSON-рядка значення поля "key".
 *        Підтримує як рядкові ("key":"value"), так і числові ("key":123.45) значення.
 * @return true, якщо ключ знайдено і значення скопійовано в out
 */

static bool json_extract_value(const char *json, const char *key, char *out, size_t out_size) {
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char *p = strstr(json, pattern);
    if (p == NULL) {
        return false;
    }
    p += strlen(pattern);

    while (*p == ' ' || *p == '\t') p++;
    if (*p != ':') {
        return false;
    }
    p++;
    while (*p == ' ' || *p == '\t') p++;

    size_t i = 0;
    if (*p == '"') {
        p++;
        while (*p != '\0' && *p != '"' && i < out_size - 1) {
            out[i++] = *p++;
        }
    } else {
        while (*p != '\0' && *p != ',' && *p != '}' && *p != ' ' && i < out_size - 1) {
            out[i++] = *p++;
        }
    }
    out[i] = '\0';

    return (i > 0);
}

static bool json_get_double(const char *json, const char *key, double *out) {
    char buf[32];
    if (!json_extract_value(json, key, buf, sizeof(buf))) {
        return false;
    }
    *out = strtod(buf, NULL);
    return true;
}

static uint16_t clamp_to_u16(double value) {
    if (value < 0.0) value = 0.0;
    if (value > 65535.0) value = 65535.0;
    return (uint16_t)(value + 0.5);
}

static int16_t clamp_to_i16(double value) {
    if (value < -32768.0) value = -32768.0;
    if (value > 32767.0) value = 32767.0;
    return (int16_t)(value >= 0.0 ? value + 0.5 : value - 0.5);
}

void update_weather_station_data(void) {
    char rx_json[WS_RX_BUF_SIZE];

    uart1_flush();
    UART1_SendString(WS_REQUEST_STR);

    if (!uart1_read_json_response(rx_json, sizeof(rx_json))) {
        return;
    }

    char *start = strchr(rx_json, '{');
    char *end   = strrchr(rx_json, '}');
    if (start == NULL || end == NULL || end <= start) {
        return;
    }
    end[1] = '\0';

    char id_buf[16];
    double lat = 0.0, lon = 0.0, temp = 0.0, hum = 0.0;

    bool has_id   = json_extract_value(start, "id",   id_buf, sizeof(id_buf));
    bool has_lat  = json_get_double(start, "lat",  &lat);
    bool has_lng  = json_get_double(start, "lng",  &lon);
    bool has_temp = json_get_double(start, "temp", &temp);
    bool has_hum  = json_get_double(start, "hum",  &hum);

    if (!has_id && !has_lat && !has_lng && !has_temp && !has_hum) {
        return;
    }

    if (has_id) {
        weather_station.station_id = (uint16_t)strtoul(id_buf, NULL, 10);
    }
    if (has_lat) {
        weather_station.lat = clamp_to_u16(lat * WS_LATLON_SCALE);
    }
    if (has_lng) {
        weather_station.lon = clamp_to_u16(lon * WS_LATLON_SCALE);
    }
    if (has_temp) {
        weather_station.temp = clamp_to_i16(temp * WS_TEMP_SCALE);
    }
    if (has_hum) {
        weather_station.humidity = clamp_to_u16(hum);
    }

    weather_station.timestamp = seconds;
}