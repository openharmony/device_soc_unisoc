/*
 * Copyright 2016-2026 Unisoc (Shanghai) Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WATERMARK_H_
#define _WATERMARK_H_
#include "cmr_common.h"

#define WATERMARK_LOGO (1 << 0)
#define WATERMARK_TIME (1 << 1)

typedef struct {
    int imgW;
    int imgH;
    int logoW;
    int logoH;
    int posX;
    int posY;
    int isMirror; /* !0: mirror */
    int angle;    /* angle,use:[0,90,180,270] */
    char *filename;
} sizeParam_t;

/* watermark_add_yuv
 *      add logo watermark and time watermark to yuv420 image
 * input: flag: bit0:1 add logo watermark, bit1:1 add time watermark
 *        pyuv: yuv420 image
 *        sizeparam: parameter for image
 *        time: for time logo, if time == 0, get from system
 * return: 0: not add logo or time; !0: add,for flush memory
 *         WATERMARK_LOGO: add logo
 *         WATERMARK_TIME: add time
 */
int watermark_add_yuv(int flag, unsigned char *pyuv, sizeParam_t *sizeparam,
                      time_t cap_time);

#endif //_WATERMARK_H_
