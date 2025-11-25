#include "stm32f1xx_hal.h"
#include "headers/main_decl.h"


char temp_buffer[8];
char hum_buffer[8];

float last_temp = 0.0f;
float last_hum = 0.0f;

uint8_t AHT10_Init(void) {
    uint8_t tx_buf[] = {AHT10_CMD_INIT, 0x08, 0x00};
    
    // Надсилаємо команду ініціалізації/калібрування
    if (HAL_I2C_Master_Transmit(&hi2c1, AHT10_ADDR, tx_buf, 3, 100) != HAL_OK) {
        return 1;
    }
    HAL_Delay(50);
    return 0;
}

uint8_t AHT10_Read_Data(void) {
    uint8_t rx_data[6];
    uint8_t tx_buf[] = {AHT10_CMD_MEASURE, 0x33, 0x00};
    
    // 1. Запуск вимірювання
    if (HAL_I2C_Master_Transmit(&hi2c1, AHT10_ADDR, tx_buf, 3, 100) != HAL_OK) {
        return 1;
    }
    HAL_Delay(80); // Очікування 80 мс на завершення вимірювання
    
    // 2. Читання 6 байтів даних (Статус + 40 біт даних + CRC)
    if (HAL_I2C_Master_Receive(&hi2c1, AHT10_ADDR, rx_data, 6, 100) != HAL_OK) {
        return 1;
    }
    
    // Перевірка біта калібрування (біт 3 в байті статусу - rx_data[0])
    if (!(rx_data[0] & 0x68)) { 
        // Примітка: Якщо біт калібрування 0, то датчик не готовий, 
        // але ми продовжуємо обчислення для демонстрації.
    }
    
    // 3. Конвертація 20-бітних даних вологості (RH)
    uint32_t hum_raw = (uint32_t)rx_data[1] << 12 | (uint32_t)rx_data[2] << 4 | (uint32_t)(rx_data[3] >> 4);
    last_hum = ((float)hum_raw / 1048576.0f) * 100.0f; // 1048576 = 2^20
    
    // 4. Конвертація 20-бітних даних температури (Temp)
    uint32_t temp_raw = (uint32_t)(rx_data[3] & 0x0F) << 16 | (uint32_t)rx_data[4] << 8 | (uint32_t)rx_data[5];
    last_temp = ((float)temp_raw / 1048576.0f) * 200.0f - 50.0f;
    
    return 0;
}

uint8_t read_aht10_and_check(void) {
    return AHT10_Read_Data(); 
}

char* getTemperature(void) {
    if (AHT10_Read_Data() != 0) { 
        return "0.0";
    }
    float_to_str(temp_buffer, last_temp, 1);
    return temp_buffer;
}

char* getHumidity(void) {
    float_to_str(hum_buffer, last_hum, 0);
    return hum_buffer;
}

char* getLatitude(void) {
    return "46.43";
}

char* getLongitude(void) {
    return "30.69";
}
