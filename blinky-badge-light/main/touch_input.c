#include "driver/touch_pad.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "pins.h"
#include "touch_input.h"
#include "led_control.h"
#include "storage.h"
#include "genes.h"
#include "battery_level_pattern.h"
#include "battery_monitor.h"

static const char *TAG = "TOUCH_INPUT";

// Configuration
#define TOUCH_THRESHOLD 30000
#define DEBOUNCE_DELAY_MS 50 // Debounce delay in milliseconds
#define LONG_PRESS_THRESHOLD 100 // Long press threshold in milliseconds

// Array to store the press duration for each pad
static int press_durations[NUM_TOUCH_PADS] = {0};
// Array to store the press state for each pad
static bool is_pressed[NUM_TOUCH_PADS] = {false};


bool any_pad_pressed() {
    for (int i = 0; i < NUM_TOUCH_PADS; i++) {
        uint32_t value = 0;
        touch_pad_filter_read_smooth(touch_pads[i], &value);
        if (value > TOUCH_THRESHOLD) {
            return true; // At least one pad is pressed
        }
    }
    return false; // No pads are pressed
}

// Initialize touch hardware and configure pads helper
void touch_init_and_configure(void) {
    ESP_ERROR_CHECK(touch_pad_init());
    for (int i = 0; i < NUM_TOUCH_PADS; i++) {
        touch_pad_config(touch_pads[i]);
        is_pressed[i] = false;
        press_durations[i] = 0;
    }
    touch_filter_config_t filter_info = {
        .mode = TOUCH_PAD_FILTER_IIR_4,
        .debounce_cnt = 1,
        .noise_thr = 0,
        .jitter_step = 4,
        .smh_lvl = 2,
    };
    ESP_ERROR_CHECK(touch_pad_filter_set_config(&filter_info));
    ESP_ERROR_CHECK(touch_pad_filter_enable());
    ESP_ERROR_CHECK(touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER));
    ESP_ERROR_CHECK(touch_pad_fsm_start());
}


// Initialize touch input
void init_touch() {
    ESP_LOGI(TAG, "Initializing touch input");
    touch_init_and_configure();
    ESP_LOGI(TAG, "Initializing touch input, threshold: %d", TOUCH_THRESHOLD);
}

// Used to check if pad is pressed at startup to trigger test sequence
bool get_is_touched(int pad_num) {
    uint32_t value = 0;
    touch_pad_filter_read_smooth(touch_pads[0], &value);
    if (value > TOUCH_THRESHOLD) {
        return true;
    }
    return false;
}


// Get touch event for a specific pad
bool get_touch_event(int pad_num) {
    if (pad_num < 0 || pad_num >= NUM_TOUCH_PADS) {
        ESP_LOGI(TAG, "Invalid touch pad number: %d", pad_num);
        return false;
    }

    uint32_t touch_value = 0;
    touch_pad_filter_read_smooth(touch_pads[pad_num], &touch_value);

    if (touch_value > TOUCH_THRESHOLD) { // Touch detected
        if (!is_pressed[pad_num]) {
            is_pressed[pad_num] = true;
            press_durations[pad_num] = 0;
        }
        press_durations[pad_num]++;
        if (press_durations[pad_num] > (DEBOUNCE_DELAY_MS / portTICK_PERIOD_MS)) {
            if (press_durations[pad_num] == (DEBOUNCE_DELAY_MS / portTICK_PERIOD_MS) + 1) {
                ESP_LOGI(TAG, "Press detected on pad %d", pad_num);
                return true;
            }
        }
    } else {
        is_pressed[pad_num] = false;
        press_durations[pad_num] = 0;
    }

    return false;
}


void periodic_touch_recalibration_task(void *pvParameter) {
    while (1) {
        bool any_stuck = false;

        // 1. Check for stuck pads (4194303 = max filter value)
        for (int i = 0; i < NUM_TOUCH_PADS; i++) {
            uint32_t filtered = 0;
            touch_pad_filter_read_smooth(touch_pads[i], &filtered);

            if (filtered == 4194303 || filtered > (TOUCH_THRESHOLD * 30)) { // TODO: Adjust threshold multiplier as needed, this is a guess
                any_stuck = true;
                ESP_LOGW(TAG, "Pad %d stuck high (FILTER=%lu), will force full recalibration!", i, (unsigned long)filtered);
                break; // No need to check others; force reset if any are stuck
            }
        }

        if (any_stuck) {
            ESP_LOGW(TAG, "Forcing immediate full touch reset due to stuck pads!");
            touch_pad_filter_disable();
            touch_pad_deinit();
            vTaskDelay(pdMS_TO_TICKS(50));
            touch_init_and_configure();
            ESP_LOGW(TAG, "Touch pads fully reset and recalibrated after stuck pad!");
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Run every 5 seconds
    }
}


void touch_debug_task(void *pvParameter) {
    uint32_t raw, filtered;
    while (1) {
        ESP_LOGI(TAG, "\n[TOUCH DEBUG] ---------------------\n");
        for (int i = 0; i < NUM_TOUCH_PADS; i++) {
            touch_pad_read_raw_data(touch_pads[i], &raw);
            touch_pad_filter_read_smooth(touch_pads[i], &filtered);

            const char *state = is_pressed[i] ? "pressed" : "untouched";

            ESP_LOGI(TAG, "Pad %d: RAW=%6lu  FILTER=%6lu  State: %s\n",
                i, (unsigned long)raw, (unsigned long)filtered, state);
        }
        ESP_LOGI(TAG, "[Threshold] Using: %lu\n", (unsigned long)TOUCH_THRESHOLD);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


static void handle_touch_action(int pad) {
    switch (pad) {
        case 0:
            settings.pattern_id = (settings.pattern_id + 1) % NUM_PATTERNS;
            set_pattern(settings.pattern_id);
            ESP_LOGI(TAG, "Current pattern: %d", settings.pattern_id);
            save_settings(&settings);
            break;
        case 1:
            settings.brightness = (settings.brightness + 1) % NUM_BRIGHTNESS_LEVELS;
            set_brightness(settings.brightness);
            save_settings(&settings);
            break;
        case 2:
            generate_gene(&patterns[settings.pattern_id]);
            save_genomes_to_storage();
            flash_feedback_pattern();
            break;
        case 3:
            turn_off();
            break;
        case 4:
            show_battery_meter = true;
            battery_meter_start_time = esp_timer_get_time() / 1000;
            break;
    }
}


// Touch input task
void touch_task(void *param) {
    while (1) {
        for (int i = 0; i < NUM_TOUCH_PADS; i++) {
            if (get_touch_event(i)) {
                handle_touch_action(i);
                break;
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}