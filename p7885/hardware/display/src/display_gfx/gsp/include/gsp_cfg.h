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

#ifndef UAPI_VIDEO_GSP_CFG_H
#define UAPI_VIDEO_GSP_CFG_H

#include <linux/ioctl.h>
#include <linux/types.h>
#include <asm/ioctl.h>
#include <stdbool.h>
#include "drm.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {

/* Primary IO control definitions */
#define UISOC_GSP_IO_MAGIC          ('G')
#define UISOC_GSP_IO_SHIFT          (5)
#define UISOC_GSP_CAP_SHIFT         (6)
#define UISOC_GSP_TRIG_SHIFT        (5)
#define UISOC_GSP_ASYNC_SHIFT       (4)
#define UISOC_GSP_SPLIT_SHIFT       (3)
#define UISOC_GSP_CNT_SHIFT         (0)

#define UISOC_GSP_GET_CAP           (0x1 << UISOC_GSP_CAP_SHIFT)
#define UISOC_GSP_TRIGGER           (0x1 << UISOC_GSP_TRIG_SHIFT)
#define UISOC_GSP_IO_MASK           (0x7 << UISOC_GSP_IO_SHIFT)
#define UISOC_GSP_ASYNC_MASK        (0x1 << UISOC_GSP_ASYNC_SHIFT)
#define UISOC_GSP_SPLIT_MASK        (0x1 << UISOC_GSP_SPLIT_SHIFT)
#define UISOC_GSP_CNT_MASK          (0x7 << UISOC_GSP_CNT_SHIFT)

#define UISOC_GSP_CAP_MAGIC         0xDEEFBEEF

enum UisocLayerType {
    UISOC_LYR_IMG,
    UISOC_LYR_OSD,
    UISOC_LYR_DES,
    UISOC_LYR_INVAL
};

enum UisocAddrType {
    UISOC_ADDR_INVAL,
    UISOC_ADDR_PHYS,
    UISOC_ADDR_IOVA,
    UISOC_ADDR_MAX,
};

enum UisocIrqMode {
    UISOC_IRQ_PULSE = 0x00,
    UISOC_IRQ_LEVEL,
    UISOC_IRQ_INVALID,
};

enum UisocIrqType {
    UISOC_IRQ_DISABLE = 0x00,
    UISOC_IRQ_ENABLE,
    UISOC_IRQ_TYPE_INVALID,
};

enum UisocRotation {
    UISOC_ROT_0 = 0x00,
    UISOC_ROT_90,
    UISOC_ROT_180,
    UISOC_ROT_270,
    UISOC_ROT_FLIP_H,
    UISOC_ROT_FLIP_V,
    UISOC_ROT_180_FLIP,
    UISOC_ROT_270_FLIP,
    UISOC_ROT_MAX,
};

struct UisocColor {
    __u8 b;
    __u8 g;
    __u8 r;
    __u8 a;
};

struct UisocPt {
    __u16 x;
    __u16 y;
};

struct UisocRect {
    __u16 x;
    __u16 y;
    __u16 w;
    __u16 h;
};

struct UisocAddr {
    __u32 y;
    __u32 uv;
    __u32 va;
};

struct UisocOff {
    __u32 uv;
    __u32 v;
};

struct UisocYuvAdj {
    __u32 brightness;
    __u32 contrast;
    __u32 uOff;
    __u32 uSat;
    __u32 vOff;
    __u32 vSat;
};

struct UisocBgPara {
    __u32 enable;
    __u32 mode;
    struct UisocColor color;
};

struct UisocScalePara {
    __u32 enable;
    __u32 hTap;
    __u32 vTap;
    struct UisocRect in;
    struct UisocRect out;
};

struct UisocLayerCfg {
    __u32 type;
    __u32 enable;
    __s32 fdShare;
    __s32 fdWait;
    __s32 fdSync;
    struct UisocAddr addr;
    struct UisocOff off;
};

struct UisocCapa {
    __u32 magic;
    char ver[32];
    __u32 size;
    __u32 ioCnt;
    __u32 coreCnt;
    __u32 lyrMax;
    __u32 lyrImgMax;
    struct UisocRect cropMax;
    struct UisocRect cropMin;
    struct UisocRect outMax;
    struct UisocRect outMin;
    __u32 memType;
};

} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS

#endif /* UAPI_VIDEO_GSP_CFG_H */
