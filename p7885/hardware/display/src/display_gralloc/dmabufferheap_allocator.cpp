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
#include "dmabufferheap_allocator.h"
#include <cerrno>
#include <cstdlib>
#include <securec.h>
#include "display_common.h"
#include "dmabuf_alloc.h"
namespace OHOS {
namespace HDI {
namespace DISPLAY {
int32_t DmaBufferHeapAllocator::Init()
{
    DISPLAY_LOGD();
    static constexpr const char *heapName = "system";
    deviceFd_ = DmabufHeapOpen(heapName);
    if (deviceFd_ < 0) {
        DISPLAY_LOGE("Failed to open dmabufferheap errno %{public}d", errno);
        return DISPLAY_FAILURE;
    }
    return DISPLAY_SUCCESS;
}

int32_t DmaBufferHeapAllocator::Allocate(const BufferInfo &bufferInfo, BufferHandle &handle)
{
    DISPLAY_LOGD();
    DmabufHeapBuffer buffer;
    buffer.size = bufferInfo.size_;
    buffer.heapFlags = 0;
    DmabufHeapBufferAlloc(deviceFd_, &buffer);
    handle.fd = buffer.fd;
    handle.size = bufferInfo.size_;
    handle.format = bufferInfo.format_;
    handle.width = bufferInfo.width_;
    handle.height = bufferInfo.height_;
    handle.stride = bufferInfo.widthStride_ * bufferInfo.bytesPerPixel_;
    handle.usage = bufferInfo.usage_;
    handle.phyAddr = 0;
    handle.virAddr = nullptr;
    handle.reserveFds = 0;
    handle.reserveInts = 0;
    return DISPLAY_SUCCESS;
}

int32_t DmaBufferHeapAllocator::Allocate(const BufferInfo &bufferInfo, BufferHandle **handle)
{
    DISPLAY_CHK_RETURN((handle == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("handle is nullptr"));
    BufferHandle *priBuffer = static_cast<BufferHandle *>(malloc(sizeof(BufferHandle)));
    DISPLAY_CHK_RETURN((priBuffer == nullptr), DISPLAY_NOMEM, DISPLAY_LOGE("malloc buffer handle failed"));
    errno_t eok = memset_s(priBuffer, sizeof(BufferHandle), 0, sizeof(BufferHandle));
    if (eok != EOK) {
        free(priBuffer);
        DISPLAY_LOGE("memset_s failed");
        return DISPLAY_FAILURE;
    }
    priBuffer->fd = -1;
    int32_t ret = Allocate(bufferInfo, *priBuffer);
    if (ret != DISPLAY_SUCCESS) {
        free(priBuffer);
        return ret;
    }
    *handle = priBuffer;
    return DISPLAY_SUCCESS;
}

DmaBufferHeapAllocator::~DmaBufferHeapAllocator()
{
    DISPLAY_LOGD();
    if (deviceFd_ >= 0) {
        DmabufHeapClose(deviceFd_);
        deviceFd_ = -1;
    }
}
} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS
