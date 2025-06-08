#include <math.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pins.h"
#include "microphone.h"

#define TAG "MICROPHONE"

static i2s_chan_handle_t rx_chan; 
#define SAMPLE_BUFF_SIZE 2048

void init_microphone(void) {
    i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &rx_chan));

    i2s_std_config_t rx_std_cfg = {
      .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(44100),
      .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
      .gpio_cfg = {
          .mclk = I2S_GPIO_UNUSED,
          .bclk = I2S_SCK_PIN,
          .ws   = I2S_WS_PIN,
          .dout = I2S_GPIO_UNUSED,
          .din  = I2S_DI_PIN,
          .invert_flags = {
              .mclk_inv = false,
              .bclk_inv = false,
              .ws_inv   = false,
          },
      },
    };
    
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &rx_std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
    ESP_LOGI(TAG, "Microphone initialized successfully");
}

float get_sound_level(void) {
    uint8_t *r_buf = (uint8_t *)calloc(1, SAMPLE_BUFF_SIZE);
    assert(r_buf);
    size_t r_bytes = 0;

    if (i2s_channel_read(rx_chan, r_buf, SAMPLE_BUFF_SIZE, &r_bytes, 1000) == ESP_OK) {
        int32_t *samples = (int32_t *)r_buf;
        int sample_count = r_bytes / sizeof(int32_t);
        double sum_squares = 0.0;

        for (int i = 0; i < sample_count; i+=2) {
            float normalized_to_1 = ((float)(samples[i] << 1)) / ((float)(1 << 31));
            sum_squares += normalized_to_1 * normalized_to_1;
        }
        float rms = sqrtf(sum_squares / sample_count / 2); // Divide by 2 because we are using mono input
        float dbfs = 20.0f * log10f(rms + 1e-8f);
        float dbspl = dbfs + 120.0f;

        free(r_buf);
        return dbspl;
    } else {
        ESP_LOGW(TAG, "Read Task: i2s read failed");
        free(r_buf);
        return 0.0f;
    }
}


void microphone_task(void *param) {
    while (1) {
        float sound_level = get_sound_level();
        ESP_LOGI(TAG, "Sound Level: %.2f dB", sound_level);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

// for debugging
void i2s_matrix_dump_task(void *param) {
    const int NUM_SAMPLES = 6;
    while (1) {
        uint8_t *r_buf = (uint8_t *)calloc(1, NUM_SAMPLES * sizeof(int32_t));
        if (!r_buf) {
            ESP_LOGE(TAG, "Failed to allocate memory for sample dump");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        size_t r_bytes = 0;
        esp_err_t err = i2s_channel_read(rx_chan, r_buf, NUM_SAMPLES * sizeof(int32_t), &r_bytes, 1000);
        if (err == ESP_OK) {
            int32_t *samples = (int32_t *)r_buf;
            for (int i = 0; i < NUM_SAMPLES; i++) {
                printf("%08" PRIx32 "\n", (uint32_t)samples[i]);
            }
            printf("\n");
        } else {
            ESP_LOGE(TAG, "i2s_channel_read failed: %s", esp_err_to_name(err));
        }
        free(r_buf);
        vTaskDelay(pdMS_TO_TICKS(400));
    }
}
