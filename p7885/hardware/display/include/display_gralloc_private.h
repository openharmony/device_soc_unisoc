/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef DISPLAY_GRALLOC_INTERNAL_H
#define DISPLAY_GRALLOC_INTERNAL_H
#include "v1_0/display_buffer_type.h"
#include "v1_0/display_composer_type.h"
namespace OHOS {
namespace HDI {
namespace DISPLAY {
using namespace OHOS::HDI::Display::Composer::V1_0;
using namespace OHOS::HDI::Display::Buffer::V1_0;

#define GRALLOC_NUM_FDS 1
#define GRALLOC_NUM_INTS ((sizeof(struct PrivBufferHandle) - sizeof(BufferHandle)) / sizeof(int) - GRALLOC_NUM_FDS)

#define INVALID_PIXEL_FMT 0

typedef struct {
    BufferHandle hdl;
} PriBufferHandle;
/**
 * @brief Defines pointers to the memory driver functions.
 */
typedef struct {
    /**
     * @brief Allocates memory based on the parameters passed by the GUI.
     *
     * @param info Indicates the pointer to the description info of the memory to allocate.
     * @param handle Indicates the double pointer to the buffer of the memory to allocate.
     *
     * @return Returns <b>0</b> if the operation is successful; returns an error code defined in {@link DispErrCode}
     * otherwise.
     * @since 1.0
     * @version 1.0
     */
    int32_t (*AllocMem)(const AllocInfo* info, BufferHandle** handle);

    /**
     * @brief Releases memory.
     *
     * @param handle Indicates the pointer to the buffer of the memory to release.
     *
     * @since 1.0
     * @version 1.0
     */
    void (*FreeMem)(BufferHandle *handle);

    /**
     * @brief Maps memory to memory without cache in the process's address space.
     *
     * @param handle Indicates the pointer to the buffer of the memory to map.
     *
     * @return Returns the pointer to a valid address if the operation is successful; returns <b>NULL</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    void *(*Mmap)(BufferHandle *handle);

    /**
     * @brief Maps memory to memory with cache in the process's address space.
     *
     * @param handle Indicates the pointer to the buffer of the memory to map.
     *
     * @return Returns the pointer to a valid address if the operation is successful; returns <b>NULL</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    void *(*MmapCache)(BufferHandle *handle);

    /**
     * @brief Unmaps memory, that is, removes any mappings from the process's address space.
     *
     * @param handle Indicates the pointer to the buffer of the memory to unmap.
     *
     * @return Returns <b>0</b> if the operation is successful; returns an error code defined in {@link DispErrCode}
     * otherwise.
     * @since 1.0
     * @version 1.0
     */
    int32_t (*Unmap)(BufferHandle *handle);

    /**
     * @brief Flushes data from the cache to memory and invalidates the data in the cache.
     *
     * @param handle Indicates the pointer to the buffer of the cache to flush.
     *
     * @return Returns <b>0</b> if the operation is successful; returns an error code defined in {@link DispErrCode}
     * otherwise.
     * @since 1.0
     * @version 1.0
     */
    int32_t (*FlushCache)(BufferHandle *handle);

    /**
     * @brief Flushes data from the cache mapped via {@link Mmap} to memory and invalidates the data in the cache.
     *
     * @param handle Indicates the pointer to the buffer of the cache to flush.
     *
     * @return Returns <b>0</b> if the operation is successful; returns an error code defined in {@link DispErrCode}
     * otherwise.
     * @since 1.0
     * @version 1.0
     */
    int32_t (*FlushMCache)(BufferHandle *handle);

    /**
     * @brief Invalidates the cache to update it from memory.
     *
     * @param handle Indicates the pointer to the buffer of the cache, which will been invalidated.
     *
     * @return Returns <b>0</b> if the operation is successful; returns an error code defined in {@link DispErrCode}
     * otherwise.
     * @since 1.0
     * @version 1.0
     */
    int32_t (*InvalidateCache)(BufferHandle* handle);

    /**
     * @brief Checks whether the given VerifyAllocInfo array is allocatable.
     *
     * @param num Indicates the size of infos array.
     * @param infos Indicates the pointer to the VerifyAllocInfo array.
     * @param supporteds Indicates the pointer to the array that can be allocated.
     *
     * @return Returns <b>0</b> if the operation is successful; returns an error code defined in {@link DispErrCode}
     * otherwise.
     * @since 1.0
     * @version 1.0
     */
    int32_t (*IsSupportedAlloc)(uint32_t num, const VerifyAllocInfo *infos, bool *supporteds);

    /**
     * @brief Maps memory for YUV.
     *
     * @param handle Indicates the pointer to the buffer of the memory to map.
     * @param info Indicates the pointer to the YUVDescInfo of the memory to map.
     *
     * @return Returns the pointer to a valid address if the operation is successful; returns <b>NULL</b> otherwise.
     * @since 3.2
     * @version 1.0
     */
    void *(*MmapYUV)(BufferHandle *handle, YUVDescInfo *info);
} GrallocFuncs;

/**
 * @brief Initializes the memory module to obtain the pointer to functions for memory operations.
 *
 * @param funcs Indicates the double pointer to functions for memory operations. Memory is allocated automatically when
 * you initiate the memory module initialization, so you can simply use the pointer to gain access to the functions.
 *
 * @return Returns <b>0</b> if the operation is successful; returns an error code defined in {@link DispErrCode}
 * otherwise.
 * @since 1.0
 * @version 1.0
 */
int32_t GrallocInitialize(GrallocFuncs **funcs);

/**
 * @brief Deinitializes the memory module to release the memory allocated to the pointer to functions for memory
 * operations.
 *
 * @param funcs Indicates the pointer to functions for memory operations.
 *
 * @return Returns <b>0</b> if the operation is successful; returns an error code defined in {@link DispErrCode}
 * otherwise.
 * @since 1.0
 * @version 1.0
 */
int32_t GrallocUninitialize(GrallocFuncs *funcs);

} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS
#endif