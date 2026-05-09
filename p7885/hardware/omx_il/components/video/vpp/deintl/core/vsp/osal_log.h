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
#ifndef OMX_OSAL_LOG_H
#define OMX_OSAL_LOG_H
#include <hilog/log.h>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <utils/omx_log.h>
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern "C"
{
#endif
#undef LOG_TAG
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD001406
#define LOG_TAG "VSPDEINTAPI"
#define OMX_DEBUG_LEVEL 4 /* _LOG_INFO */
#ifndef SPRD_CODEC_LOGE
#define SPRD_CODEC_LOGE(format, ...) \
    do { \
        HILOG_ERROR(LOG_CORE, "[%{public}s@%{public}s:%{public}d] " format "\n", \
            __FUNCTION__, FILENAME_ONLY, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif
#ifndef SPRD_CODEC_LOGI
#define SPRD_CODEC_LOGI(format, ...) \
    do { \
        HILOG_INFO(LOG_CORE, "[%{public}s@%{public}s:%{public}d] " format "\n", \
            __FUNCTION__, FILENAME_ONLY, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif
#ifndef SPRD_CODEC_LOGV
#define SPRD_CODEC_LOGV(format, ...) \
    do { \
        HILOG_WARN(LOG_CORE, "[%{public}s@%{public}s:%{public}d] " format "\n", \
            __FUNCTION__, FILENAME_ONLY, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif
#ifndef SPRD_CODEC_LOGW
#define SPRD_CODEC_LOGV(format, ...) \
    do { \
        HILOG_WARN(LOG_CORE, "[%{public}s@%{public}s:%{public}d] " format "\n", \
            __FUNCTION__, FILENAME_ONLY, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif
#ifndef SPRD_CODEC_LOGD
#define SPRD_CODEC_LOGD(format, ...) \
    do { \
        HILOG_DEBUG(LOG_CORE, "[%{public}s@%{public}s:%{public}d] " format "\n", \
            __FUNCTION__, FILENAME_ONLY, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //OMX_OSAL_LOG_H
