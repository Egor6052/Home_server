#include "../src/headers/DS18B20.h"

static DS18B20_t *_sensor = NULL;

void delay_us_16(uint16_t us)
{
    // Обнуляємо лічильник. Переконайся, що TIM1 налаштований: Prescaler=71 (для 72MHz)
	_sensor->_tim->Instance->CNT = 0;
	while (_sensor->_tim->Instance->CNT < us);
}

// Управління піном в режимі Open Drain
// true  = 1 (High, відпустити лінію, підтягується резистором)
// false = 0 (Low, притягнути до землі)
void set_data_pin(bool on)
{
	HAL_GPIO_WritePin(_sensor->_port, _sensor->_pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

GPIO_PinState read_data_pin()
{
	return HAL_GPIO_ReadPin(_sensor->_port, _sensor->_pin);
}

// ЦІ ФУНКЦІЇ МАЮТЬ БУТИ ПОРОЖНІМИ
// Ми не хочемо витрачати час на переініціалізацію GPIO
void set_pin_output() {}
void set_pin_input() { set_data_pin(true); } // Для входу просто відпускаємо лінію

void DS18B20_Init(DS18B20_t *sensor, TIM_HandleTypeDef *tim, GPIO_TypeDef *port, uint16_t pin)
{
	sensor->_tim = tim;
	sensor->_port = port;
	sensor->_pin = pin;
	_sensor = sensor;
	HAL_TIM_Base_Start(sensor->_tim);

	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin   = pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(port, &GPIO_InitStruct);

    // Важливо: відпустити лінію при старті
    set_data_pin(true);
}

void start_sensor()
{
    // Reset Pulse
	set_data_pin(false);
	delay_us_16(480);

    // Presence Pulse check
	set_data_pin(true);
	delay_us_16(80);
    // Тут можна читати read_data_pin(), щоб дізнатися чи є датчик, але поки пропускаємо
	delay_us_16(400);
}

void writeData(uint8_t data)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		// Початок слоту завжди Low
        set_data_pin(false);
        delay_us_16(2);

		if (data & (1 << i))
		{
            // Пишемо 1: відпускаємо одразу
			set_data_pin(true);
			delay_us_16(60);
		}
		else
		{
            // Пишемо 0: тримаємо Low
            delay_us_16(60);
			set_data_pin(true);
		}
        delay_us_16(2); // Recovery time
	}
}

uint8_t read_data()
{
	uint8_t value = 0;

	for (uint8_t i = 0; i < 8; i++)
	{
		set_data_pin(false); // Start slot
		delay_us_16(2);

        set_data_pin(true);  // Release for reading
		delay_us_16(10);        // Wait for sample point (близько 12-14мкс від старту)

		if (read_data_pin())
		{
			value |= 1 << i;
		}
		delay_us_16(50); // End of slot
	}
	return value;
}


void DS18B20_RequestTemperature(void) {
    start_sensor();
    delay_us_16(1000); // 1 мс не повредит UART
    writeData(0xCC);   // Skip ROM
    writeData(0x44);   // Convert T
}

uint16_t DS18B20_ReadTemperature() {
    start_sensor();
    writeData(0xCC); // Skip ROM
    writeData(0xBE); // Read Scratchpad

    uint8_t temp1 = read_data(); // LSB
    uint8_t temp2 = read_data(); // MSB

    int16_t temp_com = (temp2 << 8) | temp1;
    float temp_c = temp_com / 16.0f;
	// uint16_t temp_c = (uint16_t)(temp_com * 100 / 16);

    // Проверка на обрыв (обычно датчик шлет 85.0 при ошибке питания или <-50 при обрыве)
    if (temp_c < -50.0f || temp_c > 125.0f) {
        return 0x02; // Добавляем ошибку, не затирая статус INA219
    } else {
        return (int16_t)(temp_c * 100.0f); // На беке делим на 100.0f
        // sensor_data.status &= ~0x02; // Сбрасываем только бит ошибки температуры
    }
}