#include "stm32f4xx_hal.h"
#include "headers/main_decl.h"

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
// I2C_HandleTypeDef  hi2c1;
TIM_HandleTypeDef  htim1;

volatile uint32_t systick_10us_ticks = 0;
volatile uint32_t milliseconds       = 0;
volatile uint32_t seconds            = 0;

/* Кільцевий буфер прийому UART1, заповнюється з USART1_IRQHandler */
volatile uint8_t  uart1_rx_buff[RX_BUFF_SIZE];
volatile uint16_t uart1_rx_head = 0;
volatile uint16_t uart1_rx_tail = 0;

/* Кільцевий буфер прийому UART2, заповнюється з USART2_IRQHandler */
volatile uint8_t  uart2_rx_buff[RX_BUFF_SIZE];
volatile uint16_t uart2_rx_head = 0;
volatile uint16_t uart2_rx_tail = 0;

/* Налаштування системного тактування */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    /* HSI на F401 = 16MHz. Налаштування на 64MHz */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 128;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; /* 16/16 * 128 / 2 = 64MHz */
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
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


void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* RESTART_PIN / RESTART_GPIO_Port */
    GPIO_InitStruct.Pin = RESTART_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RESTART_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(RESTART_GPIO_Port, RESTART_PIN, GPIO_PIN_RESET);

    /* Вбудований LED PC13 */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = LED_Pin;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

    // POWER_LED, HDD_1_LED, HDD_2_LED
    GPIO_InitStruct.Pin = POWER_LED | HDD_1_LED | HDD_2_LED;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(POWER_LED_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED | HDD_1_LED | HDD_2_LED, GPIO_PIN_RESET);

    // POWER, RESET 1 и RESET 2
    GPIO_InitStruct.Pin = POWER_ON_BTN | RESET_BTN_1 | RESET_BTN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;     // Режим входа
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}


void UART2_Init(void) {
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* A2 = TX, A3 = RX: 9600 */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();

    /* Увімкнути переривання по RX (ручний ISR, без HAL_UART_Receive_IT) */
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
    HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

void UART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PB7 RX */
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PB6 TX */
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();

    HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void UART1_SendString(const char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), 100);
}

void UART2_SendString(const char *str) {
    HAL_UART_Transmit(&huart2, (uint8_t *)str, strlen(str), 100);
}

void uart1_flush(void) {
    uint8_t dummy;
    /* Вичитуємо всі байти з регістра даних */
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
        HAL_UART_Receive(&huart1, &dummy, 1, 0);
    }
}

void uart2_flush(void) {
    uint8_t dummy;
    /* Вичитуємо всі байти з регістра даних */
    while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
        HAL_UART_Receive(&huart2, &dummy, 1, 0);
    }
}

// можно использовать вместо этого: HAL_UART_IRQHandler(&huart1);
/* UART2 ISR — кладе байт у кільцевий буфер */
// void USART2_IRQHandler(void) {
//     if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
//         uint8_t byte = (uint8_t)(huart2.Instance->DR & 0xFF);

//         uint16_t next_head = (uart2_rx_head + 1) % RX_BUFF_SIZE;

//         if (next_head != uart2_rx_tail) {
//             uart2_rx_buff[uart2_rx_head] = byte;
//             uart2_rx_head = next_head;
//         }
//     }
// }

void USART1_IRQHandler(void) {
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
        uint8_t byte = (uint8_t)(huart1.Instance->DR & 0xFF);
        uint16_t next_head = (uart1_rx_head + 1) % RX_BUFF_SIZE;

        if (next_head != uart1_rx_tail) {
            uart1_rx_buff[uart1_rx_head] = byte;
            uart1_rx_head = next_head;
        }
    }
}

// void i2c1_init(void) {
//     GPIO_InitTypeDef GPIO_InitStruct = {0};

//     /* Увімкнення тактування */
//     __HAL_RCC_GPIOB_CLK_ENABLE();
//     __HAL_RCC_I2C1_CLK_ENABLE();

//     /* Налаштування пінів PB6 та PB7 для I2C1 */
//     GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
//     GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
//     GPIO_InitStruct.Pull = GPIO_PULLUP;
//     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

//     /* Налаштування I2C1 */
//     hi2c1.Instance = I2C1;
//     hi2c1.Init.ClockSpeed = 400000;
//     hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
//     hi2c1.Init.OwnAddress1 = 0;
//     hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
//     hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
//     hi2c1.Init.OwnAddress2 = 0;
//     hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
//     hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

//     if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
//         Error_Handler();
//     }
// }

void MX_TIM1_Init(void) {
    TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM1_CLK_ENABLE();

    // Ніякого ремапу не потрібно — PA8 за замовчуванням

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 71;              // 72 MHz / 72 = 1 MHz (1 мкс/такт)
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

    // Важливо для TIM1: ввімкнути Main Output Enable
    __HAL_TIM_MOE_ENABLE(&htim1);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
}

void Error_Handler(void) {
    __disable_irq();
    while (1)
    {
        // Зависаємо тут у разі помилки
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