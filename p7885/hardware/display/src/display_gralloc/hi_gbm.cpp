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
#include "hi_gbm.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>

#include <drm_fourcc.h>
#include <securec.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "display_common.h"
#include "hi_gbm_internal.h"

namespace {
struct PlaneLayoutInfo {
    uint32_t numPlanes;
    uint32_t ratio[MAX_PLANES_GBM];
};

struct FormatInfo {
    uint32_t format;
    uint32_t bitsPerPixel;
    const PlaneLayoutInfo *planes;
};

const PlaneLayoutInfo G_YUV420SP_LAYOUT = {
    .numPlanes = 2,
    .ratio = {4, 2},
};

const PlaneLayoutInfo G_YUV420P_LAYOUT = {
    .numPlanes = 3,
    .ratio = {4, 1, 1},
};

const PlaneLayoutInfo G_YUV422SP_LAYOUT = {
    .numPlanes = 2,
    .ratio = {4, 4},
};

const PlaneLayoutInfo G_YUV422P_LAYOUT = {
    .numPlanes = 3,
    .ratio = {4, 2, 2},
};

const FormatInfo *GetFormatInfo(uint32_t format)
{
    static const FormatInfo FORMAT_INFOS[] = {
        {DRM_FORMAT_RGBX8888, 32, nullptr},
        {DRM_FORMAT_RGBA8888, 32, nullptr},
        {DRM_FORMAT_BGRX8888, 32, nullptr},
        {DRM_FORMAT_BGRA8888, 32, nullptr},
        {DRM_FORMAT_RGBA1010102, 32, nullptr},
        {DRM_FORMAT_RGB888, 24, nullptr},
        {DRM_FORMAT_RGB565, 16, nullptr},
        {DRM_FORMAT_BGR565, 16, nullptr},
        {DRM_FORMAT_BGRX4444, 16, nullptr},
        {DRM_FORMAT_BGRA4444, 16, nullptr},
        {DRM_FORMAT_RGBA4444, 16, nullptr},
        {DRM_FORMAT_RGBX4444, 16, nullptr},
        {DRM_FORMAT_BGRX5551, 16, nullptr},
        {DRM_FORMAT_BGRA5551, 16, nullptr},
        {DRM_FORMAT_RGBA5551, 16, nullptr},
        {DRM_FORMAT_RGBX5551, 16, nullptr},
        {DRM_FORMAT_NV12, 8, &G_YUV420SP_LAYOUT},
        {DRM_FORMAT_NV21, 8, &G_YUV420SP_LAYOUT},
        {DRM_FORMAT_NV16, 8, &G_YUV422SP_LAYOUT},
        {DRM_FORMAT_NV61, 8, &G_YUV422SP_LAYOUT},
        {DRM_FORMAT_YUV420, 8, &G_YUV420P_LAYOUT},
        {DRM_FORMAT_YVU420, 8, &G_YUV420P_LAYOUT},
        {DRM_FORMAT_YUV422, 8, &G_YUV422P_LAYOUT},
        {DRM_FORMAT_YVU422, 8, &G_YUV422P_LAYOUT},
    };

    for (size_t i = 0; i < sizeof(FORMAT_INFOS) / sizeof(FORMAT_INFOS[0]); i++) {
        if (FORMAT_INFOS[i].format == format) {
            return &FORMAT_INFOS[i];
        }
    }
    DISPLAY_LOGE("unsupported drm format 0x%{public}x", format);
    return nullptr;
}

void InitGbmBo(struct gbm_bo *bo, const drm_mode_create_dumb *dumb)
{
    DISPLAY_CHK_RETURN_NOT_VALUE((bo == nullptr || dumb == nullptr), DISPLAY_LOGE("gbm bo init param is null"));
    bo->stride = dumb->pitch;
    bo->size = static_cast<uint32_t>(dumb->size);
    bo->handle = dumb->handle;
}

uint32_t AdjustHeightFromFormat(uint32_t format, uint32_t height)
{
    const FormatInfo *formatInfo = GetFormatInfo(format);
    if ((formatInfo == nullptr) || (formatInfo->planes == nullptr)) {
        return height;
    }

    uint32_t sum = formatInfo->planes->ratio[0];
    for (uint32_t i = 1; i < formatInfo->planes->numPlanes && i < MAX_PLANES_GBM; i++) {
        sum += formatInfo->planes->ratio[i];
    }
    if (sum == 0) {
        return height;
    }
    return DIV_ROUND_UP_GBM(height * sum, formatInfo->planes->ratio[0]);
}
} // namespace

