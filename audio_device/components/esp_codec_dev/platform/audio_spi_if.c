/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2022 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "audio_codec_ctrl_if.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "codec_dev_err.h"
#include "esp_err.h"
#include "esp_log.h"

#define TAG "SPI_If"

typedef struct {
    audio_codec_ctrl_if_t base;
    bool                  is_open;
    uint8_t               port;
    spi_device_handle_t   spi_handle;
} spi_ctrl_t;

int _spi_ctrl_open(const audio_codec_ctrl_if_t *ctrl, void *cfg, int cfg_size)
{
    if (ctrl == NULL || cfg == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (cfg_size != sizeof(codec_spi_dev_cfg_t)) {
        return CODEC_DEV_INVALID_ARG;
    }
    spi_ctrl_t *spi_ctrl = (spi_ctrl_t *) ctrl;
    codec_spi_dev_cfg_t *spi_cfg = (codec_spi_dev_cfg_t *) cfg;
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 1000000, // Clock out at 10 MHz
        .mode = 0,                 // SPI mode 0
        .queue_size = 6,           // queue 7 transactions at a time
    };
    dev_cfg.spics_io_num = spi_cfg->cs_pin;
    int ret;
#if (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0))
    ret = spi_bus_add_device(HSPI_HOST, &dev_cfg, &spi_ctrl->spi_handle);
#else
    ret = spi_bus_add_device(SPI3_HOST, &dev_cfg, &spi_ctrl->spi_handle);
#endif
    if (ret == 0) {
        gpio_set_pull_mode(spi_cfg->cs_pin, GPIO_FLOATING);
        spi_ctrl->is_open = true;
    }
    return ret == 0 ? CODEC_DEV_OK : CODEC_DEV_DRV_ERR;
}

bool _spi_ctrl_is_open(const audio_codec_ctrl_if_t *ctrl)
{
    if (ctrl) {
        spi_ctrl_t *spi_ctrl = (spi_ctrl_t *) ctrl;
        return spi_ctrl->is_open;
    }
    return false;
}

static int _spi_ctrl_read_addr(const audio_codec_ctrl_if_t *ctrl, int addr, int addr_len, void *data, int data_len)
{
    if (ctrl == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    spi_ctrl_t *spi_ctrl = (spi_ctrl_t *) ctrl;
    if (spi_ctrl->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    esp_err_t ret = ESP_OK;
    if (addr_len) {
        uint16_t *v = (uint16_t *) &addr;
        while (addr_len >= 2) {
            spi_transaction_t t = {0};
            t.length = 2 * 8;
            t.tx_buffer = v;
            ret = spi_device_transmit(spi_ctrl->spi_handle, &t);
            v++;
            addr_len -= 2;
        }
    }
    if (data_len) {
        spi_transaction_t t = {0};
        t.length = data_len * 8;
        t.rxlength = data_len * 8;
        t.rx_buffer = data;
        ret = spi_device_transmit(spi_ctrl->spi_handle, &t);
    }
    return ret == ESP_OK ? CODEC_DEV_OK : CODEC_DEV_DRV_ERR;
}

static int _spi_ctrl_write_addr(const audio_codec_ctrl_if_t *ctrl, int addr, int addr_len, void *data, int data_len)
{
    spi_ctrl_t *spi_ctrl = (spi_ctrl_t *) ctrl;
    if (ctrl == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (spi_ctrl->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    esp_err_t ret = ESP_OK;
    if (addr_len) {
        uint16_t *v = (uint16_t *) &addr;
        while (addr_len >= 2) {
            spi_transaction_t t = {0};
            t.length = 2 * 8;
            t.tx_buffer = v;
            ret = spi_device_transmit(spi_ctrl->spi_handle, &t);
            v++;
            addr_len -= 2;
        }
    }
    if (data_len) {
        spi_transaction_t t = {0};
        t.length = data_len * 8;
        t.tx_buffer = data;
        ret = spi_device_transmit(spi_ctrl->spi_handle, &t);
    }
    return ret == ESP_OK ? CODEC_DEV_OK : CODEC_DEV_DRV_ERR;
}

int _spi_ctrl_close(const audio_codec_ctrl_if_t *ctrl)
{
    spi_ctrl_t *spi_ctrl = (spi_ctrl_t *) ctrl;
    if (ctrl == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = 0;
    if (spi_ctrl->spi_handle) {
        ret = spi_bus_remove_device(spi_ctrl->spi_handle);
    }
    spi_ctrl->is_open = false;
    return ret == ESP_OK ? CODEC_DEV_OK : CODEC_DEV_DRV_ERR;
}

const audio_codec_ctrl_if_t *audio_codec_new_spi_ctrl_if(codec_spi_dev_cfg_t *spi_cfg)
{
    spi_ctrl_t *ctrl = calloc(1, sizeof(spi_ctrl_t));
    if (ctrl == NULL) {
        return NULL;
    }
    ctrl->base.open = _spi_ctrl_open;
    ctrl->base.is_open = _spi_ctrl_is_open;
    ctrl->base.read_addr = _spi_ctrl_read_addr;
    ctrl->base.write_addr = _spi_ctrl_write_addr;
    ctrl->base.close = _spi_ctrl_close;
    int ret = _spi_ctrl_open(&ctrl->base, spi_cfg, sizeof(codec_spi_dev_cfg_t));
    if (ret != 0) {
        ESP_LOGE(TAG, "Fail to open SPI driver");
        free(ctrl);
        return NULL;
    }
    return &ctrl->base;
}
