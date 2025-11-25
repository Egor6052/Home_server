#include "stm32f1xx_hal.h"
#include "headers/main_decl.h"
// #include <string.h>
// #include <stdio.h>
// #include <stdint.h>

const char* getID_stm(void) {
    return "stm32f1xROOM00001";
}

// void Generate_UUID_from_UID(uuid_t* uuid) {
//     // 1. Отримуємо 96-бітний UID
//     uint32_t uid0 = HAL_GetUIDw0(); // Перші 32 біти
//     uint32_t uid1 = HAL_GetUIDw1(); // Середні 32 біти
//     uint32_t uid2 = HAL_GetUIDw2(); // Останні 32 біти

//     // 2. Ініціалізуємо UUID нулями
//     memset(uuid->bytes, 0, 16);

//     // 3. Змішуємо 96-бітний UID в 16-байтовий буфер (128 біт)
//     // Це проста техніка "розтягування" для унікальності
    
//     // Заповнюємо перші 4 байти (UID0)
//     memcpy(&uuid->bytes[0], &uid0, 4); 
    
//     // Заповнюємо наступні 4 байти (UID1)
//     memcpy(&uuid->bytes[4], &uid1, 4); 
    
//     // Заповнюємо наступні 4 байти (UID2)
//     memcpy(&uuid->bytes[8], &uid2, 4); 
    
//     // 4. Використовуємо XOR для заповнення останніх 4 байтів 
//     // та змішування: uid0 XOR uid1 XOR uid2
//     uint32_t mixed = uid0 ^ uid1 ^ uid2;
//     memcpy(&uuid->bytes[12], &mixed, 4); 

//     // 5. Встановлюємо біти UUID Version (наприклад, 3) та Variant (RFC 4122)
//     // Щоб ID виглядав як стандартний UUID
//     uuid->bytes[6] = (uuid->bytes[6] & 0x0F) | 0x30; // Версія 3
//     uuid->bytes[8] = (uuid->bytes[8] & 0x3F) | 0x80; // Варіант 1
// }

// /**
//  * @brief Форматування UUID у рядок (стандартний формат: 8-4-4-4-12 символів)
//  * * @param buffer Буфер для вихідного рядка (мінімум 37 символів: 32 цифри + 4 дефіси + 1 нуль)
//  * @param uuid Вказівник на структуру uuid_t
//  */
// void Format_UUID_String(char* buffer, const uuid_t* uuid) {
//     sprintf(buffer, 
//             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
//             uuid->bytes[0], uuid->bytes[1], uuid->bytes[2], uuid->bytes[3],
//             uuid->bytes[4], uuid->bytes[5],
//             uuid->bytes[6], uuid->bytes[7],
//             uuid->bytes[8], uuid->bytes[9],
//             uuid->bytes[10], uuid->bytes[11], 
//             uuid->bytes[12], uuid->bytes[13], uuid->bytes[14], uuid->bytes[15]);
// }

// // --------------------------------------------------------------------------
// // --- Єдина Функція, Яку Ви Будете Викликати ---
// // --------------------------------------------------------------------------

// /**
//  * @brief Основна функція: Генерує унікальний UUID на основі UID STM32 
//  * та повертає його у вигляді рядка.
//  * * @param buffer Буфер для вихідного рядка (повинен бути не менше 37 символів!)
//  */
// void Get_Unique_ID_String(char* buffer) {
//     uuid_t my_uuid;
//     Generate_UUID_from_UID(&my_uuid);
//     Format_UUID_String(buffer, &my_uuid);
// }