#include "power_button.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "pins.h"
#include "battery_monitor.h"

#define STARTUP_DELAY_MS  3000
#define DEBOUNCE_US     20000
#define SHORT_MIN_MS    50
#define LONG_MS         1500
#define TRIPLE_WINDOW_MS 2000  // time window for 3 short presses

static const char *TAG = "POWER_BUTTON";
static QueueHandle_t s_evt_q = NULL;
static bool s_isr_installed = false;

typedef enum {
    BUTTON_EVENT_SHORT,
    BUTTON_EVENT_LONG
} button_event_t;

static void IRAM_ATTR button_isr(void *arg)
{
    uint32_t gpio = POWER_BUTTON_PIN;
    BaseType_t hpw = pdFALSE;
    xQueueSendFromISR(s_evt_q, &gpio, &hpw);
    if (hpw) portYIELD_FROM_ISR();
}

void power_button_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << POWER_BUTTON_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io);

    if (!s_evt_q) s_evt_q = xQueueCreate(8, sizeof(uint32_t));
    if (!s_isr_installed) {
        gpio_install_isr_service(0);
        s_isr_installed = true;
    }
    gpio_isr_handler_add(POWER_BUTTON_PIN, button_isr, NULL);
}

void power_button_task(void *param)
{
    bool down = false;
    int64_t t_down = 0;
    int64_t last_short_time = 0;
    int short_count = 0;

    vTaskDelay(pdMS_TO_TICKS(STARTUP_DELAY_MS));

    while (1) {
        uint32_t gpio;
        if (xQueueReceive(s_evt_q, &gpio, portMAX_DELAY)) {
            int lvl1 = gpio_get_level(POWER_BUTTON_PIN);
            esp_rom_delay_us(DEBOUNCE_US);
            int lvl2 = gpio_get_level(POWER_BUTTON_PIN);
            if (lvl1 != lvl2) continue;

            int64_t now = esp_timer_get_time();

            if (!down && lvl2 == 0) {
                down = true;
                t_down = now;

            } else if (down && lvl2 == 1) {
                down = false;
                int64_t dur_ms = (now - t_down) / 1000;

                if (dur_ms >= LONG_MS) {
                    ESP_LOGI(TAG, "Long press detected → shutting down");
                    turn_off();
                } else if (dur_ms >= SHORT_MIN_MS) {
                    ESP_LOGI(TAG, "Short press detected");

                    // count for triple-short detection
                    if (now - last_short_time < (TRIPLE_WINDOW_MS * 1000LL))
                        short_count++;
                    else
                        short_count = 1;

                    last_short_time = now;

                    if (short_count >= 3) {
                        ESP_LOGI(TAG, "Triple short detected!");
                        // TODO: triple-short action goes here
                        short_count = 0;
                    }
                }
            }
        }
    }
}