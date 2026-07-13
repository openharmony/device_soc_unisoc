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
#include <cinttypes>
#include "allocator_manager.h"
#include "display_common.h"
#include "framebuffer_allocator.h"
#include "dmabufferheap_allocator.h"
#include "drm_allocator.h"
#include "sprd_allocator.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
namespace {
bool IsPureFramebufferUsage(uint64_t usage)
{
    return usage == HBM_USE_MEM_FB;
}

bool IsKnownDrmFallbackFormat(PixelFormat format)
{
    // The Sprd wrapper reports RGBA_4444 support, but the underlying allocator rejects it.
    return format == PIXEL_FMT_RGBA_4444;
}

bool IsKnownUnsupportedRequest(const AllocInfo &info)
{
    if (static_cast<PixelFormat>(info.format) != PIXEL_FMT_RGBX_8888) {
        return false;
    }
    return info.usage == HBM_USE_HW_RENDER || info.usage == HBM_USE_HW_TEXTURE;
}

bool IsKnownUnsupportedMmapRequest(const BufferHandle *handle)
{
    if (handle == nullptr || static_cast<PixelFormat>(handle->format) != PIXEL_FMT_RGBX_8888) {
        return false;
    }
    // Vendor-only RGBX_8888 is kept allocatable for compatibility, but not CPU-mappable.
    return handle->usage == HBM_USE_VENDOR_PRI0;
}

bool ShouldPreferDrmAllocator(const AllocInfo &info, const std::shared_ptr<Allocator> &allocator,
    const std::shared_ptr<Allocator> &drmAllocator)
{
    if (IsPureFramebufferUsage(info.usage)) {
        return false;
    }
    if (drmAllocator == nullptr) {
        return false;
    }

    PixelFormat format = static_cast<PixelFormat>(info.format);
    if (!DrmAllocator::SupportsFormat(format)) {
        return false;
    }
    if (IsKnownDrmFallbackFormat(format)) {
        return true;
    }
    if (allocator == nullptr || allocator.get() == drmAllocator.get()) {
        return true;
    }
    return !SprdAllocator::SupportsFormat(format);
}
}

int32_t AllocatorManager::GetHandleKey(const BufferHandle *handle)
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

int32_t AllocatorManager::Init()
{
    DISPLAY_LOGD("AllocatorManager::Init");
    if (init_) {
        DISPLAY_LOGW("allocator has initialized");
        return DISPLAY_SUCCESS;
    }
    // first use sprd allocator
    std::shared_ptr<Allocator> sprdAllocator = std::make_shared<SprdAllocator>();
    int ret = sprdAllocator->Init();
    if (ret == DISPLAY_SUCCESS) {
        frameBufferAllocator_ = sprdAllocator;
        allocator_ = sprdAllocator;
        drmAllocator_ = std::make_shared<DrmAllocator>();
        ret = drmAllocator_->Init();
        if (ret != DISPLAY_SUCCESS) {
            DISPLAY_LOGW("Failed init drm allocator");
            drmAllocator_.reset();
        }
    } else {
        drmAllocator_ = std::make_shared<DrmAllocator>();
        ret = drmAllocator_->Init();
        if (ret == DISPLAY_SUCCESS) {
            frameBufferAllocator_ = drmAllocator_;
            allocator_ = drmAllocator_;
        } else {
            std::shared_ptr<Allocator> fbAllocator = std::make_shared<FramebufferAllocator>();
            ret = fbAllocator->Init();
            DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE,
                DISPLAY_LOGE("Failed init framebuffer allocator"));

            dmaBufferAllocator_ = std::make_shared<DmaBufferHeapAllocator>();
            ret = dmaBufferAllocator_->Init();
            DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE,
                DISPLAY_LOGE("Failed init Dmabuffer allocator"));

            frameBufferAllocator_ = fbAllocator;
            allocator_ = dmaBufferAllocator_;
        }
    }

    init_ = true;
    DISPLAY_LOGD("init success");
    return DISPLAY_SUCCESS;
}

int32_t AllocatorManager::DeInit()
{
    DISPLAY_LOGD();
    std::lock_guard<std::mutex> lock(mutex_);
    init_ = false;
    frameBufferAllocator_.reset();
    allocator_.reset();
    drmAllocator_.reset();
    dmaBufferAllocator_.reset();
    allocatorOwners_.clear();
    return DISPLAY_SUCCESS;
}

