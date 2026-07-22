#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KS203_HEX_MAX 96

typedef struct {
    bool ok;                 /* any RX bytes */
    bool has_mm;             /* true only when RX is exactly 2 bytes distance */
    uint16_t mm;
    int rx_len;
    char tx_hex[KS203_HEX_MAX];
    char rx_hex[KS203_HEX_MAX];
} ks203_frame_t;

esp_err_t ks203_init(void);
esp_err_t ks203_transact(ks203_frame_t *out);

#ifdef __cplusplus
}
#endif
