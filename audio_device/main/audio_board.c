#include <string.h>
#include <stdio.h>
#include "audio_board.h"
#include "driver/i2s.h"
#include "driver/i2c.h"
#include "driver/spi_common.h"
#include "board_cfg_parse.h"
#include "codec_dev_defaults.h"
#include "esp_log.h"

#define TAG                    "Audio_Board"

#define ESP_INTR_FLG_DEFAULT   (0)
#define ESP_I2C_MASTER_BUF_LEN (0)

#define DEFAULT_I2S_CFG                                             \
    {.mode = (i2s_mode_t) (I2S_MODE_TX | I2S_MODE_RX),              \
     .sample_rate = 44100,                                          \
     .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                  \
     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                  \
     .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM, \
     .dma_buf_count = 2,                                            \
     .dma_buf_len = 128,                                            \
     .use_apll = true,                                              \
     .tx_desc_auto_clear = true,                                    \
     .fixed_mclk = 0};

extern const char board_cfg_start[] asm("_binary_board_cfg_txt_start");
extern const char board_cfg_end[] asm("_binary_board_cfg_txt_end");

#define SAFE_FREE(ptr) \
    if (ptr)           \
        free(ptr);

void clear_i2s_cfg(audio_board_i2s_cfg_t *i2s_cfg)
{
    i2s_cfg->master_mode = true;
    i2s_cfg->bck_pin = -1;
    i2s_cfg->ws_pin = -1;
    i2s_cfg->data_out_pin = -1;
    i2s_cfg->data_out_pin = -1;
    i2s_cfg->data_in_pin = -1;
    i2s_cfg->mck_pin = -1;
    i2s_cfg->dac_mode = false;
}

static int fill_i2c_cfg(audio_board_cfg_t *cfg, board_cfg_attr_t *attr)
{
    int size = (cfg->i2c_bus_num + 1) * sizeof(audio_board_i2c_cfg_t);
    audio_board_i2c_cfg_t *new_i2c_bus = realloc(cfg->i2c_bus_cfg, size);
    if (new_i2c_bus == NULL) {
        return -1;
    }
    cfg->i2c_bus_cfg = new_i2c_bus;
    audio_board_i2c_cfg_t *i2c_cfg = &new_i2c_bus[cfg->i2c_bus_num];
    i2c_cfg->master_mode = true;
    cfg->i2c_bus_num++;
    while (attr) {
        if (str_same(attr->attr, "sda")) {
            i2c_cfg->sda_pin = atoi(attr->value);
        } else if (str_same(attr->attr, "scl")) {
            i2c_cfg->scl_pin = atoi(attr->value);
        }
        attr = attr->next;
    }
    return 0;
}

static int fill_i2s_cfg(audio_board_cfg_t *cfg, board_cfg_attr_t *attr)
{
    int size = (cfg->i2s_bus_num + 1) * sizeof(audio_board_i2s_cfg_t);
    audio_board_i2s_cfg_t *new_i2s_bus = realloc(cfg->i2s_bus_cfg, size);
    if (new_i2s_bus == NULL) {
        return -1;
    }
    cfg->i2s_bus_cfg = new_i2s_bus;
    audio_board_i2s_cfg_t *i2s_cfg = &new_i2s_bus[cfg->i2s_bus_num];
    i2s_cfg->master_mode = true;
    cfg->i2s_bus_num++;
    clear_i2s_cfg(i2s_cfg);
    while (attr) {
        if (str_same(attr->attr, "bck")) {
            i2s_cfg->bck_pin = atoi(attr->value);
        } else if (str_same(attr->attr, "data_in")) {
            i2s_cfg->data_in_pin = atoi(attr->value);
        } else if (str_same(attr->attr, "data_out")) {
            i2s_cfg->data_out_pin = atoi(attr->value);
        } else if (str_same(attr->attr, "ws")) {
            i2s_cfg->ws_pin = atoi(attr->value);
        } else if (str_same(attr->attr, "mck")) {
            i2s_cfg->mck_pin = atoi(attr->value);
        }
        attr = attr->next;
    }
    return 0;
}

static void clear_spi_cfg(audio_board_spi_cfg_t *spi_cfg)
{
    spi_cfg->mosi_pin = -1;
    spi_cfg->miso_pin = -1;
    spi_cfg->sclk_pin = -1;
    spi_cfg->quadwp_pin = -1;
    spi_cfg->quadhd_pin = -1;
}

