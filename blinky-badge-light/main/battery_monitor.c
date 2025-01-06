#include "battery_monitor.h"
#include "pins.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BATTERY_MONITOR";

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;

volatile bool limit_brightness = false;
volatile bool force_safety_pattern = false;

void init_battery_monitor() {
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
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_cfg, &cali_handle));

    ESP_LOGI(TAG, "Battery monitor initialized");
}

float get_battery_voltage() {
    int raw_adc = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_adc));

    int voltage_mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw_adc, &voltage_mv));

    float battery_voltage = (voltage_mv / 1000.0) * VOLTAGE_DIVIDER_RATIO;
    ESP_LOGI(TAG, "Battery voltage: %.2fV", battery_voltage);

    return battery_voltage;
}

void battery_monitor_task(void *param) {
    while (1) {
        float battery_voltage = get_battery_voltage();

        if (battery_voltage > BRIGHT_THRESH / 1000.0) {
            ESP_LOGI(TAG, "Battery is normal: %.2fV", battery_voltage);
            limit_brightness = false;
        } else if (battery_voltage > SAFETY_THRESH / 1000.0) {
            ESP_LOGW(TAG, "Battery low: %.2fV. Limiting brightness.", battery_voltage);
            limit_brightness = true;
        } else if (battery_voltage > SHIPMODE_THRESH / 1000.0) {
            ESP_LOGE(TAG, "Battery critically low: %.2fV. Entering safety mode.", battery_voltage);
            limit_brightness = true;
            force_safety_pattern = true;
        } else {
            ESP_LOGE(TAG, "Battery extremely low: %.2fV. Goodbye, cruel world!!!", battery_voltage);
            limit_brightness = true;
            force_safety_pattern = true;
            //enter_ship_mode();
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 5 seconds
    }
}


void cleanup_battery_monitor() {
    adc_cali_delete_scheme_line_fitting(cali_handle);
    adc_oneshot_del_unit(adc_handle);
    ESP_LOGI(TAG, "Battery monitor resources cleaned up");
}
