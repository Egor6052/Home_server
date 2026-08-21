#include "stm32f1xx_hal.h"
#include "headers/main_decl.h"
#include "headers/weather_station.h"
#include "headers/inside_server.h"
#include "headers/DS18B20.h"
#include <string.h>
#include <stdio.h>

DS18B20_t temp_sensor;

uint8_t MY_ID = 100;

int main(void) {
    HAL_Init();
    SystemClock_Config();
    SysTick_Init_100kHz();
    GPIO_Init();
    UART1_Init();
    UART2_Init();
    MX_TIM1_Init();

    // Init temperature sensor
    DS18B20_Init(&temp_sensor, &htim1, DS18B20_PORT, DS18B20_PIN);

    for (int i = 0; i < 3; i++) {
        blink_led(100);
    }

    blink_hdd_led(1, 100);
    blink_hdd_led(2, 100);
    PlayStartupSong();
    
    restart_helper_init();
    culler_init();

    culler_apply_command(0x01); // насильно включить
    HAL_Delay(2000);
    culler_apply_command(0x00); // насильно выключить


    while (1) {
        restart_helper();
        update_weather_station_data();
        update_inside_server_data();

        if (PorewButton()) {
            trigger_restart();
        }

        if (RestartButton1()) {
            blink_hdd_led(1, 100);
        }

        if (RestartButton2()) {
            blink_hdd_led(2, 100);
        }
    }
}