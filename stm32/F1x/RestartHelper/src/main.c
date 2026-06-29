#include "stm32f1xx_hal.h"
#include "headers/main_decl.h"
#include "../lib/ssd1306/ssd1306.h"
#include "../lib/ssd1306/ssd1306_fonts.h"

I2C_HandleTypeDef hi2c1;


int main(void) {
    HAL_Init();
    
    SystemClock_Config();
    SysTick_Init_100kHz();
    GPIO_Init();
    UART1_Init();

    // Initialize I2C1
    i2c1_init();
    
    // Initialize SSD1306 display
    ssd1306_Init();

    screen_logo();

    // Clear screen
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();

    for (int i = 0; i < 3; i++) {
        blink_led(100);
    }

    restart_helper_init();

    while (1) {
        restart_helper();
    }
}