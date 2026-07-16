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
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <linux/dma-buf.h>
#include <securec.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "display_common.h"
#include "drm_fourcc.h"
#include "hi_gbm.h"
#include "hisilicon_drm.h"
namespace OHOS {
namespace HDI {
namespace DISPLAY {
namespace {
struct PixelFormatConvertTbl {
    uint32_t drmFormat;
    PixelFormat pixFormat;
};

constexpr const char *DRM_FILE_NODES[] = {
    "/dev/dri/renderD128",
    "/dev/dri/card0",
};

constexpr uint64_t DRM_PRIMARY_USAGE_MASK =
    HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_HW_RENDER |
    HBM_USE_HW_TEXTURE | HBM_USE_MEM_DMA;

bool HasPrimaryUsage(uint64_t usage)
{
    return (usage & DRM_PRIMARY_USAGE_MASK) != 0;
}

bool CanUseLegacyFallbackUsage(uint64_t usage, PixelFormat format)
{
    if (usage == 0) {
        return false;
    }
    if (format == PIXEL_FMT_RGBX_8888 && !HasPrimaryUsage(usage)) {
        return usage == HBM_USE_VENDOR_PRI0;
    }
    if ((usage == HBM_USE_HW_RENDER || usage == HBM_USE_HW_TEXTURE) && format == PIXEL_FMT_RGBX_8888) {
        return false;
    }
    if (HasPrimaryUsage(usage)) {
        return true;
    }
    return true;
}
}

int32_t DrmAllocator::GetHandleKey(const BufferHandle *handle)
{
    if (handle == nullptr) {
        return -1;
    }
    if (handle->fd >= 0) {
        return handle->fd;
    }
    if (handle->reserveFds > 0 && handle->reserve[0] >= 0) {
        return handle->reserve[0];
    }
    return -1;
}

int32_t DrmAllocator::Init()
{
    DISPLAY_LOGD();
    for (const char *filePath : DRM_FILE_NODES) {
        drmFd_ = open(filePath, O_RDWR | O_CLOEXEC);
        if (drmFd_ >= 0) {
            DISPLAY_LOGI("open drm file %{public}s success", filePath);
            break;
        }
    }
    DISPLAY_CHK_RETURN((drmFd_ < 0), DISPLAY_FAILURE,
        DISPLAY_LOGE("can not open drm file errno: %{public}d ", errno));
    int32_t ret = drmDropMaster(drmFd_);
    if (ret != 0) {
        DISPLAY_LOGW("can not drop master");
    }
    gbmDevice_ = hdi_gbm_create_device(drmFd_);
    if (gbmDevice_ == nullptr) {
        DISPLAY_LOGE("create gbm device failed");
        close(drmFd_);
        drmFd_ = -1;
        return DISPLAY_FAILURE;
    }
    return DISPLAY_SUCCESS;
}

int32_t DrmAllocator::Allocate(const BufferInfo &bufferInfo, BufferHandle &handle)
{
    ALLOC_UNUSED(bufferInfo);
    ALLOC_UNUSED(handle);
    DISPLAY_LOGE("stack BufferHandle allocation is not supported");
    return DISPLAY_NOT_SUPPORT;
}

int32_t DrmAllocator::Allocate(const BufferInfo &bufferInfo, BufferHandle **handle)
{
    DISPLAY_CHK_RETURN((handle == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("handle is nullptr"));
    DISPLAY_CHK_RETURN((gbmDevice_ == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("gbm device is nullptr"));
    DISPLAY_CHK_RETURN((!SupportsUsage(bufferInfo.usage_, bufferInfo.format_)), DISPLAY_NOT_SUPPORT,
        DISPLAY_LOGE("usage 0x%{public}" PRIx64 " format %{public}d is not supported by drm allocator",
            bufferInfo.usage_, bufferInfo.format_));

    uint32_t drmFormat = ConvertFormatToDrm(bufferInfo.format_);
    DISPLAY_CHK_RETURN((drmFormat == 0), DISPLAY_NOT_SUPPORT,
        DISPLAY_LOGE("format %{public}d is not supported by drm allocator", bufferInfo.format_));

    struct gbm_bo *bo = hdi_gbm_bo_create(gbmDevice_, bufferInfo.width_, bufferInfo.height_, drmFormat,
        static_cast<uint32_t>(ConvertUsageToGbm(bufferInfo.usage_)));
    DISPLAY_CHK_RETURN((bo == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("gbm create bo failed"));

    int32_t fd = hdi_gbm_bo_get_fd(bo);
    if (fd < 0) {
        hdi_gbm_bo_destroy(bo);
        DISPLAY_LOGE("can not get prime fd from gbm bo");
        return DISPLAY_FD_ERR;
    }

    BufferHandle *priBuffer = static_cast<BufferHandle *>(malloc(sizeof(BufferHandle)));
    if (priBuffer == nullptr) {
        close(fd);
        hdi_gbm_bo_destroy(bo);
        DISPLAY_LOGE("malloc buffer handle failed");
        return DISPLAY_NOMEM;
    }
    errno_t eok = memset_s(priBuffer, sizeof(BufferHandle), 0, sizeof(BufferHandle));
    if (eok != EOK) {
        close(fd);
        hdi_gbm_bo_destroy(bo);
        free(priBuffer);
        DISPLAY_LOGE("memset_s failed");
        return DISPLAY_FAILURE;
    }

    priBuffer->fd = fd;
    priBuffer->size = hdi_gbm_bo_get_size(bo);
    priBuffer->format = bufferInfo.format_;
    priBuffer->width = bufferInfo.width_;
    priBuffer->height = bufferInfo.height_;
    priBuffer->stride = hdi_gbm_bo_get_stride(bo);
    priBuffer->usage = bufferInfo.usage_;
    priBuffer->phyAddr = GetPhysicalAddr(fd);
    priBuffer->virAddr = nullptr;
    priBuffer->reserveFds = 0;
    priBuffer->reserveInts = 0;

    DISPLAY_LOGI("drm alloc fmt %{public}d -> fourcc 0x%{public}x size %{public}u stride %{public}u fd %{public}d",
        bufferInfo.format_, drmFormat, priBuffer->size, priBuffer->stride, priBuffer->fd);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        bufferObjects_[fd] = bo;
    }
    *handle = priBuffer;
    return DISPLAY_SUCCESS;
}

int32_t DrmAllocator::FreeMem(BufferHandle *handle)
{
    DISPLAY_CHK_RETURN((handle == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("buffer is null"));

    if (handle->virAddr != nullptr) {
        int32_t ret = Unmap(handle);
        if (ret != DISPLAY_SUCCESS) {
            DISPLAY_LOGW("drm buffer unmap during free failed, ret %{public}d", ret);
        }
    }

    struct gbm_bo *bo = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int32_t key = GetHandleKey(handle);
        auto it = bufferObjects_.find(key);
        if (it != bufferObjects_.end()) {
            bo = it->second;
            bufferObjects_.erase(it);
        }
    }

    int32_t ret = Allocator::FreeMem(handle);
    if (bo != nullptr) {
        hdi_gbm_bo_destroy(bo);
    }
    return ret;
}

void *DrmAllocator::Mmap(BufferHandle *handle)
{
    DISPLAY_CHK_RETURN((handle == nullptr), nullptr, DISPLAY_LOGE("handle is nullptr"));
    if (handle->virAddr != nullptr) {
        return handle->virAddr;
    }

    struct gbm_bo *bo = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int32_t key = GetHandleKey(handle);
        auto it = bufferObjects_.find(key);
        if (it != bufferObjects_.end()) {
            bo = it->second;
        }
    }
    void *virAddr = (bo != nullptr) ? hdi_gbm_bo_mmap(bo) : MmapFromPrimeFd(handle);
    DISPLAY_CHK_RETURN((virAddr == nullptr), nullptr, DISPLAY_LOGE("drm gbm mmap failed"));
    handle->virAddr = virAddr;
    return virAddr;
}

void *DrmAllocator::MmapFromPrimeFd(BufferHandle *handle)
{
    DISPLAY_CHK_RETURN((handle == nullptr), nullptr, DISPLAY_LOGE("handle is nullptr"));
    DISPLAY_CHK_RETURN((drmFd_ < 0), nullptr, DISPLAY_LOGE("drm fd is invalid"));
    DISPLAY_CHK_RETURN((handle->fd < 0), nullptr, DISPLAY_LOGE("buffer fd is invalid"));

    uint32_t gemHandle = 0;
    int32_t ret = drmPrimeFDToHandle(drmFd_, handle->fd, &gemHandle);
    DISPLAY_CHK_RETURN((ret != 0), nullptr,
        DISPLAY_LOGE("drmPrimeFDToHandle failed, fd %{public}d errno %{public}d", handle->fd, errno));

    drm_mode_map_dumb mapDumb = {};
    mapDumb.handle = gemHandle;
    ret = drmIoctl(drmFd_, DRM_IOCTL_MODE_MAP_DUMB, &mapDumb);
    if (ret != 0) {
        DISPLAY_LOGE("DRM_IOCTL_MODE_MAP_DUMB failed, handle %{public}u errno %{public}d", gemHandle, errno);
        CloseGemHandle(gemHandle);
        return nullptr;
    }

    void *virAddr = mmap(nullptr, handle->size, PROT_READ | PROT_WRITE, MAP_SHARED, drmFd_, mapDumb.offset);
    if (virAddr == MAP_FAILED) {
        DISPLAY_LOGE("drm prime mmap failed errno %{public}d:%{public}s", errno, strerror(errno));
        CloseGemHandle(gemHandle);
        return nullptr;
    }

    CloseGemHandle(gemHandle);
    return virAddr;
}

void DrmAllocator::CloseGemHandle(uint32_t gemHandle)
{
    if (gemHandle == 0 || drmFd_ < 0) {
        return;
    }
    struct drm_gem_close gemClose = {};
    gemClose.handle = gemHandle;
    if (drmIoctl(drmFd_, DRM_IOCTL_GEM_CLOSE, &gemClose) != 0) {
        DISPLAY_LOGW("can not close gem handle %{public}u errno %{public}d", gemHandle, errno);
    }
}

int32_t DrmAllocator::Unmap(BufferHandle *handle)
{
    DISPLAY_CHK_RETURN((handle == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("handle is nullptr"));
    return Allocator::Unmap(handle);
}

bool DrmAllocator::SupportsFormat(PixelFormat format)
{
    return ConvertFormatToDrm(format) != 0;
}

bool DrmAllocator::SupportsUsage(uint64_t usage, PixelFormat format)
{
    return CanUseLegacyFallbackUsage(usage, format);
}

uint32_t DrmAllocator::ConvertFormatToDrm(PixelFormat format)
{
    static const PixelFormatConvertTbl CONVERT_TABLE[] = {
        {DRM_FORMAT_RGBX8888, PIXEL_FMT_RGBX_8888},
        {DRM_FORMAT_RGBA8888, PIXEL_FMT_RGBA_8888},
        {DRM_FORMAT_RGB888, PIXEL_FMT_RGB_888},
        {DRM_FORMAT_BGR565, PIXEL_FMT_BGR_565},
        {DRM_FORMAT_BGRX4444, PIXEL_FMT_BGRX_4444},
        {DRM_FORMAT_BGRA4444, PIXEL_FMT_BGRA_4444},
        {DRM_FORMAT_RGBA4444, PIXEL_FMT_RGBA_4444},
        {DRM_FORMAT_RGBX4444, PIXEL_FMT_RGBX_4444},
        {DRM_FORMAT_BGRX5551, PIXEL_FMT_BGRX_5551},
        {DRM_FORMAT_BGRA5551, PIXEL_FMT_BGRA_5551},
        {DRM_FORMAT_RGBA5551, PIXEL_FMT_RGBA_5551},
        {DRM_FORMAT_BGRX8888, PIXEL_FMT_BGRX_8888},
        {DRM_FORMAT_BGRA8888, PIXEL_FMT_BGRA_8888},
        {DRM_FORMAT_RGBA1010102, PIXEL_FMT_RGBA_1010102},
        {DRM_FORMAT_NV12, PIXEL_FMT_YCBCR_420_SP},
        {DRM_FORMAT_NV21, PIXEL_FMT_YCRCB_420_SP},
        {DRM_FORMAT_YUV420, PIXEL_FMT_YCBCR_420_P},
        {DRM_FORMAT_YVU420, PIXEL_FMT_YCRCB_420_P},
        {DRM_FORMAT_NV16, PIXEL_FMT_YCBCR_422_SP},
        {DRM_FORMAT_NV61, PIXEL_FMT_YCRCB_422_SP},
        {DRM_FORMAT_YUV422, PIXEL_FMT_YCBCR_422_P},
        {DRM_FORMAT_YVU422, PIXEL_FMT_YCRCB_422_P},
    };

    for (uint32_t i = 0; i < sizeof(CONVERT_TABLE) / sizeof(CONVERT_TABLE[0]); i++) {
        if (CONVERT_TABLE[i].pixFormat == format) {
            return CONVERT_TABLE[i].drmFormat;
        }
    }
    return 0;
}

uint64_t DrmAllocator::ConvertUsageToGbm(uint64_t usage)
{
    uint64_t gbmUsage = GBM_BO_USE_TEXTURING;
    if (usage & HBM_USE_CPU_READ) {
        gbmUsage |= GBM_BO_USE_SW_READ_OFTEN;
    }
    if (usage & HBM_USE_CPU_WRITE) {
        gbmUsage |= GBM_BO_USE_SW_WRITE_OFTEN;
    }
    if (usage & HBM_USE_HW_RENDER) {
        gbmUsage |= GBM_BO_USE_RENDERING;
    }
    if (usage & HBM_USE_HW_TEXTURE) {
        gbmUsage |= GBM_BO_USE_TEXTURING;
    }
    return gbmUsage;
}

int32_t DrmAllocator::DmaBufferSync(BufferHandle *handle, bool start)
{
    DISPLAY_CHK_RETURN((handle == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("handle is nullptr"));
    if (handle->fd < 0) {
        DISPLAY_LOGW("buffer fd is invalid");
        return DISPLAY_SUCCESS;
    }

    struct dma_buf_sync syncPrm = {};
    errno_t eok = memset_s(&syncPrm, sizeof(syncPrm), 0, sizeof(syncPrm));
    DISPLAY_CHK_RETURN((eok != EOK), DISPLAY_FAILURE, DISPLAY_LOGE("memset_s failed"));

    if (handle->usage & HBM_USE_CPU_WRITE) {
        syncPrm.flags |= DMA_BUF_SYNC_WRITE;
    }
    if (handle->usage & HBM_USE_CPU_READ) {
        syncPrm.flags |= DMA_BUF_SYNC_READ;
    }
    syncPrm.flags |= start ? DMA_BUF_SYNC_START : DMA_BUF_SYNC_END;

    constexpr int32_t RETRY_MAX = 6;
    for (int32_t retry = 0; retry < RETRY_MAX; retry++) {
        int32_t ret = ioctl(handle->fd, DMA_BUF_IOCTL_SYNC, &syncPrm);
        if (ret == 0) {
            return DISPLAY_SUCCESS;
        }
        if (errno != EAGAIN && errno != EINTR) {
            break;
        }
    }
    DISPLAY_LOGE("dma buffer sync failed errno %{public}d", errno);
    return DISPLAY_SYS_BUSY;
}

int32_t DrmAllocator::InvalidateCache(BufferHandle *handle)
{
    return DmaBufferSync(handle, true);
}

int32_t DrmAllocator::FlushCache(BufferHandle *handle)
{
    return DmaBufferSync(handle, false);
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
    for (auto &entry : bufferObjects_) {
        hdi_gbm_bo_destroy(entry.second);
    }
    bufferObjects_.clear();
    if (gbmDevice_ != nullptr) {
        hdi_gbm_device_destroy(gbmDevice_);
        gbmDevice_ = nullptr;
    }
    if (drmFd_ >= 0) {
        close(drmFd_);
        drmFd_ = -1;
    }
}
} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS
