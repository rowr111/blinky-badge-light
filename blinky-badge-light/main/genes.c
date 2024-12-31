#include "genes.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"

static const char *TAG = "GENES";

void generate_gene(genome *g) {
    g->cd_period = rand() % 6;
    g->cd_rate = rand() % 255;
    g->cd_dir = rand() % 2;
    g->sat = rand() % 255;
    g->hue_base = rand() % 255;
    g->hue_ratedir = rand() % 255;
    g->hue_bound = rand() % 255;
    g->lin = rand() % 255;
    g->strobe = rand() % 255;
    g->accel = rand() % 255;
    g->nonlin = rand() % 255;

    // Generate a name for the pattern
    snprintf(g->name, GENE_NAMELENGTH, "Pattern %d", rand() % 100);

    // Log the generated pattern
    ESP_LOGI(TAG, "Generated genome: %s", g->name);
}
