/*
 * Copyright (C) 2025 Archermind Technology Co., Ltd.
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
#ifndef OMX_LOG
#define OMX_LOG
#include <hilog/log.h>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <securec.h>
#ifdef __cplusplus
extern "C"
{
#endif
#undef LOG_TAG
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD001407
#define LOG_TAG "OMXUTILS"
#define FILENAME_ONLY (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)
// Log level enumeration
enum OMXLogLevel {
    OMX_LOG_LEVEL_INFO = 0,
    OMX_LOG_LEVEL_DEBUG,
    OMX_LOG_LEVEL_WARN,
    OMX_LOG_LEVEL_ERROR,
    OMX_LOG_LEVEL_FATAL
};
// Maximum log length
const int OMX_MAX_LOG_LENGTH = 2048;

[[maybe_unused]] static void OMXDispatchLog(int level, const char *message)
{
    switch (level) {
        case OMX_LOG_LEVEL_INFO:
            HILOG_INFO(LOG_CORE, "[OMX] %{public}s", message);
            break;
        case OMX_LOG_LEVEL_DEBUG:
            HILOG_DEBUG(LOG_CORE, "[OMX] %{public}s", message);
            break;
        case OMX_LOG_LEVEL_WARN:
            HILOG_WARN(LOG_CORE, "[OMX] %{public}s", message);
            break;
        case OMX_LOG_LEVEL_ERROR:
            HILOG_ERROR(LOG_CORE, "[OMX] %{public}s", message);
            break;
        case OMX_LOG_LEVEL_FATAL:
            HILOG_FATAL(LOG_CORE, "[OMX] %{public}s", message);
            break;
        default:
            HILOG_INFO(LOG_CORE, "[OMX] %{public}s", message);
            break;
    }
}

/**
 * @brief Wrapper function for OMX logging
 *
 * This function provides a unified logging interface that handles string formatting
 * before passing to the HILOG system, avoiding format string issues in macro expansions.
 *
 * @param level Log level (OMX_LOG_LEVEL_*)
 * @param format Format string with placeholders
 * @param ... Variable arguments for the format string
 */
[[maybe_unused]] static bool OMXFormatLog(char *logStr, size_t logLen, const char *format, va_list args)
{
    (void)memset_s(logStr, logLen, 0, logLen);
    if (vsprintf_s(logStr, logLen, format, args) < 0) {
        HILOG_ERROR(LOG_CORE, "OMXWrapperLog: Failed to format log string");
        return false;
    }
    return true;
}

[[maybe_unused]] static void OMXWrapperLog(int level, const char *format, ...)
{
    char logStr[OMX_MAX_LOG_LENGTH];
    va_list args;
    va_start(args, format);
    if (!OMXFormatLog(logStr, sizeof(logStr), format, args)) {
        va_end(args);
        return;
    }
    va_end(args);
    OMXDispatchLog(level, logStr);
}
// Error level log macro
#ifndef OMX_LOGE
#define OMX_LOGE(format, ...) \
    do { \
        OMXWrapperLog(OMX_LOG_LEVEL_ERROR, "[%s@%s:%d] " format, \
            __FUNCTION__, FILENAME_ONLY, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif
// Info level log macro
#ifndef OMX_LOGI
#define OMX_LOGI(format, ...) \
    do { \
        OMXWrapperLog(OMX_LOG_LEVEL_INFO, "[%s@%s:%d] " format, \
            __FUNCTION__, FILENAME_ONLY, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif
// Verbose level log macro (mapped to WARN for compatibility)
#ifndef OMX_LOGV
#define OMX_LOGV(format, ...) \
    do { \
        OMXWrapperLog(OMX_LOG_LEVEL_WARN, "[%s@%s:%d] " format, \
            __FUNCTION__, FILENAME_ONLY, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif
// Warning level log macro
#ifndef OMX_LOGW
#define OMX_LOGW(format, ...) \
    do { \
        OMXWrapperLog(OMX_LOG_LEVEL_WARN, "[%s@%s:%d] " format, \
            __FUNCTION__, FILENAME_ONLY, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif
// Debug level log macro
#ifndef OMX_LOGD
#define OMX_LOGD(format, ...) \
    do { \
        OMXWrapperLog(OMX_LOG_LEVEL_DEBUG, "[%s@%s:%d] " format, \
            __FUNCTION__, FILENAME_ONLY, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif
// Direct log macro for special cases (bypasses WrapperLog)
#ifndef OMX_DIRECT_LOGE
#define OMX_DIRECT_LOGE(format, ...) \
    do { \
        HILOG_ERROR(LOG_CORE, "[%{public}s@%{public}s:%{public}d] " format "\n", \
            __FUNCTION__, FILENAME_ONLY, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif


// Type-safe formatting helper functions
// Function for formatting pointers
[[maybe_unused]] static void *OmxLogPtrFunc(void *ptr)
{
    return ptr;
}

// Functions for formatting 64-bit integers
[[maybe_unused]] static long long OmxLogI64Func(long long value)
{
    return static_cast<long long>(value);
}

[[maybe_unused]] static unsigned long long OmxLogU64Func(unsigned long long value)
{
    return static_cast<unsigned long long>(value);
}

// Function for formatting size_t values
[[maybe_unused]] static size_t OmxLogSizeTFunc(size_t value)
{
    return static_cast<size_t>(value);
}
#ifdef __cplusplus
}
#endif
#endif // OMX_LOG
