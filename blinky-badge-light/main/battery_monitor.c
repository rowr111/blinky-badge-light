#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "battery_monitor.h"
#include "pins.h"

static const char *TAG = "BATTERY_MONITOR";

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;

volatile bool limit_brightness = false;
volatile bool force_safety_pattern = false;
volatile uint16_t current_battery_voltage = 0;

uint16_t get_battery_voltage() {
    // Enable battery monitor n-MOSFET
    gpio_set_level(BATTERY_MONITOR_ENABLE_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    int raw_adc = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_adc));

    int voltage_mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw_adc, &voltage_mv));

    // Disable battery monitor n-MOSFET
    gpio_set_level(BATTERY_MONITOR_ENABLE_PIN, 0);

    // Calculate actual battery voltage
    uint16_t battery_voltage = (uint16_t)(voltage_mv * VOLTAGE_DIVIDER_RATIO);
    vTaskDelay(pdMS_TO_TICKS(10));

    return battery_voltage;
}


void turn_off() {
    ESP_LOGI(TAG, "Shutting down...");

    // Turn off battery monitor
    gpio_set_level(BATTERY_MONITOR_ENABLE_PIN, 0);

    // Turn off power
    gpio_set_level(MOSFET_GATE_PIN, 1);  // Pull MOSFET gate HIGH to cut power

    // Small delay to ensure MOSFET powers down cleanly
    vTaskDelay(pdMS_TO_TICKS(100));

    // Enter infinite loop to ensure ESP32 doesn't continue execution (it'll lose power)
    while (1);
}


void init_battery_monitor() {
    // Set MOSFET gate LOW to keep power on
    gpio_set_direction(MOSFET_GATE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(MOSFET_GATE_PIN, 0);

    // Set battery monitor n-MOSFET OFF by default
    gpio_set_direction(BATTERY_MONITOR_ENABLE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BATTERY_MONITOR_ENABLE_PIN, 0); 

    // Initialize ADC for oneshot mode
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    // Configure ADC channel
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

    // Set up ADC calibration
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle));

    // Check battery voltage
    uint16_t battery_voltage = get_battery_voltage();
    ESP_LOGI(TAG, "initial battery voltage: %d mV.", battery_voltage); // Somehow, writing this line causes the battery_monitor_task to not get a low voltage the first time it runs.. a mystery.
    ESP_LOGI(TAG, "Battery monitor initialized");
}

static int off_thresh_count = 0;

void battery_monitor_task(void *param) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000)); // Check every 30 seconds

        current_battery_voltage = get_battery_voltage();

        // --- Brightness limiting buffer zone ---
        if (limit_brightness) {
            if (current_battery_voltage > RECOVERY_THRESH) {
                limit_brightness = false; // Yay! Battery voltage is back to normal
                ESP_LOGI(TAG, "Battery recovered: %d mV. Returning to normal brightness.", current_battery_voltage);
            }  else {
                ESP_LOGW(TAG, "Battery low: %d mV. Limiting brightness.", current_battery_voltage);
            }
        } else {
            if (current_battery_voltage < BRIGHT_THRESH) {
                limit_brightness = true; // Limit brightness to extend battery life
                ESP_LOGW(TAG, "Battery low: %d mV. Limiting brightness.", current_battery_voltage);
            }
            else if (current_battery_voltage > BRIGHT_THRESH) {
                limit_brightness = false; // Just in case..
                ESP_LOGI(TAG, "Battery is normal: %d mV", current_battery_voltage);
            }
        }


        // --- Safety mode buffer zone ---
        if (force_safety_pattern) {
            if (current_battery_voltage > SAFETY_RECOVERY_THRESH) {
                force_safety_pattern = false;
                ESP_LOGI(TAG, "Battery recovered: %d mV. Exiting safety mode.", current_battery_voltage);
            } else {
                ESP_LOGE(TAG, "Battery critically low: %d mV. In safety mode.", current_battery_voltage);
            }
        } else {
            if (current_battery_voltage < SAFETY_THRESH) {
                force_safety_pattern = true; // Force safety pattern
                ESP_LOGE(TAG, "Battery critically low: %d mV. Entering safety mode.", current_battery_voltage);
            }
        }

        // --- Off threshold - off after 3 checks of passing the threshold! ---
        if (current_battery_voltage < OFF_THRESH) {
            off_thresh_count++;
            ESP_LOGE(TAG, "Battery extremely low: %d mV. OFF threshold count: %d", current_battery_voltage, off_thresh_count);
            if (off_thresh_count >= 3) {
                ESP_LOGE(TAG, "Battery extremely low: %d mV. Shutting down.", current_battery_voltage);
                limit_brightness = true;
                force_safety_pattern = true;
                turn_off();
            }
        } else {
            off_thresh_count = 0; // Reset count if voltage recovers
        }

    }
}
