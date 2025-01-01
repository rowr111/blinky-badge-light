#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

// Settings structure
typedef struct {
    int pattern_id;       // Current pattern ID
    uint8_t brightness;   // Current brightness level
} badge_settings_t;

// Function prototypes
void init_storage();
void save_settings(const badge_settings_t *settings);
void load_settings(badge_settings_t *settings);
void load_genomes_from_storage(void);
void save_genomes_to_storage(void);

#endif // STORAGE_H