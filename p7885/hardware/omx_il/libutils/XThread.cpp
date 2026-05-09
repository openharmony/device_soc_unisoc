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
#define LOG_TAG "XThread"
#include <utils/omx_log.h>
#include "XThread.h"
#include <cstring>
#include <new>
#include <ctime>
#include <cerrno>
#include <unistd.h>
namespace OHOS {
namespace OMX {
static bool JoinThreadWithRetry(pthread_t threadToWait, int maxRetries, int retryIntervalUs)
{
    for (int i = 0; i < maxRetries; ++i) {
        int ret = pthread_join(threadToWait, nullptr);
        if (ret == 0 || ret == ESRCH) {
            return true;
        }
        usleep(retryIntervalUs);
    }
    return false;
}

XThread::XThread() : exit(false), pt(0), mName(nullptr)
{
}
XThread::~XThread()
{
    if (mName != nullptr) {
        delete[] mName;
        mName = nullptr;
    }
}
void XThread::setName(const char *name)
{
    // Step 1: Free existing name buffer (original logic preserved)
    if (mName != nullptr) {
        delete[] mName;
        mName = nullptr;
    }
    // Step 2: Critical null check for input name (prevents strlen(nullptr) crash)
    if (name == nullptr) {
        mName = nullptr;
        return;
    }
    // Step 3: Calculate string length (safe now that name is non-null)
    size_t len = strlen(name);  // Use size_t (unsigned) for length (matches memmove_s params)
    // Step 4: Allocate new buffer (use std::nothrow to avoid exceptions in embedded systems)
    mName = new (std::nothrow) char[len + 1];
    if (mName == nullptr) { // Check for memory allocation failure
        OMX_LOGE("XThread::setName: Failed to allocate memory for thread name (len=%zu)", len);
        return;
    }
    // Step 5: Replace memcpy with safe memmove_s
    // - dest: mName (newly allocated buffer)
    // - destsz: Total size of mName (len + 1, ensures no overflow)
    // - src: name (input string)
    // - count: Number of bytes to copy (len, matches original memcpy logic)
    errno_t ret = memmove_s(
        mName,      // Destination buffer
        len + 1,    // Total size of destination buffer (critical for safety)
        name,       // Source string
        len);         // Number of bytes to copy (excludes null terminator)
    // Step 6: Handle memmove_s failure (roll back memory allocation)
    if (ret != 0) {
        OMX_LOGE("XThread::setName: memmove_s failed (ret=%d, len=%zu)", ret, len);
        delete[] mName;    // Clean up failed allocation
        mName = nullptr;
        return;
    }
    // Step 7: Null-terminate the string (original logic preserved)
    mName[len] = '\0';     // Use '\0' instead of 0 for readability (equivalent)
}
void XThread::start()
{
    exit = false;
    pthread_create(&pt, nullptr, XThread::threadFun, this);
}
void XThread::requestExit()
{
    exit = true;
}
void XThread::stop()
{
    exit = true;
    if (pt != 0) {
        const int maxRetries = 10;
        const int retryIntervalUS = 100000;
        pthread_t threadToWait = pt;
        pt = 0;
        if (!JoinThreadWithRetry(threadToWait, maxRetries, retryIntervalUS)) {
            OMX_LOGW("Thread %s did not exit within 1s timeout, detaching",
                     mName != nullptr ? mName : "(nullptr)");
            pthread_detach(threadToWait);
        }
    }
}
bool XThread::isExit()
{
    return exit;
}
void *XThread::threadFun(void *val)
{
    XThread *th = (XThread *) val;
    OMX_LOGI("Thread %s is Running ...", th->mName != nullptr ? th->mName : "(nullptr)");
    while (!th->exit) {
        if (!th->loop()) {
            OMX_LOGI("Thread %s break loop.", th->mName != nullptr ? th->mName : "(nullptr)");
            break;
        }
    }
    OMX_LOGI("Thread %s is Stopped ...", th->mName != nullptr ? th->mName : "(nullptr)");
    return nullptr;
}
};  // namespace OMX
};  // namespace OHOS
