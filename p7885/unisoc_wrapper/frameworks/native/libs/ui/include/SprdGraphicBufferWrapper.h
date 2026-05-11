/*
 * Copyright (C) 2023 HiHope Open Source Organization.
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

#ifndef SPRD_GRAPHIC_BUFFER_WRAPPER_H
#define SPRD_GRAPHIC_BUFFER_WRAPPER_H

#include <stdint.h>


/**
 * @brief Compatible structure mirroring sprd_ar_src's native_handle_t.
 *        The closed-source wrapper returns a pointer to this layout.
 */
typedef struct {
    int version;    /* sizeof(NativeHandle) */
    int numFds;     /* number of file-descriptors in data[] */
    int numInts;    /* number of ints in data[] */
    int data[0];    /* numFds file descriptors, then numInts ints */
} NativeHandle;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ADAPTER_PIXEL_FORMAT_UNKNOWN       = 0,
    ADAPTER_PIXEL_FORMAT_RGBA_8888     = 1,
    ADAPTER_PIXEL_FORMAT_RGBX_8888     = 2,
    ADAPTER_PIXEL_FORMAT_RGB_888       = 3,
    ADAPTER_PIXEL_FORMAT_RGB_565       = 4,
    ADAPTER_PIXEL_FORMAT_BGRA_8888     = 5,
    ADAPTER_PIXEL_FORMAT_RGBA_5551     = 6,
    ADAPTER_PIXEL_FORMAT_RGBA_4444     = 7,
    ADAPTER_PIXEL_FORMAT_YVU420_SP     = 0x11,
    ADAPTER_PIXEL_FORMAT_YUV422_SP     = 0x10,
} AdapterPixelFormat;

typedef struct {
    uint32_t width;
    uint32_t height;
    AdapterPixelFormat format;
    uint64_t usage;
    uint32_t stride;
    NativeHandle *handle;
} AdapterGraphicBuffer;

/**
 * @brief Allocate a graphic buffer.
 * @param width Buffer width
 * @param height Buffer height
 * @param format Buffer format
 * @param usage Buffer usage
 * @param requestName Name for the allocation
 * @return void* Pointer to the internal tracking object, NULL on failure.
 */
void* AdapterGraphicBufferAllocate(uint32_t width, uint32_t height,
    AdapterPixelFormat format, uint64_t usage,
    const char *requestName);

/**
 * @brief Free the graphic buffer.
 * @param buffer Pointer returned by AdapterGraphicBufferAllocate
 */
void AdapterGraphicBufferFree(void *buffer);

/**
 * @brief Get the native handle from the allocated buffer.
 * @param buffer Pointer returned by AdapterGraphicBufferAllocate
 * @return NativeHandle* The native handle (mirrors native_handle_t layout)
 */
NativeHandle* AdapterGraphicBufferGetHandle(void *buffer);

#ifdef __cplusplus
}
#endif

#endif // SPRD_GRAPHIC_BUFFER_WRAPPER_H
