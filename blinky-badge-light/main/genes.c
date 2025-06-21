#include "genes.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "GENES";

void generate_gene(genome *g) {
    g->cd_period = esp_random() % 7;
    g->cd_rate = esp_random() & 0xFF;
    g->cd_dir = esp_random() & 0xFF;
    g->sat = 200 + (esp_random() % 56); // even more vivid
    g->hue_base = esp_random() & 0xFF;

    // Occasionally force full rainbow
    if ((esp_random() % 3) == 0) {
        g->hue_base = 0;
        g->hue_bound = 255;
    } else {
        uint8_t span = 160 + (esp_random() % 96); // 160–255
        g->hue_bound = (g->hue_base + span) % 256;
    }

    g->hue_ratedir = (esp_random() & 0xFF) / 2; // 0–127
    g->lin = esp_random() & 0xFF;
    g->strobe = esp_random() & 0xFF;
    g->accel = esp_random() & 0xFF;
    g->nonlin = esp_random() & 0xFF;

    // Generate a name for the pattern
    snprintf(g->name, GENE_NAMELENGTH, "Pattern %d", (int)(esp_random() % 100));

    // Log the generated pattern
    ESP_LOGI(TAG, "Generated genome: %s", g->name);
}
