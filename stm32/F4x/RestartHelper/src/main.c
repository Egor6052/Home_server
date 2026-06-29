#include "stm32f4xx_hal.h"
#include "headers/main_decl.h"


int main(void) {
    HAL_Init();
    
    SystemClock_Config();
    SysTick_Init_100kHz();
    GPIO_Init();
    UART1_Init();
    UART2_Init();

    for (int i = 0; i < 3; i++) {
        blink_led(100);
    }

    restart_helper_init();

    while (1) {
        restart_helper();
    }
}