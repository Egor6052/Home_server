#include "stm32f1xx_hal.h"
#include <stdlib.h>

#include "headers/main_decl.h"
#include "../lib/ssd1306/ssd1306.h"
#include "../lib/ssd1306/ssd1306_conf.h"
#include "../lib/ssd1306/ssd1306_fonts.h"


// --- НАЛАШТУВАННЯ ДЕМО-СЦЕНИ ---
#define NUM_STARS 100 // Кількість зірок
#define MAX_SPEED 3   // Максимальна швидкість (1, 2 або 3)
#define LOGO_DURATION_MS 3500 // Тривалість заставки в мілісекундах (3.5 сек)

// Створюємо структуру для опису кожної зірки
typedef struct {
    int16_t x;
    int16_t y;
    uint8_t speed;
} Star;

// Створюємо масив для зберігання всіх зірок
static Star stars[NUM_STARS];

// Функція для ініціалізації/перезапуску зірки
static void reset_star(int i) {
    stars[i].x = 0; // З'являється зліва
    stars[i].y = rand() % SSD1306_HEIGHT; // Випадкова висота
    stars[i].speed = (rand() % MAX_SPEED) + 1; // Випадкова швидкість від 1 до MAX_SPEED
}

void screen_logo(void) {
    
    // --- ЕТАП 1: ІНІЦІАЛІЗАЦІЯ ЗІРОК ---
    // Ініціалізуємо генератор випадкових чисел
    srand(HAL_GetTick()); 

    // Розміщуємо кожну зірку у випадковій позиції на екрані
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = rand() % SSD1306_WIDTH; // Випадкова позиція по X
        stars[i].y = rand() % SSD1306_HEIGHT; // Випадкова позиція по Y
        stars[i].speed = (rand() % MAX_SPEED) + 1; // Випадкова швидкість
    }

    // --- ЕТАП 2: ОСНОВНИЙ ЦИКЛ АНІМАЦІЇ ---
    
    // Засікаємо час початку
    uint32_t start_time = HAL_GetTick();

    // Виконуємо цикл, поки не пройде LOGO_DURATION_MS
    while (HAL_GetTick() - start_time < LOGO_DURATION_MS)
    {
        // КРОК 1: ОЧИЩЕННЯ БУФЕРА
        ssd1306_Fill(Black);

        // КРОК 2: ОНОВЛЕННЯ ТА МАЛЮВАННЯ КОЖНОЇ ЗІРКИ
        for (int i = 0; i < NUM_STARS; i++) {
            // Рухаємо зірку вправо
            stars[i].x += stars[i].speed;

            // Якщо зірка вийшла за межі екрана, перезапускаємо її зліва
            if (stars[i].x >= SSD1306_WIDTH) {
                reset_star(i);
            }

            // Малюємо зірку (ефект паралаксу)
            if (stars[i].speed == 3) {
                ssd1306_Line(stars[i].x, stars[i].y, stars[i].x - 2, stars[i].y, White);
            } else if (stars[i].speed == 2) {
                ssd1306_Line(stars[i].x, stars[i].y, stars[i].x - 1, stars[i].y, White);
            } else {
                ssd1306_DrawPixel(stars[i].x, stars[i].y, White);
            }
        }
        
        // --- КРОК 3: МАЛЮВАННЯ ТЕКСТУ ПОВЕРХ ЗІРОК ---
        ssd1306_SetCursor(20, 20);
        ssd1306_WriteString("Home", Font_11x18, White);
        ssd1306_SetCursor(40, 40);
        ssd1306_WriteString("Server", Font_11x18, White);

        // --- КРОК 4: ОНОВЛЕННЯ ЕКРАНА ---
        ssd1306_UpdateScreen();

        // --- КРОК 5: НЕВЕЛИКА ЗАТРИМКА ---
        HAL_Delay(10); 
    }

    // --- ЕТАП 3: ОЧИЩЕННЯ ЕКРАНА ---
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
}