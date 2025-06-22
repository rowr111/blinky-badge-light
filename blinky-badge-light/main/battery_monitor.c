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

    int raw_adc = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_adc));

    int voltage_mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw_adc, &voltage_mv));

    // Disable battery monitor n-MOSFET
    gpio_set_level(BATTERY_MONITOR_ENABLE_PIN, 0);

    // Calculate actual battery voltage
    uint16_t battery_voltage = (uint16_t)(voltage_mv * VOLTAGE_DIVIDER_RATIO);

    //ESP_LOGI(TAG, "Battery voltage: %d mV", battery_voltage);

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

    // Check battery voltage and turn off if it's too low
    uint16_t battery_voltage = get_battery_voltage();
    if (battery_voltage < OFF_THRESH) {
        //ESP_LOGW(TAG, "Battery extremely low (%d mV). Goodbye, cruel world!!!", battery_voltage);
        //turn_off();
    }

    ESP_LOGI(TAG, "Battery monitor initialized");
}


void battery_monitor_task(void *param) {
    while (1) {
        int battery_voltage = get_battery_voltage();
        current_battery_voltage = (uint16_t)battery_voltage;

        if (battery_voltage > BRIGHT_THRESH) {
            //ESP_LOGI(TAG, "Battery is normal: %d mV", battery_voltage);
            limit_brightness = false;
        } else if (battery_voltage > SAFETY_THRESH) {
            //ESP_LOGW(TAG, "Battery low: %d mV. Limiting brightness.", battery_voltage);
            //limit_brightness = true;
        } else if (battery_voltage > OFF_THRESH) {
            //ESP_LOGE(TAG, "Battery critically low: %d mV. Entering safety mode.", battery_voltage);
            //limit_brightness = true;
            //force_safety_pattern = true;
        } else {
            //ESP_LOGE(TAG, "Battery extremely low: %d mV. Goodbye, cruel world!!!", battery_voltage);
            //limit_brightness = true;
            //force_safety_pattern = true;
            //turn_off();
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 5 seconds
    }
}
