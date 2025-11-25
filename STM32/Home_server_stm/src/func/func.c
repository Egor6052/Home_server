#include "headers/main_decl.h"

// Обробник помилок
void Error_Handler(void) {
    __disable_irq();
    while (1)
    {
        // Зависаємо тут у разі помилки
    }
}

// Обробник SysTick (потрібен для HAL_Delay)
void SysTick_Handler(void) {
    HAL_IncTick();
}

void Blink_LED() {
    for (int i = 0; i < 3; i++) {
        HAL_GPIO_WritePin(GPIOC, LED_Pin, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(GPIOC, LED_Pin, GPIO_PIN_RESET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(GPIOC, LED_Pin, GPIO_PIN_SET);
        HAL_Delay(100);
    }
}


void float_to_str(char* buffer, float f, int precision) {
    if (isnan(f)) {
        strcpy(buffer, "0.0");
        return;
    }
    
    // 1. Обчислення цілої частини
    int whole = (int)f;
    
    // 2. Обчислення дробової частини
    float frac_val = f - whole;
    int frac = (int)(frac_val * pow(10, precision)); // pow() вимагає <math.h> і лінковки з -lm
    
    // 3. Форматування (використовуємо %d, який підтримується)
    // Формат: [ЦІЛА].[ДРОБОВА]
    sprintf(buffer, "%d.%d", whole, frac);
}