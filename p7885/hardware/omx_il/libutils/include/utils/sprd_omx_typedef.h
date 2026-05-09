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
#ifndef VIDEO_MEM_ALLOCATOR_GUARD
#define VIDEO_MEM_ALLOCATOR_GUARD
#pragma once
#include <mutex>
#include <utils/Errors.h>
#include "VideoMemAllocator.h"
[[maybe_unused]] static void TrespassFunc() {}

template<typename T, size_t N>
static size_t arraySizeFunc(const T (&)[N])
{
    return N;
}

[[maybe_unused]] static size_t alignFunc(size_t x, size_t a)
{
    return (((x) + (a) - 1) & ~((a) - 1));
}
template <typename T>
using Vector = std::vector<T>;
typedef  std::mutex Mutex;
typedef std::lock_guard<std::mutex> AutoMutex;
typedef OHOS::OMX::VideoMemAllocator MemIon;
typedef OHOS::sptr<OHOS::OMX::VideoMemAllocator> PMemIon;
typedef unsigned char uint8;
#define VERSIONMAJOR_NUMBER                1
#define VERSIONMINOR_NUMBER                0
#define REVISION_NUMBER                    0
#define STEP_NUMBER                        0
typedef enum OMX_INDEXEXEXTTYPE {
    OMX_INDEX_ROCKCHIP_EXTENSIONS = 0x70000000,
    OMX_INDEX_PARAM_RK_ENC_EXTENDED_VIDEO,
} OMX_INDEXEXEXTTYPE;
#define OHOS_INDEX_PARAM_EXTENDED_VIDEO \
    "OMX.Topaz.index.param.extended_video"
#endif // VIDEO_MEM_ALLOCATOR_GUARD
