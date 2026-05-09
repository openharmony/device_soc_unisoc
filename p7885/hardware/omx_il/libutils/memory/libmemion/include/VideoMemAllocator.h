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
#ifndef VIDEO_MEM_ALLOCATOR_H
#define VIDEO_MEM_ALLOCATOR_H
#include "refbase.h"
#include "C2UnisocDmaBuffer.h"
namespace OHOS {
namespace OMX {
class VideoMemAllocator : public OHOS::RefBase {
public:
    VideoMemAllocator(size_t bufferLen, bool isCache, bool IOMMUEnabled);
    ~VideoMemAllocator();
    static bool CheckIonSupport();  //check use ion or dma
    int getHeapID();
    void *getBase();
    static int GetPhyAddrFromMemAlloc(int bufferFd, unsigned long *phyAddr, size_t *size);
    int getPhyAddrFromMemAlloc(unsigned long *phyAddr, size_t *size);
    static void CpuSyncStart(int bufferFd);
    void cpuSyncStart();
    static void CpuSyncEnd(int bufferFd);
    void cpuSyncEnd();
    static const bool CACHE = true;
    static const bool NO_CACHE = false;
private:
    C2UnisocDmaBuffer *mHeapDma;
    int mFd;
    void *mBase;
    bool mIsUseDma;
};
} // namespace OMX
} // namespace OHOS
#endif // VIDEO_MEM_ALLOCATOR_H
