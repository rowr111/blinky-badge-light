#include <string.h>
#include "wifi_portal.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

static const char *TAG = "PORTAL_WIFI";
static bool s_wifi_started = false;

esp_err_t portal_wifi_start(const portal_wifi_cfg_t *cfg)
{
    if (s_wifi_started) return ESP_OK;

    // Make these inits tolerant if already done elsewhere
    esp_err_t err;

    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    // Create default netifs if not present yet
    if (!esp_netif_get_handle_from_ifkey("WIFI_AP_DEF")) {
        esp_netif_create_default_wifi_ap();
    }
    if (!esp_netif_get_handle_from_ifkey("WIFI_STA_DEF")) {
        esp_netif_create_default_wifi_sta();
    }

    wifi_init_config_t wic = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wic));

    // AP config
    wifi_config_t ap_cfg = (wifi_config_t){0};
    strncpy((char*)ap_cfg.ap.ssid, cfg->ssid, sizeof(ap_cfg.ap.ssid)-1);
    ap_cfg.ap.ssid_len      = 0;            // use strlen
    ap_cfg.ap.channel       = cfg->channel; // AP will start on this channel
    ap_cfg.ap.max_connection= 4;

    if (cfg->password[0] && strlen(cfg->password) >= 8) {
        ap_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
        strncpy((char*)ap_cfg.ap.password, cfg->password, sizeof(ap_cfg.ap.password)-1);
    } else {
        ap_cfg.ap.authmode = WIFI_AUTH_OPEN;
    }

    // Empty STA (we won’t connect; STA is for ESP-NOW)
    wifi_config_t sta_cfg = (wifi_config_t){0};

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP,  &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));

    // Start Wi-Fi first (important: set_channel after start to avoid 0x3002)
    ESP_ERROR_CHECK(esp_wifi_start());

    // Keep phones happy on AP; enable LR on STA for ESP-NOW
    ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_AP,  WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
    ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_LR));

    // Force both IFs to the same channel (now safe because Wi-Fi is started)
    ESP_ERROR_CHECK(esp_wifi_set_channel(cfg->channel, WIFI_SECOND_CHAN_NONE));

    s_wifi_started = true;
    ESP_LOGI(TAG, "AP+STA up. SSID:%s ch:%u auth:%s",
             ap_cfg.ap.ssid, cfg->channel,
             ap_cfg.ap.authmode == WIFI_AUTH_OPEN ? "open" : "WPA2");
    return ESP_OK;
}

void portal_wifi_stop(void)
{
    if (!s_wifi_started) return;
    esp_wifi_stop();
    s_wifi_started = false;
}
