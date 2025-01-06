#include "driver/touch_pad.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pins.h"
#include "touch_input.h"

static const char *TAG = "TOUCH_INPUT";

// Configuration
#define TOUCH_THRESHOLD 300 // Adjust based on your hardware
#define DEBOUNCE_DELAY_MS 50 // Debounce delay in milliseconds
#define LONG_PRESS_THRESHOLD 100 // Long press threshold in milliseconds
#define TOUCH_THRESH_NO_USE   (0)

// Array to store the press duration for each pad
static int press_durations[NUM_TOUCH_PADS] = {0};

// Array to store the press state for each pad
static bool is_pressed[NUM_TOUCH_PADS] = {false};

// Array to store if a long press has been detected
static bool long_press_detected[NUM_TOUCH_PADS] = {false};

// Array to store if a short press has been detected
static bool short_press_detected[NUM_TOUCH_PADS] = {false};


// Array of touch pad numbers
static const touch_pad_t touch_pads[NUM_TOUCH_PADS] = {
    TOUCH_PAD_1,
    TOUCH_PAD_2,
    TOUCH_PAD_3,
};

// Initialize touch input
void init_touch() {
    ESP_LOGI(TAG, "Initializing touch input");
    ESP_ERROR_CHECK(touch_pad_init());

    // Set reference voltage for charging/discharging
    // In this case, the high reference valtage will be 2.7V - 1V = 1.7V
    // The low reference voltage will be 0.5
    // The larger the range, the larger the pulse count value.
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);

    for (int i = 0; i < NUM_TOUCH_PADS; i++) {
        touch_pad_config(touch_pads[i], TOUCH_THRESH_NO_USE);
    }

    //ESP_ERROR_CHECK(touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER)); // Enable periodic sampling
}

// Get touch event for a specific pad
int get_touch_event(int pad_num) {
    if (pad_num < 0 || pad_num >= NUM_TOUCH_PADS) {
        ESP_LOGI(TAG, "Invalid touch pad number: %d", pad_num);
        return NO_TOUCH;
    }

    uint16_t touch_value = 0;
    touch_pad_read(touch_pads[pad_num], &touch_value);

    if (touch_value < TOUCH_THRESHOLD) { // Touch detected
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