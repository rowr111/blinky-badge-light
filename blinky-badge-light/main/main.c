#include "led_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main() {
    init_leds(); // Initialize the LED system

    while (1) {
        for (int i = 0; i < 5; i++) {
            set_pattern(i); // Set the current pattern
            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 ms (1 second)
        }
    }
}
