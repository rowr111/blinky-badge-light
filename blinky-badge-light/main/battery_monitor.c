#include "battery_monitor.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BATTERY_MONITOR";

// Initialize the ADC for battery monitoring
void init_battery_monitor(void) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_11);
}

// Battery monitoring task
void battery_monitor_task(void *param) {
    while (1) {
        // Read raw ADC value
        int adc_raw = adc1_get_raw(ADC_CHANNEL);

        // Convert raw ADC value to voltage
        float adc_voltage = (adc_raw * 3.3) / 4095; // Scale ADC value
        float battery_voltage = adc_voltage * VOLTAGE_DIVIDER_RATIO; // Adjust for divider

        // Log the battery voltage
        ESP_LOGI(TAG, "Battery Voltage: %.2f V", battery_voltage);

        // Add delay for periodic updates (5 seconds)
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
