#include "../headers/inside_server.h"

SensorsData sensor_data = {0, 0};

uint32_t sensor_timer = 0;
uint32_t temp_request_time = 0;
bool temp_requested = false;

void update_inside_server_data(void) {
    // Каждые 500 мс читаем INA219 и ЗАПУСКАЕМ чтение температуры
    if (HAL_GetTick() - sensor_timer > 500) {
        sensor_timer = HAL_GetTick();
        
        // Если конвертация еще не идет, стартуем её
        if (!temp_requested) {
            DS18B20_RequestTemperature();
            temp_request_time = HAL_GetTick();
            temp_requested = true;
        }
    }

    // Если прошло 750 мс с момента запроса - ЗАБИРАЕМ температуру
    if (temp_requested && (HAL_GetTick() - temp_request_time >= 750)) {
        sensor_data.temperature = DS18B20_ReadTemperature();
        // sensor_data.culler_status = HAL_GPIO_ReadPin(CULLER_GPIO_Port, CULLER_PIN) == GPIO_PIN_SET ? 0x01 : 0x00;

        temp_requested = false;
    }
}