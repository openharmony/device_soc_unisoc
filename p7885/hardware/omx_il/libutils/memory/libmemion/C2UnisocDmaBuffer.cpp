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
#define LOG_NDEBUG 0
#define LOG_TAG "C2UnisocDmaBuffer"
#include <utils/omx_log.h>
#include <cerrno>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include "dmaphyaddr-defs.h"
#include "C2UnisocDmaBuffer.h"
template<typename Func>
static auto OmxFailureRetryFunc(Func exp) -> decltype(exp())
{
    decltype(exp()) rc;
    do {
        rc = exp();
    } while (rc == -1 && errno == EINTR);
    return rc;
}
namespace OHOS {
namespace OMX {
C2UnisocDmaBuffer::C2UnisocDmaBuffer(const char *heapName, size_t bufferLen)
    : mFd(-1),
    mBase(nullptr),
    mSize(bufferLen),
    mHeapName(heapName ? heapName : "")
{
    if (heapName) {
        allocateFromDmaHeap(heapName, bufferLen);
    }
}
C2UnisocDmaBuffer::~C2UnisocDmaBuffer()
{
    if (mBase != nullptr && mBase != MAP_FAILED) {
        munmap(mBase, mSize);
        mBase = nullptr;
    }

    if (mFd >= 0) {
        close(mFd);
        mFd = -1;
    }
}
bool C2UnisocDmaBuffer::allocateFromDmaHeap(const char *heapName, size_t bufferLen)
{
    std::string heapPath = "/dev/dma_heap/";
    heapPath += heapName;
    OMX_LOGD("Opening DMA heap: %s", heapPath.c_str());
    int heapFd = OmxFailureRetryFunc([&]() {
        return open(heapPath.c_str(), O_RDONLY | O_CLOEXEC);
    });
    if (heapFd < 0) {
        OMX_LOGE("Failed to open DMA heap %s: %s", heapName, strerror(errno));
        return false;
    }
    struct dma_heap_allocation_data allocData = {
        .len = bufferLen,
        .fd_flags = O_RDWR | O_CLOEXEC,
    };
    OMX_LOGD("Allocating %zu bytes from DMA heap", bufferLen);
    int ret = OmxFailureRetryFunc([&]() {
        return ioctl(heapFd, DMA_HEAP_IOCTL_ALLOC, &allocData);
    });
    if (ret < 0) {
        OMX_LOGE("Failed to allocate from DMA heap %s: %s", heapName, strerror(errno));
        close(heapFd);
        return false;
    }
    mFd = allocData.fd;
    close(heapFd);  // Close heap device fd, we only need the buffer fd
    OMX_LOGD("Allocated DMA buffer fd: %d, mapping to userspace", mFd);
    // Map the buffer to userspace
    mBase = mmap(0, bufferLen, PROT_READ | PROT_WRITE, MAP_SHARED, mFd, 0);
    if (mBase == MAP_FAILED) {
        OMX_LOGE("Failed to mmap DMA buffer: %s", strerror(errno));
        close(mFd);
        mFd = -1;
        mBase = nullptr;
        return false;
    }
    OMX_LOGD("Successfully allocated DMA buffer: fd=%d, size=%zu, base=%p", mFd, bufferLen, mBase);
    return true;
}
int C2UnisocDmaBuffer::getDmaHeapID()
{
    return mFd;
}
void *C2UnisocDmaBuffer::getBase()
{
    return mBase;
}
int C2UnisocDmaBuffer::doSync(int bufferFd, bool start, SyncType sync_type)
{
    if (bufferFd < 0) {
        OMX_LOGE("Invalid buffer fd for sync operation");
        return -EINVAL;
    }
    uint64_t sync_flags = 0;
    // Set start/end flag
    if (start) {
        sync_flags |= DMA_BUF_SYNC_START;
    } else {
        sync_flags |= DMA_BUF_SYNC_END;
    }
    // Set read/write flags based on sync_type
    switch (sync_type) {
        case kSyncRead:
            sync_flags |= DMA_BUF_SYNC_READ;
            break;
        case kSyncWrite:
            sync_flags |= DMA_BUF_SYNC_WRITE;
            break;
        case kSyncReadWrite:
            sync_flags |= DMA_BUF_SYNC_READ | DMA_BUF_SYNC_WRITE;
            break;
        default:
            OMX_LOGE("Invalid sync type: %d", sync_type);
            return -EINVAL;
    }
    struct dma_buf_sync sync = {
        .flags = sync_flags,
    };
    int ret = OmxFailureRetryFunc([&]() {
        return ioctl(bufferFd, DMA_BUF_IOCTL_SYNC, &sync);
    });
    if (ret < 0) {
        OMX_LOGE("DMA buffer sync failed: %s (fd=%d, flags=0x%llx)",
            strerror(errno), bufferFd, (unsigned long long)sync_flags);
    }
    return ret;
}
//static
int C2UnisocDmaBuffer::GetPhyAddrFromDmaHeap(int bufferFd, unsigned long *phyAddr, size_t *size)
{
    if (bufferFd < 0 || !phyAddr || !size) {
        OMX_LOGE("Invalid parameters for GetPhyAddrFromDmaHeap");
        return -EINVAL;
    }
    int fd = open("/dev/sprd_cma", O_SYNC);
    if (fd < 0) {
        OMX_LOGE("%s, %d, open dev error! fd: %d", __FUNCTION__, __LINE__, bufferFd);
        return -1;
    }
    struct DmabufPhyData phy_data;
    phy_data.fd = bufferFd;
    auto ret = OmxFailureRetryFunc([&]() {
        return ioctl(fd, DMABUF_IOC_PHY, &phy_data);
    });
    if (ret < 0) {
        OMX_LOGE("%s, %d, get phyAddr error! fd: %d, error: %s",
            __FUNCTION__, __LINE__, bufferFd, strerror(errno));
        close(fd);
        return -1;
    }
    *phyAddr = phy_data.addr;
    *size = phy_data.len;
    close(fd);
    OMX_LOGD("Got physical address: 0x%lx, size: %zu for fd: %d", *phyAddr, *size, bufferFd);
    return 0;
}
int C2UnisocDmaBuffer::getPhyAddrFromDmaHeap(unsigned long *phyAddr, size_t *size)
{
    return GetPhyAddrFromDmaHeap(mFd, phyAddr, size);
}
//static
void C2UnisocDmaBuffer::InvalidateDmaBuffer(int bufferFd)
{
    C2UnisocDmaBuffer tempBuffer(nullptr, 0);
    tempBuffer.doSync(bufferFd, true, kSyncReadWrite);
}
int C2UnisocDmaBuffer::invalidateDmaBuffer()
{
    return doSync(mFd, true, kSyncReadWrite);
}
//static
void C2UnisocDmaBuffer::FlushDmaBuffer(int bufferFd)
{
    C2UnisocDmaBuffer tempBuffer(nullptr, 0);
    tempBuffer.doSync(bufferFd, false, kSyncReadWrite);
}
void C2UnisocDmaBuffer::flushDmaBuffer()
{
    doSync(mFd, false, kSyncReadWrite);
}
} // namespace OMX
} // namespace OHOS