struct gbm_bo *hdi_gbm_bo_create(struct gbm_device *gbm, uint32_t width, uint32_t height, uint32_t format,
    uint32_t usage)
{
    DISPLAY_UNUSED(usage);
    DISPLAY_CHK_RETURN((gbm == nullptr), nullptr, DISPLAY_LOGE("gbm device is null"));
    const FormatInfo *formatInfo = GetFormatInfo(format);
    DISPLAY_CHK_RETURN((formatInfo == nullptr), nullptr, DISPLAY_LOGE("format 0x%{public}x unsupported", format));

    auto *bo = static_cast<gbm_bo *>(calloc(1, sizeof(gbm_bo)));
    DISPLAY_CHK_RETURN((bo == nullptr), nullptr, DISPLAY_LOGE("calloc gbm bo failed"));
    (void)memset_s(bo, sizeof(gbm_bo), 0, sizeof(gbm_bo));

    bo->width = width;
    bo->height = height;
    bo->gbm = gbm;
    bo->format = format;

    drm_mode_create_dumb dumb = {};
    dumb.height = ALIGN_UP_GBM(AdjustHeightFromFormat(format, height), HEIGHT_ALIGN_GBM);
    dumb.width = ALIGN_UP_GBM(width, WIDTH_ALIGN_GBM);
    dumb.bpp = formatInfo->bitsPerPixel;

    int ret = drmIoctl(gbm->fd, DRM_IOCTL_MODE_CREATE_DUMB, &dumb);
    if (ret != 0) {
        DISPLAY_LOGE("DRM_IOCTL_MODE_CREATE_DUMB failed, format 0x%{public}x errno %{public}d", format, errno);
        free(bo);
        return nullptr;
    }

    InitGbmBo(bo, &dumb);
    return bo;
}

struct gbm_device *hdi_gbm_create_device(int fd)
{
    auto *gbm = static_cast<gbm_device *>(calloc(1, sizeof(gbm_device)));
    DISPLAY_CHK_RETURN((gbm == nullptr), nullptr, DISPLAY_LOGE("calloc gbm device failed"));
    gbm->fd = fd;
    return gbm;
}

void hdi_gbm_device_destroy(struct gbm_device *gbm)
{
    free(gbm);
}

uint32_t hdi_gbm_bo_get_stride(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == nullptr), 0, DISPLAY_LOGE("gbm bo is null"));
    return bo->stride;
}

uint32_t hdi_gbm_bo_get_width(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == nullptr), 0, DISPLAY_LOGE("gbm bo is null"));
    return bo->width;
}

uint32_t hdi_gbm_bo_get_height(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == nullptr), 0, DISPLAY_LOGE("gbm bo is null"));
    return bo->height;
}

uint32_t hdi_gbm_bo_get_size(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == nullptr), 0, DISPLAY_LOGE("gbm bo is null"));
    return bo->size;
}

void *hdi_gbm_bo_mmap(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == nullptr), nullptr, DISPLAY_LOGE("gbm bo is null"));

    drm_mode_map_dumb mapDumb = {};
    mapDumb.handle = bo->handle;
    int ret = drmIoctl(bo->gbm->fd, DRM_IOCTL_MODE_MAP_DUMB, &mapDumb);
    DISPLAY_CHK_RETURN((ret != 0), nullptr,
        DISPLAY_LOGE("DRM_IOCTL_MODE_MAP_DUMB failed, handle %{public}u errno %{public}d", bo->handle, errno));

    void *virAddr = mmap(nullptr, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED, bo->gbm->fd, mapDumb.offset);
    if (virAddr == MAP_FAILED) {
        DISPLAY_LOGE("gbm mmap failed errno %{public}d:%{public}s", errno, strerror(errno));
        return nullptr;
    }
    return virAddr;
}

void hdi_gbm_bo_destroy(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN_NOT_VALUE((bo == nullptr), DISPLAY_LOGE("gbm bo is null"));
    drm_mode_destroy_dumb dumb = {};
    dumb.handle = bo->handle;
    int ret = drmIoctl(bo->gbm->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dumb);
    DISPLAY_CHK_RETURN_NOT_VALUE((ret != 0), DISPLAY_LOGE("destroy dumb failed errno %{public}d", errno));
    free(bo);
}

int hdi_gbm_bo_get_fd(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == nullptr), -1, DISPLAY_LOGE("gbm bo is null"));
    int fd = -1;
    int ret = drmPrimeHandleToFD(bo->gbm->fd, bo->handle, DRM_CLOEXEC | DRM_RDWR, &fd);
    DISPLAY_CHK_RETURN((ret != 0), -1,
        DISPLAY_LOGE("drmPrimeHandleToFD failed ret %{public}d errno %{public}d", ret, errno));
    return fd;
}
