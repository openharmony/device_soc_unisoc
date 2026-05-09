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
#ifndef SPRD_DEINTERLACE_H_
#define SPRD_DEINTERLACE_H_
#include "SprdSimpleOMXComponent.h"
#include "vpp_drv_interface.h"
#include "sprd_omx_typedef.h"
#include <list>
#include <cstddef>
#include <condition_variable>
namespace OHOS {
namespace OMX {
struct BufferInfoBase {  //must be same to SprdSimpleOMXComponent::BufferInfo
        OMX_BUFFERHEADERTYPE *mHeader;
        bool mOwnedByUs;
        uint32_t deintlWidth;
        uint32_t deintlHeight;
        int32_t picId;
        int32_t mNodeId;
};
struct BufferInfo: public BufferInfoBase {  //for the intel deintelace function only
        OMX_BUFFERHEADERTYPE *mDecHeader;
};
typedef void (*DrainOneOutputBuffercallback)(void *decoder, int32_t picId, void *pBufferHeader, unsigned long long pTs);
typedef struct DeinterlaceInfo {
    int width;
    int height;
    DrainOneOutputBuffercallback callback;
    void *decoder;
    std::mutex* threadLock;
} DeinterlaceInfo;
struct SPRDDeinterlace {
public:
    explicit SPRDDeinterlace(const void *decOutputBufQueue, const void *deinterInputBufQueue,
        const void *outQueue, const DeinterlaceInfo& info);
    virtual ~SPRDDeinterlace();
    bool mDone;
    bool mUseNativeBuffer;
    uint32_t mDeintlFrameNum;
    std::condition_variable mDeinterReadyCondition;
    void StartDeinterlaceThread();
    void StopDeinterlaceThread();
    int32 VspFreeIova(unsigned long iova, size_t size);
    int32 VspGetIova(int fd, unsigned long *iova, size_t *size);
private:
    bool mIommuVPPEnabled;
    int32_t  mIommuVPPID;
    pthread_t   mThread;                // Thread id for deinterlace
    VPPObject* mVPPHeader;
    std::list<BufferInfoBase *>* mDecOutputBufQueue;
    std::list<BufferInfoBase *>* mDeinterInputBufQueue;
    std::list<BufferInfoBase *>* mOutQueue;
    DeinterlaceInfo mDeinterlaceInfo;
    void DeintlThreadFunc();
    static void *ThreadWrapper(void *me);
    int remapDispBuffer(OMX_BUFFERHEADERTYPE *header, unsigned long* pHyAddr);
    int RemapDeintlSrcBuffer(BufferInfoBase *itBuffer,  unsigned long* pHyAddr);
    void UnmapDeintlSrcBuffer(BufferInfoBase *itBuffer);
    void FillDeintlSrcBuffer(BufferInfoBase *pre, BufferInfoBase *cur);
    bool WaitForDeintlBuffers(std::unique_lock<std::mutex>& autoLock);
    bool PrepareSourceFrames(BufferInfoBase *&pre, BufferInfoBase *&cur,
        unsigned long *srcPhyAddr, unsigned long *refPhyAddr) const;
    bool GetFramePhyAddr(BufferInfoBase *bufferNode, unsigned long *phyAddr) const;
    bool getDisplayPhyAddr(OMX_BUFFERHEADERTYPE *outHeader, unsigned long *dstPhyAddr) const;
    int32_t ProcessDeinterlace(BufferInfoBase *cur, unsigned long srcPhyAddr,
        unsigned long refPhyAddr, unsigned long dstPhyAddr);
    void finishDeinterlace(BufferInfoBase *pre, BufferInfoBase *cur,
        OMX_BUFFERHEADERTYPE *outHeader, int32_t ret);
    SPRDDeinterlace(const SPRDDeinterlace &);
    SPRDDeinterlace &operator=(const SPRDDeinterlace &);
};
}  // namespace OMX
}  // namespace OHOS
#endif  // SPRD_AVC_DECODER_H_
