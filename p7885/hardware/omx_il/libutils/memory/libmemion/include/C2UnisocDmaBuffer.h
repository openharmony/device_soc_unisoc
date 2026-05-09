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
#ifndef C2_UNISOC_DMA_BUFFER_H
#define C2_UNISOC_DMA_BUFFER_H
#include "refbase.h"
#include <string>
namespace OHOS {
namespace OMX {
class C2UnisocDmaBuffer : public OHOS::RefBase {
public:
    enum SyncType {
        kSyncRead = 1,
        kSyncWrite = 2,
        kSyncReadWrite = 3
    };
    C2UnisocDmaBuffer(const char *heapName, size_t bufferLen);
    ~C2UnisocDmaBuffer();
    int getDmaHeapID();
    void *getBase();
    static int GetPhyAddrFromDmaHeap(int bufferFd, unsigned long *phyAddr, size_t *size);
    int getPhyAddrFromDmaHeap(unsigned long *phyAddr, size_t *size);
    static void InvalidateDmaBuffer(int bufferFd);
    int invalidateDmaBuffer();
    static void FlushDmaBuffer(int bufferFd);
    void flushDmaBuffer();
private:
    bool allocateFromDmaHeap(const char *heapName, size_t bufferLen);
    int doSync(int bufferFd, bool start, SyncType sync_type);
    int mFd;
    void *mBase;
    size_t mSize;
    std::string mHeapName;
};
} // namespace OMX
} // namespace OHOS
#endif // C2_UNISOC_DMA_BUFFER_H
