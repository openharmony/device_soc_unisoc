/*
 * Copyright (C) 2023 HiHope Open Source Organization.
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

#ifndef _UAPI_VIDEO_GSP_CFG_H
#define _UAPI_VIDEO_GSP_CFG_H

#include <linux/ioctl.h>
#include <linux/types.h>
#include <asm/ioctl.h>
#include <stdbool.h>
#include "drm.h"

/* define ioctl code for gsp */
#define GSP_IO_MAGIC  ('G')

#define GSP_IO_SHIFT  (5)

#define GSP_GET_CAPABILITY_SHIFT  (6)
#define GSP_TRIGGER_SHIFT  (5)
#define GSP_ASYNC_SHIFT  (4)
#define GSP_SPLIT_SHIFT  (3)
#define GSP_CNT_SHIFT  (0)

/*
 * _IO_NR() has 8 bits
 * bit7: MAX_NR indicate max validate code
 * bit6: indicate GSP_GET_CAPABILTY code
 * bit5: indicate GSP_TRIGGER code
 * bit4: indicate whether kcfgs are async
 * bit3: indicate whether kcfgs are split which
 *     respond to split-case
 * bit2-bit0: indicate how many kcfgs there are
 */
#define GSP_GET_CAPABILITY  (0x1 << GSP_GET_CAPABILITY_SHIFT)
#define GSP_TRIGGER  (0x1 << GSP_TRIGGER_SHIFT)
#define GSP_IO_MASK  (0x7 << GSP_IO_SHIFT)
#define GSP_ASYNC_MASK (0x1 << GSP_ASYNC_SHIFT)
#define GSP_SPLIT_MASK (0x1 << GSP_SPLIT_SHIFT)
#define GSP_CNT_MASK (0x7 << GSP_CNT_SHIFT)

#define GSP_CAPABILITY_MAGIC  0xDEEFBEEF

enum GspLayerType {
    GSP_IMG_LAYER,
    GSP_OSD_LAYER,
    GSP_DES_LAYER,
    GSP_INVAL_LAYER
};

/*the address type of gsp can process*/
enum GspAddrType {
    GSP_ADDR_TYPE_INVALUE,
    GSP_ADDR_TYPE_PHYSICAL,
    GSP_ADDR_TYPE_IOVIRTUAL,
    GSP_ADDR_TYPE_MAX,
};

enum GspIrqMod {
    GSP_IRQ_MODE_PULSE = 0x00,
    GSP_IRQ_MODE_LEVEL,
    GSP_IRQ_MODE_LEVEL_INVALID,
};

enum GspIrqType {
    GSP_IRQ_TYPE_DISABLE = 0x00,
    GSP_IRQ_TYPE_ENABLE,
    GSP_IRQ_TYPE_INVALID,
};

enum GspRotAngle {
    GSP_ROT_ANGLE_0 = 0x00,
    GSP_ROT_ANGLE_90,
    GSP_ROT_ANGLE_180,
    GSP_ROT_ANGLE_270,
    GSP_ROT_ANGLE_0_M,
    GSP_ROT_ANGLE_90_M,
    GSP_ROT_ANGLE_180_M,
    GSP_ROT_ANGLE_270_M,
    GSP_ROT_ANGLE_MAX_NUM,
};

struct GspRgb {
    __u8 bVal;
    __u8 gVal;
    __u8 rVal;
    __u8 aVal;
};

struct GspPos {
    __u16 ptX;
    __u16 ptY;
};

struct GspRect {
    __u16 stX;
    __u16 stY;
    __u16 rectW;
    __u16 rectH;
};

struct GspAddrData {
    __u32 addrY;
    __u32 addrUv;
    __u32 addrVa;
};

struct GspOffset {
    __u32 uvOffset;
    __u32 vOffset;
};

struct GspYuvAdjustPara {
    __u32 yBrightness;
    __u32 yContrast;
    __u32 uOffset;
    __u32 uSaturation;
    __u32 vOffset;
    __u32 vSaturation;
};

struct GspBackgroundPara {
    __u32 bkEnable;
    __u32 bkBlendMod;
    struct GspRgb backgroundRgb;
};

struct GspScalePara {
    __u32 scaleEn;
    __u32 htapMod;
    __u32 vtapMod;
    struct GspRect scaleRectIn;
    struct GspRect scaleRectOut;
};

/*
 * to distinguish struct from uapi gsp cfg header file
 * and no uapi gsp cfg header file. structure at uapi
 * header file has suffix "_user"
 */

struct GspLayerUser {
    __u32 type;
    __u32 enable;
    __s32 shareFd;
    __s32 waitFd;
    __s32 sigFd;
    struct GspAddrData srcAddr;
    struct GspOffset offset;
};

#define CAPABILITY_MAGIC_NUMBER 0xDEEFBEEF
struct GspCapability {
    /*used to indicate struct is initialized*/
    __u32 magic;
    char version[32];

    __u32 capaSize;
    __u32 ioCnt;
    __u32 coreCnt;

    __u32 maxLayer;
    __u32 maxImgLayer;

    struct GspRect cropMax;
    struct GspRect cropMin;
    struct GspRect outMax;
    struct GspRect outMin;

    /* GSP_ADDR_TYPE_PHYSICAL:phy addr
     * GSP_ADDR_TYPE_IOVIRTUAL:iova addr*/
    __u32 bufType;
};

#endif
