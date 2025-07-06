#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "battery_monitor.h"
#include "battery_level_pattern.h"
#include "led_control.h"
#include "touch_input.h"
#include "storage.h"
#include "genes.h"
#include "pins.h"
#include "microphone.h"

void app_main() {
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
    init_storage();
    load_settings(&settings);

    set_pattern(settings.pattern_id);
    set_brightness(settings.brightness);

    // Create tasks
    xTaskCreatePinnedToCore(lighting_task, "Lighting Task", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(touch_task, "Touch Task", 4096, NULL, 5, NULL, 0);
    //xTaskCreatePinnedToCore(touch_debug_task, "Touch Debug Task", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(periodic_touch_recalibration_task, "Periodic Touch Recalibration Task", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(battery_monitor_task, "Battery Monitor Task", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(microphone_task, "Microphone Task", 4096, NULL, 5, NULL, 0);
}