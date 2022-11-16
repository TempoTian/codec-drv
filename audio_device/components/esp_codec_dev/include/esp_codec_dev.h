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
#ifndef ESP_CODEC_DEV_H
#define ESP_CODEC_DEV_H

#include "audio_codec_if.h"
#include "audio_codec_data_if.h"
#include "codec_dev_types.h"
#include "codec_dev_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Codec device configuration
 */
typedef struct {
    codec_dev_type_t             dev_type; /*!< Codec device type */
    const audio_codec_if_t      *codec_if; /*!< Codec interface */
    const audio_codec_data_if_t *data_if;  /*!< Codec data interface */
} esp_codec_dev_cfg_t;

/**
 * @brief Codec hardware gain setting
 *        Notes: To make codec and PA finally output 0DB if input 0 DB data
 *               Need to get hardware gain firstly
 *               Hardware gain generally consist of 2 part
 *               1. codec DAC voltage not same as PA voltage
 *               2. PA gain can be calculate by connected resistor
 */
typedef struct {
    float pa_voltage;        /*!< PA voltage: typical 5.0v */
    float codec_dac_voltage; /*!< Codec chip DAC voltage: typical 3.3v */
    float pa_gain;           /*!< PA gain in decibel unit */
} esp_codec_dev_hw_gain_t;

/**
 * @brief Codec volume curve configuration
 */
typedef struct {
    codec_dev_vol_map_t *vol_map; /*!< Point of volume curve */
    int                  count;   /*!< Curve point number  */
} esp_codec_dev_vol_curve_t;

/**
 * @brief Codec device handle
 */
typedef void *esp_codec_dev_handle_t;

/**
 * @brief         New codec device
 * @param         codec_dev_cfg: Codec device configuration
 * @return        NULL: Fail to new codec device
 *                -Others: Codec device handle        
 */
esp_codec_dev_handle_t esp_codec_dev_new(esp_codec_dev_cfg_t *codec_dev_cfg);

/**
 * @brief         Open codec device
 * @param         codec: Codec device handle
 * @param         fs: Audio sample information
 * @return        CODEC_DEV_OK: Open success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 *                CODEC_DEV_NOT_SUPPORT: Codec not support or driver not ready yet
 */
int esp_codec_dev_open(esp_codec_dev_handle_t codec, codec_sample_info_t *fs);

/**
 * @brief         Read data from codec
 * @param         codec: Codec device handle
 * @param         data: Data to be read
 * @param         len: Data length to be read
 * @return        CODEC_DEV_OK: Read success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 *                CODEC_DEV_NOT_SUPPORT: Codec not support
 *                CODEC_DEV_WRONG_STATE: Driver not open yet
 */
int esp_codec_dev_read(esp_codec_dev_handle_t codec, void *data, int len);

/**
 * @brief         Write data to codec
 *                Notes: when enable software volume, it will change input data level directly without copy
 *                Make sure that input data is writable
 * @param         codec: Codec device handle
 * @param         data: Data to be wrote
 * @param         len: Data length to be wrote
 * @return        CODEC_DEV_OK: Write success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 *                CODEC_DEV_NOT_SUPPORT: Codec not support
 *                CODEC_DEV_WRONG_STATE: Driver not open yet
 */
int esp_codec_dev_write(esp_codec_dev_handle_t codec, void *data, int len);

/**
 * @brief         Set codec hardware gain
 * @param         codec: Codec device handle
 * @param         hw_gain: Hardware gain setting
 * @return        CODEC_DEV_OK: Open success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 */
int esp_codec_dev_set_hw_gain(esp_codec_dev_handle_t codec, esp_codec_dev_hw_gain_t *hw_gain);

/**
 * @brief         Set codec hardware gain
 * @param         codec: Codec device handle
 * @param         hw_gain: Hardware gain setting
 * @return        CODEC_DEV_OK: Set output volume success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 *                CODEC_DEV_NOT_SUPPORT: Codec not support output mode
 *                CODEC_DEV_WRONG_STATE: Driver not open yet
 */
int esp_codec_dev_set_out_vol(esp_codec_dev_handle_t codec, int volume);

