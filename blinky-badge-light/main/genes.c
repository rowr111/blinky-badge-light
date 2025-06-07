#include "genes.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "GENES";

void generate_gene(genome *g) {
    g->cd_period = esp_random() % 6;
    g->cd_rate = esp_random() % 255;
    g->cd_dir = esp_random() % 2;
    g->sat = esp_random() % 255;
    g->hue_base = esp_random() % 255;
    g->hue_ratedir = esp_random() % 255;
    g->hue_bound = esp_random() % 255;
    g->lin = esp_random() % 255;
    g->strobe = esp_random() % 255;
    g->accel = esp_random() % 255;
    g->nonlin = esp_random() % 255;

    // Generate a name for the pattern
    snprintf(g->name, GENE_NAMELENGTH, "Pattern %d", (int)(esp_random() % 100));

    // Log the generated pattern
    ESP_LOGI(TAG, "Generated genome: %s", g->name);
}
