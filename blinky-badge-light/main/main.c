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

// Shared settings
badge_settings_t settings;

// Lighting task
void lighting_task(void *param) {
    int loop = 0;
    uint8_t framebuffer[LED_COUNT * 3];

    while (1) { 
        if (!flash_active) { // Only render patterns if no feedback is active
            // Render the current pattern 
            if (force_safety_pattern) {
                safety_pattern(framebuffer);
            } else {
                render_pattern(settings.pattern_id, framebuffer, LED_COUNT, loop);
            }
            // Update LEDs
            update_leds(framebuffer);

            // Increment loop counter for animation
            loop = (loop + 1) % 256;

            // Delay for smooth animation (adjust as needed)
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }
}

// Touch input task
void touch_task(void *param) {
    while (1) {
        for (int i = 0; i < NUM_TOUCH_PADS; i++) {
            int event = get_touch_event(i);
            if (event == SHORT_PRESS) {
                if (i == 0) {
                    // Pad 1: Cycle patterns
                    settings.pattern_id = (settings.pattern_id + 1) % NUM_PATTERNS;
                    set_pattern(settings.pattern_id);
                    ESP_LOGI("MAIN", "Current pattern: %d", settings.pattern_id);
                    // Save the updated settings
                    save_settings(&settings);
                    break;
                } else if (i == 1) {
                    // Pad 2: Adjust brightness
                    settings.brightness = (settings.brightness + 1) % NUM_BRIGHTNESS_LEVELS;
                    set_brightness(settings.brightness);
                    save_settings(&settings);
                    break;
                } else if (i == 4) {
                    // Pad 5: Toggle battery meter
                    show_battery_meter = true;
                    battery_meter_start_time = esp_timer_get_time() / 1000;
                    break;
                }
            } else if (event == LONG_PRESS) {
                ESP_LOGI("MAIN", "Long press on pad %d", i);
                if (i == 2) {
                    // Pad 3: Generate new genome for current pattern
                    generate_gene(&patterns[settings.pattern_id]);
                    save_genomes_to_storage();
                    flash_feedback_pattern();

                    // use this for testing battery meter pattern
                    //show_battery_meter = true;
                    //battery_meter_start_time = esp_timer_get_time() / 1000;
                    break;
                }
                if (i==3) {
                    // Pad 4: turn off.
                    turn_off();
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); // Adjust polling rate
    }
}

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
    xTaskCreatePinnedToCore(periodic_touch_recalibration_task, "Periodic Touch Recalibration Task", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(battery_monitor_task, "Battery Monitor Task", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(microphone_task, "Microphone Task", 4096, NULL, 5, NULL, 0);
}