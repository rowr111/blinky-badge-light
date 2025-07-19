#include <math.h>
#include <stdbool.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_utils.h"
#include "led_control.h"
#include "pins.h"
#include "microphone.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"


static const char *TAG = "TESTING";

uint8_t white_brightness = 40; // Brightness for white
uint8_t color_brightness = 100; // Brightness for colors
float threshold_db_level = 40.0f; // Threshold volume for microphone test


uint8_t framebuffer[LED_COUNT * 3];

const char *reset_reason_str(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_UNKNOWN:    return "Unknown";
        case ESP_RST_POWERON:    return "Power-on";
        case ESP_RST_EXT:        return "External pin";
        case ESP_RST_SW:         return "Software restart";
        case ESP_RST_PANIC:      return "Exception/Panic";
        case ESP_RST_INT_WDT:    return "Interrupt Watchdog";
        case ESP_RST_TASK_WDT:   return "Task Watchdog";
        case ESP_RST_WDT:        return "Other Watchdog";
        case ESP_RST_DEEPSLEEP:  return "Wake from Deep Sleep";
        case ESP_RST_BROWNOUT:   return "Brownout (low voltage)";
        case ESP_RST_SDIO:       return "SDIO";
        case ESP_RST_USB:        return "USB";
        case ESP_RST_JTAG:       return "JTAG";
        case ESP_RST_EFUSE:      return "eFuse Error";
        case ESP_RST_PWR_GLITCH: return "Power Glitch";
        case ESP_RST_CPU_LOCKUP: return "CPU Lockup";
        default:                 return "Invalid";
    }
}

void blink_test_warning() {
    for (int blink = 0; blink < 5; blink++) {
        // All on
        for (int i = 0; i < LED_COUNT; i++) {
            set_pixel(framebuffer, i, white_brightness, white_brightness, white_brightness);
        }
        update_leds(framebuffer);
        vTaskDelay(pdMS_TO_TICKS(200));

        // All off
        for (int i = 0; i < LED_COUNT; i++) {
            set_pixel(framebuffer, i, 0, 0, 0);
        }
        update_leds(framebuffer);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


void led_rgb_test() {
    // RED
    for (int i = 0; i < LED_COUNT; i++) {
        set_pixel(framebuffer, i, color_brightness, 0, 0);
    }
    update_leds(framebuffer);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // GREEN
    for (int i = 0; i < LED_COUNT; i++) {
        set_pixel(framebuffer, i, 0, color_brightness, 0);
    }
    update_leds(framebuffer);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // BLUE
    for (int i = 0; i < LED_COUNT; i++) {
        set_pixel(framebuffer, i, 0, 0, color_brightness);
    }
    update_leds(framebuffer);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Turn off all LEDs at end
    for (int i = 0; i < LED_COUNT; i++) {
        set_pixel(framebuffer, i, 0, 0, 0);
    }
    update_leds(framebuffer);
}


void test_microphone() {
    // Test lasts for 2 seconds, checking every 100 ms
    const int test_duration_ms = 2000;
    const int interval_ms = 100;
    const int loops = test_duration_ms / interval_ms;
    bool sound_detected = false;

    ESP_LOGI(TAG, "Starting microphone test... Make a loud sound near the badge!");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1 seconds before starting the test

    for (int i = 0; i < loops; i++) {
        float db = get_sound_level();
        ESP_LOGI(TAG, "Mic Test: Sound Level = %.2f dB", db);

        // Visual feedback: if dB is over threshold, flash green; else, red.
        uint8_t r = 0, g = 0, b = 0;
        if (db > threshold_db_level) {
            g = color_brightness; // Green for detected sound
            sound_detected = true;
        } else {
            r = color_brightness; // Red for quiet
        }
        for (int j = 0; j < LED_COUNT; j++) {
            set_pixel(framebuffer, j, r, g, b);
        }
        update_leds(framebuffer);

        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }
    
    // Turn off all LEDs briefly before final result
    for (int i = 0; i < LED_COUNT; i++) {
        set_pixel(framebuffer, i, 0, 0, 0);
    }
    update_leds(framebuffer);
    vTaskDelay(pdMS_TO_TICKS(500)); 

    // Final result: solid green (pass) or solid red (fail)
    uint8_t r = 0, g = 0, b = 0;
    if (sound_detected) {
        g = color_brightness;
        ESP_LOGI(TAG, "Mic Test: PASSED!");
    } else {
        r = color_brightness;
        ESP_LOGI(TAG, "Mic Test: FAILED. No sound detected.");
    }
    for (int j = 0; j < LED_COUNT; j++) {
        set_pixel(framebuffer, j, r, g, b);
    }
    update_leds(framebuffer);
    vTaskDelay(pdMS_TO_TICKS(3000)); // Hold result for a few sec for the result
}


void storage_self_test() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    bool pass = true;

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS storage test: Failed to open NVS");
        pass = false;
        goto test_end;
    }

    uint32_t test_value = 0x5A5A1234;
    err = nvs_set_u32(nvs_handle, "selftest", test_value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS storage test: Failed to write test value");
        pass = false;
        goto close_and_end;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS storage test: Commit failed");
        pass = false;
        goto close_and_end;
    }

    uint32_t check_value = 0;
    err = nvs_get_u32(nvs_handle, "selftest", &check_value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS storage test: Failed to read back value");
        pass = false;
        goto close_and_end;
    }

    if (check_value != test_value) {
        ESP_LOGE(TAG, "NVS storage test: Readback value mismatch (wrote 0x%08" PRIx32 ", read 0x%08" PRIx32 ")", test_value, check_value);        pass = false;
        goto close_and_end;
    }

    ESP_LOGI(TAG, "NVS storage self-test PASSED!");

close_and_end:
    nvs_close(nvs_handle);

test_end:
    // Flash result: green = pass, red = fail
    uint8_t r = 0, g = 0, b = 0;
    if (pass) {
        g = color_brightness;
    } else {
        r = color_brightness;
    }
    for (int i = 0; i < LED_COUNT; i++) {
        set_pixel(framebuffer, i, r, g, b);
    }
    update_leds(framebuffer);
    vTaskDelay(pdMS_TO_TICKS(3000)); // 3 second

    // Turn off LEDs after
    for (int i = 0; i < LED_COUNT; i++) {
        set_pixel(framebuffer, i, 0, 0, 0);
    }
    update_leds(framebuffer);
}




void testing_routine() {
    ESP_LOGI(TAG, "Starting testing routine...");

    // Test LED strip
    ESP_LOGI(TAG, "Testing LED strip...");
    blink_test_warning();
    led_rgb_test();
    ESP_LOGI(TAG, "LED strip test completed.");
    
    ESP_LOGI(TAG, "Testing microphone...");
    blink_test_warning();
    test_microphone();
    ESP_LOGI(TAG, "Microphone test completed.");

    ESP_LOGI(TAG, "Testing NVS storage...");
    blink_test_warning();
    storage_self_test();
    ESP_LOGI(TAG, "NVS storage test completed.");

    ESP_LOGI(TAG, "Testing routine completed.");
}