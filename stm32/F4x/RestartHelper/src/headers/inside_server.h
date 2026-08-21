#ifndef INSIDE_SERVER_H
#define INSIDE_SERVER_H

#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdbool.h>
#include "DS18B20.h"

// Inside temperature sensor, working with DS18B20
typedef struct {
    int16_t temperature;  // * 100
    uint8_t culler_status; // 0x00 = off, 0x01 = on
} SensorsData;

extern SensorsData sensor_data;

extern uint32_t sensor_timer;
extern uint32_t temp_request_time;
extern bool temp_requested;

void update_inside_server_data(void);

#endif /* INSIDE_SERVER_H */