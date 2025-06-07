#include <math.h>
#include <stdint.h>
#include <string.h>
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pins.h"
#include "microphone.h"

#define TAG "MICROPHONE"
#define BUFFER_SIZE 1024
#define SAMPLE_SIZE_BYTES 4 // 32 bits
#define RX_BUFFER_BYTES (BUFFER_SIZE * SAMPLE_SIZE_BYTES)

volatile bool mic_initialized = false;
static i2s_chan_handle_t rx_handle = NULL;
static uint8_t i2s_rx_buff[RX_BUFFER_BYTES];

void init_microphone(void) {
    mic_initialized = false;
    rx_handle = NULL;

    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6,
        .dma_frame_num = 240,
        .auto_clear = true,
    };

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_LEFT,
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_SCK_PIN,
            .ws   = I2S_WS_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S_DI_PIN,
        },
    };

    esp_err_t err = i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    if (err != ESP_OK || rx_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(err));
        return;
    }

    err = i2s_channel_init_std_mode(rx_handle, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S: %s", esp_err_to_name(err));
        i2s_del_channel(rx_handle);
        rx_handle = NULL;
        return;
    }

    err = i2s_channel_enable(rx_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S: %s", esp_err_to_name(err));
        i2s_del_channel(rx_handle);
        rx_handle = NULL;
        return;
    }

    mic_initialized = true;
    ESP_LOGI(TAG, "Microphone initialized successfully");
}

float get_sound_level(void) {
    if (!mic_initialized || rx_handle == NULL) {
        ESP_LOGW(TAG, "Microphone/I2S not initialized");
        return -180.0f;
    }

    size_t bytes_read = 0;
    esp_err_t err = i2s_channel_read(rx_handle, i2s_rx_buff, RX_BUFFER_BYTES, &bytes_read, 1000);
    if (err != ESP_OK || bytes_read == 0) {
        ESP_LOGE(TAG, "I2S read failed: %s", esp_err_to_name(err));
        return -180.0f;
    }

    size_t sample_count = bytes_read / SAMPLE_SIZE_BYTES;
    if (sample_count == 0) {
      ESP_LOGI(TAG, "No samples read from I2S");
      return -180.0f;
    }

    double sum_squares = 0.0;
    for (size_t i = 0; i < sample_count; i++) {
        int32_t sample = 0;
        memcpy(&sample, &i2s_rx_buff[i * SAMPLE_SIZE_BYTES], SAMPLE_SIZE_BYTES);
        double normalized = (double)sample / 2147483648.0; // 32-bit signed
        sum_squares += normalized * normalized;
    }
    double rms = sqrt(sum_squares / sample_count);
    if (rms < 1e-9) rms = 1e-9;
    float db_level = 20.0f * log10f(rms);
    return db_level;
}

void microphone_task(void *param) {
    while (1) {
        float sound_level = get_sound_level();
        ESP_LOGI(TAG, "Sound Level: %.2f dB", sound_level);

        vTaskDelay(pdMS_TO_TICKS(400));
    }
}
