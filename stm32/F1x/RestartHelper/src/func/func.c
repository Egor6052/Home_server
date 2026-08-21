#include "stm32f1xx_hal.h"
#include "headers/main_decl.h"
 
/* Усе UART- і restart-специфічне виїхало в uart.c / restart.c. Тут -
   лишок: тактування, SysTick, GPIO_Init, TIM1/PWM, Error_Handler,
   blink-хелпери, CRC16, читання кнопок. */
 
TIM_HandleTypeDef htim1;
 
volatile uint32_t systick_10us_ticks = 0;
volatile uint32_t milliseconds       = 0;
volatile uint32_t seconds            = 0;
 
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState  = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL    = RCC_PLL_MUL16; /* 4MHz * 16 = 64MHz */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
 
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}
 
/* === SysTick_Init_100kHz / SysTick_Handler - без змін, як були у func.c === */
 
void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
 
    GPIO_InitStruct.Pin   = RESTART_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RESTART_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(RESTART_GPIO_Port, RESTART_PIN, GPIO_PIN_RESET);
 
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    GPIO_InitStruct.Pin  = LED_Pin;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);
 
    GPIO_InitStruct.Pin   = POWER_LED | HDD_1_LED | HDD_2_LED;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(POWER_LED_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED | HDD_1_LED | HDD_2_LED, GPIO_PIN_RESET);
 
    GPIO_InitStruct.Pin  = POWER_ON_BTN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(POWER_ON_GPIO_Port, &GPIO_InitStruct);
 
    GPIO_InitStruct.Pin  = RESET_BTN_1 | RESET_BTN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
 
    // CULLER
    GPIO_InitStruct.Pin   = CULLER_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CULLER_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(CULLER_GPIO_Port, CULLER_PIN, GPIO_PIN_RESET);

    // Buzzer на PA15
    HAL_GPIO_WritePin(GPIOA, BUUZER_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = BUUZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
 
 
void Error_Handler(void) {
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        for (volatile uint32_t i = 0; i < 500000; i++) {}
    }
}

void blink_led(int value_time) {
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED, GPIO_PIN_RESET);
    HAL_Delay(value_time);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED, GPIO_PIN_SET);
    HAL_Delay(value_time);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED, GPIO_PIN_RESET);
}

void blink_power_led(int value_time) {
    HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED, GPIO_PIN_SET);
    HAL_Delay(value_time);
    HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED, GPIO_PIN_RESET);
    HAL_Delay(value_time);
    HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED, GPIO_PIN_SET);
}

void blink_hdd_led(uint8_t hdd_num, int value_time) {
    uint16_t hdd_pin;

    if (hdd_num == 1) {
        hdd_pin = HDD_1_LED;
    } else if (hdd_num == 2) {
        hdd_pin = HDD_2_LED;
    } else {
        return;
    }

    HAL_GPIO_WritePin(HDD_LED_GPIO_Port, hdd_pin, GPIO_PIN_SET);
    HAL_Delay(value_time);
    HAL_GPIO_WritePin(HDD_LED_GPIO_Port, hdd_pin, GPIO_PIN_RESET);
    HAL_Delay(value_time);
    HAL_GPIO_WritePin(HDD_LED_GPIO_Port, hdd_pin, GPIO_PIN_SET);
}

uint16_t CalculateCRC16(uint8_t *buffer, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < length; pos++) {
        crc ^= (uint16_t)buffer[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool PorewButton(void) {
    return (HAL_GPIO_ReadPin(POWER_ON_GPIO_Port, POWER_ON_BTN) == GPIO_PIN_SET);
}

bool RestartButton1(void) {
    if (HAL_GPIO_ReadPin(RESET_BTN_1_GPIO_Port, RESET_BTN_1) == GPIO_PIN_SET) {
        return true;
    } else {
        return false;
    }
}

bool RestartButton2(void){
    if (HAL_GPIO_ReadPin(RESET_BTN_2_GPIO_Port, RESET_BTN_2) == GPIO_PIN_SET) {
        return true;
    } else {
        return false;
    }
}


void SysTick_Init_100kHz(void) {
    SystemCoreClockUpdate();
    SysTick->CTRL = 0;
    uint32_t reload = SystemCoreClock / 100000U;
    SysTick->LOAD = reload - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void) {
    static uint16_t cnt_1ms = 0;
    static uint32_t cnt_1s = 0;
    systick_10us_ticks++;
    if (++cnt_1ms >= 100) { cnt_1ms = 0; HAL_IncTick(); milliseconds++; }
    if (++cnt_1s >= 100000) { cnt_1s = 0; seconds++; }
}



void MX_TIM1_Init(void) {
    TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM1_CLK_ENABLE();

    // Ніякого ремапу не потрібно — PA8 за замовчуванням

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 63;              // 64 MHz / 64 = 1 MHz (1 мкс/такт)
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 19999;              // 20 мс = 50 Hz
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1500;                 // Початково 1.5 мс = 90°
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }

    __HAL_TIM_MOE_ENABLE(&htim1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
}