static int fill_spi_cfg(audio_board_cfg_t *cfg, board_cfg_attr_t *attr)
{
    int size = (cfg->spi_bus_num + 1) * sizeof(audio_board_spi_cfg_t);
    audio_board_spi_cfg_t *new_spi_bus = realloc(cfg->spi_bus_cfg, size);
    if (new_spi_bus == NULL) {
        return -1;
    }
    cfg->spi_bus_cfg = new_spi_bus;
    audio_board_spi_cfg_t *spi_cfg = &new_spi_bus[cfg->spi_bus_num];
    clear_spi_cfg(spi_cfg);
    cfg->spi_bus_num++;
    while (attr) {
        if (str_same(attr->attr, "mosi")) {
            spi_cfg->mosi_pin = atoi(attr->value);
        } else if (str_same(attr->attr, "sclk")) {
            spi_cfg->sclk_pin = atoi(attr->value);
        } else if (str_same(attr->attr, "miso")) {
            spi_cfg->miso_pin = atoi(attr->value);
        }
        attr = attr->next;
    }
    return 0;
}

static void clear_codec_cfg(audio_board_codec_cfg_t *codec_cfg)
{
    memset(codec_cfg, 0, sizeof(audio_board_codec_cfg_t));
    codec_cfg->ctrl_media = AUDIO_BOARD_MEDIA_I2C;
    codec_cfg->data_media = AUDIO_BOARD_MEDIA_I2S;
    codec_cfg->io_cfg.reset_pin = -1;
    codec_cfg->io_cfg.pa_pin = -1;
}

static audio_board_codec_type_t get_codec_type(const char *type)
{
    // TODO add other types
    if (str_same(type, "ES8311")) {
        return AUDIO_BOARD_CODEC_ES8311;
    }
    if (str_same(type, "ES7210")) {
        return AUDIO_BOARD_CODEC_ES7210;
    }
    if (str_same(type, "ES8388")) {
        return AUDIO_BOARD_CODEC_ES8388;
    }
    if (str_same(type, "ES7243")) {
        return AUDIO_BOARD_CODEC_ES7243;
    }
    if (str_same(type, "ES7243E")) {
        return AUDIO_BOARD_CODEC_ES7243E;
    }
    if (str_same(type, "ES8374")) {
        return AUDIO_BOARD_CODEC_ES8374;
    }
    if (str_same(type, "ES8156")) {
        return AUDIO_BOARD_CODEC_ES8156;
    }
    if (str_same(type, "TAS5805M")) {
        return AUDIO_BOARD_CODEC_TAS5805M;
    }
    if (str_same(type, "ZL38063")) {
        return AUDIO_BOARD_CODEC_ZL38063;
    }
    return AUDIO_BOARD_CODEC_NONE;
}

static void reset_i2c_dev_cfg(codec_i2c_dev_cfg_t *i2c_cfg)
{
    i2c_cfg->port = 0;
    i2c_cfg->addr = 0;
}

static codec_i2c_dev_cfg_t *codec_check_i2c_ready(audio_board_cfg_t *cfg, int port)
{
    int need_num = port + 1;
    if (need_num < cfg->i2c_dev_num) {
        return &cfg->i2c_dev_cfg[port];
    }
    int size = need_num * sizeof(codec_i2c_dev_cfg_t);
    codec_i2c_dev_cfg_t *i2c_cfg = realloc(cfg->i2c_dev_cfg, size);
    if (i2c_cfg == NULL) {
        return NULL;
    }
    for (int i = cfg->i2c_dev_num; i < need_num; i++) {
        reset_i2c_dev_cfg(&i2c_cfg[i]);
    }
    // port not set for multi-codec share the same i2c driver port
    cfg->i2c_dev_num = need_num;
    cfg->i2c_dev_cfg = i2c_cfg;
    return &i2c_cfg[port];
}

static void reset_i2s_dev_cfg(codec_i2s_dev_cfg_t *i2s_cfg)
{
    i2s_cfg->port = 0;
}

