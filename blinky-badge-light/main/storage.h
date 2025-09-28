#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include <stddef.h>
#include "genes.h"
#include "led_control.h"
#include "esp_err.h"


// Settings structure
typedef struct {
    int pattern_id;       // Current pattern ID
    uint8_t brightness;   // Current brightness level
} badge_settings_t;

extern badge_settings_t settings;
extern genome patterns[NUM_PATTERNS];

// Function prototypes
void init_storage();
void save_settings(const badge_settings_t *settings);
void load_settings(badge_settings_t *settings);
void load_genomes_from_storage(void);
void save_genomes_to_storage(void);
esp_err_t storage_get_name(char *out, size_t out_size);
esp_err_t storage_set_name(const char *name);

#endif // STORAGE_H