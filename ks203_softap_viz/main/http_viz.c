#include <stdio.h>
#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "http_viz.h"

static const char *TAG = "http_viz";

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

static SemaphoreHandle_t s_lock;
static ks203_frame_t s_frame = {
    .ok = false,
    .has_mm = false,
    .mm = 0,
    .rx_len = 0,
    .tx_hex = "",
    .rx_hex = "starting",
};

void frame_store_update(const ks203_frame_t *frame)
{
    if (!frame || !s_lock) {
        return;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_frame = *frame;
    xSemaphoreGive(s_lock);
}

void frame_store_get(ks203_frame_t *out)
{
    if (!out) {
        return;
    }
    if (!s_lock) {
        memset(out, 0, sizeof(*out));
        snprintf(out->rx_hex, sizeof(out->rx_hex), "starting");
        return;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_frame;
    xSemaphoreGive(s_lock);
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    const size_t html_len = (size_t)(index_html_end - index_html_start);
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, (const char *)index_html_start, html_len);
}

static esp_err_t api_frame_get_handler(httpd_req_t *req)
{
    ks203_frame_t frame;
    frame_store_get(&frame);

    char buf[320];
    if (frame.has_mm) {
        snprintf(buf, sizeof(buf),
                 "{\"ok\":true,\"has_mm\":true,\"mm\":%u,\"cm\":%.1f,\"rx_len\":%d,\"tx\":\"%s\",\"rx\":\"%s\"}",
                 (unsigned)frame.mm,
                 frame.mm / 10.0f,
                 frame.rx_len,
                 frame.tx_hex[0] ? frame.tx_hex : "",
                 frame.rx_hex[0] ? frame.rx_hex : "");
    } else {
        snprintf(buf, sizeof(buf),
                 "{\"ok\":%s,\"has_mm\":false,\"mm\":null,\"cm\":null,\"rx_len\":%d,\"tx\":\"%s\",\"rx\":\"%s\"}",
                 frame.ok ? "true" : "false",
                 frame.rx_len,
                 frame.tx_hex[0] ? frame.tx_hex : "",
                 frame.rx_hex[0] ? frame.rx_hex : "");
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
}

esp_err_t http_viz_start(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) {
        return ESP_ERR_NO_MEM;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
    };
    httpd_uri_t api = {
        .uri = "/api/frame",
        .method = HTTP_GET,
        .handler = api_frame_get_handler,
    };
    httpd_uri_t api_old = {
        .uri = "/api/distance",
        .method = HTTP_GET,
        .handler = api_frame_get_handler,
    };
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &api);
    httpd_register_uri_handler(server, &api_old);

    ESP_LOGI(TAG, "HTTP ready http://192.168.4.1/");
    return ESP_OK;
}
