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
#ifndef P7885_HI_GBM_INTERNAL_H
#define P7885_HI_GBM_INTERNAL_H

#include <cstdint>

#define DIV_ROUND_UP_GBM(n, d) (((n) + (d)-1) / (d))
#define ALIGN_UP_GBM(x, a) ((((x) + ((a)-1)) / (a)) * (a))
#define HEIGHT_ALIGN_GBM 2U
#define WIDTH_ALIGN_GBM 8U
#define MAX_PLANES_GBM 3

struct PlaneLayoutInfo {
    uint32_t numPlanes;
    uint32_t ratio[MAX_PLANES_GBM];
};

struct FormatInfo {
    uint32_t format;
    uint32_t bitsPerPixel;
    const PlaneLayoutInfo *planes;
};

const FormatInfo *GetFormatInfo(uint32_t format);

struct gbm_device {
    int fd;
};

struct gbm_bo {
    struct gbm_device *gbm;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t handle;
    uint32_t stride;
    uint32_t size;
};

#endif // P7885_HI_GBM_INTERNAL_H
