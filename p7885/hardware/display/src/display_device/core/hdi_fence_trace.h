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
#ifndef HDI_FENCE_TRACE_H
#define HDI_FENCE_TRACE_H

#include <atomic>
#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include "display_common.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
namespace FenceTrace {
namespace {
constexpr uint64_t FENCE_TRACE_LOG_INTERVAL = 128;
}

inline std::atomic<int64_t>& OwnedFenceCount()
{
    static std::atomic<int64_t> value(0);
    return value;
}

inline std::atomic<uint64_t>& AssignCount()
{
    static std::atomic<uint64_t> value(0);
    return value;
}

inline std::atomic<uint64_t>& DupCount()
{
    static std::atomic<uint64_t> value(0);
    return value;
}

inline std::atomic<uint64_t>& CloseCount()
{
    static std::atomic<uint64_t> value(0);
    return value;
}

inline std::atomic<uint64_t>& SampleCount()
{
    static std::atomic<uint64_t> value(0);
    return value;
}

inline std::atomic<int64_t>& OwnedBufferFdCount()
{
    static std::atomic<int64_t> value(0);
    return value;
}

inline std::atomic<uint64_t>& BufferDupCount()
{
    static std::atomic<uint64_t> value(0);
    return value;
}

inline std::atomic<uint64_t>& BufferCloseCount()
{
    static std::atomic<uint64_t> value(0);
    return value;
}

inline std::atomic<uint64_t>& ExportFenceCount()
{
    static std::atomic<uint64_t> value(0);
    return value;
}

inline std::atomic<uint64_t>& CommitExportFenceCount()
{
    static std::atomic<uint64_t> value(0);
    return value;
}

inline std::atomic<uint64_t>& ReleaseExportFenceCount()
{
    static std::atomic<uint64_t> value(0);
    return value;
}

inline int32_t CountProcessFds()
{
    DIR* dir = opendir("/proc/self/fd");
    if (dir == nullptr) {
        return -errno;
    }

    int32_t count = 0;
    for (dirent* entry = readdir(dir); entry != nullptr; entry = readdir(dir)) {
        if (entry->d_name[0] == '.' &&
            (entry->d_name[1] == '\0' || strcmp(entry->d_name, "..") == 0)) {
            continue;
        }
        ++count;
    }
    closedir(dir);
    return count;
}

inline void DumpStats(const char* stage, const char* tag, int32_t oldFd, int32_t newFd)
{
    DISPLAY_LOGI("FENCE_TRACE stage=%{public}s tag=%{public}s oldFd=%{public}d newFd=%{public}d "
                 "owned=%{public}" PRId64 " dup=%{public}" PRIu64 " assign=%{public}" PRIu64
                 " close=%{public}" PRIu64 " bufOwned=%{public}" PRId64 " bufDup=%{public}" PRIu64
                 " bufClose=%{public}" PRIu64 " export=%{public}" PRIu64
                 " commitExport=%{public}" PRIu64 " releaseExport=%{public}" PRIu64 " procFd=%{public}d",
        stage, tag, oldFd, newFd, OwnedFenceCount().load(), DupCount().load(),
        AssignCount().load(), CloseCount().load(), OwnedBufferFdCount().load(), BufferDupCount().load(),
        BufferCloseCount().load(), ExportFenceCount().load(), CommitExportFenceCount().load(),
        ReleaseExportFenceCount().load(), CountProcessFds());
}

inline void Sample(const char* stage, const char* tag, int32_t oldFd, int32_t newFd, bool force = false)
{
    uint64_t sample = SampleCount().fetch_add(1) + 1;
    if (force || (sample % FENCE_TRACE_LOG_INTERVAL) == 0) {
        DumpStats(stage, tag, oldFd, newFd);
    }
}

inline void RecordAssign(const char* tag, int32_t oldFd, int32_t newFd)
{
    AssignCount().fetch_add(1);
    if (oldFd >= 0) {
        CloseCount().fetch_add(1);
        OwnedFenceCount().fetch_sub(1);
    }
    if (newFd >= 0) {
        OwnedFenceCount().fetch_add(1);
    }
    bool force = (oldFd >= 0 || newFd >= 0);
    Sample("assign", tag, oldFd, newFd, force);
}

inline void RecordConstruct(const char* tag, int32_t fd)
{
    if (fd >= 0) {
        OwnedFenceCount().fetch_add(1);
    }
    Sample("construct", tag, -1, fd);
}

inline void RecordClose(const char* tag, int32_t fd)
{
    if (fd >= 0) {
        CloseCount().fetch_add(1);
        OwnedFenceCount().fetch_sub(1);
    }
    Sample("close", tag, fd, -1);
}

inline void RecordDup(const char* tag, int32_t srcFd, int32_t dupFd)
{
    if (dupFd >= 0) {
        DupCount().fetch_add(1);
    }
    Sample("dup", tag, srcFd, dupFd, dupFd < 0);
}

inline void RecordBufferDup(const char* tag, int32_t srcFd, int32_t dupFd)
{
    if (dupFd >= 0) {
        BufferDupCount().fetch_add(1);
        OwnedBufferFdCount().fetch_add(1);
    }
    Sample("buffer_dup", tag, srcFd, dupFd, dupFd < 0);
}

inline void RecordBufferClose(const char* tag, int32_t fd)
{
    if (fd >= 0) {
        BufferCloseCount().fetch_add(1);
        OwnedBufferFdCount().fetch_sub(1);
    }
    Sample("buffer_close", tag, fd, -1);
}

inline void RecordExportFence(const char* tag, int32_t srcFd, int32_t exportFd)
{
    if (exportFd >= 0) {
        ExportFenceCount().fetch_add(1);
        if (tag != nullptr) {
            if (strstr(tag, "Commit") != nullptr) {
                CommitExportFenceCount().fetch_add(1);
            } else if (strstr(tag, "GetDisplayReleaseFence") != nullptr) {
                ReleaseExportFenceCount().fetch_add(1);
            }
        }
    }
    Sample("export", tag, srcFd, exportFd, exportFd < 0);
}
} // namespace FenceTrace
} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS

#endif // HDI_FENCE_TRACE_H
