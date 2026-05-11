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
#ifndef SPRD_OMX_COMPONENT_BASE_H
#define SPRD_OMX_COMPONENT_BASE_H
#include "refbase.h"
#include "utils/omx_log.h"
#define OMX_SPRD_COLOR_FORMAT_YV_U420_SEMI_PLANAR ((OMX_COLOR_FORMATTYPE)0x7FD00001)
#define OMX_COLOR_FORMAT_OPAQUE ((OMX_COLOR_FORMATTYPE)0x7F000789)
#define OMX_COLOR_FORMAT_YU_V420_FLEXIBLE ((OMX_COLOR_FORMATTYPE)0x7F420888)

#include <type_traits>
#include <cstddef>

namespace OHOS {
namespace OMX {
typedef struct SprdOMXComponentBase : public OHOS::RefBase {
} SprdOMXComponentBase;

template<typename T>
bool IsValidOmxParam(T *a)
{
    // 确保类型是标准布局类型，以便安全使用offsetof
    static_assert(std::is_standard_layout_v<T>,
                  "T must be a standard layout type for offsetof");
    
    // 使用std::remove_reference_t来获取原始类型，因为decltype(*a)返回引用类型
    using raw_type = std::remove_reference_t<decltype(*a)>;
    static_assert(offsetof(raw_type, nSize) == 0, "nSize not at offset 0");
    static_assert(std::is_same_v<std::remove_reference_t<decltype(a->nSize)>, OMX_U32>, "nSize has wrong type");
    static_assert(offsetof(raw_type, nVersion) == 4, "nVersion not at offset 4");
    static_assert(std::is_same_v<std::remove_reference_t<decltype(a->nVersion)>, OMX_VERSIONTYPE>,
                  "nVersion has wrong type");
    
    if (a->nSize < sizeof(*a)) {
        OMX_LOGE("b/27207275: need %zu, got %u", sizeof(*a), a->nSize);
        return false;
    }
    return true;
}
}  // namespace OMX
}  // namespace OHOS
#endif  // SPRD_OMX_COMPONENT_BASE_H
