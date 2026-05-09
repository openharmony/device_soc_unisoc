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

#ifndef DISP_COMMON_H
#define DISP_COMMON_H
#include <string.h>
#include <cstdint>
#include <unistd.h>
#include "hilog/log.h"
#include "stdio.h"
#ifdef HDF_LOG_TAG
#undef HDF_LOG_TAG
#endif

#undef LOG_TAG
#undef LOG_DOMAIN
#define LOG_TAG "DISP"
#define LOG_DOMAIN 0xD001400

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef DISPLAY_UNUSED
#if defined(__cplusplus)
} // extern "C"
template <typename T>
inline void DISPLAY_UNUSED(T &&) {}
extern "C" {
#else
static inline void DISPLAY_UNUSED(const void *x) { (void)x; }
#endif
#endif


#define DISP_FILENAME (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)

#ifndef DISPLAY_DEBUG_ENABLE
#define DISPLAY_DEBUG_ENABLE 1
#endif

#ifndef DISPLAY_LOGD
#define DISPLAY_LOGD(format, ...)                                                                                     \
    do {                                                                                                              \
        if (DISPLAY_DEBUG_ENABLE) {                                                                                   \
            HILOG_DEBUG(LOG_CORE, "[%{public}s@%{public}s:%{public}d] " format "\n",                                  \
                __FUNCTION__, DISP_FILENAME, __LINE__,                                                                \
                ##__VA_ARGS__);                                                                                       \
        }                                                                                                             \
    } while (0)
#endif

#ifndef DISPLAY_LOGI
#define DISPLAY_LOGI(format, ...)                                                                                     \
    do {                                                                                                              \
        HILOG_INFO(LOG_CORE, "[%{public}s@%{public}s:%{public}d] " format "\n", __FUNCTION__, DISP_FILENAME, __LINE__, \
            ##__VA_ARGS__);                                                                                           \
    } while (0)
#endif

#ifndef DISPLAY_LOGW
#define DISPLAY_LOGW(format, ...)                                                                                     \
    do {                                                                                                              \
        HILOG_WARN(LOG_CORE, "[%{public}s@%{public}s:%{public}d] " format "\n", __FUNCTION__, DISP_FILENAME, __LINE__, \
            ##__VA_ARGS__);                                                                                           \
    } while (0)
#endif

#ifndef DISPLAY_LOGE
#define DISPLAY_LOGE(format, ...)                                 \
    do {                                                          \
        HILOG_ERROR(LOG_CORE,                                     \
            "\033[0;32;31m"                                       \
            "[%{public}s@%{public}s:%{public}d] " format "\033[m" \
            "\n",                                                 \
            __FUNCTION__, DISP_FILENAME, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif

// CHECK_NULLPOINTER_RETURN_VALUE and CHECK_NULLPOINTER_RETURN are removed to comply with G.PRE.02-CPP.
// Use explicit if checks and DISPLAY_LOGE instead.

#ifndef DISPLAY_CHK_RETURN
#define DISPLAY_CHK_RETURN(val, ret, ...) \
    do {                                  \
        if (val) {                        \
            __VA_ARGS__;                  \
            return (ret);                 \
        }                                 \
    } while (0)
#endif

#ifndef DISPLAY_CHK_RETURN_NOT_VALUE
#define DISPLAY_CHK_RETURN_NOT_VALUE(val, ...) \
    do {                                            \
        if (val) {                                  \
            __VA_ARGS__;                            \
            return;                                 \
        }                                           \
    } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* DISP_COMMON_H */
