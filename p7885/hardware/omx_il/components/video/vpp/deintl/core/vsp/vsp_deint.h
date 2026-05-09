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
/******************************************************************************
 ** File Name:    vsp_deint.h                                                   *
 ** Author: *
 ** DATE:         5/4/2017                                                   *
 ** Copyright:    2019 Unisoc, Incorporated. All Rights Reserved.         *
 ** Description:  define data structures for vsp deint                      *
 *****************************************************************************/
#ifndef VSP_DEINT_H_
#define VSP_DEINT_H_
#include <cstring>
#define IMG_WIDTH 176
#define IMG_HEIGHT 144
#define INTERLEAVE 1
#define THRESHOLD 20
#define MAX_DECODE_FRAME_NUM 25
#define PUBLIC
typedef unsigned char BOOLEAN;
typedef unsigned char uint8;
typedef signed char int8;
typedef signed short int16;
typedef unsigned short uint16;
typedef signed int int32;
typedef unsigned int uint32;
typedef unsigned long        uint32or64;
typedef struct DeintParams {
    uint32 width;
    uint32 height;
    uint8 interleave;
    uint8 threshold;
    uint32 yLen;
    uint32 cLen;
} DEINT_PARAMS_T;
typedef enum {
    SRC_FRAME_ADDR = 0,
    REF_FRAME_ADDR,
    DST_FRAME_ADDR,
    MAX_FRAME_NUM
} DEINT_BUF_TYPE;
#endif
