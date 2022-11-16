
set(COMPONENT_SRCS render.c board_cfg_parse.c audio_board.c)
set(COMPONENT_ADD_INCLUDEDIRS "")
set(COMPONENT_EMBED_TXTFILES board_cfg.txt file_2_8000.pcm)
register_component()
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Werror=all 
-Werror=incompatible-pointer-types
-Werror=discarded-qualifiers
-Werror=deprecated-declarations -Werror=unused-variable -Werror=unused-but-set-variable -Werror=unused-function)


