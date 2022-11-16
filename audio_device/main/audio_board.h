#include "codec_dev_defaults.h"
#include "codec_dev_types.h"

typedef struct {
    bool    master_mode;
    int16_t scl_pin;
    int16_t sda_pin;
} audio_board_i2c_cfg_t;

typedef struct {
    bool    master_mode;
    int16_t bck_pin;
    int16_t ws_pin;
    int16_t data_out_pin;
    int16_t data_in_pin;
    int16_t mck_pin;
    bool    dac_mode;
} audio_board_i2s_cfg_t;

typedef struct {
    bool    master_mode;
    int16_t mosi_pin;
    int16_t miso_pin;
    int16_t sclk_pin;
    int16_t quadwp_pin;
    int16_t quadhd_pin;
} audio_board_spi_cfg_t;

typedef enum {
    AUDIO_BOARD_MEDIA_I2C,
    AUDIO_BOARD_MEDIA_I2S,
    AUDIO_BOARD_MEDIA_SPI,
} audio_board_media_t;

typedef enum {
    AUDIO_BOARD_CODEC_NONE,
    AUDIO_BOARD_CODEC_ES8311,
    AUDIO_BOARD_CODEC_ES7210,
    AUDIO_BOARD_CODEC_ES7243,
    AUDIO_BOARD_CODEC_ES7243E,
    AUDIO_BOARD_CODEC_ES8374,
    AUDIO_BOARD_CODEC_ES8388,
    AUDIO_BOARD_CODEC_ES8156,
    AUDIO_BOARD_CODEC_TAS5805M,
    AUDIO_BOARD_CODEC_ZL38063,
} audio_board_codec_type_t;

typedef struct {
    bool              master_mode;
    codec_work_mode_t codec_mode;
    bool              use_mclk;
    bool              revert_mclk;
    int16_t           reset_pin;
    int16_t           pa_pin;
    uint16_t          mic_channel_sel;
} audio_board_codec_io_cfg_t;

typedef struct {
    audio_board_codec_type_t   codec_type;
    audio_board_media_t        ctrl_media;
    audio_board_media_t        data_media;
    uint8_t                    ctrl_port;
    uint8_t                    data_port;
    audio_board_codec_io_cfg_t io_cfg;
    float                      pa_gain;
} audio_board_codec_cfg_t;

typedef struct {
    int16_t power_pin;
} audio_board_sdcard_cfg_t;

typedef struct {
    int vol_up_voltage;
    int vol_down_voltage;
    int adc_channel;
} audio_board_key_cfg_t;

typedef struct {
    uint8_t                  i2c_bus_num;
    uint8_t                  i2s_bus_num;
    uint8_t                  spi_bus_num;
    audio_board_i2c_cfg_t   *i2c_bus_cfg;
    audio_board_spi_cfg_t   *spi_bus_cfg;
    audio_board_i2s_cfg_t   *i2s_bus_cfg;
    uint8_t                  i2c_dev_num;
    uint8_t                  i2s_dev_num;
    uint8_t                  spi_dev_num;
    uint8_t                  codec_num;
    codec_i2s_dev_cfg_t     *i2s_dev_cfg;
    codec_i2c_dev_cfg_t     *i2c_dev_cfg;
    codec_spi_dev_cfg_t     *spi_dev_cfg;
    audio_board_codec_cfg_t *codec_cfg;
    audio_board_sdcard_cfg_t sdcard_cfg;
    audio_board_key_cfg_t    key_cfg;
} audio_board_cfg_t;

audio_board_cfg_t *audio_board_get_cfg(const char *board_name);

int audio_board_install_device(audio_board_cfg_t *cfg);

int audio_board_uninstall_device(audio_board_cfg_t *cfg);

void audio_board_free_cfg(audio_board_cfg_t *cfg);
