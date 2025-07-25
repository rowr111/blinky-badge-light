#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_system.h"

#include "battery_monitor.h"
#include "battery_level_pattern.h"
#include "led_control.h"
#include "touch_input.h"
#include "storage.h"
#include "genes.h"
#include "pins.h"
#include "microphone.h"
#include "now.h"
#include "testing_routine.h"

void app_main() {
    esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI("MAIN", "Reset reason: %s", reset_reason_str(reason));

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret != ESP_OK) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Load or generate genomes
    load_genomes_from_storage();
    // Initialize components
    init_battery_monitor();
    init_leds();
    init_touch();
    init_microphone();
    now_init();
    init_storage();
    load_settings(&settings);

    set_pattern(settings.pattern_id);
    set_brightness(settings.brightness);

    vTaskDelay(pdMS_TO_TICKS(50)); // give touch pads a little time to settle
    if (get_is_touched(0)) { // Check if pad 0 is touched on startup
        ESP_LOGI("TOUCH", "Touch detected on pad 0 on startup, starting testing routine.");
        testing_routine();
    } else {
        ESP_LOGI("TOUCH", "No testing on startup, starting normal operation.");
    }

    ESP_LOGI("MAIN", "System initialized. Starting tasks...");
    // Create tasks
    xTaskCreatePinnedToCore(lighting_task, "Lighting Task", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(battery_monitor_task, "Battery Monitor Task", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(microphone_task, "Microphone Task", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(touch_task, "Touch Task", 4096, NULL, 5, NULL, 0);
}