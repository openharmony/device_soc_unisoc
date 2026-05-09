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
#include "SPRDDeinterlace.h"
#include <securec.h>
#include <sys/prctl.h>
#include <mutex>
#undef LOG_TAG
#define LOG_TAG "SPRDDeinterlace"
/* Deinterlace configuration parameters */
#define DEINTERLACE_INIT_SUCCESS_RETCODE      0      /* Success return code for VppDeintInit */
#define DEINTERLACE_IOMMU_STATUS_FAIL         (-1)     /* IOMMU status failure code */
#define DEINTERLACE_PHYADDR_INVALID           0      /* Invalid physical address value */
#define DEINTERLACE_MIN_INPUT_BUFFERS         2      /* Minimum buffers required for deinterlace processing */
#define DEINTERLACE_INTERLEAVE_VALUE          0      /* Interleave mode parameter */
#define DEINTERLACE_THRESHOLD_VALUE           10     /* Motion threshold parameter */
#define YUV420_UV_SIZE_DIVISOR                4      /* UV plane size divisor for YUV420 format (Y size >> 2) */
#define YUV_COMPONENT_RATIO_SHIFT             2      /* Bit shift for YUV component ratio calculation */
namespace OHOS {
namespace OMX {
SPRDDeinterlace::SPRDDeinterlace(const void *decOutputBufQueue, const void *deinterInputBufQueue,
                                 const void *outQueue, const DeinterlaceInfo& info) : mDone(false),
    mUseNativeBuffer(false),
    mDeintlFrameNum(0),
    mIommuVPPEnabled(false),
    mIommuVPPID(-1),
    mDecOutputBufQueue((std::list<BufferInfoBase *>*)decOutputBufQueue),
    mDeinterInputBufQueue((std::list<BufferInfoBase *>*)deinterInputBufQueue),
    mOutQueue((std::list<BufferInfoBase *>*)outQueue)
{
    (void)decOutputBufQueue;
    (void)deinterInputBufQueue;
    (void)outQueue;
    OMX_LOGI("Construct SPRDDeinterlace, this: %p", static_cast<void *>(this));
    errno_t moveRet = memmove_s(&mDeinterlaceInfo, sizeof(DeinterlaceInfo), &info, sizeof(DeinterlaceInfo));
    if (moveRet != EOK) {
        OMX_LOGE("memmove_s failed in ctor, ret=%d", moveRet);
        (void)memset_s(&mDeinterlaceInfo, sizeof(DeinterlaceInfo), 0, sizeof(DeinterlaceInfo));
    }
    mVPPHeader = new VPPObject;
    // mThread is a pthread_t of unknown size so we need memset.
    (void)memset_s(&mThread, sizeof(mThread), 0, sizeof(mThread));
    int ret = VppDeintInit (mVPPHeader);
    if (ret != DEINTERLACE_INIT_SUCCESS_RETCODE) {
        OMX_LOGE("failed to init deint module");
    }
    ret = VppDeintGetIommuStatus(mVPPHeader);
    if (ret < DEINTERLACE_INIT_SUCCESS_RETCODE) {
        OMX_LOGE("Failed to get IOMMU status, ret: %d(%s)", ret, strerror(errno));
        mIommuVPPEnabled = false;
    } else {
        OMX_LOGI("VSP IOMMU is enabled");
        mIommuVPPEnabled = true;
    }
    OMX_LOGI("%s, is mIommuVPPEnabled : %d, ID: %d", __FUNCTION__, mIommuVPPEnabled, mIommuVPPID);
}
SPRDDeinterlace::~SPRDDeinterlace()
{
    OMX_LOGI("Destruct SPRDDeinterlace, this: %p", static_cast<void *>(this));
    void *dummy;
    pthread_join(mThread, &dummy);
    OMX_LOGI("Deintl thread finished\n");
    VppDeintRelease(mVPPHeader);
    if (mVPPHeader) {
        delete mVPPHeader;
        mVPPHeader = nullptr;
    }
}
int32 SPRDDeinterlace::VspFreeIova(unsigned long iova, size_t size)
{
    int32 ret = 0;
    ret = VppDeintFreeIova(mVPPHeader, iova, size);
    OMX_LOGV("VspFreeIova : %d\n", ret);
    return  ret;
}
int32 SPRDDeinterlace::VspGetIova(int fd, unsigned long *iova, size_t *size)
{
    int32 ret = 0;
    ret = VppDeintGetIova(mVPPHeader, fd, iova, size);
    OMX_LOGV("VspGetIova : %d\n", ret);
    return  ret;
}
void SPRDDeinterlace::FillDeintlSrcBuffer(BufferInfoBase *pre, BufferInfoBase *cur)
{
    if (mDeintlFrameNum > DEINTERLACE_MIN_INPUT_BUFFERS - 1) {
        mDecOutputBufQueue->push_back(pre);
        mDeinterInputBufQueue->erase(mDeinterInputBufQueue->begin());
        if ((cur->mHeader->nFlags == OMX_BUFFERFLAG_EOS)) {
            mDone = true;
            mDecOutputBufQueue->push_back(cur);
            mDeinterInputBufQueue->erase(mDeinterInputBufQueue->begin());
            OMX_LOGE("%s, %d: Deinterlace meet EOS, mDeinterInputBufQueue size: %d, mDecOutputBufQueue size: %d\n",
                __FUNCTION__, __LINE__, static_cast<int>(mDeinterInputBufQueue->size()),
                static_cast<int>(mDecOutputBufQueue->size()));
        }
    }
    return;
}
bool SPRDDeinterlace::WaitForDeintlBuffers(std::unique_lock<std::mutex>& autoLock)
{
    while (mOutQueue->empty() || mDone || mDeinterInputBufQueue->empty() ||
        (mDeinterInputBufQueue->size() < DEINTERLACE_MIN_INPUT_BUFFERS &&
        mDeintlFrameNum > DEINTERLACE_INIT_SUCCESS_RETCODE)) {
        if (mDone) {
            OMX_LOGI("%s, %d: deintl thread done\n", __FUNCTION__, __LINE__);
            return false;
        }
        OMX_LOGI("%s, %d, mDeinterReadyCondition wait\n", __FUNCTION__, __LINE__);
        mDeinterReadyCondition.wait(autoLock);
    }
    return true;
}

bool SPRDDeinterlace::GetFramePhyAddr(BufferInfoBase *bufferNode, unsigned long *phyAddr) const
{
    if (!phyAddr) {
        OMX_LOGE("%s: phyAddr is null", __FUNCTION__);
        return false;
    }
    
    BufferCtrlStruct* pBufCtrl = (BufferCtrlStruct*)(bufferNode->mHeader->pOutputPortPrivate);
    *phyAddr = pBufCtrl->phyAddr;
    if (*phyAddr != DEINTERLACE_PHYADDR_INVALID) {
        return true;
    }
    OMX_LOGE("%s, %d, obtain deint processing buffer failed\n", __FUNCTION__, __LINE__);
    return false;
}

bool SPRDDeinterlace::PrepareSourceFrames(BufferInfoBase *&pre, BufferInfoBase *&cur,
    unsigned long *srcPhyAddr, unsigned long *refPhyAddr) const
{
    if (!srcPhyAddr || !refPhyAddr) {
        OMX_LOGE("%s: srcPhyAddr or refPhyAddr is null", __FUNCTION__);
        return false;
    }
    
    std::list<BufferInfoBase *>::iterator itor = mDeinterInputBufQueue->begin();
    pre = nullptr;
    cur = *itor;
    *refPhyAddr = DEINTERLACE_PHYADDR_INVALID;
    if (mDeintlFrameNum != DEINTERLACE_INIT_SUCCESS_RETCODE) {
        pre = *itor;
        cur = *(++itor);
        if (!GetFramePhyAddr(pre, refPhyAddr)) {
            return false;
        }
    }
    return GetFramePhyAddr(cur, srcPhyAddr);
}

bool SPRDDeinterlace::getDisplayPhyAddr(OMX_BUFFERHEADERTYPE *outHeader, unsigned long *dstPhyAddr) const
{
    if (!dstPhyAddr) {
        OMX_LOGE("%s: dstPhyAddr is null", __FUNCTION__);
        return false;
    }
    
    BufferCtrlStruct* pBufCtrl = (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
    if (pBufCtrl->phyAddr != DEINTERLACE_PHYADDR_INVALID) {
        *dstPhyAddr = pBufCtrl->phyAddr;
        OMX_LOGV("%s, %d, pBufCtrl %p, dstPhyAddr 0x%lx", __FUNCTION__, __LINE__, pBufCtrl, *dstPhyAddr);
        return true;
    }
    if (!mIommuVPPEnabled) {
        OMX_LOGV(" CAN NOT USE NATIVEHANDLE IN OHOS %s, %d, pBufCtrl %p",
            __FUNCTION__, __LINE__, pBufCtrl);
        *dstPhyAddr = DEINTERLACE_PHYADDR_INVALID;
        return true;
    }
    int32_t ret = VppDeintGetIova(mVPPHeader, pBufCtrl->bufferFd, &(pBufCtrl->phyAddr), &(pBufCtrl->bufferSize));
    OMX_LOGV("%s, %d, bufferFd : %d, phyAddr : 0x%lx, ret : %d", __FUNCTION__, __LINE__,
        pBufCtrl->bufferFd, pBufCtrl->phyAddr, ret);
    *dstPhyAddr = pBufCtrl->phyAddr;
    return *dstPhyAddr != DEINTERLACE_PHYADDR_INVALID;
}

int32_t SPRDDeinterlace::ProcessDeinterlace(BufferInfoBase *cur, unsigned long srcPhyAddr,
    unsigned long refPhyAddr, unsigned long dstPhyAddr)
{
    if (cur->mHeader->nFilledLen == DEINTERLACE_INIT_SUCCESS_RETCODE) {
        if (cur->mHeader->nFlags != OMX_BUFFERFLAG_EOS) {
            OMX_LOGE("%s, %d, nFilledLen is 0 !!!\n", __FUNCTION__, __LINE__);
            return -1;
        }
        OMX_LOGI("%s, %d, see eos flag from decoder output buffer, mNodeId: %d\n",
            __FUNCTION__, __LINE__, cur->mNodeId);
        return DEINTERLACE_INIT_SUCCESS_RETCODE;
    }
#ifdef DEINT_NULL
    if (yuv) {
        OMX_LOGI("threadFunc() copy one frame start");
        memmove_s(yuv, cur->mHeader->nAllocLen, cur->mHeader->pBuffer, cur->mHeader->nFilledLen);
        OMX_LOGI("threadFunc() copy one frame end");
    }
    return DEINTERLACE_INIT_SUCCESS_RETCODE;
#else
    DEINT_PARAMS_T params;
    params.width  = cur->deintlWidth;
    params.height = cur->deintlHeight;
    params.interleave = INTERLEAVE;
    params.threshold = THRESHOLD;
    params.yLen = params.width * params.height;
    params.cLen = params.yLen >> YUV_COMPONENT_RATIO_SHIFT;
    VppDeintProcessParams processParams = {srcPhyAddr, refPhyAddr, dstPhyAddr, mDeintlFrameNum, &params};
    return VppDeintProcess(mVPPHeader, &processParams);
#endif
}

void SPRDDeinterlace::finishDeinterlace(BufferInfoBase *pre, BufferInfoBase *cur,
    OMX_BUFFERHEADERTYPE *outHeader, int32_t ret)
{
    if (ret == DEINTERLACE_INIT_SUCCESS_RETCODE) {
        mDeintlFrameNum++;
        outHeader->nTimeStamp = cur->mHeader->nTimeStamp;
        outHeader->nFlags = cur->mHeader->nFlags;
        outHeader->nFilledLen = cur->mHeader->nFilledLen;
        mDeinterlaceInfo.callback(mDeinterlaceInfo.decoder, mDeintlFrameNum, outHeader,
            (unsigned long long)outHeader->nTimeStamp);
    } else {
        OMX_LOGE("%s, %d, deint process error, the current frame will not be displayed\n",
            __FUNCTION__, __LINE__);
    }
    FillDeintlSrcBuffer(pre, cur);
}

//deinterlace thread
void SPRDDeinterlace::DeintlThreadFunc()
{
    prctl(PR_SET_NAME, reinterpret_cast<unsigned long>("Deinterlace"), 0, 0, 0);
    OMX_LOGD("thread id:%lu tid:%d", pthread_self(), gettid());
    BufferInfoBase *itBufferNodePre = nullptr;
    BufferInfoBase *itBufferNodeCur = nullptr;
    unsigned long dstPhyAddr = DEINTERLACE_PHYADDR_INVALID;
    unsigned long srcPhyAddr = DEINTERLACE_PHYADDR_INVALID;
    unsigned long refPhyAddr = DEINTERLACE_PHYADDR_INVALID;
    BufferCtrlStruct* pBufCtrl = nullptr;
    int32_t ret = DEINTERLACE_INIT_SUCCESS_RETCODE;
    while (!mDone) {
        std::unique_lock<std::mutex> autoLock(*mDeinterlaceInfo.threadLock);
        if (!WaitForDeintlBuffers(autoLock)) {
            return;
        }
        OMX_LOGI("%s, %d, start to do deinterlace, mDeintlFrameNum = %d\n", __FUNCTION__, __LINE__, mDeintlFrameNum);
        if (!PrepareSourceFrames(itBufferNodePre, itBufferNodeCur, &srcPhyAddr, &refPhyAddr)) {
            return;
        }
        //find an available display buffer from native window buffer queue
        std::list<BufferInfoBase *>::iterator itBuffer = mOutQueue->begin();
        OMX_BUFFERHEADERTYPE *outHeader = (*itBuffer)->mHeader;
        if (!getDisplayPhyAddr(outHeader, &dstPhyAddr)) {
            return;
        }
        ret = ProcessDeinterlace(itBufferNodeCur, srcPhyAddr, refPhyAddr, dstPhyAddr);
        finishDeinterlace(itBufferNodePre, itBufferNodeCur, outHeader, ret);
    }
    OMX_LOGI("%s, %d, deintl thread exit\n", __FUNCTION__, __LINE__);
}
void *SPRDDeinterlace::ThreadWrapper(void *me)
{
    OMX_LOGV("ThreadWrapper: %p", me);
    SPRDDeinterlace *deintl = reinterpret_cast<SPRDDeinterlace *>(me);
    deintl->DeintlThreadFunc();
    return nullptr;
}
void SPRDDeinterlace::StartDeinterlaceThread()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&mThread, &attr, ThreadWrapper, this);
    pthread_attr_destroy(&attr);
}
void SPRDDeinterlace::StopDeinterlaceThread()
{
    OMX_LOGI("StopDeinterlaceThread B");
    mDone = true;
    mDeinterReadyCondition.notify_one();
    void *dummy;
    pthread_join(mThread, &dummy);
    OMX_LOGI("StopDeinterlaceThread E");
}
}  // namespace OMX
}  // namespace OHOS
