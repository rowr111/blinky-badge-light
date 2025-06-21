#include "genes.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "GENES";

void generate_gene(genome *g) {
    g->cd_period = 1 + (esp_random() % 6);
    g->cd_rate = esp_random() & 0xFF;
    g->cd_dir = esp_random() & 0xFF;
    g->sat = 200 + (esp_random() % 56); // even more vivid
    // Occasionally force full rainbow
    if ((esp_random() % 3) == 0) {
        g->hue_base = 1;
        g->hue_bound = 254;
    } else {
        uint8_t a = 1 + (esp_random() % 255);
        uint8_t b = 1 + (esp_random() % 255);
        g->hue_base = (a < b) ? a : b;
        g->hue_bound = (a > b) ? a : b;
        if (g->hue_base == g->hue_bound) g->hue_bound++; // ensure at least 1 span
    }
    g->hue_rate = esp_random() % 8;
    g->hue_dir = esp_random() % 2; // 0 or 1 for direction
    g->nonlin = esp_random() & 0xFF;

    // Generate a name for the pattern
    snprintf(g->name, GENE_NAMELENGTH, "Pattern %d", (int)(esp_random() % 100));

    // Log the generated pattern
    ESP_LOGI(TAG, "Generated genome: %s", g->name);
}
