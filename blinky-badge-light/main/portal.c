#include "portal.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "storage.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "PORTAL_HTTP";
static httpd_handle_t s_server = NULL;

static const char *PAGE =
"<!doctype html><html><head><meta charset='utf-8'/>"
"<meta name='viewport' content='width=device-width,initial-scale=1'/>"
"<title>Blinky Badge Portal</title>"
"<style>"
"body{font-family:sans-serif;margin:16px;}"
".card{max-width:560px;padding:16px;border:1px solid #ddd;border-radius:12px;}"
"label{display:block;margin:10px 0 4px;font-weight:600}"
"input[type=text]{width:100%;padding:8px;border:1px solid #ccc;border-radius:8px}"
"button{margin-top:12px;padding:10px 14px;border:0;border-radius:10px}"
"#out{white-space:pre-wrap;margin-top:8px;color:#444}"
"</style>"
"</head><body><div class='card'>"
"<h2>Blinky Badge – Setup</h2>"
"<p>Identifier / Name (shown on your badge or used for discovery):</p>"
"<label for='nm'>Badge name / ID</label>"
"<input id='nm' type='text' maxlength='31' placeholder='e.g. TheBlinkiestBadge'/>"
"<div>"
"  <button onclick='saveName()'>Save Name</button>"
"  <span id='out'></span>"
"</div>"
"<script>"
"async function loadName(){"
"  try{"
"    const r = await fetch('/api/name');"
"    if(r.ok){ const t = await r.text(); document.getElementById('nm').value = t || ''; }"
"  }catch(e){}"
"}"
"async function saveName(){"
"  const nm = document.getElementById('nm').value || '';"
"  try{"
"    const r = await fetch('/api/name',{method:'POST',headers:{'Content-Type':'text/plain'},body:nm});"
"    const t = await r.text();"
"    document.getElementById('out').textContent = t;"
"  }catch(e){ document.getElementById('out').textContent = String(e); }"
"}"
"loadName();"
"</script>"
"</body></html>";


static esp_err_t root_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, PAGE, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t redirect_to_root(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, "", 0);
    return ESP_OK;
}

static esp_err_t name_get(httpd_req_t *req)
{
    ESP_LOGI(TAG, "name_get: method=%d uri=%s", req->method, req->uri);

    char buf[NAME_MAX + 1] = {0}; // NAME_MAX from storage.c; or just 32 here
    esp_err_t err = storage_get_name(buf, sizeof(buf));
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        buf[0] = '\0'; // no name yet → empty
    }
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_sendstr(req, buf);
}


static esp_err_t name_post(httpd_req_t *req)
{
    ESP_LOGI(TAG, "name_post: method=%d uri=%s", req->method, req->uri);

    // Read raw body (we accept text/plain or JSON; treat body as the whole name)
    char body[128];
    int received = 0;
    while (received < (int)sizeof(body) - 1) {
        int r = httpd_req_recv(req, body + received, sizeof(body) - 1 - received);
        if (r <= 0) break;
        received += r;
    }
    body[received] = 0;

    // If client sent JSON like {"name":"Foo"}, pull the value; otherwise treat as plain text
    const char *nm = body;
    char parsed[NAME_MAX + 1];
    const char *k = "\"name\":";
    char *p = strstr(body, k);
    if (p) {
        p += strlen(k);
        // skip spaces/quotes
        while (*p == ' ' || *p == '\"' || *p == '\'') p++;
        // copy until closing quote or end
        int i = 0;
        while (*p && *p != '\"' && *p != '\'' && *p != '\n' && *p != '\r' && i < NAME_MAX) {
            parsed[i++] = *p++;
        }
        parsed[i] = 0;
        nm = parsed;
    }

    esp_err_t err = storage_set_name(nm);
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    if (err == ESP_OK) {
        return httpd_resp_sendstr(req, "OK: name saved");
    } else {
        return httpd_resp_sendstr(req, "ERR: failed to save name");
    }
}


esp_err_t portal_http_start(void)
{
    if (s_server) return ESP_OK;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.lru_purge_enable = true;
    config.max_uri_handlers = 16;
    if (httpd_start(&s_server, &config) != ESP_OK) return ESP_FAIL;

    // Your root page
    httpd_uri_t uri_root = { .uri="/", .method=HTTP_GET, .handler=root_get };
    httpd_register_uri_handler(s_server, &uri_root);

    // Common captive probes → redirect to "/"
    httpd_uri_t uri_android  = { .uri="/generate_204", .method=HTTP_GET, .handler=redirect_to_root };
    httpd_uri_t uri_android2 = { .uri="/gen_204",      .method=HTTP_GET, .handler=redirect_to_root };
    httpd_uri_t uri_apple    = { .uri="/hotspot-detect.html", .method=HTTP_GET, .handler=redirect_to_root };
    httpd_uri_t uri_ms1      = { .uri="/connecttest.txt", .method=HTTP_GET, .handler=redirect_to_root };
    httpd_uri_t uri_ms2      = { .uri="/ncsi.txt", .method=HTTP_GET, .handler=redirect_to_root };

    httpd_register_uri_handler(s_server, &uri_android);
    httpd_register_uri_handler(s_server, &uri_android2);
    httpd_register_uri_handler(s_server, &uri_apple);
    httpd_register_uri_handler(s_server, &uri_ms1);
    httpd_register_uri_handler(s_server, &uri_ms2);

    // Safety net: catch-all → root (optional)
    httpd_uri_t uri_catch = { .uri="/*", .method=HTTP_GET, .handler=redirect_to_root };
    httpd_register_uri_handler(s_server, &uri_catch);

    ESP_LOGI(TAG, "HTTP server + captive handlers on :%d", config.server_port);

        // GET /api/name → returns saved name (text/plain)
    httpd_uri_t uri_name_get = {
        .uri = "/api/name",
        .method = HTTP_GET,
        .handler = name_get,
        .user_ctx = NULL
    };

    // POST /api/name → save posted name (text/plain or JSON)
    httpd_uri_t uri_name_post = {
        .uri = "/api/name",
        .method = HTTP_POST,
        .handler = name_post,
        .user_ctx = NULL
    };

    if (httpd_register_uri_handler(s_server, &uri_name_get) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register GET /api/name");
    }
    if (httpd_register_uri_handler(s_server, &uri_name_post) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register POST /api/name");
    }
    return ESP_OK;
}


void portal_http_stop(void)
{
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
    }
}
