# get IDF version for comparison
set(idf_version "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}")

# set conversion sources
set(COMPONENT_SRCS
  audio_device_if.c
  audio_codec_dev.c
  audio_codec_vol.c
  codec_dev_utils.c
)

set(COMPONENT_PRIV_INCLUDEDIRS "internal" "driver/zl38063/api_lib" "driver/zl38063/firmware")

set(COMPONENT_ADD_INCLUDEDIRS include interface driver/include)

set(COMPONENT_REQUIRES driver)

# set driver sources only for supported platforms
list(APPEND COMPONENT_SRCS
  driver/es8156/es8156.c
  driver/es7243e/es7243e.c
  driver/es8311/es8311.c
  driver/es7210/es7210.c
  driver/es7243/es7243.c
  driver/es8388/es8388.c
  driver/tas5805m/tas5805m.c
  driver/es8374/es8374.c
  driver/zl38063/zl38063.c
  driver/zl38063/api_lib/vprocTwolf_access.c
  driver/zl38063/api_lib/vproc_common.c
  driver/zl38063/example_apps/tw_hal_verify.c
  driver/zl38063/example_apps/tw_ldcfg.c
  driver/zl38063/example_apps/tw_ldfw.c
  driver/zl38063/example_apps/tw_ldfwcfg.c
  driver/zl38063/example_apps/tw_spi_access.c
)

list(APPEND COMPONENT_SRCS
    platform/audio_gpio_if.c
    platform/audio_i2c_if.c
    platform/audio_i2s_if.c
    platform/audio_spi_if.c
    platform/audio_device_os.c
)

set(COMPONENT_PRIV_REQUIRES freertos)

register_component()

IF (NOT (CONFIG_IDF_TARGET STREQUAL "esp32c3"))
target_link_libraries(${COMPONENT_TARGET} INTERFACE "-L${CMAKE_CURRENT_LIST_DIR}/driver/zl38063/firmware")
target_link_libraries(${COMPONENT_TARGET} INTERFACE firmware)
ENDIF()

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Werror=all 
-Werror=incompatible-pointer-types
-Werror=discarded-qualifiers
-Werror=int-conversion
-Werror=deprecated-declarations -Werror=unused-variable -Werror=unused-but-set-variable -Werror=unused-function)
