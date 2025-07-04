#include "driver/touch_pad.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pins.h"
#include "touch_input.h"

static const char *TAG = "TOUCH_INPUT";

// Configuration
#define TOUCH_THRESHOLD 30000
#define DEBOUNCE_DELAY_MS 50 // Debounce delay in milliseconds
#define LONG_PRESS_THRESHOLD 100 // Long press threshold in milliseconds

// Array to store the press duration for each pad
static int press_durations[NUM_TOUCH_PADS] = {0};

// Array to store the press state for each pad
static bool is_pressed[NUM_TOUCH_PADS] = {false};

// Array to store if a long press has been detected
static bool long_press_detected[NUM_TOUCH_PADS] = {false};

// Array to store if a short press has been detected
static bool short_press_detected[NUM_TOUCH_PADS] = {false};


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
        long_press_detected[i] = false;
        short_press_detected[i] = false;
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

// Get touch event for a specific pad
int get_touch_event(int pad_num) {
    if (pad_num < 0 || pad_num >= NUM_TOUCH_PADS) {
        ESP_LOGI(TAG, "Invalid touch pad number: %d", pad_num);
        return NO_TOUCH;
    }

    uint32_t touch_value = 0;
    touch_pad_filter_read_smooth(touch_pads[pad_num], &touch_value);

    if (touch_value > TOUCH_THRESHOLD) { // Touch detected
        if (!is_pressed[pad_num]) {
            is_pressed[pad_num] = true; // Mark pad as pressed
            press_durations[pad_num] = 0; // Reset press duration
            long_press_detected[pad_num] = false; // Reset long press detection
            short_press_detected[pad_num] = false; // Reset short press detection
        }
        press_durations[pad_num]++;
        if (press_durations[pad_num] > (LONG_PRESS_THRESHOLD / portTICK_PERIOD_MS)) {
            if (!long_press_detected[pad_num]) {
                long_press_detected[pad_num] = true;
                ESP_LOGI(TAG, "Long press detected on pad %d", pad_num);
                return LONG_PRESS;
            }
        } else if (press_durations[pad_num] > (DEBOUNCE_DELAY_MS / portTICK_PERIOD_MS)) {
            if (!short_press_detected[pad_num]) {
                short_press_detected[pad_num] = true;
                ESP_LOGI(TAG, "Short press detected on pad %d", pad_num);
                return SHORT_PRESS;
            }
        }
    } else { // No touch detected
        is_pressed[pad_num] = false; // Reset state for the next touch
        press_durations[pad_num] = 0; // Reset press duration
        short_press_detected[pad_num] = false; // Reset short press detection
    }

    return NO_TOUCH;
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
        // ..not sure if I want this or not but keeping the code around just in case
        // } else {
        //     // Recalibrate if pads are held too long
        //     int wait_time = 0;
        //     while (any_pad_pressed()) {
        //         vTaskDelay(pdMS_TO_TICKS(500));
        //         wait_time += 500;
        //         if (wait_time > 10000) {
        //             ESP_LOGW(TAG, "Pads held for >10s, recalibrating pads.");
        //             for (int i = 0; i < NUM_TOUCH_PADS; i++) {
        //                 touch_pad_config(touch_pads[i]);
        //                 is_pressed[i] = false;
        //                 press_durations[i] = 0;
        //                 long_press_detected[i] = false;
        //                 short_press_detected[i] = false;
        //             }
        //             break;
        //         }
        //     }
        // }

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

            const char *state = "untouched";
            if (is_pressed[i]) {
                state = long_press_detected[i] ? "long" : (short_press_detected[i] ? "short" : "pressed");
            }

            ESP_LOGI(TAG, "Pad %d: RAW=%6lu  FILTER=%6lu  State: %s\n", i, (unsigned long)raw, (unsigned long)filtered, state);
        }
        ESP_LOGI(TAG, "[Threshold] Using: %lu\n", (unsigned long)TOUCH_THRESHOLD);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
