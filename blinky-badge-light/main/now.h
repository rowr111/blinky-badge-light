#ifndef NOW_H
#define NOW_H

#include <stdint.h>
#include <stdbool.h>

void now_init(void);
void now_send_firework(void);

typedef struct {
    uint8_t type;        // Message type (e.g., FIREWORK_MSG_TYPE)
    uint32_t msg_id;     // Random nonce for deduplication
    uint8_t ttl;         // Hop limit
} __attribute__((packed)) firework_packet_t;

#endif // NOW_H