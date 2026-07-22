#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ks203.h"
#include "sdkconfig.h"

static const char *TAG = "ks203";

#define KS203_UART_PORT ((uart_port_t)CONFIG_KS203_UART_NUM)
#define KS203_REG       0x02
#define KS203_CMD_RANGE 0x01
#define KS203_ERR_MM    0xEEEE

static void bytes_to_hex(const uint8_t *data, int len, char *out, size_t out_sz)
{
    out[0] = '\0';
    if (!data || len <= 0 || out_sz < 4) {
        return;
    }
    int pos = 0;
    for (int i = 0; i < len; ++i) {
        int n = snprintf(out + pos, out_sz - (size_t)pos, "%s%02X", (i ? " " : ""), data[i]);
        if (n <= 0 || (size_t)(pos + n) >= out_sz) {
            break;
        }
        pos += n;
    }
}

static uint8_t bcc_xor(const uint8_t *data, size_t len)
{
    uint8_t b = 0;
    for (size_t i = 0; i < len; ++i) {
        b ^= data[i];
    }
    return b;
}

/* KS203 default reply: exactly 2 bytes, big-endian mm. */
static void fill_mm_from_rx(const uint8_t *rx, int got, ks203_frame_t *out)
{
    out->has_mm = false;
    out->mm = 0;
    if (got != 2) {
        return;
    }
    uint16_t mm = ((uint16_t)rx[0] << 8) | rx[1];
    if (mm == KS203_ERR_MM) {
        return;
    }
    out->has_mm = true;
    out->mm = mm;
}

esp_err_t ks203_init(void)
{
    uart_config_t uart_cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_RETURN_ON_ERROR(uart_driver_install(KS203_UART_PORT, 1024, 0, 0, NULL, 0), TAG, "uart install");
    ESP_RETURN_ON_ERROR(uart_param_config(KS203_UART_PORT, &uart_cfg), TAG, "uart param");
    ESP_RETURN_ON_ERROR(uart_set_pin(KS203_UART_PORT,
                                     CONFIG_KS203_UART_TX_GPIO,
                                     CONFIG_KS203_UART_RX_GPIO,
                                     UART_PIN_NO_CHANGE,
                                     UART_PIN_NO_CHANGE),
                        TAG, "uart set pin");

    ESP_LOGI(TAG, "UART%d TX=%d RX=%d 115200 addr=0x%02X",
             CONFIG_KS203_UART_NUM,
             CONFIG_KS203_UART_TX_GPIO,
             CONFIG_KS203_UART_RX_GPIO,
             CONFIG_KS203_ADDR);
    return ESP_OK;
}

static void one_shot(const uint8_t *cmd, size_t cmd_len, ks203_frame_t *out)
{
    bytes_to_hex(cmd, (int)cmd_len, out->tx_hex, sizeof(out->tx_hex));
    out->has_mm = false;
    out->mm = 0;

    uart_flush_input(KS203_UART_PORT);

    if (uart_write_bytes(KS203_UART_PORT, (const char *)cmd, cmd_len) != (int)cmd_len) {
        snprintf(out->rx_hex, sizeof(out->rx_hex), "tx_fail");
        out->ok = false;
        out->rx_len = 0;
        return;
    }
    uart_wait_tx_done(KS203_UART_PORT, pdMS_TO_TICKS(20));
    esp_rom_delay_us(500);

    uint8_t junk[8];
    uart_read_bytes(KS203_UART_PORT, junk, sizeof(junk), pdMS_TO_TICKS(3));

    vTaskDelay(pdMS_TO_TICKS(25));

    uint8_t rx[32];
    int got = uart_read_bytes(KS203_UART_PORT, rx, sizeof(rx), pdMS_TO_TICKS(20));
    out->rx_len = got > 0 ? got : 0;
    out->ok = (got > 0);
    if (got > 0) {
        bytes_to_hex(rx, got, out->rx_hex, sizeof(out->rx_hex));
        fill_mm_from_rx(rx, got, out);
    } else {
        snprintf(out->rx_hex, sizeof(out->rx_hex), "(empty)");
    }
}

esp_err_t ks203_transact(ks203_frame_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(out, 0, sizeof(*out));

    uint8_t plain[3] = {
        (uint8_t)CONFIG_KS203_ADDR,
        KS203_REG,
        KS203_CMD_RANGE,
    };
    uint8_t with_bcc[4] = {
        (uint8_t)CONFIG_KS203_ADDR,
        KS203_REG,
        KS203_CMD_RANGE,
        0,
    };
    with_bcc[3] = bcc_xor(with_bcc, 3);

    /* BCC reply is what currently works on this unit; try it first. */
    one_shot(with_bcc, sizeof(with_bcc), out);
    ESP_LOGI(TAG, "tx: %s", out->tx_hex);
    if (out->has_mm) {
        ESP_LOGI(TAG, "rx (%d): %s -> %u mm", out->rx_len, out->rx_hex, (unsigned)out->mm);
    } else {
        ESP_LOGI(TAG, "rx (%d): %s", out->rx_len, out->rx_hex);
    }
    if (out->ok) {
        return ESP_OK;
    }

    one_shot(plain, sizeof(plain), out);
    ESP_LOGI(TAG, "tx: %s", out->tx_hex);
    if (out->has_mm) {
        ESP_LOGI(TAG, "rx (%d): %s -> %u mm", out->rx_len, out->rx_hex, (unsigned)out->mm);
    } else {
        ESP_LOGI(TAG, "rx (%d): %s", out->rx_len, out->rx_hex);
    }
    return ESP_OK;
}
