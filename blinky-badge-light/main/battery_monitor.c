#include "battery_monitor.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define LONG_PRESS_THRESHOLD_MS 3000 // 3 seconds
#define BUTTON_INIT_IGNORE_TIME_MS 10000 // 10 seconds - Ignore button presses during this time after boot

static const char *TAG = "BATTERY_MONITOR";

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;
static uint32_t button_press_time = 0;
static uint32_t power_on_time = 0;

volatile bool limit_brightness = false;
volatile bool force_safety_pattern = false;

void turn_off(void) {
    ESP_LOGI(TAG, "Shutting down...");
    gpio_set_level(MOSFET_GATE_PIN, 1);  // Pull MOSFET gate HIGH to cut power

    // Small delay to ensure MOSFET powers down cleanly
    vTaskDelay(pdMS_TO_TICKS(100));

    // Enter infinite loop to ensure ESP32 doesn't continue execution (it'll lose power)
    while (1);
}

// Interrupt handler for the button
void IRAM_ATTR button_isr_handler(void *arg) {
    uint32_t current_time = esp_timer_get_time() / 1000; // Convert to milliseconds

    // Ignore button presses during the ignore window
    if ((current_time - power_on_time) < BUTTON_INIT_IGNORE_TIME_MS) {
        ESP_LOGI(TAG, "Ignoring button press during power-on window.");
        return;
    }

    if (gpio_get_level(BUTTON_PIN) == 0) { // Button pressed
        button_press_time = current_time;
    } else { // Button released
        if (button_press_time != 0 && (current_time - button_press_time) >= LONG_PRESS_THRESHOLD_MS) {
            ESP_LOGI(TAG, "Long press detected, shutting down.");
            turn_off();
        }
        button_press_time = 0; // Reset the press time
    }
}

float get_battery_voltage() {
    int raw_adc = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &raw_adc));

    int voltage_mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw_adc, &voltage_mv));

    float battery_voltage = (voltage_mv / 1000.0) * VOLTAGE_DIVIDER_RATIO;
    ESP_LOGI(TAG, "Battery voltage: %.2fV", battery_voltage);

    return battery_voltage;
}

void init_battery_monitor() {
    // Configure ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &config));

    // Initialize ADC calibration
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle));

    // Configure button and MOSFET gate
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MOSFET_GATE_PIN) | (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT_OUTPUT, // Configure for both input and output
        .pull_up_en = GPIO_PULLUP_ENABLE, // Enable pull-up for button
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_conf);

    // Install ISR service and add ISR handler for the button
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, (void *)BUTTON_PIN);

    ESP_LOGI(TAG, "Battery monitor initialized");
}

void battery_monitor_task(void *param) {
    while (1) {
        float battery_voltage = get_battery_voltage();

        if (battery_voltage > BRIGHT_THRESH / 1000.0) {
            ESP_LOGI(TAG, "Battery is normal: %.2fV", battery_voltage);
            //limit_brightness = false;
        } else if (battery_voltage > SAFETY_THRESH / 1000.0) {
            ESP_LOGW(TAG, "Battery low: %.2fV. Limiting brightness.", battery_voltage);
            //limit_brightness = true;
        } else if (battery_voltage > OFF_THRESH / 1000.0) {
            ESP_LOGE(TAG, "Battery critically low: %.2fV. Entering safety mode.", battery_voltage);
            //limit_brightness = true;
            //force_safety_pattern = true;
        } else {
            ESP_LOGE(TAG, "Battery extremely low: %.2fV. Goodbye, cruel world!!!", battery_voltage);
            //limit_brightness = true;
            //force_safety_pattern = true;
            //turn_off();
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 5 seconds
    }
}