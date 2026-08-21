// #include "json.h"
#include <string.h>
#include "../headers/main_decl.h"
#include "../headers/DS18B20.h"
#include "stm32f1xx_hal.h"

#define NOTE_C6  1047
#define NOTE_E6  1319
#define NOTE_G6  1568
#define NOTE_A6  1760
#define NOTE_C7  2093
#define NOTE_G7  3136
#define NOTE_D7  2349


// Проста функція затримки в мікросекундах (приблизна для 72MHz)
// Краще використовувати DWT або таймер, але це працюватиме для простих цілей
void delay_us(uint32_t us) {
    us *= 7; // Множник підбирається експериментально для 72MHz
    while (us--) {
        __NOP();
    }
}

// Функція програвання ноти
// frequency: частота в Гц
// duration_ms: тривалість в мс
// Функція програвання ноти (Active Low версія)
void beep(int frequency, int duration_ms) {
    if (frequency == 0) {
        // Якщо частота 0, просто тримаємо бузер вимкненим (HIGH)
        HAL_GPIO_WritePin(GPIOA, BUUZER_PIN, GPIO_PIN_SET);
        HAL_Delay(duration_ms);
        return;
    }

    int period_us = 1500000 / frequency;
    int half_period = period_us / 2;
    int cycles = (long)frequency * duration_ms / 1000;

    for (int i = 0; i < cycles; i++) {
        // УВІМКНУТИ звук (притягнути до землі/RESET)
        HAL_GPIO_WritePin(GPIOA, BUUZER_PIN, GPIO_PIN_RESET); 
        delay_us(half_period);
        
        // ВИМКНУТИ звук (підняти до VCC/SET)
        HAL_GPIO_WritePin(GPIOA, BUUZER_PIN, GPIO_PIN_SET);
        delay_us(half_period);
    }
    
    // Гарантовано вимикаємо в кінці ноти
    HAL_GPIO_WritePin(GPIOA, BUUZER_PIN, GPIO_PIN_SET);
}

void PlayStartupSong(void) {
    beep(2047, 10);   // C6
    HAL_Delay(10);

    // beep(2093, 220);  // C7
    // HAL_Delay(10);

    beep(3010, 220);
    HAL_Delay(10);


    // beep(1319, 90);   // E6
    // HAL_Delay(10);

    // beep(1568, 90);   // G6
    // HAL_Delay(10);

    // beep(2093, 220);  // C7
}

// void PlayTamagotchiAlert(void) {
//     // Высокая нота A6, сыгранная дважды
//     int duration = 120;
//     int rest_duration = 80;
    
//     beep(NOTE_A6, duration);
//     beep(0, rest_duration);
//     beep(NOTE_A6, duration);
    
//     // Гарантированно глушим линию
//     HAL_GPIO_WritePin(GPIOA, BUUZER_PIN, GPIO_PIN_SET);
// }