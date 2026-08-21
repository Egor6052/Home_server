#include "stm32f1xx_hal.h"
#include "headers/main_decl.h"
#include <stdbool.h>

/* Логіка автономного керування кулером. STM32 - єдина точка, яка має
   пережити зникнення/зависання Raspberry Pi, тож весь стан і рішення
   про ввімкнення/вимкнення живуть тут, а не на бекенді.

   Правила (за вашою специфікацією):
   - Raspberry Pi шле culler_status кожним пакетом як прапорець
     "що робити зараз": 0x01 - примусово увімкнути, 0x00 - примусово
     вимкнути, 0xFF - не займатись (тримати поточний стан).
   - culler_apply_command() викликається лише з process_packet(), тобто
     лише поки Raspberry жива й шле валідні пакети - "тримати стан, поки
     Raspberry не пропаде" виходить само собою: між пакетами сюди ніхто
     не заходить і стан просто лишається яким був.
   - Коли Raspberry зникає (спрацював watchdog-таймаут) або система в
     процесі рестарту (RESTART_STATE_HOLDING) - в обох випадках Raspberry
     зараз НЕ керує кулером наживо, тож перемикаємось на автономний
     температурний режим (culler_autonomous_tick), що використовує ту ж
     саму ідею таймауту, що вже є для тригера рестарту. */



#define CULLER_TEMP_DEFAULT_C 50

#define CULLER_TEMP_MIN_C         20
#define CULLER_TEMP_MAX_C         90
#define CULLER_HYSTERESIS_C        5   /* ширина гістерезису; можна змінити, якщо треба інша */

/* Якщо показання виглядає як сигнал помилки DS18B20 (0x02 з
   DS18B20_ReadTemperature(), тобто <=0.50 С у форматі *100) - довіряти
   йому не можна: скоріш за все це не реальна температура, а
   несправність/відсутність датчика. Fail-safe: краще зайвий раз
   увімкнути кулер, ніж мовчки не охолоджувати сервер через хибне
   "холодне" покажчик. Якщо це небажано - скажіть, приберу. */
#define CULLER_TEMP_SUSPECT_C100   50

static bool    culler_running        = false;
static uint8_t culler_temp_threshold = CULLER_TEMP_DEFAULT_C;

static void culler_drive(bool on) {
    culler_running = on;
    HAL_GPIO_WritePin(CULLER_GPIO_Port, CULLER_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* Викликати один раз при старті (поруч з restart_helper_init()) - явно
   виставляє безпечний стан за замовчуванням і синхронізує пін з ним
   (GPIO_Init() вже опускає пін у RESET, тут - той самий стан, але й
   внутрішній прапорець culler_running теж стає узгодженим). */
void culler_init(void) {
    culler_drive(false);
}

/* cmd - те саме, що прийшло в req->field.culler_status від Raspberry.
   Викликати лише для валідного, щойно прийнятого пакета (тобто з
   process_packet()) - тоді "тримати стан, поки Raspberry жива" не
   вимагає окремого таймера: просто ніхто не змінює culler_running між
   пакетами. */
void culler_apply_command(uint8_t cmd) {
    switch (cmd) {
        case 0x01: culler_drive(true);  break;
        case 0x00: culler_drive(false); break;
        default: break; /* 0xFF і будь-що інше - не займати поточний стан */
    }
}

/* new_threshold_c - те саме, що прийшло в req->field.culler_temp.
   0xFF за аналогією з rest_time означає "не змінювати". Клемпінг у
   межах CULLER_TEMP_MIN_C..CULLER_TEMP_MAX_C - захист від випадкового
   "0 С" чи "255 С" з пошкодженого/старого клієнта. */
void culler_set_threshold(uint8_t new_threshold_c) {
    if (new_threshold_c == 0xFF) return;
    if (new_threshold_c < CULLER_TEMP_MIN_C) new_threshold_c = CULLER_TEMP_MIN_C;
    if (new_threshold_c > CULLER_TEMP_MAX_C) new_threshold_c = CULLER_TEMP_MAX_C;
    culler_temp_threshold = new_threshold_c;
}

/* Для send_response(): звітуємо РЕАЛЬНИЙ поточний стан кулера (0x00/0x01),
   а не просто відлунюємо команду з запиту - так Raspberry завжди бачить
   правду, навіть якщо зараз керує автономна логіка. */
uint8_t culler_get_status(void) {
    return culler_running ? 0x01 : 0x00;
}

/* Викликати з restart_helper() у ті моменти, коли Raspberry зараз НЕ
   керує кулером наживо (RESTART_STATE_HOLDING або спрацював watchdog
   timeout) - тобто "якщо нема Raspberry, діяти по-своєму".
   temperature_c100 - те саме sensor_data.temperature (int16_t, град.*100). */
void culler_autonomous_tick(int16_t temperature_c100) {
    if (temperature_c100 <= CULLER_TEMP_SUSPECT_C100) {
        culler_drive(true);
        return;
    }

    int16_t threshold_on_c100  = (int16_t)culler_temp_threshold * 100;
    int16_t threshold_off_c100 = (int16_t)(culler_temp_threshold - CULLER_HYSTERESIS_C) * 100;

    if (!culler_running && temperature_c100 >= threshold_on_c100) {
        culler_drive(true);
    } else if (culler_running && temperature_c100 <= threshold_off_c100) {
        culler_drive(false);
    }
    /* інакше - в "мертвій зоні" гістерезису, стан не чіпаємо */
}