#include <math.h>
#include <inttypes.h>

#include "driver/i2s_std.h"  // Use the new I2S API
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pins.h"
#include "microphone.h"

static const char *TAG = "MICROPHONE";
#define BUFFER_SIZE 1024
#define RX_SAMPLE_SIZE (I2S_DATA_BIT_WIDTH_32BIT * BUFFER_SIZE)

volatile bool mic_active = false; // Initialize the microphone flag
static TaskHandle_t microphone_task_handle = NULL; // Task handle for the microphone task
i2s_chan_handle_t rx_handle;  // I2S receive channel handle
char i2s_rx_buff[RX_SAMPLE_SIZE];
size_t bytes_read;

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

    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0, // Using I2S port 0
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6, // DMA descriptors
        .dma_frame_num = 240, // Number of frames per DMA buffer
        .auto_clear = true, // Auto-clear RX buffer on overflow
    };

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg =  {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT, // Match microphone output
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT, // Ensure alignment
            .slot_mode = I2S_SLOT_MODE_MONO,            // Use mono mode
            .slot_mask = I2S_STD_SLOT_LEFT, // The L/R pin of MIC is pulled down, which selects left slot
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,  
            .bclk = I2S_SCK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_DI_PIN,
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
    char *buf_ptr_read = i2s_rx_buff;


    if (i2s_channel_read(rx_handle, i2s_rx_buff, RX_SAMPLE_SIZE, &bytes_read, 1000) == ESP_OK) {
        ESP_LOGI(TAG, "bytes read: %d; raw buf size: %d", bytes_read, sizeof(i2s_rx_buff) / sizeof(i2s_rx_buff[0]));
        int raw_samples_read = bytes_read / 2 / (I2S_DATA_BIT_WIDTH_32BIT / 8);
        ESP_LOGI(TAG, "raw samples read: %d", raw_samples_read);

        double sum_squares = 0.0; // To calculate RMS

        for (int i = 0; i < raw_samples_read; i++)
        {
            // let's inspect our input once in a while
            if (i % 10000 == 0) { // the first sample of every new buffer
                ESP_LOGI(TAG, "rx packet looks like %04x %04x %04x %04x", i2s_rx_buff[i], i2s_rx_buff[i+1], i2s_rx_buff[i+2], i2s_rx_buff[i+3]);
            }
            // Convert the 32-bit sample to a signed 16-bit integer
            int16_t sample = (int16_t)(i2s_rx_buff[i] & 0xFFFF);

            // Normalize the sample to the range [-1.0, 1.0]
            double normalized_sample = (double)sample / 32768.0;

            // Accumulate the square of the normalized sample
            sum_squares += normalized_sample * normalized_sample;
        }

        // Compute RMS value
        double rms = sqrt(sum_squares / raw_samples_read);

        // Avoid log(0) by setting a small minimum value for RMS
        if (rms < 1e-9) {
            rms = 1e-9;
        }

        // Convert RMS to dB
        float db_level = 20.0f * log10f(rms);

        ESP_LOGI(TAG, "RMS: %.8f, dB Level: %.2f", rms, db_level);

        return db_level;

    } else {
        ESP_LOGI(TAG, "Read Failed!");
    }

    return 0;
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