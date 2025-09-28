#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "genes.h"

static const char *TAG = "STORAGE";

#define NAME_NS   "storage"
#define NAME_KEY  "badge_name"
#define NAME_MAX  31

badge_settings_t settings;
genome patterns[NUM_PATTERNS];

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
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Storage error, opening NVS failed: %s", esp_err_to_name(err));
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
        settings->brightness = 1;
        save_settings(settings);
    }

    nvs_close(nvs_handle);
}

void save_genomes_to_storage() {
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));
    ESP_ERROR_CHECK(nvs_set_blob(nvs_handle, "genomes", patterns, sizeof(patterns)));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Genomes saved to storage.");
    
}

void load_genomes_from_storage() {
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));

    size_t required_size = 0;
    esp_err_t err = nvs_get_blob(nvs_handle, "genomes", NULL, &required_size);

    if (err != ESP_OK || required_size != sizeof(patterns)) {
        ESP_LOGW(TAG, "No valid genomes found in storage. Generating new patterns.");
        for (int i = 0; i < NUM_PATTERNS; i++) {
            generate_gene(&patterns[i]);
        }
        save_genomes_to_storage();
    } else {
        required_size = sizeof(patterns); // Ensure correct size for reading
        err = nvs_get_blob(nvs_handle, "genomes", patterns, &required_size);
        if (err == ESP_OK && required_size == sizeof(patterns)) {
            ESP_LOGI(TAG, "Genomes loaded from storage.");
        } else {
            ESP_LOGW(TAG, "Genomes blob corrupted or wrong size. Generating new patterns.");
            for (int i = 0; i < NUM_PATTERNS; i++) {
                generate_gene(&patterns[i]);
            }
            save_genomes_to_storage();
        }
    }

    nvs_close(nvs_handle);
}


esp_err_t storage_get_name(char *out, size_t out_size)
{
    if (!out || out_size == 0) return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NAME_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    // Ask NVS for length first
    size_t needed = 0;
    err = nvs_get_str(h, NAME_KEY, NULL, &needed);
    if (err != ESP_OK) { nvs_close(h); return err; }

    // Clamp to caller buffer
    if (needed > out_size) needed = out_size;

    err = nvs_get_str(h, NAME_KEY, out, &needed);
    nvs_close(h);
    return err;
}


esp_err_t storage_set_name(const char *name)
{
    if (!name) return ESP_ERR_INVALID_ARG;

    // Copy and clamp to NAME_MAX (avoid huge writes)
    char tmp[NAME_MAX + 1];
    size_t n = 0;
    while (name[n] && n < NAME_MAX) { tmp[n] = name[n]; n++; }
    tmp[n] = '\0';

    nvs_handle_t h;
    esp_err_t err = nvs_open(NAME_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_str(h, NAME_KEY, tmp);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err == ESP_OK) {
        ESP_LOGI("STORAGE", "Saved badge_name=\"%s\"", tmp);
    }
    return err;
}
