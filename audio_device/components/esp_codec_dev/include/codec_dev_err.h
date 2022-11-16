
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
#ifndef CODEC_DEV_ERR_H
#define CODEC_DEV_ERR_H

/**
 * @brief Define error number of codec device module
 */
#define CODEC_DEV_OK          (0)
#define CODEC_DEV_INVALID_ARG (-1)
#define CODEC_DEV_NO_MEM      (-2)
#define CODEC_DEV_NOT_SUPPORT (-3)
#define CODEC_DEV_NOT_FOUND   (-4)
#define CODEC_DEV_WRONG_STATE (-5)
#define CODEC_DEV_WRITE_FAIL  (-6)
#define CODEC_DEV_READ_FAIL   (-7)
#define CODEC_DEV_DRV_ERR     (-8)

#endif
