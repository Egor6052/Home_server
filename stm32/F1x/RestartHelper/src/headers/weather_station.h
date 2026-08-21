#ifndef WEATHER_STATION_H
#define WEATHER_STATION_H

#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdbool.h>

// Структура для хранения данных с датчиков
typedef struct {
    uint16_t station_id;
    int16_t temp;  // * 100
    uint16_t humidity;
    uint16_t lat;
    uint16_t lon;
    uint32_t timestamp; // unix timestamp
} WeatherStation;

extern WeatherStation weather_station;

// Для оновленя даними структури weather_station
void update_weather_station_data(void);

#endif /* WEATHER_STATION_H */