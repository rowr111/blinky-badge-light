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

static int64_t last_hi_time = 0;  // microseconds since boot
#define HI_NOTIFICATION_COOLDOWN_US (10 * 1000 * 1000)  // 10 sec in microseconds

static uint32_t recent_nonces[NONCE_HISTORY_SIZE] = {0};
static uint8_t nonce_index = 0;

static const uint8_t broadcast_addr[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

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

    int64_t now = esp_timer_get_time();
    if (!show_firework_notification && (now - last_hi_time > HI_NOTIFICATION_COOLDOWN_US)) {
        last_hi_time = now;
        ESP_LOGI(TAG, "Received FIREWORK from %02x:%02x:%02x:%02x:%02x:%02x",
            recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
            recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
        show_firework_notification = true;
        firework_notification_start_time = now / 1000; // ms
    }

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

    // 1. Initialize Wi-Fi in STA mode (but not connected to any AP)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // 2. Init ESP-NOW
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

// Send a firework message to all peers (broadcast)
void now_send_firework(void) {
    firework_packet_t packet = {
        .type = FIREWORK_MSG,
        .msg_id = esp_random(),
        .ttl = 4,
    };
    esp_err_t err = esp_now_send(broadcast_addr, (uint8_t *)&packet, sizeof(packet));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send ESP-NOW firework: %s", esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "Sending ESP-NOW firework with ID %" PRIu32, packet.msg_id);
    nonce_store(packet.msg_id);
}
