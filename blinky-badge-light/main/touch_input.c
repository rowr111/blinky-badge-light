#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/touch_sens.h"

#include "touch_input.h"
#include "led_control.h"
#include "storage.h"
#include "genes.h"
#include "battery_level_pattern.h"
#include "battery_monitor.h"
#include "now.h"
#include "firework_notification_pattern.h"
#include "testing_routine.h"

#define NUM_TOUCH_PADS 6
static const char *TAG = "TOUCH_INPUT";
#define OFF_PAD_IDX 3

static bool is_pressed[NUM_TOUCH_PADS] = {false};
static touch_sensor_handle_t touch_handle = NULL;
static touch_channel_handle_t chan_handles[NUM_TOUCH_PADS];

static const int pad_ids[NUM_TOUCH_PADS] = {
    5, // Next pattern touchpad
    6, // Brightness level touchpad
    3, // Replace pattern touchpad
    4, // OFF touchpad
    7, // Battery check touchpad
    8  // ? spot notification touchpad
};

static TimerHandle_t off_hold_timer = NULL;
static const int OFF_HOLD_TIME_MS = 500; // 500 ms

static TimerHandle_t combo_hold_timer = NULL;
static const int COMBO_HOLD_TIME_MS = 2000; // 2000 ms

static const float TOUCH_THRESH = 0.02f;
static float thresh2bm_ratio[NUM_TOUCH_PADS] = {TOUCH_THRESH,TOUCH_THRESH,TOUCH_THRESH,TOUCH_THRESH,TOUCH_THRESH,TOUCH_THRESH};

#define TOUCH_SAMPLE_CFG_DEFAULT() ((touch_sensor_sample_config_t[]){TOUCH_SENSOR_V2_DEFAULT_SAMPLE_CONFIG(500, TOUCH_VOLT_LIM_L_0V5, TOUCH_VOLT_LIM_H_2V2)})
#define TOUCH_CHAN_CFG_DEFAULT() ((touch_channel_config_t){ \
    .active_thresh = {2000}, \
    .charge_speed = TOUCH_CHARGE_SPEED_7, \
    .init_charge_volt = TOUCH_INIT_CHARGE_VOLT_DEFAULT, \
})

touch_sensor_sample_config_t sample_cfg[TOUCH_SAMPLE_CFG_NUM] = TOUCH_SAMPLE_CFG_DEFAULT();
touch_channel_config_t chan_cfg = TOUCH_CHAN_CFG_DEFAULT();


// Action queue for pad press events
static QueueHandle_t touch_action_queue = NULL;

void handle_touch_action(int pad) {
    switch (pad) {
        case 0: settings.pattern_id = (settings.pattern_id + 1) % NUM_PATTERNS;
                set_pattern(settings.pattern_id);
                save_settings(&settings);
                break;
        case 1: settings.brightness = (settings.brightness + 1) % NUM_BRIGHTNESS_LEVELS;
                set_brightness(settings.brightness);
                save_settings(&settings);
                break;
        case 2: generate_gene(&patterns[settings.pattern_id]);
                save_genomes_to_storage();
                flash_feedback_pattern();
                break;
        case 3: turn_off(); break;
        case 4: show_battery_meter = true;
                battery_meter_start_time = esp_timer_get_time() / 1000;
                break;
        case 5: now_send_firework(); break;
    }
}

static int find_pad_idx(int chan_id) {
    for (int i = 0; i < NUM_TOUCH_PADS; i++) {
        if (chan_id == pad_ids[i]) return i;
    }
    return -1;
}


static void off_hold_timer_callback(TimerHandle_t xTimer) {
    // Trigger the OFF action
    int pad_idx = OFF_PAD_IDX;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(touch_action_queue, &pad_idx, &xHigherPriorityTaskWoken);
}


static void combo_hold_timer_callback(TimerHandle_t xTimer) {
    show_testing_routine = true;
    xTaskCreatePinnedToCore((TaskFunction_t)testing_routine, "TestingRoutine", 4096, NULL, 5, NULL, 1);
}

static bool touch_on_active_callback(touch_sensor_handle_t sens_handle, const touch_active_event_data_t *event, void *user_ctx)
{
    int pad_idx = find_pad_idx(event->chan_id);
    if (pad_idx >= 0) {
        is_pressed[pad_idx] = true;
        if (pad_idx == OFF_PAD_IDX) {
            if (off_hold_timer == NULL) {
                off_hold_timer = xTimerCreate("OffHold", pdMS_TO_TICKS(OFF_HOLD_TIME_MS), pdFALSE, NULL, off_hold_timer_callback);
            }
            xTimerStart(off_hold_timer, 0); // Start or restart the timer for 1 second
            return false; // Don't queue yet
        }

        // if all three pads are pressed, start the combo hold timer for the testing routine
        if (is_pressed[0] && is_pressed[1] && is_pressed[2]) {
            if (combo_hold_timer == NULL) {
                combo_hold_timer = xTimerCreate("ComboHold", pdMS_TO_TICKS(COMBO_HOLD_TIME_MS), pdFALSE, NULL, combo_hold_timer_callback);
            }
            xTimerStart(combo_hold_timer, 0); // Start or restart the timer
        } else {
            if (combo_hold_timer != NULL) {
                xTimerStop(combo_hold_timer, 0);
            }
        }
        
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(touch_action_queue, &pad_idx, &xHigherPriorityTaskWoken);
        return xHigherPriorityTaskWoken == pdTRUE;
    }
    return false;
}

