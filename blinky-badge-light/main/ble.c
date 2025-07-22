#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "firework_notification_pattern.h"


static const char *TAG = "BLE";

#define HI_ADV_SERVICE_UUID      0xBEE7 // Unique "hi" notification UUID
static int64_t last_hi_time = 0;  // microseconds since boot
#define HI_NOTIFICATION_COOLDOWN_US (10 * 1000 * 1000)  // 10 sec in microseconds

TaskHandle_t ble_adv_task_handle = NULL;

esp_ble_scan_params_t my_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_PASSIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x50,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};


void my_gap_callback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (event == ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT) {
        // Start scanning forever (0 = continuous)
        esp_ble_gap_start_scanning(0);
        ESP_LOGI(TAG, "Started BLE scan");
        return;
    }

    if (event == ESP_GAP_BLE_SCAN_RESULT_EVT) {
        esp_ble_gap_cb_param_t *scan_result = param;
        if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            // Scan result received! Let's check if our UUID is in the payload
            uint8_t *adv_data = scan_result->scan_rst.ble_adv;
            uint8_t adv_len = scan_result->scan_rst.adv_data_len;

            // Look for: 0x03 (length), 0x03 (type), UUID_L, UUID_H
            for (int i = 0; i < adv_len - 3; i++) {
                if (adv_data[i]   == 0x03 &&
                    adv_data[i+1] == 0x03 &&
                    adv_data[i+2] == (HI_ADV_SERVICE_UUID & 0xFF) &&
                    adv_data[i+3] == ((HI_ADV_SERVICE_UUID >> 8) & 0xFF)) {
                    int64_t now = esp_timer_get_time();
                    if (!show_firework_notification && (now - last_hi_time > HI_NOTIFICATION_COOLDOWN_US)) {
                        last_hi_time = now;
                        ESP_LOGI(TAG, "Received 'hi' BLE notification from nearby badge!");
                        ESP_LOGI(TAG, "Source MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                            scan_result->scan_rst.bda[0], scan_result->scan_rst.bda[1], scan_result->scan_rst.bda[2],
                            scan_result->scan_rst.bda[3], scan_result->scan_rst.bda[4], scan_result->scan_rst.bda[5]);
                        show_firework_notification = true;
                        firework_notification_start_time = esp_timer_get_time() / 1000; // convert to ms
                    }
                    break;
                }
            }
        }
    }
}




void ble_init(void) {
    // Release memory for BT Classic if not used
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    esp_err_t ret;

    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "Bluetooth controller init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid stack init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return;
    }

    // Register GAP callback (required for scan/advertise)
    ret = esp_ble_gap_register_callback(my_gap_callback);
    if (ret) {
        ESP_LOGE(TAG, "GAP callback register failed: %s", esp_err_to_name(ret));
        return;
    }

    esp_ble_gap_set_scan_params(&my_scan_params);

    ESP_LOGI(TAG, "BLE stack initialized!");
}

void ble_adv_start(void) {
    // Advertising data: Flags + Complete List of 16-bit Service UUIDs
    uint8_t adv_data[] = {
        0x02, 0x01, 0x06,                  // Flags: general discoverable mode, BR/EDR not supported
        0x03, 0x03,                        // Length=3, Type=Complete List of 16-bit UUIDs
        (HI_ADV_SERVICE_UUID & 0xFF),      // UUID low byte
        (HI_ADV_SERVICE_UUID >> 8) & 0xFF  // UUID high byte
        // could add more fields (like a device ID) here!
    };

    esp_ble_gap_config_adv_data_raw(adv_data, sizeof(adv_data));
    esp_ble_gap_start_advertising(&(esp_ble_adv_params_t) {
        .adv_int_min = 0x20,
        .adv_int_max = 0x40,
        .adv_type = ADV_TYPE_NONCONN_IND,     // Non-connectable
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    });
}


void ble_advertise_task(void *param) {
    while (1) {
        // Wait for a notification from button task or event
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until notified

        // Start advertising for 2 seconds
        ESP_LOGI(TAG, "Started advertising 'hi' notification!");
        ble_adv_start();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        esp_ble_gap_stop_advertising();
    }
}
