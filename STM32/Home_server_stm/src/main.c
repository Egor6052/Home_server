#include "stm32f1xx_hal.h"
#include "headers/main_decl.h"
#include <string.h>
#include <stdio.h>

int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART2_Init();
    I2C_Init();
    AHT10_Init();

    Blink_LED();
    
    const char *stmID = getID_stm();

    while (1) {

        if (UART2_ReceiveByte_NonBlocking(&rx_byte)) {
            if (rx_index < sizeof(rx_buffer) - 1) {
                rx_buffer[rx_index++] = rx_byte;
                rx_buffer[rx_index] = '\0';
            }

            if (rx_byte == '}') {
                if (strstr(rx_buffer, "\"mk\":\"1\"") && strstr(rx_buffer, "\"data\":\"true\"")) {

                    if (AHT10_Read_Data() != 0) {
                        // Якщо помилка I2C, використовуємо заглушки "0.0"
                        char *temp = "0.0"; 
                        char *hum = "0";
                        char *lat = getLatitude();
                        char *lng = getLongitude();
                        
                        // Формування JSON
                        char response[256];
                        snprintf(response, sizeof(response),
                            "{\"id\":\"%s\",\"lat\":%s,\"lng\":%s,\"temp\":%s,\"hum\":%s}\r\n",
                            stmID, lat, lng, temp, hum);
                        
                        HAL_UART_Transmit(&huart2, (uint8_t*)response, strlen(response), HAL_MAX_DELAY);
                        Blink_LED();

                    } else {
                        // Якщо I2C успішне, використовуємо функції, які конвертують last_temp/last_hum
                        char *temp = getTemperature();
                        char *hum = getHumidity();
                        char *lat = getLatitude();
                        char *lng = getLongitude();

                        // Формування JSON
                        char response[256];
                        snprintf(response, sizeof(response),
                            "{\"id\":\"%s\",\"lat\":%s,\"lng\":%s,\"temp\":%s,\"hum\":%s}\r\n",
                            stmID, lat, lng, temp, hum);

                        HAL_UART_Transmit(&huart2, (uint8_t*)response, strlen(response), HAL_MAX_DELAY);
                        Blink_LED();
                    }
                }
                rx_index = 0;
                memset(rx_buffer, 0, sizeof(rx_buffer));
            }
        }
    }
}