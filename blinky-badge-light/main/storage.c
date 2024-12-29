#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "STORAGE";

// Initialize NVS
void init_storage() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");
}

// Save settings to NVS
void save_settings(const badge_settings_t *settings) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle");
        return;
    }

    // Write settings
    err = nvs_set_blob(nvs_handle, "badge_settings", settings, sizeof(badge_settings_t));
    if (err == ESP_OK) {
        ESP_ERROR_CHECK(nvs_commit(nvs_handle));
        ESP_LOGI(TAG, "Settings saved");
    } else {
        ESP_LOGE(TAG, "Failed to save settings");
    }

    nvs_close(nvs_handle);
}

// Load settings from NVS
void load_settings(badge_settings_t *settings) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No settings found, using defaults");
        settings->pattern_id = 0;    // Default pattern
        settings->brightness = 128; // Default brightness
        return;
    }

    // Read settings
    size_t required_size = sizeof(badge_settings_t);
    err = nvs_get_blob(nvs_handle, "badge_settings", settings, &required_size);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Settings loaded");
    } else {
        ESP_LOGW(TAG, "Failed to load settings, using defaults");
        settings->pattern_id = 0;
        settings->brightness = 128;
    }

    nvs_close(nvs_handle);
}