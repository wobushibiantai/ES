#pragma once

#include "esp_err.h"
#include "ks203.h"

#ifdef __cplusplus
extern "C" {
#endif

void frame_store_update(const ks203_frame_t *frame);
void frame_store_get(ks203_frame_t *out);

esp_err_t http_viz_start(void);

#ifdef __cplusplus
}
#endif
