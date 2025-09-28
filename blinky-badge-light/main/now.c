#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "firework_notification_pattern.h"
#include "now.h"

static const char *TAG = "ESP_NOW";
#define NONCE_HISTORY_SIZE 32

// Firework packet definition
#define FIREWORK_MSG 0x42
static const uint8_t broadcast_addr[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; //everyone

static uint32_t recent_nonces[NONCE_HISTORY_SIZE] = {0};
static uint8_t nonce_index = 0;

#define MAX_SEND_PER_WINDOW 4
#define WINDOW_MS (40 * 1000)  // 40 seconds

static int64_t last_sent_times[MAX_SEND_PER_WINDOW] = {0};
static uint8_t last_sent_idx = 0;

// Helper to make notifications work immediately after startup
void reset_sent_history(void) {
    int64_t now = esp_timer_get_time() / 1000; // ms
    for (int i = 0; i < MAX_SEND_PER_WINDOW; ++i) {
        last_sent_times[i] = now - WINDOW_MS - 1;
    }
}

bool can_send_firework(void) {
    int64_t now = esp_timer_get_time() / 1000; // ms
    int send_count = 0;
    for (int i = 0; i < MAX_SEND_PER_WINDOW; i++) {
        if (now - last_sent_times[i] < WINDOW_MS) {
            send_count++;
        }
    }
    return send_count < MAX_SEND_PER_WINDOW;
}

void record_sent_firework(void) {
    int64_t now = esp_timer_get_time() / 1000;
    last_sent_times[last_sent_idx] = now;
    last_sent_idx = (last_sent_idx + 1) % MAX_SEND_PER_WINDOW;
}


static bool nonce_seen(uint32_t nonce) {
    for (int i = 0; i < NONCE_HISTORY_SIZE; ++i) {
        if (recent_nonces[i] == nonce) return true;
    }
    return false;
}


static void nonce_store(uint32_t nonce) {
    recent_nonces[nonce_index] = nonce;
    nonce_index = (nonce_index + 1) % NONCE_HISTORY_SIZE;
}


// Callback for receiving ESP-NOW data
static void now_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (len != sizeof(firework_packet_t)) return;
    const firework_packet_t *pkt = (const firework_packet_t *)data;
    if (pkt->type != FIREWORK_MSG) return;
    if (nonce_seen(pkt->msg_id)) return;

    nonce_store(pkt->msg_id);

    if (!show_firework_notification) {
        ESP_LOGI(TAG, "Received FIREWORK from %02x:%02x:%02x:%02x:%02x:%02x",
            recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
            recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
        show_firework_notification = true;
        firework_notification_start_time = esp_timer_get_time() / 1000; // ms
    }

    // Relay the firework packet to other badges if TTL > 0, for extended reach
    if (pkt->ttl > 0) {
        firework_packet_t relay = *pkt;
        relay.ttl--;
        esp_err_t err = esp_now_send(broadcast_addr, (uint8_t*)&relay, sizeof(relay));
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to relay firework: %s", esp_err_to_name(err));
        }
    }
}

// Initialize ESP-NOW and Wi-Fi in station mode (required)
void now_init(void) {
    ESP_LOGI(TAG, "Initializing ESP-NOW");
    reset_sent_history();

    // Init ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    // Add the broadcast address as a peer
    esp_now_peer_info_t peerInfo = {0};
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcast_addr, 6);
    peerInfo.channel = 0;  // 0 = current Wi-Fi channel
    peerInfo.ifidx = ESP_IF_WIFI_STA;
    peerInfo.encrypt = false; // No encryption for broadcasts

    esp_err_t err = esp_now_add_peer(&peerInfo);
    if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
        ESP_LOGW(TAG, "Failed to add ESP-NOW broadcast peer: %s", esp_err_to_name(err));
    }

    ESP_ERROR_CHECK(esp_now_register_recv_cb(now_recv_cb));
}

#define FIREWORK_RETRIES 3
#define FIREWORK_RETRY_DELAY_MS 20

// Send a firework message to all peers (broadcast)
void now_send_firework(void) {
    if (!can_send_firework()) {
        ESP_LOGI(TAG, "Rate limit reached: You can send up to %d fireworks every %d seconds.",
                 MAX_SEND_PER_WINDOW, WINDOW_MS / 1000);
        return;
    }

    firework_packet_t packet = {
        .type = FIREWORK_MSG,
        .msg_id = esp_random(),
        .ttl = 4,
    };
    nonce_store(packet.msg_id); // Record our own sent firework so we don't get triggered by badges further relaying it

    // sending the same notification multiple times to ensure delivery
    for (int i = 0; i < FIREWORK_RETRIES; ++i) {
        esp_err_t err = esp_now_send(broadcast_addr, (uint8_t *)&packet, sizeof(packet));
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to send ESP-NOW firework: %s", esp_err_to_name(err));
        }
        vTaskDelay(FIREWORK_RETRY_DELAY_MS / portTICK_PERIOD_MS);  // short delay between retries
    }
    ESP_LOGI(TAG, "Sending ESP-NOW firework with ID %" PRIu32, packet.msg_id);
    record_sent_firework(); // Record the sent firework for rate limiting
}