int32_t AllocatorManager::AllocMem(const AllocInfo &info, BufferHandle **handle)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((handle == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("handle is nullptr"));
    DISPLAY_CHK_RETURN((IsKnownUnsupportedRequest(info)), DISPLAY_FAILURE,
        DISPLAY_LOGE("format %{public}d usage 0x%{public}" PRIx64 " is not supported", info.format, info.usage));

    const bool useFramebufferAllocator = IsPureFramebufferUsage(info.usage);
    Allocator *preferredAllocator = ShouldPreferDrmAllocator(info, allocator_, drmAllocator_) ?
        drmAllocator_.get() : allocator_.get();
    Allocator *candidates[] = {
        useFramebufferAllocator ? frameBufferAllocator_.get() : nullptr,
        preferredAllocator,
        (preferredAllocator == drmAllocator_.get()) ? allocator_.get() : drmAllocator_.get(),
        dmaBufferAllocator_.get(),
    };

    int32_t lastRet = DISPLAY_FAILURE;
    Allocator *lastAllocator = nullptr;
    for (Allocator *allocator : candidates) {
        if (allocator == nullptr || allocator == lastAllocator) {
            continue;
        }
        lastAllocator = allocator;
        *handle = nullptr;
        lastRet = allocator->AllocMem(info, handle);
        if (lastRet == DISPLAY_SUCCESS && *handle != nullptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            TrackAllocatorLocked(*handle, allocator);
            return DISPLAY_SUCCESS;
        }
    }
    DISPLAY_LOGE("AllocMem failed for all allocator backends, format %{public}d usage 0x%{public}" PRIx64,
        info.format, info.usage);
    return lastRet;
}

void AllocatorManager::FreeMem(BufferHandle *handle)
{
    DISPLAY_CHK_RETURN_NOT_VALUE((handle == nullptr), DISPLAY_LOGE("handle is nullptr"));
    Allocator *allocator = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        allocator = ResolveAllocatorLocked(handle);
        ForgetAllocatorLocked(handle);
    }
    DISPLAY_CHK_RETURN_NOT_VALUE((allocator == nullptr), DISPLAY_LOGE("can not resolve allocator"));
    allocator->FreeMem(handle);
}

void *AllocatorManager::Mmap(BufferHandle *handle)
{
    DISPLAY_CHK_RETURN((handle == nullptr), nullptr, DISPLAY_LOGE("handle is nullptr"));
    DISPLAY_CHK_RETURN((IsKnownUnsupportedMmapRequest(handle)), nullptr,
        DISPLAY_LOGE("format %{public}d usage 0x%{public}" PRIx64 " mmap is not supported",
            handle->format, handle->usage));
    std::lock_guard<std::mutex> lock(mutex_);
    Allocator *allocator = ResolveAllocatorLocked(handle);
    DISPLAY_CHK_RETURN((allocator == nullptr), nullptr, DISPLAY_LOGE("can not resolve allocator"));
    return allocator->Mmap(handle);
}

int32_t AllocatorManager::Unmap(BufferHandle *handle)
{
    DISPLAY_CHK_RETURN((handle == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("handle is nullptr"));
    std::lock_guard<std::mutex> lock(mutex_);
    Allocator *allocator = ResolveAllocatorLocked(handle);
    DISPLAY_CHK_RETURN((allocator == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("can not resolve allocator"));
    return allocator->Unmap(handle);
}

int32_t AllocatorManager::FlushCache(BufferHandle *handle)
{
    DISPLAY_CHK_RETURN((handle == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("handle is nullptr"));
    std::lock_guard<std::mutex> lock(mutex_);
    Allocator *allocator = ResolveAllocatorLocked(handle);
    DISPLAY_CHK_RETURN((allocator == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("can not resolve allocator"));
    return allocator->FlushCache(handle);
}

int32_t AllocatorManager::InvalidateCache(BufferHandle *handle)
{
    DISPLAY_CHK_RETURN((handle == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("handle is nullptr"));
    std::lock_guard<std::mutex> lock(mutex_);
    Allocator *allocator = ResolveAllocatorLocked(handle);
    DISPLAY_CHK_RETURN((allocator == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("can not resolve allocator"));
    return allocator->InvalidateCache(handle);
}

Allocator *AllocatorManager::ResolveAllocatorLocked(const BufferHandle *handle)
{
    int32_t key = GetHandleKey(handle);
    if (key >= 0) {
        auto it = allocatorOwners_.find(key);
        if (it != allocatorOwners_.end()) {
            return it->second;
        }
    }
    if (IsPureFramebufferUsage(handle->usage)) {
        return frameBufferAllocator_.get();
    }
    PixelFormat format = static_cast<PixelFormat>(handle->format);
    if (drmAllocator_ != nullptr && DrmAllocator::SupportsFormat(format) &&
        (IsKnownDrmFallbackFormat(format) || !SprdAllocator::SupportsFormat(format))) {
        return drmAllocator_.get();
    }
    if (allocator_ != nullptr) {
        return allocator_.get();
    }
    if (drmAllocator_ != nullptr) {
        return drmAllocator_.get();
    }
    return dmaBufferAllocator_.get();
}

void AllocatorManager::TrackAllocatorLocked(const BufferHandle *handle, Allocator *allocator)
{
    int32_t key = GetHandleKey(handle);
    if (key < 0) {
        DISPLAY_LOGW("skip allocator track for invalid handle key");
        return;
    }
    allocatorOwners_[key] = allocator;
}

void AllocatorManager::ForgetAllocatorLocked(const BufferHandle *handle)
{
    int32_t key = GetHandleKey(handle);
    if (key < 0) {
        return;
    }
    allocatorOwners_.erase(key);
}
}  // namespace DISPLAY
}  // namespace HDI
}  // namespace OHOS