static codec_i2s_dev_cfg_t *codec_check_i2s_ready(audio_board_cfg_t *cfg, int port)
{
    int need_num = port + 1;
    if (need_num < cfg->i2s_dev_num) {
        return &cfg->i2s_dev_cfg[port];
    }
    int size = need_num * sizeof(codec_i2s_dev_cfg_t);
    codec_i2s_dev_cfg_t *i2s_cfg = realloc(cfg->i2s_dev_cfg, size);
    if (i2s_cfg == NULL) {
        return NULL;
    }
    for (int i = cfg->i2s_dev_num; i < need_num; i++) {
        reset_i2s_dev_cfg(&i2s_cfg[i]);
    }
    // port not set for multi-codec share the same i2s driver port
    cfg->i2s_dev_num = need_num;
    cfg->i2s_dev_cfg = i2s_cfg;
    return &i2s_cfg[port];
}

static void reset_spi_dev_cfg(codec_spi_dev_cfg_t *spi_cfg)
{
    spi_cfg->cs_pin = -1;
    spi_cfg->spi_port = 0;
}

static codec_spi_dev_cfg_t *codec_check_spi_ready(audio_board_cfg_t *cfg, int port)
{
    int need_num = port + 1;
    if (need_num < cfg->spi_dev_num) {
        return &cfg->spi_dev_cfg[port];
    }
    int size = need_num * sizeof(codec_spi_dev_cfg_t);
    codec_spi_dev_cfg_t *spi_cfg = realloc(cfg->spi_dev_cfg, size);
    if (spi_cfg == NULL) {
        return NULL;
    }
    for (int i = cfg->spi_dev_num; i < need_num; i++) {
        reset_spi_dev_cfg(&spi_cfg[i]);
    }
    cfg->spi_dev_num = need_num;
    cfg->spi_dev_cfg = spi_cfg;
    spi_cfg[port].spi_port = port;
    return &spi_cfg[port];
}

static int get_ctrl_media_idx(audio_board_cfg_t *cfg, audio_board_media_t media)
{
    int idx = 0;
    for (int i = 0; i < cfg->codec_num; i++) {
        if (cfg->codec_cfg[i].ctrl_media == media) {
            idx++;
        }
    }
    return idx;
}

static int get_data_media_idx(audio_board_cfg_t *cfg, audio_board_media_t media)
{
    int idx = 0;
    for (int i = 0; i < cfg->codec_num; i++) {
        if (cfg->codec_cfg[i].data_media == media) {
            idx++;
        }
    }
    return idx;
}

