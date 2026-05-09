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
#define LOG_TAG "VideoMemAllocator"
#include <utils/omx_log.h>
#include <sys/stat.h>
#include "dmaphyaddr-defs.h"
#include "VideoMemAllocator.h"
namespace OHOS {
namespace OMX {
VideoMemAllocator::VideoMemAllocator(size_t bufferLen, bool isCache, bool IOMMUEnabled)
    : mHeapDma(nullptr),
    mFd(-1),
    mBase(nullptr),
    mIsUseDma(false)
{
    mIsUseDma = !CheckIonSupport();
    if (mIsUseDma) {
        if (IOMMUEnabled) {
            OMX_LOGD("IOMMUEnabled IS TRUE");
            mHeapDma = new C2UnisocDmaBuffer(isCache ? K_SYSTEM_HEAP_NAME : K_SYSTEM_UNCACHED_HEAP_NAME, bufferLen);
        } else {
            mHeapDma = new C2UnisocDmaBuffer(K_WIDE_VINE_HEAP_NAME, bufferLen);
        }
    } else {
        OMX_LOGE("K515 DRIVER DO NOT USE  Ion Heap");
    }
}
VideoMemAllocator::~VideoMemAllocator()
{
    if (mIsUseDma) {
        if (mHeapDma != nullptr) {
            delete mHeapDma;
            mHeapDma = nullptr;
        }
    } else {
        OMX_LOGE("WARNING--DO NOT USE ION");
    }
}
//static
bool VideoMemAllocator::CheckIonSupport()
{
    int ionSupport = -1;
    struct stat buffer;
    ionSupport = (stat("/dev/ion", &buffer) == 0);
    return (ionSupport == 1);
}
int VideoMemAllocator::getHeapID()
{
    if (mIsUseDma) {
        mFd = mHeapDma->getDmaHeapID();
    } else {
        OMX_LOGE("WARNING--DO NOT USE ION");
    }
    return mFd;
}
void *VideoMemAllocator::getBase()
{
    if (mIsUseDma) {
        mBase = mHeapDma->getBase();
    } else {
        OMX_LOGE("WARNING--DO NOT USE ION");
    }
    return mBase;
}
//static
int VideoMemAllocator::GetPhyAddrFromMemAlloc(int bufferFd, unsigned long *phyAddr, size_t *size)
{
    int ret = -1;
    if (!CheckIonSupport()) {
        ret = C2UnisocDmaBuffer::GetPhyAddrFromDmaHeap(bufferFd, phyAddr, size);
    } else {
        OMX_LOGE("WARNING--DO NOT USE ION");
    }
    return ret;
}
int VideoMemAllocator::getPhyAddrFromMemAlloc(unsigned long *phyAddr, size_t *size)
{
    int ret = -1;
    if (mIsUseDma) {
        ret = mHeapDma->getPhyAddrFromDmaHeap(phyAddr, size);
    } else {
        OMX_LOGE("WARNING--DO NOT USE ION");
    }
    return ret;
}
//static
void VideoMemAllocator::CpuSyncStart(int bufferFd)
{
    if (!CheckIonSupport()) {
        C2UnisocDmaBuffer::InvalidateDmaBuffer(bufferFd);
    } else {
        OMX_LOGE("WARNING--DO NOT USE ION");
    }
}
void VideoMemAllocator::cpuSyncStart()
{
    if (mIsUseDma) {
        mHeapDma->invalidateDmaBuffer();
    } else {
        OMX_LOGE("WARNING--DO NOT USE ION");
    }
}
//static
void VideoMemAllocator::CpuSyncEnd(int bufferFd)
{
    if (!CheckIonSupport()) {
        C2UnisocDmaBuffer::FlushDmaBuffer(bufferFd);
    } else {
        OMX_LOGE("WARNING--DO NOT USE ION");
    }
}
void VideoMemAllocator::cpuSyncEnd()
{
    if (mIsUseDma) {
        mHeapDma->flushDmaBuffer();
    } else {
        OMX_LOGE("WARNING--DO NOT USE ION");
    }
}
} // namespace OMX
} // namespace OHOS
