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
#ifndef DRM_ALLOCATOR_H
#define DRM_ALLOCATOR_H
#include <map>
#include <mutex>
#include <string>
#include "allocator.h"

struct gbm_device;
struct gbm_bo;

namespace OHOS {
namespace HDI {
namespace DISPLAY {
class DrmAllocator : public Allocator {
public:
    int32_t Init() override;
    int32_t Allocate(const BufferInfo &bufferInfo, BufferHandle &handle) override;
    int32_t Allocate(const BufferInfo &bufferInfo, BufferHandle **handle) override;
    int32_t FreeMem(BufferHandle *handle) override;
    void *Mmap(BufferHandle *handle) override;
    int32_t Unmap(BufferHandle *handle) override;
    int32_t InvalidateCache(BufferHandle *handle) override;
    int32_t FlushCache(BufferHandle *handle) override;
    static bool SupportsFormat(PixelFormat format);
    ~DrmAllocator() override;

private:
    static int32_t GetHandleKey(const BufferHandle *handle);
    static bool SupportsUsage(uint64_t usage, PixelFormat format);
    static uint32_t ConvertFormatToDrm(PixelFormat format);
    static uint64_t ConvertUsageToGbm(uint64_t usage);
    static int32_t DmaBufferSync(BufferHandle *handle, bool start);
    void *MmapFromPrimeFd(BufferHandle *handle);
    void CloseGemHandle(uint32_t gemHandle);
    uint64_t GetPhysicalAddr(int primeFd);
    std::mutex mutex_;
    std::map<int32_t, struct gbm_bo *> bufferObjects_;
    int32_t drmFd_ = -1;
    gbm_device *gbmDevice_ = nullptr;
};
}
}
}
#endif // DRM_ALLOCATOR_H