static int fill_codec_cfg(audio_board_cfg_t *cfg, board_cfg_attr_t *attr, codec_work_mode_t codec_mode)
{
    int size = (cfg->codec_num + 1) * sizeof(audio_board_codec_cfg_t);
    audio_board_codec_cfg_t *new_codec_cfg = realloc(cfg->codec_cfg, size);
    if (new_codec_cfg == NULL) {
        return -1;
    }
    cfg->codec_cfg = new_codec_cfg;
    audio_board_codec_cfg_t *codec_cfg = &new_codec_cfg[cfg->codec_num];
    clear_codec_cfg(codec_cfg);
    codec_cfg->io_cfg.codec_mode = codec_mode;

    // set i2c default control port
    if (codec_cfg->ctrl_media == AUDIO_BOARD_MEDIA_I2C) {
        int port = get_ctrl_media_idx(cfg, AUDIO_BOARD_MEDIA_I2C);
        codec_cfg->ctrl_port = port;
    }
    if (codec_cfg->data_media == AUDIO_BOARD_MEDIA_I2S) {
        int port = get_data_media_idx(cfg, AUDIO_BOARD_MEDIA_I2S);
        codec_cfg->data_port = port;
    }
    while (attr) {
        if (str_same(attr->attr, "type")) {
            codec_cfg->codec_type = get_codec_type(attr->value);
        } else if (str_same(attr->attr, "reset")) {
            codec_cfg->io_cfg.reset_pin = (int16_t) atoi(attr->value);
        } else if (str_same(attr->attr, "pa")) {
            codec_cfg->io_cfg.pa_pin = (int16_t) atoi(attr->value);
        } else if (str_same(attr->attr, "pa_gain")) {
            codec_cfg->pa_gain = atof(attr->value);
        } else if (str_same(attr->attr, "spi_port")) {
            int port = get_ctrl_media_idx(cfg, AUDIO_BOARD_MEDIA_SPI);
            codec_spi_dev_cfg_t *spi_dev = codec_check_spi_ready(cfg, port);
            if (spi_dev) {
                codec_cfg->ctrl_media = AUDIO_BOARD_MEDIA_SPI;
                codec_cfg->ctrl_port = port;
                port = atoi(attr->value);
                spi_dev->spi_port = port;
            } else {
                ESP_LOGE(TAG, "Fail to alloc spi dev memory");
            }
        } else if (str_same(attr->attr, "i2s_port")) {
            codec_i2s_dev_cfg_t *i2s_dev = codec_check_i2s_ready(cfg, codec_cfg->data_port);
            if (i2s_dev) {
                i2s_dev->port = atoi(attr->value);
            } else {
                ESP_LOGE(TAG, "Fail to alloc i2s dev memory");
            }
        } else if (str_same(attr->attr, "cs")) {
            if (codec_cfg->ctrl_media == AUDIO_BOARD_MEDIA_SPI) {
                codec_spi_dev_cfg_t *spi_dev = codec_check_spi_ready(cfg, codec_cfg->ctrl_port);
                if (spi_dev) {
                    spi_dev->cs_pin = atoi(attr->value);
                }
            }
        } else if (str_same(attr->attr, "i2c_port")) {
            if (codec_cfg->ctrl_media == AUDIO_BOARD_MEDIA_I2C) {
                codec_i2c_dev_cfg_t *i2c_dev = codec_check_i2c_ready(cfg, codec_cfg->ctrl_port);
                if (i2c_dev) {
                    // set driver port
                    i2c_dev->port = atoi(attr->value);
                }
            }
        } else if (str_same(attr->attr, "i2c_addr")) {
            if (codec_cfg->ctrl_media == AUDIO_BOARD_MEDIA_I2C) {
                codec_i2c_dev_cfg_t *i2c_dev = codec_check_i2c_ready(cfg, codec_cfg->ctrl_port);
                if (i2c_dev) {
                    i2c_dev->addr = atoi(attr->value);
                }
            }
        }
        attr = attr->next;
    }
    // alloc default dev for i2c
    if (codec_cfg->ctrl_media == AUDIO_BOARD_MEDIA_I2C) {
        codec_check_i2c_ready(cfg, codec_cfg->ctrl_port);
    }
    // alloc default dev for i2s
    if (codec_cfg->data_media == AUDIO_BOARD_MEDIA_I2S) {
        codec_check_i2s_ready(cfg, codec_cfg->data_port);
    }
    // increase codec num
    cfg->codec_num++;
    return 0;
}

static int fill_sdcard_cfg(audio_board_cfg_t *cfg, board_cfg_attr_t *attr)
{
    audio_board_sdcard_cfg_t *sdcard_cfg = &cfg->sdcard_cfg;
    while (attr) {
        if (str_same(attr->attr, "power")) {
            sdcard_cfg->power_pin = atoi(attr->value);
        }
        attr = attr->next;
    }
    return 0;
}

static int fill_key_cfg(audio_board_cfg_t *cfg, board_cfg_attr_t *attr)
{
    audio_board_key_cfg_t *key_cfg = &cfg->key_cfg;
    while (attr) {
        if (str_same(attr->attr, "vol_up")) {
            key_cfg->vol_up_voltage = atoi(attr->value);
        } else if (str_same(attr->attr, "vol_down")) {
            key_cfg->vol_down_voltage = atoi(attr->value);
        } else if (str_same(attr->attr, "adc_channel")) {
            key_cfg->adc_channel = atoi(attr->value);
        }
        attr = attr->next;
    }
    return 0;
}

