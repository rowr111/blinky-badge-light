#include "led_control.h"
#include "touch_input.h"
#include "storage.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main() {
    init_storage();
    init_leds();
    init_touch();

    badge_settings_t settings;
    load_settings(&settings);

    set_pattern(settings.pattern_id); // Restore saved pattern
    set_brightness(settings.brightness); // Restore saved brightness

    while (1) {
        for (int i = 0; i < NUM_TOUCH_PADS; i++) {
            int event = get_touch_event(i);
            if (event == SHORT_PRESS) {
                ESP_LOGI("MAIN", "Short press on pad %d", i);
                switch (i) {
                    case 0:
                        // Pad 1: Cycle patterns
                        settings.pattern_id = (settings.pattern_id + 1) % NUM_PATTERNS;
                        set_pattern(settings.pattern_id);
                        // Save the updated settings
                        save_settings(&settings);
                    case 1:
                        // Pad 2: Adjust brightness
                        settings.brightness = (settings.brightness + 1) % NUM_BRIGHTNESS_LEVELS;
                        set_brightness(settings.brightness);
                        // Save the updated settings
                        save_settings(&settings);
                    default:
                        // Handle other pads if needed
                        break;
                }
            } else if (event == LONG_PRESS) {
                ESP_LOGI("MAIN", "Long press on pad %d", i);
                // Add long press actions if needed
            }
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}