/*
 * Copyright 2023 Shenzhen Kaihong DID Co., Ltd..
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

#include "display_common.h"
#include "display_adapter.h"
#include "sprd_allocator.h"

/* GRALLOC_USAGE_* constants from sprd_ar_src hardware/gralloc.h */
#ifndef GRALLOC_USAGE_SW_READ_OFTEN
#define GRALLOC_USAGE_SW_READ_OFTEN     0x00000003U
#endif
#ifndef GRALLOC_USAGE_SW_WRITE_OFTEN
#define GRALLOC_USAGE_SW_WRITE_OFTEN    0x00000030U
#endif
#ifndef GRALLOC_USAGE_HW_TEXTURE
#define GRALLOC_USAGE_HW_TEXTURE        0x00000100U
#endif
#ifndef GRALLOC_USAGE_HW_RENDER
#define GRALLOC_USAGE_HW_RENDER         0x00000200U
#endif

namespace OHOS {
namespace HDI {
namespace DISPLAY {
namespace {
constexpr uint64_t SPRD_PRIMARY_USAGE_MASK =
    HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_HW_RENDER |
    HBM_USE_HW_TEXTURE | HBM_USE_MEM_DMA;

bool HasPrimaryUsage(uint64_t usage)
{
    return (usage & SPRD_PRIMARY_USAGE_MASK) != 0;
}

bool CanUseLegacyFallbackUsage(uint64_t usage, PixelFormat format)
{
    if (HasPrimaryUsage(usage)) {
        return true;
    }
    return format == PIXEL_FMT_BGRA_8888 || format == PIXEL_FMT_RGBA_8888;
}
}

int32_t SprdAllocator::GetHandleKey(const BufferHandle *handle)
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

SprdAllocator::~SprdAllocator()
{
    DISPLAY_LOGD();
}

int32_t SprdAllocator::Init()
{
    DISPLAY_LOGD();
    mHandle = nullptr;
    mStride = 0;
    mUsage = 0;
    mFormat = ADAPTER_PIXEL_FORMAT_UNKNOWN;

    return DISPLAY_SUCCESS;
}

int32_t SprdAllocator::Allocate(const BufferInfo &bufferInfo, BufferHandle **handle)
{
    DISPLAY_LOGD("");
    if (!bufferInfo.width_ || !bufferInfo.height_) {
        DISPLAY_LOGE("param error, width or height error!");
        return DISPLAY_FAILURE;
    }

    mUsage = ConvertUsageToGpu(bufferInfo.usage_, bufferInfo.format_);
    DISPLAY_CHK_RETURN((mUsage == 0), DISPLAY_FAILURE, DISPLAY_LOGE("usage is error"));
    mFormat = ConvertFormatToGpu(bufferInfo.format_);
    DISPLAY_CHK_RETURN((mFormat == ADAPTER_PIXEL_FORMAT_UNKNOWN), DISPLAY_PARAM_ERR, DISPLAY_LOGE("format is error"));

    DISPLAY_LOGD("bufferInfo %{public}d x %{public}d, "
        "stride:%{public}d x %{public}d",
        bufferInfo.width_, bufferInfo.height_,
        bufferInfo.widthStride_, bufferInfo.heightStride_);
    std::lock_guard<std::mutex> lock(m);

    void *gbuffer = AdapterGraphicBufferAllocate(bufferInfo.widthStride_, bufferInfo.heightStride_,
        mFormat, mUsage, "SprdAllocMem");
    if (gbuffer == nullptr) {
        DISPLAY_LOGE("memory allocate failed.");
        return DISPLAY_FAILURE;
    }

    mHandle = AdapterGraphicBufferGetHandle(gbuffer);
    if (mHandle == nullptr) {
        DISPLAY_LOGE("get native handle failed.");
        AdapterGraphicBufferFree(gbuffer);
        return DISPLAY_FAILURE;
    }

    int32_t ret = InitBufferhandle(bufferInfo, handle);
    if (ret != DISPLAY_SUCCESS) {
        AdapterGraphicBufferFree(gbuffer);
        return ret;
    }

    int32_t key = GetHandleKey(*handle);
    if (key < 0) {
        DISPLAY_LOGW("allocate success but handle key invalid");
    } else {
        mBufferMap[key] = gbuffer;
    }
    return DISPLAY_SUCCESS;
}

bool SprdAllocator::SupportsFormat(PixelFormat format)
{
    return ConvertFormatToGpu(format) != ADAPTER_PIXEL_FORMAT_UNKNOWN;
}

int32_t SprdAllocator::Allocate(const BufferInfo &bufferInfo, BufferHandle &handle)
{
    ALLOC_UNUSED(bufferInfo);
    ALLOC_UNUSED(handle);

    DISPLAY_LOGE("AllocMem do not implement");
    return DISPLAY_NOT_SUPPORT;
}

int32_t SprdAllocator::FreeMem(BufferHandle *handle)
{
    DISPLAY_LOGD("");
    std::lock_guard<std::mutex> lock(m);
    DISPLAY_CHK_RETURN((handle == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("buffer is null"));

    int32_t key = GetHandleKey(handle);
    if (key >= 0) {
        auto it = mBufferMap.find(key);
        if (it != mBufferMap.end()) {
            AdapterGraphicBufferFree(it->second);
            mBufferMap.erase(it);
        }
    }

    if (handle->fd >= 0) {
        if (handle->reserveFds > 0 && handle->fd != handle->reserve[0]) {
            DISPLAY_LOGD("release the fd is %{public}d", handle->fd);
            close(handle->fd);
        }
        handle->fd = -1;
    }
    const uint32_t reserveFds = handle->reserveFds;
    for (uint32_t i = 0; i < reserveFds; i++) {
        if (handle->reserve[i] >= 0) {
            DISPLAY_LOGD("release the fd is %{public}d", handle->reserve[i]);
            close(handle->reserve[i]);
            handle->reserve[i] = -1;
        }
    }
    free(handle);

    return DISPLAY_SUCCESS;
}

int32_t SprdAllocator::InitBufferhandle(const BufferInfo &bufferInfo, BufferHandle **handle)
{
    DISPLAY_LOGD();
    DISPLAY_LOGD("mHandle data[0]:%d, numFds:%d, numInts:%d",
        mHandle->data[0], mHandle->numFds, mHandle->numInts);

    size_t mallocSize = sizeof(BufferHandle) + sizeof(int32_t) * (mHandle->numFds + mHandle->numInts);
    BufferHandle *priBuffer = reinterpret_cast<BufferHandle *>(malloc(mallocSize));
    if (priBuffer == nullptr) {
        DISPLAY_LOGE("BufferHandle malloc failed");
        return DISPLAY_NOMEM;
    }

    priBuffer->width = bufferInfo.width_;
    priBuffer->height = bufferInfo.heightStride_;
    priBuffer->stride = bufferInfo.widthStride_ * bufferInfo.bytesPerPixel_;
    priBuffer->size = bufferInfo.size_;
    priBuffer->format = bufferInfo.format_;
    priBuffer->usage = bufferInfo.usage_;
    priBuffer->reserveFds = mHandle->numFds;
    priBuffer->reserveInts = mHandle->numInts;
    for (int i = 0; i < mHandle->numFds; i++) {
        priBuffer->reserve[i] = dup(mHandle->data[i]);
        if (priBuffer->reserve[i] == -1) {
            DISPLAY_LOGE("memory fd %d dup failed with %d", mHandle->data[i], errno);
            goto ERR_FD;
        }
    }
    memcpy_s(&priBuffer->reserve[mHandle->numFds],
        sizeof(int32_t) * mHandle->numInts,
        &mHandle->data[mHandle->numFds],
        sizeof(int32_t) * mHandle->numInts);

    priBuffer->fd = priBuffer->reserve[0];
    priBuffer->virAddr = nullptr;
    priBuffer->phyAddr = 0;
    *handle = priBuffer;
    DISPLAY_LOGI("buffer handle size %{public}d, width %{public}d, height %{public}d, "
                 "stride %{public}d, fd %{public}d, format: %{public}d, "
                 "phy 0x%{public}llx, usage 0x%{public}llx, viraddr %{public}p",
        priBuffer->size, priBuffer->width, priBuffer->height, priBuffer->stride,
        priBuffer->fd, priBuffer->format,
        (unsigned long long)priBuffer->phyAddr,
        (unsigned long long)priBuffer->usage,
        priBuffer->virAddr);
    return DISPLAY_SUCCESS;

ERR_FD:
    for (uint32_t i = 0; i < priBuffer->reserveFds; i++) {
        if (priBuffer->reserve[i] >= 0) {
            close(priBuffer->reserve[i]);
        }
    }
    free(priBuffer);

    return DISPLAY_FD_ERR;
}

uint64_t SprdAllocator::ConvertUsageToGpu(uint64_t inUsage, PixelFormat format)
{
    DISPLAY_LOGD();
    uint64_t outUsage = 0;
    if (inUsage & HBM_USE_CPU_READ) {
        outUsage |= GRALLOC_USAGE_SW_READ_OFTEN;
    }
    if (inUsage & HBM_USE_CPU_WRITE) {
        outUsage |= GRALLOC_USAGE_SW_WRITE_OFTEN;
    }
    if (inUsage & HBM_USE_HW_RENDER) {
        outUsage |= GRALLOC_USAGE_HW_RENDER;
    }
    if (inUsage & HBM_USE_HW_TEXTURE) {
        outUsage |= GRALLOC_USAGE_HW_TEXTURE;
    }
    if ((outUsage == 0) && CanUseLegacyFallbackUsage(inUsage, format)) {
        outUsage = GRALLOC_USAGE_HW_TEXTURE;
    }

    return outUsage;
}

AdapterPixelFormat SprdAllocator::ConvertFormatToGpu(PixelFormat inFormat)
{
    AdapterPixelFormat outFormat = ADAPTER_PIXEL_FORMAT_UNKNOWN;

    if (inFormat == PIXEL_FMT_RGBA_8888) {
        outFormat = ADAPTER_PIXEL_FORMAT_RGBA_8888;
    } else if (inFormat == PIXEL_FMT_RGBX_8888) {
        outFormat = ADAPTER_PIXEL_FORMAT_RGBX_8888;
    } else if (inFormat == PIXEL_FMT_RGB_888) {
        outFormat = ADAPTER_PIXEL_FORMAT_RGB_888;
    } else if (inFormat == PIXEL_FMT_BGR_565) {
        outFormat = ADAPTER_PIXEL_FORMAT_RGB_565;
    } else if (inFormat == PIXEL_FMT_BGRA_8888) {
        outFormat = ADAPTER_PIXEL_FORMAT_BGRA_8888;
    } else if (inFormat == PIXEL_FMT_RGBA_5551) {
        outFormat = ADAPTER_PIXEL_FORMAT_RGBA_5551;
    } else if (inFormat == PIXEL_FMT_RGBA_4444) {
        outFormat = ADAPTER_PIXEL_FORMAT_RGBA_4444;
    } else if ((inFormat == PIXEL_FMT_YCBCR_420_SP) ||
        (inFormat == PIXEL_FMT_YCRCB_420_SP) ||
        (inFormat == PIXEL_FMT_YCBCR_420_P) ||
        (inFormat == PIXEL_FMT_YCRCB_420_P)) {
        outFormat = ADAPTER_PIXEL_FORMAT_YVU420_SP;
    } else if ((inFormat == PIXEL_FMT_YCBCR_422_SP) ||
        (inFormat == PIXEL_FMT_YCRCB_422_SP) ||
        (inFormat == PIXEL_FMT_YCBCR_422_P) ||
        (inFormat == PIXEL_FMT_YCRCB_422_P)) {
        outFormat = ADAPTER_PIXEL_FORMAT_YUV422_SP;
    }

    return outFormat;
}

} /*namespace DISPLAY*/
} /*namespace HDI*/
} /*namespace OHOS*/