static int fill_cfg(audio_board_cfg_t *cfg, board_cfg_line_t *cfg_line)
{
    board_cfg_attr_t *attr = cfg_line->attr;
    if (str_same(cfg_line->type, "i2c")) {
        if (fill_i2c_cfg(cfg, attr) != 0) {
            return -1;
        }
    } else if (str_same(cfg_line->type, "i2s")) {
        if (fill_i2s_cfg(cfg, attr) != 0) {
            return -1;
        }
    } else if (str_same(cfg_line->type, "spi")) {
        if (fill_spi_cfg(cfg, attr) != 0) {
            return -1;
        }
    } else if (str_same(cfg_line->type, "play_record")) {
        if (fill_codec_cfg(cfg, attr, CODEC_WORK_MODE_BOTH) != 0) {
            return -1;
        }
    } else if (str_same(cfg_line->type, "play")) {
        if (fill_codec_cfg(cfg, attr, CODEC_WORK_MODE_DAC) != 0) {
            return -1;
        }
    } else if (str_same(cfg_line->type, "record")) {
        if (fill_codec_cfg(cfg, attr, CODEC_WORK_MODE_ADC) != 0) {
            return -1;
        }
    } else if (str_same(cfg_line->type, "sdcard")) {
        if (fill_sdcard_cfg(cfg, attr) != 0) {
            return -1;
        }
    } else if (str_same(cfg_line->type, "key")) {
        if (fill_key_cfg(cfg, attr) != 0) {
            return -1;
        }
    }
    return 0;
}

static int parse_cfg(const char *section, int size, audio_board_cfg_t *cfg)
{
    int consume = 0;
    while (1) {
        board_cfg_line_t *cfg_line = parse_section(section, size, &consume);
        if (cfg_line == NULL) {
            break;
        }
        size -= consume;
        section += consume;
        print_cfg_line(cfg_line);
        int ret = fill_cfg(cfg, cfg_line);
        free_cfg_line(cfg_line);
        if (ret != 0) {
            return ret;
        }
    }
    return 0;
}

audio_board_cfg_t *audio_board_get_cfg(const char *board_name)
{
    audio_board_cfg_t *cfg = calloc(1, sizeof(audio_board_cfg_t));
    if (cfg == NULL) {
        return NULL;
    }
    int cfg_size = board_cfg_end - board_cfg_start;
    do {
        const char *section = get_section_data(board_cfg_start, cfg_size, board_name);
        if (section == NULL) {
            break;
        }
        int left_size = cfg_size - (section - board_cfg_start);
        if (parse_cfg(section, left_size, cfg) != 0) {
            break;
        }
        return cfg;
    } while (0);
    audio_board_free_cfg(cfg);
    return NULL;
}

void audio_board_free_cfg(audio_board_cfg_t *cfg)
{
    SAFE_FREE(cfg->codec_cfg);
    SAFE_FREE(cfg->i2c_bus_cfg);
    SAFE_FREE(cfg->i2s_bus_cfg);
    SAFE_FREE(cfg->spi_bus_cfg);
    SAFE_FREE(cfg->i2c_dev_cfg);
    SAFE_FREE(cfg->i2s_dev_cfg);
    SAFE_FREE(cfg->spi_dev_cfg);
    free(cfg);
}

