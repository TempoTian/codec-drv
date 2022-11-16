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

#ifndef __ES8388_REG__
#define __ES8388_REG__

#ifdef __cplusplus
extern "C" {
#endif

/* ES8388 address */
#define ES8388_ADDR         0x20 /*!< 0x22:CE=1;0x20:CE=0*/

/* ES8388 register */
#define ES8388_CONTROL1     0x00
#define ES8388_CONTROL2     0x01

#define ES8388_CHIPPOWER    0x02

#define ES8388_ADCPOWER     0x03
#define ES8388_DACPOWER     0x04

#define ES8388_CHIPLOPOW1   0x05
#define ES8388_CHIPLOPOW2   0x06

#define ES8388_ANAVOLMANAG  0x07

#define ES8388_MASTERMODE   0x08
/* ADC */
#define ES8388_ADCCONTROL1  0x09
#define ES8388_ADCCONTROL2  0x0a
#define ES8388_ADCCONTROL3  0x0b
#define ES8388_ADCCONTROL4  0x0c
#define ES8388_ADCCONTROL5  0x0d
#define ES8388_ADCCONTROL6  0x0e
#define ES8388_ADCCONTROL7  0x0f
#define ES8388_ADCCONTROL8  0x10
#define ES8388_ADCCONTROL9  0x11
#define ES8388_ADCCONTROL10 0x12
#define ES8388_ADCCONTROL11 0x13
#define ES8388_ADCCONTROL12 0x14
#define ES8388_ADCCONTROL13 0x15
#define ES8388_ADCCONTROL14 0x16
/* DAC */
#define ES8388_DACCONTROL1  0x17
#define ES8388_DACCONTROL2  0x18
#define ES8388_DACCONTROL3  0x19
#define ES8388_DACCONTROL4  0x1a
#define ES8388_DACCONTROL5  0x1b
#define ES8388_DACCONTROL6  0x1c
#define ES8388_DACCONTROL7  0x1d
#define ES8388_DACCONTROL8  0x1e
#define ES8388_DACCONTROL9  0x1f
#define ES8388_DACCONTROL10 0x20
#define ES8388_DACCONTROL11 0x21
#define ES8388_DACCONTROL12 0x22
#define ES8388_DACCONTROL13 0x23
#define ES8388_DACCONTROL14 0x24
#define ES8388_DACCONTROL15 0x25
#define ES8388_DACCONTROL16 0x26
#define ES8388_DACCONTROL17 0x27
#define ES8388_DACCONTROL18 0x28
#define ES8388_DACCONTROL19 0x29
#define ES8388_DACCONTROL20 0x2a
#define ES8388_DACCONTROL21 0x2b
#define ES8388_DACCONTROL22 0x2c
#define ES8388_DACCONTROL23 0x2d
#define ES8388_DACCONTROL24 0x2e
#define ES8388_DACCONTROL25 0x2f
#define ES8388_DACCONTROL26 0x30
#define ES8388_DACCONTROL27 0x31
#define ES8388_DACCONTROL28 0x32
#define ES8388_DACCONTROL29 0x33
#define ES8388_DACCONTROL30 0x34

#ifdef __cplusplus
}
#endif

#endif //__ES8388_H__
