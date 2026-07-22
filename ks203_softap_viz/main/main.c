#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "http_viz.h"
#include "ks203.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "wifi_softap.h"

static const char *TAG = "main";

static void ranging_task(void *arg)
{
    (void)arg;
    while (1) {
        ks203_frame_t frame;
        if (ks203_transact(&frame) != ESP_OK) {
            ESP_LOGW(TAG, "UART write failed");
        }
        frame_store_update(&frame);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_KS203_POLL_MS));
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(ks203_init());
    ESP_ERROR_CHECK(wifi_softap_start());
    ESP_ERROR_CHECK(http_viz_start());

    xTaskCreate(ranging_task, "ranging", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "send/recv/forward running");
}
