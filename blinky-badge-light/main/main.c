#include "led_control.h"
#include "touch_input.h"
#include "storage.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Shared settings
badge_settings_t settings;

// Lighting task
void lighting_task(void *param) {
    while (1) {
        // Update LEDs based on the current pattern
        //update_leds();
        vTaskDelay(50 / portTICK_PERIOD_MS); // Adjust delay for smoother animations
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
                    // Save the updated settings
                    save_settings(&settings);
                    break;
                } else if (i == 1) {
                    // Pad 2: Adjust brightness
                    settings.brightness = (settings.brightness + 1) % NUM_BRIGHTNESS_LEVELS;
                    set_brightness(settings.brightness);
                    save_settings(&settings);
                    break;
                }
            } else if (event == LONG_PRESS) {
                ESP_LOGI("MAIN", "Long press on pad %d", i);
                // Handle long press actions (e.g., save settings)
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); // Adjust polling rate
    }
}

// Battery monitoring task
void battery_task(void *param) {
    while (1) {
        /*
        int battery_level = get_battery_level(); // Implement this function
        if (battery_level < 20) {
            // Low battery warning
            ESP_LOGW("BATTERY", "Battery is low: %d%%", battery_level);
        }
        */
        vTaskDelay(5000 / portTICK_PERIOD_MS); // Check battery every 5 seconds
    }
}

void app_main() {
    // Initialize components
    init_leds();
    init_touch();
    init_storage();
    load_settings(&settings);

    set_pattern(settings.pattern_id);
    set_brightness(settings.brightness);

    // Create tasks
    xTaskCreate(lighting_task, "Lighting Task", 2048, NULL, 5, NULL);
    xTaskCreate(touch_task, "Touch Task", 2048, NULL, 5, NULL);
    xTaskCreate(battery_task, "Battery Task", 2048, NULL, 3, NULL);
}