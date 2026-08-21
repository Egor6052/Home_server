#ifndef DS18B20_H
#define DS18B20_H

// Use the following flags for compiling the right library, e.g.: -D STM32F1
#if defined(STM32F0)
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_tim.h"
#elif defined(STM32F1)
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_tim.h"
#elif defined(STM32F2)
#include "stm32f2xx_hal.h"
#include "stm32f2xx_hal_tim.h"
#elif defined(STM32F3)
#include "stm32f3xx_hal.h"
#include "stm32f3xx_hal_tim.h"
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#elif defined(STM32F7)
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_tim.h"
#else
#error "Unsupported STM32 microcontroller. Make sure you build with -STM32F1 for example!"
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>  // For uint8_t, etc.

// DS18B20 Temperature sensor
#define DS18B20_PIN         GPIO_PIN_6
#define DS18B20_PORT        GPIOA

typedef struct {
    uint8_t address[8];  // ROM address
    float temperature;   // Current temperature
    // Add other fields as needed, e.g., resolution, etc.
	
	TIM_HandleTypeDef *_tim;
	GPIO_TypeDef *_port;
	uint16_t _pin;

} DS18B20_t;

	void DS18B20_Init(DS18B20_t *sensor, TIM_HandleTypeDef *tim, GPIO_TypeDef *port, uint16_t pin);
	
	void DS18B20_RequestTemperature(void);
	uint16_t DS18B20_ReadTemperature();
	// void read_temp_celsius();
	// float read_temp_fahrenheit();

	void set_data_pin(bool on);
	// void toggle_data_pin();

	void set_pin_output();
	void set_pin_input();

	GPIO_PinState read_data_pin();

	void start_sensor();

	void writeData(uint8_t data);
	uint8_t read_data();

	void delay_us_16(uint16_t us);

#endif // DS18B20_H