static bool touch_on_inactive_callback(touch_sensor_handle_t sens_handle, const touch_inactive_event_data_t *event, void *user_ctx)
{
    int pad_idx = find_pad_idx(event->chan_id);
    if (pad_idx >= 0) {
        is_pressed[pad_idx] = false;
        if (pad_idx == OFF_PAD_IDX) {
            if (off_hold_timer != NULL) {
                xTimerStop(off_hold_timer, 0);
            }
        }

        // If any of pads 0, 1, or 2 is released and the testing routine is not showing, stop the combo hold timer
        if ((pad_idx == 0 || pad_idx == 1 || pad_idx == 2) && !show_testing_routine){
            if (combo_hold_timer != NULL) {
                xTimerStop(combo_hold_timer, 0);
            }
        }

    }
    return false;
}

static void do_initial_scanning(touch_sensor_handle_t sens_handle)
{
    ESP_ERROR_CHECK(touch_sensor_enable(sens_handle));
    for (int i = 0; i < 3; i++) {
        ESP_ERROR_CHECK(touch_sensor_trigger_oneshot_scanning(sens_handle, 2000));
    }
    ESP_ERROR_CHECK(touch_sensor_disable(sens_handle));
    for (int i = 0; i < NUM_TOUCH_PADS; i++) {
        uint32_t benchmark[TOUCH_SAMPLE_CFG_NUM] = {0};
        ESP_ERROR_CHECK(touch_channel_read_data(chan_handles[i], TOUCH_CHAN_DATA_TYPE_BENCHMARK, benchmark));
        for (int j = 0; j < TOUCH_SAMPLE_CFG_NUM; j++) {
            chan_cfg.active_thresh[j] = (uint32_t)(benchmark[j] * thresh2bm_ratio[i]);
        }
        ESP_ERROR_CHECK(touch_sensor_reconfig_channel(chan_handles[i], &chan_cfg));
    }
}

void init_touch(void)
{
    // Create action queue
    touch_action_queue = xQueueCreate(4, sizeof(int));
    // 1. Sensor config
    touch_sensor_config_t sens_cfg = TOUCH_SENSOR_DEFAULT_BASIC_CONFIG(TOUCH_SAMPLE_CFG_NUM, sample_cfg);
    ESP_ERROR_CHECK(touch_sensor_new_controller(&sens_cfg, &touch_handle));
    // 2. Create and enable each channel
    for (int i = 0; i < NUM_TOUCH_PADS; i++) {
        ESP_ERROR_CHECK(touch_sensor_new_channel(touch_handle, pad_ids[i], &chan_cfg, &chan_handles[i]));
    }
    // 3. Software filter
    touch_sensor_filter_config_t filter_cfg = TOUCH_SENSOR_DEFAULT_FILTER_CONFIG();
    ESP_ERROR_CHECK(touch_sensor_config_filter(touch_handle, &filter_cfg));
    // 4. Initial scan to calibrate thresholds
    do_initial_scanning(touch_handle);
    // 5. Register callbacks
    touch_event_callbacks_t callbacks = {
        .on_active = touch_on_active_callback,
        .on_inactive = touch_on_inactive_callback,
    };
    ESP_ERROR_CHECK(touch_sensor_register_callbacks(touch_handle, &callbacks, NULL));
    // 7. Enable sensor and start scanning
    ESP_ERROR_CHECK(touch_sensor_enable(touch_handle));
    ESP_ERROR_CHECK(touch_sensor_start_continuous_scanning(touch_handle));
    ESP_LOGI(TAG, "Touch pads initialized");
}

bool get_is_touched(int pad_num)
{
    if (pad_num < 0 || pad_num >= NUM_TOUCH_PADS)
        return false;
    return is_pressed[pad_num];
}

void touch_task(void *param)
{
    int pad_idx;
    while (1) {
        if (show_testing_routine) {
            vTaskDelay(20 / portTICK_PERIOD_MS);
            continue;
        }

        // Wait for a pad event from the ISR callback
        if (xQueueReceive(touch_action_queue, &pad_idx, portMAX_DELAY) == pdPASS) {
            // Firework notification is so bright it can cause issues with voltage and affect touch readings
            if (!show_firework_notification) {
                handle_touch_action(pad_idx);
            }
        }
    }
}