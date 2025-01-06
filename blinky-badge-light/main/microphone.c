#include <math.h>

#include "driver/i2s.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pins.h"
#include "microphone.h"

static const char *TAG = "MICROPHONE";

volatile bool mic_active = false; // Initialize the microphone flag

void set_mic_active(bool active) {
    mic_active = active;
    if (active) {
        // Resume microphone task
        vTaskResume(microphone_task);
    } else {
        // Suspend microphone task
        vTaskSuspend(microphone_task);
    }
}


void init_microphone() {
    ESP_LOGI(TAG, "Initializing microphone");

    // Configure I2S for the microphone
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX, // Master mode, RX only
        .sample_rate = 16000,                  // Sample rate in Hz
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // Mono input
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,  // Clock Pin (BCLK)
        .ws_io_num = I2S_WS_PIN,    // Word Select Pin (LRCLK)
        .data_in_num = I2S_DI_PIN,  // Data Input Pin (DOUT)
        .data_out_num = I2S_PIN_NO_CHANGE // Not used for RX
    };

    // Install and configure the I2S driver
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &pin_config));
    ESP_ERROR_CHECK(i2s_set_clk(I2S_NUM_0, 16000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO));
}


void cleanup_microphone() {
    ESP_LOGI(TAG, "Cleaning up microphone");
    i2s_driver_uninstall(I2S_NUM_0);
}


int16_t read_microphone_sample() {
    int16_t sample = 0;
    size_t bytes_read = 0;

    // Read a single 16-bit sample from the I2S buffer
    i2s_read(I2S_NUM_0, &sample, sizeof(sample), &bytes_read, portMAX_DELAY);

    if (bytes_read > 0) {
        return sample;
    } else {
        ESP_LOGW(TAG, "No microphone data available");
        return 0;
    }
}


float calculate_decibel_level() {
    int16_t sample_buffer[512];
    size_t bytes_read = 0;
    int64_t sum_of_squares = 0;

    // Read multiple samples into the buffer
    i2s_read(I2S_NUM_0, sample_buffer, sizeof(sample_buffer), &bytes_read, portMAX_DELAY);

    int num_samples = bytes_read / sizeof(int16_t);
    if (num_samples == 0) {
        ESP_LOGW(TAG, "No microphone data available");
        return 0.0f;
    }

    // Calculate the RMS (Root Mean Square) value
    for (int i = 0; i < num_samples; i++) {
        sum_of_squares += sample_buffer[i] * sample_buffer[i];
    }

    float rms = sqrtf((float)sum_of_squares / num_samples);

    // Convert RMS to decibels
    float decibels = 20.0f * log10f(rms / INT16_MAX);
    return decibels;
}


void microphone_task(void *param) {
    ESP_LOGI(TAG, "Starting microphone task");

    while (1) {
        if (mic_active) {
            float decibel_level = calculate_decibel_level();

            // Log decibel levels for debugging
            ESP_LOGI(TAG, "Decibel level: %.2f dB", decibel_level);

            //TODO: do something with dB level
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // Adjust polling rate as needed
    }
}