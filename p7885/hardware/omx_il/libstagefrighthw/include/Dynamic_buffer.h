/*
 * Copyright 2016-2026 Unisoc (Shanghai) Technologies Co., Ltd.
 *
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
#pragma once
#include "buffer_handle.h"
#include <unistd.h>
namespace OHOS {
namespace OMX {
static int32_t FreeBufferHandle(BufferHandle *handle)
{
    if (handle == nullptr) {
        OMX_LOGW("FreeBufferHandle with nullptr handle");
        return 0;
    }
    if (handle->fd >= 0) {
        close(handle->fd);
        handle->fd = -1;
    }
    const uint32_t reserveFds = handle->reserveFds;
    for (uint32_t i = 0; i < reserveFds; i++) {
        if (handle->reserve[i] >= 0) {
            close(handle->reserve[i]);
            handle->reserve[i] = -1;
        }
    }
    free(handle);
    return 0;
}
struct DynamicBuffer {
    int32_t type;
    BufferHandle *bufferHandle;
    DynamicBuffer() : type(0), bufferHandle(nullptr)
    { }
    ~DynamicBuffer()
    {
        type = 0;
        if (bufferHandle != nullptr) {
            FreeBufferHandle(bufferHandle);
        }
        bufferHandle = nullptr;
    }
    DynamicBuffer(const DynamicBuffer& other) = delete;
    DynamicBuffer(DynamicBuffer&& other) = delete;
    DynamicBuffer& operator=(const DynamicBuffer& other) = delete;
    DynamicBuffer& operator=(DynamicBuffer&& other) = delete;
};
}
}
