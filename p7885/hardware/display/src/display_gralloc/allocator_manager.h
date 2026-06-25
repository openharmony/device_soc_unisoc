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
#ifndef ALLOCATOR_MANAGER_H
#define ALLOCATOR_MANAGER_H
#include <map>
#include <memory>
#include <mutex>
#include "allocator.h"
namespace OHOS {
namespace HDI {
namespace DISPLAY {
class AllocatorManager {
public:
    static AllocatorManager& GetInstance()
    {
        static AllocatorManager instance;
        return instance;
    }
    int32_t Init();
    int32_t DeInit();
    int32_t AllocMem(const AllocInfo &info, BufferHandle **handle);
    void FreeMem(BufferHandle *handle);
    void *Mmap(BufferHandle *handle);
    int32_t Unmap(BufferHandle *handle);
    int32_t FlushCache(BufferHandle *handle);
    int32_t InvalidateCache(BufferHandle *handle);

private:
    static int32_t GetHandleKey(const BufferHandle *handle);
    Allocator *ResolveAllocatorLocked(const BufferHandle *handle);
    void TrackAllocatorLocked(const BufferHandle *handle, Allocator *allocator);
    void ForgetAllocatorLocked(const BufferHandle *handle);

    bool init_ = false;
    std::shared_ptr<Allocator> frameBufferAllocator_ = nullptr;
    std::shared_ptr<Allocator> allocator_ = nullptr;
    std::shared_ptr<Allocator> drmAllocator_ = nullptr;
    std::shared_ptr<Allocator> dmaBufferAllocator_ = nullptr;
    std::map<int32_t, Allocator *> allocatorOwners_;
    std::mutex mutex_;
};
}
}
}
#endif  // ALLOCATOR_MANAGER_H
