/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "drm_allocator.h"
#include <cinttypes>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <securec.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "display_common.h"
#include "drm_fourcc.h"
#include "hisilicon_drm.h"
namespace OHOS {
namespace HDI {
namespace DISPLAY {
int32_t DrmAllocator::Init()
{
    DISPLAY_LOGD();
    int32_t ret;
    drmFd_ = open(filePath, O_RDWR);
    DISPLAY_CHK_RETURN((drmFd_ < 0), DISPLAY_FAILURE,
        DISPLAY_LOGE("can not open drm file : %{public}s errno: %{public}d ", filePath, errno));
    ret = drmDropMaster(drmFd_);
    if (ret != 0) {
        DISPLAY_LOGW("can not drop master");
    }
    return DISPLAY_SUCCESS;
}

int32_t DrmAllocator::Allocate(const BufferInfo &bufferInfo, BufferHandle &handle)
{
    int32_t ret;
    int32_t fd;
    struct drm_mode_create_dumb dumb = {0};
    // create_dumb
    dumb.width = bufferInfo.widthStride;
    dumb.height = bufferInfo.heightStride;
    dumb.flags = 0;
    dumb.bpp = bufferInfo.bitsPerPixel;
    DISPLAY_LOGD();
    ret = drmIoctl(drmFd_, DRM_IOCTL_MODE_CREATE_DUMB, &dumb);
    DISPLAY_LOGI("fmt 0x%{public}x create dumb width: %{public}d  height: %{public}d bpp: %{public}u pitch %{public}d "
                 "size %{public}llu",
        handle.format, dumb.width, dumb.height, dumb.bpp, dumb.pitch, dumb.size);
    DISPLAY_CHK_RETURN(
        (ret != 0), DISPLAY_FAILURE, DISPLAY_LOGE("DRM_IOCTL_MODE_CREATE_DUMB failed errno %{public}d", errno));

    ret = drmPrimeHandleToFD(drmFd_, dumb.handle, DRM_CLOEXEC | DRM_RDWR, &fd);
    DISPLAY_CHK_RETURN(
        (ret != 0), DISPLAY_FAILURE, DISPLAY_LOGE("can not get fd from prime handle errno: %{public}d", errno));
    handle.fd = fd;
    handle.size = dumb.size;
    handle.format = bufferInfo.format;    //PIXEL_FMT_BGRA_8888

    struct drm_mode_destroy_dumb destoryDumb = {0};
    destoryDumb.handle = dumb.handle;
    ret = drmIoctl(drmFd_, DRM_IOCTL_MODE_DESTROY_DUMB, &destoryDumb);
    if (ret != 0) {
        DISPLAY_LOGW("can not destroy dumb errno: %{public}d", errno);
    }

    return DISPLAY_SUCCESS;
}

int32_t DrmAllocator::Allocate(const BufferInfo &bufferInfo, BufferHandle **handle)
{
    ALLOC_UNUSED(bufferInfo);

    DISPLAY_LOGE("AllocMem do not implement");
    return DISPLAY_NOT_SUPPORT;
}

uint64_t DrmAllocator::GetPhysicalAddr(int primeFd)
{
    ALLOC_UNUSED(primeFd);

    DISPLAY_LOGE("DRM_IOCTL_HISILICON_GEM_FD_TO_PHYADDR not support \n");
    return 0;
}

DrmAllocator::~DrmAllocator()
{
    DISPLAY_LOGD();
    if (drmFd_ >= 0) {
        close(drmFd_);
        drmFd_ = -1;
    }
}
} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS