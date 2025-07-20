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

//variables for microphone
static i2s_chan_handle_t rx_chan; 
#define SAMPLE_BUFF_SIZE 2048

// variables for sound level
#define DB_HISTORY_LEN 100
volatile float current_dB_level = 0.0f;
static float dbHistory[DB_HISTORY_LEN];
static int dbHistoryIdx = 0;
volatile float dB_brightness_level = 0.0f; 
volatile float smooth_dB_brightness_level = 0.0f;
float avg_low_db = 30.0f;
float avg_high_db = 150.0f;


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


// Returns a smoothed brightness level [0.0, 1.0]
void calculate_sound_brightness(void)
{
    // clamp dB range
    if (avg_high_db == avg_low_db) {
        // Avoid divide by zero
        dB_brightness_level = 0.2f;
        return;
    }

    if (current_dB_level > avg_high_db) current_dB_level = avg_high_db;
    if (current_dB_level < avg_low_db) current_dB_level = avg_low_db;

    // Normalize to [0, 1]
    float level = (current_dB_level - avg_low_db) / (avg_high_db - avg_low_db);
    if (level < 0.01f) level = 0.01f;

    // rolling average
    static float avgs[2] = {0, 0};
    static int idx = 0;
    avgs[idx++ % 2] = level;
    dB_brightness_level = (avgs[0] + avgs[1]) / 2.0f;
    
    // make it more uniform
    smooth_dB_brightness_level = powf(dB_brightness_level, 1.4f);
    //printf("Raw dB Brightness Level: %.2f\n", smooth_dB_brightness_level);

    // Clamp to [0.05, 0.8] for brightness so it won't be completely dark or too bright
    float min_bright = 0.05f;
    float max_bright = 0.8f;
    dB_brightness_level = min_bright + (max_bright - min_bright) * dB_brightness_level;
    //printf("dB Brightness Level: %.2f\n", dB_brightness_level);
}



static void dbHistory_add(float val) {
    dbHistory[dbHistoryIdx] = val;
    dbHistoryIdx = (dbHistoryIdx + 1) % DB_HISTORY_LEN;
}


static void db_get_low_high(void) {
    int i, j;
    int sample_count = 5;
    float lowest[sample_count];
    float highest[sample_count];
    for (i = 0; i < sample_count; i++) {
        lowest[i] = 150.0f; // Initialize with a high value
    }
    for (i = 0; i < sample_count; i++) {
        highest[i] = 30.0f; // Initialize with a low value
    }

    for (i = 0; i < DB_HISTORY_LEN; i++) {
        float val = dbHistory[i];
        // Find lowest
        for (j = 0; j < sample_count; j++) {
            if (val < lowest[j]) {
                float tmp = lowest[j];
                lowest[j] = val;
                val = tmp;
            }
        }
        // Find highest
        val = dbHistory[i];
        for (j = 0; j < sample_count; j++) {
            if (val > highest[j]) {
                float tmp = highest[j];
                highest[j] = val;
                val = tmp;
            }
        }
    }
    // Compute averages
    float sum_low = 0, sum_high = 0;
    for (i = 0; i < sample_count; i++) {
        sum_low += lowest[i];
        sum_high += highest[i];
    }
    avg_low_db = sum_low / sample_count;
    avg_high_db = sum_high / sample_count;
}



void microphone_task(void *param) {
    while (1) {
        current_dB_level = get_sound_level();
        dbHistory_add(current_dB_level); 
        db_get_low_high(); // Update low and high averages
        calculate_sound_brightness(); // Update dB brightness level
        //ESP_LOGI(TAG, "Sound Level: %.2f dB", current_dB_level);
        vTaskDelay(pdMS_TO_TICKS(20));
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
