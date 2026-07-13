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
#ifndef P7885_HI_GBM_H
#define P7885_HI_GBM_H

#include <cstdint>

#if defined(__cplusplus)
extern "C" {
#endif

struct gbm_device;
struct gbm_bo;

enum gbm_bo_flags {
    GBM_BO_USE_SCANOUT = (1 << 0),
    GBM_BO_USE_CURSOR = (1 << 1),
    GBM_BO_USE_CURSOR_64X64 = GBM_BO_USE_CURSOR,
    GBM_BO_USE_RENDERING = (1 << 2),
    GBM_BO_USE_WRITE = (1 << 3),
    GBM_BO_USE_LINEAR = (1 << 4),
    GBM_BO_USE_TEXTURING = (1 << 5),
    GBM_BO_USE_CAMERA_WRITE = (1 << 6),
    GBM_BO_USE_CAMERA_READ = (1 << 7),
    GBM_BO_USE_PROTECTED = (1 << 8),
    GBM_BO_USE_SW_READ_OFTEN = (1 << 9),
    GBM_BO_USE_SW_READ_RARELY = (1 << 10),
    GBM_BO_USE_SW_WRITE_OFTEN = (1 << 11),
    GBM_BO_USE_SW_WRITE_RARELY = (1 << 12),
    GBM_BO_USE_HW_VIDEO_DECODER = (1 << 13),
};

struct gbm_device *hdi_gbm_create_device(int fd);
void hdi_gbm_device_destroy(struct gbm_device *gbm);
struct gbm_bo *hdi_gbm_bo_create(struct gbm_device *gbm, uint32_t width, uint32_t height, uint32_t format,
    uint32_t usage);
uint32_t hdi_gbm_bo_get_stride(struct gbm_bo *bo);
uint32_t hdi_gbm_bo_get_width(struct gbm_bo *bo);
uint32_t hdi_gbm_bo_get_height(struct gbm_bo *bo);
uint32_t hdi_gbm_bo_get_size(struct gbm_bo *bo);
void *hdi_gbm_bo_mmap(struct gbm_bo *bo);
void hdi_gbm_bo_destroy(struct gbm_bo *bo);
int hdi_gbm_bo_get_fd(struct gbm_bo *bo);

#if defined(__cplusplus)
}
#endif

#endif // P7885_HI_GBM_H