/**
 * @brief         Set codec volume curve
 * @param         codec: Codec device handle
 * @param         curve: Volume curve setting
 * @return        CODEC_DEV_OK: Set curve success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 *                CODEC_DEV_NOT_SUPPORT: Codec not support output mode
 *                CODEC_DEV_WRONG_STATE: Driver not open yet
 *                CODEC_DEV_NO_MEM: Not enough memory to hold volume curve
 */
int esp_codec_dev_set_vol_curve(esp_codec_dev_handle_t codec, esp_codec_dev_vol_curve_t *curve);

/**
 * @brief         Get codec output volume
 * @param         codec: Codec device handle
 * @param[out]    volume: volume to get
 * @return        CODEC_DEV_OK: Get volume success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 *                CODEC_DEV_NOT_SUPPORT: Codec not support output mode
 *                CODEC_DEV_WRONG_STATE: Driver not open yet
 */
int esp_codec_dev_get_out_vol(esp_codec_dev_handle_t codec, int *volume);

/**
 * @brief         Set codec output mute
 * @param         codec: Codec device handle
 * @param         mute: Whether mute output or not
 * @return        CODEC_DEV_OK: Set output mute success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 *                CODEC_DEV_NOT_SUPPORT: Codec not support output mode
 *                CODEC_DEV_WRONG_STATE: Driver not open yet
 */
int esp_codec_dev_set_out_mute(esp_codec_dev_handle_t codec, bool mute);

/**
 * @brief         Get codec output mute setting
 * @param         codec: Codec device handle
 * @param[out]    mute: Whether mute output or not
 * @return        CODEC_DEV_OK: Get output mute success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 *                CODEC_DEV_NOT_SUPPORT: Codec not support output mode
 *                CODEC_DEV_WRONG_STATE: Driver not open yet
 */
int esp_codec_dev_get_out_mute(esp_codec_dev_handle_t codec, bool *muted);

/**
 * @brief         Set codec input gain
 * @param         codec: Codec device handle
 * @param         db_value: Input gain setting
 * @return        CODEC_DEV_OK: Set input gain success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 *                CODEC_DEV_NOT_SUPPORT: Codec not support input mode
 *                CODEC_DEV_WRONG_STATE: Driver not open yet
 */
int esp_codec_dev_set_in_gain(esp_codec_dev_handle_t codec, float db_value);

/**
 * @brief         Get codec input gain
 * @param         codec: Codec device handle
 * @param         db_value: Input gain to get
 * @return        CODEC_DEV_OK: Get input gain success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 *                CODEC_DEV_NOT_SUPPORT: Codec not support input mode
 *                CODEC_DEV_WRONG_STATE: Driver not open yet
 */
int esp_codec_dev_get_in_gain(esp_codec_dev_handle_t codec, float *db_value);

/**
 * @brief         Set codec input mute
 * @param         codec: Codec device handle
 * @param         mute: Whether mute code input or not
 * @return        CODEC_DEV_OK: Set input mute success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 *                CODEC_DEV_NOT_SUPPORT: Codec not support input mode
 *                CODEC_DEV_WRONG_STATE: Driver not open yet
 */
int esp_codec_dev_set_in_mute(esp_codec_dev_handle_t codec, bool mute);

/**
 * @brief         Get codec input mute
 * @param         codec: Codec device handle
 * @param         mute: Mute value to get
 * @return        CODEC_DEV_OK: Set input mute success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 *                CODEC_DEV_NOT_SUPPORT: Codec not support input mode
 *                CODEC_DEV_WRONG_STATE: Driver not open yet
 */
int esp_codec_dev_get_in_mute(esp_codec_dev_handle_t codec, bool *muted);

/**
 * @brief         Close codec device
 * @param         codec: Codec device handle
 * @return        CODEC_DEV_OK: Close success
 *                CODEC_DEV_INVALID_ARG: Invalid arguments
 */
int esp_codec_dev_close(esp_codec_dev_handle_t codec);

/**
 * @brief         Delete codec device
 * @param         codec: Codec device handle
 */
void esp_codec_dev_delete(esp_codec_dev_handle_t codec);

#ifdef __cplusplus
}
#endif

#endif