static int _i2c_install(int port, audio_board_i2c_cfg_t *cfg)
{
    i2c_config_t i2c_cfg = {
        .mode = cfg->master_mode ? I2C_MODE_MASTER : I2C_MODE_SLAVE,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_cfg.sda_io_num = cfg->sda_pin;
    i2c_cfg.scl_io_num = cfg->scl_pin;
    esp_err_t ret = i2c_param_config(port, &i2c_cfg);
    if (ret != ESP_OK) {
        return -1;
    }
    ret = i2c_driver_install(port, i2c_cfg.mode, ESP_I2C_MASTER_BUF_LEN, ESP_I2C_MASTER_BUF_LEN, ESP_INTR_FLG_DEFAULT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fail to Install I2C driver for port %d", port);
        return -1;
    }
    ESP_LOGI(TAG, "Install I2C driver for port %d OK", port);
    return 0;
}

static int _i2c_uninstall(int port)
{
    i2c_driver_delete(port);
    return 0;
}

static int _i2s_install(uint8_t dev_port, audio_board_i2s_cfg_t *i2s_pin)
{
    i2s_config_t i2s_config = DEFAULT_I2S_CFG;
#if (ESP_IDF_VERSION_MAJOR >= 4) && (ESP_IDF_VERSION_MINOR > 2)
    i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
#endif
    if (i2s_pin->master_mode || i2s_pin->dac_mode) {
        i2s_config.mode |= I2S_MODE_MASTER;
    }
    if (i2s_pin->dac_mode) {
#if SOC_I2S_SUPPORTS_ADC_DAC
        i2s_config.mode |= I2S_MODE_DAC_BUILT_IN;
        return -1;
#endif
    }
    esp_err_t ret = i2s_driver_install(dev_port, &i2s_config, 0, NULL);
    if (ret != 0) {
        ESP_LOGE(TAG, "Fail to install I2S driver for %d OK", dev_port);
        return -1;
    }
    ESP_LOGI(TAG, "Install I2S driver for %d OK", dev_port);
#if SOC_I2S_SUPPORTS_ADC_DAC
    if (i2s_pin->dac_mode) {
        if (i2s_pin->dac_mode) {
            i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
        }
        i2s_set_pin(dev_port, NULL);
    } else
#endif
    {
        i2s_pin_config_t i2s_pin_cfg = {
#if (ESP_IDF_VERSION_MAJOR >= 4) && (ESP_IDF_VERSION_MINOR > 2)
            .mck_io_num = i2s_pin->mck_pin,
#endif
            .bck_io_num = i2s_pin->bck_pin,
            .ws_io_num = i2s_pin->ws_pin,
            .data_out_num = i2s_pin->data_out_pin,
            .data_in_num = i2s_pin->data_in_pin,
        };
        i2s_set_pin(dev_port, &i2s_pin_cfg);
    }
    return 0;
}

static int _i2s_uninstall(uint8_t dev_port)
{
    i2s_driver_uninstall(dev_port);
    return 0;
}

static int _spi_install(audio_board_spi_cfg_t *spi_cfg)
{
    esp_err_t ret;
    spi_bus_config_t bus_cfg = {0};
    bus_cfg.mosi_io_num = spi_cfg->mosi_pin;
    bus_cfg.miso_io_num = spi_cfg->miso_pin;
    bus_cfg.sclk_io_num = spi_cfg->sclk_pin;
    bus_cfg.quadwp_io_num = spi_cfg->quadwp_pin;
    bus_cfg.quadhd_io_num = spi_cfg->quadhd_pin;
#if (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0))
    ret = spi_bus_initialize(HSPI_HOST, &bus_cfg, 0);
#else
    ret = spi_bus_initialize(SPI3_HOST, &bus_cfg, 0);
    ESP_LOGI(TAG, "Add bus mosi:%d miso:%d sclk:%d quadw:%d quadh:%d sz:%d\n", bus_cfg.mosi_io_num, bus_cfg.miso_io_num,
           bus_cfg.sclk_io_num, bus_cfg.quadwp_io_num, bus_cfg.quadhd_io_num, bus_cfg.max_transfer_sz);
#endif
    ESP_LOGI(TAG, "Initial spi bus return %d", ret);
    return ret;
}

static int _spi_uninstall()
{
    int ret;
#if (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0))
    ret = spi_bus_free(HSPI_HOST);
#else
    ret = spi_bus_free(SPI3_HOST);
#endif
    return ret;
}

int audio_board_install_device(audio_board_cfg_t *cfg)
{
    // install i2c driver
    int ret = 0;
    for (int i = 0; i < cfg->i2c_bus_num; i++) {
        ret = _i2c_install(i, cfg->i2c_bus_cfg + i);
        if (ret != 0) {
            ESP_LOGE(TAG, "Fail to install i2s driver for %d", i);
            return ret;
        }
    }
    // install i2s driver
    for (int i = 0; i < cfg->i2s_bus_num; i++) {
        ret = _i2s_install(i, cfg->i2s_bus_cfg + i);
        if (ret != 0) {
            ESP_LOGE(TAG, "Fail to install i2s driver for %d", i);
            return ret;
        }
    }
    // install spi driver
    // TODO currently only support one SPI
    if (cfg->spi_bus_num == 1) {
        ret = _spi_install(cfg->spi_bus_cfg);
        if (ret != 0) {
            ESP_LOGE(TAG, "Fail to install spi driver");
            return ret;
        }
    }
    return ret;
}

int audio_board_uninstall_device(audio_board_cfg_t *cfg)
{
    // un-install i2c driver
    for (int i = 0; i < cfg->i2c_bus_num; i++) {
        _i2c_uninstall(i);
    }
    // install i2s driver
    for (int i = 0; i < cfg->i2s_bus_num; i++) {
        _i2s_uninstall(i);
    }
    // install spi driver
    // TODO currently only support one SPI
    if (cfg->spi_bus_num == 1) {
        _spi_uninstall();
    }
    return 0;
}
