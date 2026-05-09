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

#ifndef SPRD_ALLOCATOR_H_
#define SPRD_ALLOCATOR_H_

#include <cstdint>
#include <mutex>
#include <map>
#include "allocator.h"
#include "SprdGraphicBufferWrapper.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {

class SprdAllocator : public Allocator {
public:
    int32_t Init() override;
    int32_t Allocate(const BufferInfo &bufferInfo, BufferHandle &handle) override;
    int32_t Allocate(const BufferInfo &bufferInfo, BufferHandle **handle) override;
    int32_t FreeMem(BufferHandle *handle)override;
    ~SprdAllocator() override;

protected:
    int32_t InitBufferhandle(const BufferInfo &bufferInfo, BufferHandle **handle);
    uint64_t ConvertUsageToGpu(uint64_t inUsage);
    AdapterPixelFormat ConvertFormatToGpu(PixelFormat inFormat);
    
private:
    NativeHandle *mHandle;
    uint32_t mStride;
    uint64_t mUsage;
    AdapterPixelFormat mFormat;
    std::mutex m;
    // 跟踪分配的 wrapper 对象生命周期，key 为 BufferHandle 指针
    std::map<BufferHandle*, void*> mBufferMap;
};

} /*namespace DISPLAY*/
} /*namespace HDI*/
} /*namespace OHOS*/

#endif /*_SPRD_ALLOCATOR_H_*/