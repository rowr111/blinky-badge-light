#ifndef GENES_H
#define GENES_H

#include <stdint.h>
#include <string.h>

#define NUM_PATTERNS 5
#define GENE_NAMELENGTH 16

typedef struct {
    uint8_t cd_period;       // Controls period of sine wave (e.g., pattern speed)
    uint8_t cd_rate;         // Amplitude or "volume" of brightness modulation
    uint8_t cd_dir;          // Direction for certain effects
    uint8_t sat;             // Saturation level
    uint8_t hue_base;        // Base hue
    uint8_t hue_ratedir;     // Hue change rate/direction
    uint8_t hue_bound;       // Maximum hue
    uint8_t lin;             // Linear brightness modifier
    uint8_t strobe;          // Strobe effect intensity
    uint8_t accel;           // Acceleration for dynamic effects
    uint8_t nonlin;          // Nonlinearity (gamma correction or other tweaks)
    char name[GENE_NAMELENGTH]; // Pattern name
} genome;

extern genome patterns[NUM_PATTERNS];

void generate_gene(genome *g);

#endif // GENES_H