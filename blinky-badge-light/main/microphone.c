#include <math.h>
#include <inttypes.h>

#include "driver/i2s_std.h"  // Use the new I2S API
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pins.h"
#include "microphone.h"

static const char *TAG = "MICROPHONE";

volatile bool mic_active = false; // Initialize the microphone flag
static TaskHandle_t microphone_task_handle = NULL; // Task handle for the microphone task
i2s_chan_handle_t rx_handle;  // I2S receive channel handle

void set_mic_active(bool active) {
    mic_active = active;
    if (active) {
        // Resume microphone task
        if (microphone_task_handle != NULL) {
            vTaskResume(microphone_task_handle);
        }
    } else {
        // Suspend microphone task
        if (microphone_task_handle != NULL) {
            vTaskSuspend(microphone_task_handle);
        }
    }
}

void init_microphone() {
    esp_err_t err;

    // I2S configuration
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0, // Using I2S port 0
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6, // DMA descriptors
        .dma_frame_num = 240, // Number of frames per DMA buffer
        .auto_clear = true, // Auto-clear RX buffer on overflow
    };

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT, // Match microphone output
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT, // Ensure alignment
            .slot_mode = I2S_SLOT_MODE_MONO,            // Use mono mode
            .slot_mask = I2S_STD_SLOT_LEFT, // The L/R pin of MIC is pulled down, which selects left slot
        },
        .gpio_cfg = {
            .mclk = -1,  // Define in pins.h
            .bclk = I2S_SCK_PIN,
            .ws = I2S_WS_PIN,
            .dout = -1,
            .din = I2S_DI_PIN
        },
    };

    // Create the I2S channel
    err = i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(err));
        return;
    }

    // Initialize the I2S receive channel
    err = i2s_channel_init_std_mode(rx_handle, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S standard mode: %s", esp_err_to_name(err));
        return;
    }

    // Enable the I2S channel
    err = i2s_channel_enable(rx_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Microphone initialized successfully");
}

float get_sound_level() {
    if (!rx_handle) {
        ESP_LOGE("MICROPHONE", "RX handle is null");
        return -1.0f; // Return error value
    }

    int32_t samples[SAMPLE_BUFFER_SIZE];
    size_t bytes_read = 0;

    // Read samples from the I2S microphone
    esp_err_t ret = i2s_channel_read(rx_handle, samples, sizeof(samples), &bytes_read, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE("MICROPHONE", "Failed to read I2S data: %s", esp_err_to_name(ret));
        return -1.0f; // Return error value
    }

    if (bytes_read == 0) {
        ESP_LOGW("MICROPHONE", "No data read from I2S");
        return -1.0f; // Return error value
    }

    size_t num_samples = bytes_read / sizeof(int32_t);

    // Debugging: Print some of the raw samples
    ESP_LOGI("MICROPHONE", "First 5 samples: %" PRId32 ", %" PRId32 ", %" PRId32 ", %" PRId32 ", %" PRId32,
         samples[0], samples[1], samples[2], samples[3], samples[4]);


    // Calculate RMS (Root Mean Square)
    double sum_squares = 0.0;
    for (size_t i = 0; i < num_samples; i++) {
        double sample = (double)samples[i] / (1 << 31); // Normalize to -1.0 to 1.0 range
        sum_squares += sample * sample;
    }

    float rms = sqrtf(sum_squares / num_samples);

    // Clamp RMS to avoid log10(0)
    if (rms < 1e-9) {
        ESP_LOGW("MICROPHONE", "RMS too low to calculate dB");
        rms = 1e-9; // Prevent division by zero or negative values
    }

    // Convert RMS to dB
    float db_level = 20.0f * log10f(rms);

    ESP_LOGI("MICROPHONE", "RMS: %.8f, dB Level: %.2f", rms, db_level);

    return db_level;
}


void microphone_task(void *param) {
    while (1) {
        //if (mic_active) {
            float sound_level = get_sound_level();
            //ESP_LOGI(TAG, "Sound level: %.2f dB", sound_level);
        //}
        vTaskDelay(pdMS_TO_TICKS(200)); // Adjust the delay as needed
    }
}