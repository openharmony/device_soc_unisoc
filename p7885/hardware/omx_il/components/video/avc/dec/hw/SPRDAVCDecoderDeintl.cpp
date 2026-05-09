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

#include "SPRDAVCDecoder.h"

#include <cerrno>
#include <cstring>

#include "utils/omx_log.h"

#undef LOG_TAG
#define LOG_TAG "SPRDAVCDecoder"

namespace OHOS {
namespace OMX {

void SPRDAVCDecoder::initDecOutputHeader(OMX_BUFFERHEADERTYPE *header)
{
    header->pBuffer = nullptr;
    header->nAllocLen = mPictureSize;
    header->nFilledLen = 0;
    header->nOffset = 0;
    header->pOutputPortPrivate = nullptr;
    header->pMarkData = nullptr;
    header->nTickCount = 0;
    header->nTimeStamp = 0;
    header->nFlags = 0;
}

OMX_ERRORTYPE SPRDAVCDecoder::allocateOneDecOutputBuffer(H264SwDecInfo *decoderInfo, OMX_U32 nodeId)
{
    OMX_BUFFERHEADERTYPE *header = new OMX_BUFFERHEADERTYPE;
    initDecOutputHeader(header);
    size_t bufferSize = 0;
    unsigned long phyAddr = 0;
    size_t alignedSize = decoderInfo->picWidth * decoderInfo->picHeight *
        YUV420_FRAME_SIZE_MULTIPLIER / YUV420_FRAME_SIZE_DIVISOR;
    MemIon* pMem = new VideoMemAllocator(alignedSize, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    int fd = pMem->getHeapID();
    if (fd < STATUS_CODE_OK) {
        delete pMem;
        delete header;
        OMX_LOGE("Failed to alloc outport pmem buffer");
        return OMX_ErrorInsufficientResources;
    }
    int ret = mIOMMUEnabled ?
        (mDeintl && mDecoderSwFlag ? mDeintl->VspGetIova(fd, &phyAddr, &bufferSize) :
        (*mH264DecGetIOVA)(mHandle, fd, &phyAddr, &bufferSize, true)) :
        pMem->getPhyAddrFromMemAlloc(&phyAddr, &bufferSize);
    if (ret < STATUS_CODE_OK) {
        delete pMem;
        delete header;
        OMX_LOGE("%s, get phy addr Failed: %d(%s)", __FUNCTION__, ret, strerror(errno));
        return OMX_ErrorInsufficientResources;
    }
    header->pBuffer = (OMX_U8 *)(pMem->getBase());
    header->pOutputPortPrivate = new BufferCtrlStruct;
    if (header->pOutputPortPrivate == nullptr) {
        delete pMem;
        delete header;
        return OMX_ErrorUndefined;
    }
    BufferCtrlStruct* pBufCtrl = (BufferCtrlStruct*)(header->pOutputPortPrivate);
    pBufCtrl->iRefCount = STATUS_CODE_OK;
    pBufCtrl->pMem = pMem;
    pBufCtrl->bufferFd = fd;
    pBufCtrl->phyAddr = phyAddr;
    pBufCtrl->bufferSize = bufferSize;
    BufferInfo* bufNode = new BufferInfo;
    bufNode->mHeader = header;
    bufNode->mNodeId = nodeId;
    mDecOutputBufQueue.push_back(bufNode);
    mBufferNodes.push_back(bufNode);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoder::allocateDecOutputBuffer(H264SwDecInfo* pDecoderInfo)
{
    OMX_U32 DecBufferCount = mDecoderInfo.numRefFrames + mDecoderInfo.hasBFrames+ ADDITIONAL_DEC_BUFFERS;
    for (OMX_U32 i = 0; i < DecBufferCount; ++i) {
        OMX_ERRORTYPE err = allocateOneDecOutputBuffer(pDecoderInfo, i);
        if (err != OMX_ErrorNone) {
            return err;
        }
    }
    OMX_LOGI("%s, %d, successful allocated buffer count: %d, buffer size: %d",
        __FUNCTION__, __LINE__, mDecOutputBufQueue.size(), mPictureSize);
    OMX_LOGI("total buffer count: %d, mIOMMUEnabled : %d",
        DecBufferCount, mIOMMUEnabled);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoder::releaseDecOutputNode(BufferInfo *bufferNode)
{
    OMX_BUFFERHEADERTYPE *header = bufferNode->mHeader;
    BufferCtrlStruct* pBufCtrl = (BufferCtrlStruct*)(header->pOutputPortPrivate);
    if (pBufCtrl == nullptr || pBufCtrl->pMem == nullptr) {
        OMX_LOGE("freeBuffer, pMem == nullptr");
        return OMX_ErrorUndefined;
    }
    OMX_LOGV("freeBuffer, phyAddr: 0x%lx\n", pBufCtrl->phyAddr);
    if (mIOMMUEnabled) {
        if (mDeintl && mDecoderSwFlag) {
            mDeintl->VspFreeIova(pBufCtrl->phyAddr, pBufCtrl->bufferSize);
        } else {
            (*mH264DecFreeIOVA)(mHandle, pBufCtrl->phyAddr, pBufCtrl->bufferSize, true);
        }
    }
    releaseMemIon(&pBufCtrl->pMem);
    if (header->pOutputPortPrivate != nullptr) {
        delete (BufferCtrlStruct*)(header->pOutputPortPrivate);
        header->pOutputPortPrivate = nullptr;
    }
    delete header;
    delete bufferNode;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoder::freeDecOutputBuffer()
{
    OMX_LOGI("freeBufferNodes\n");
    mDecOutputBufQueue.clear();
    mDeinterInputBufQueue.clear();
    for (size_t i = mBufferNodes.size(); i-- > 0;) {
        BufferInfo* pBufferNode = mBufferNodes.at(i);
        if (pBufferNode != nullptr && pBufferNode->mHeader != nullptr) {
            OMX_LOGV("free buffer node id: %d, i = %d\n", pBufferNode->mNodeId, i);
            OMX_ERRORTYPE err = releaseDecOutputNode(pBufferNode);
            if (err != OMX_ErrorNone) {
                return err;
            }
        }
        mBufferNodes.erase(mBufferNodes.begin() + i);
    }
    return OMX_ErrorNone;
}

void SPRDAVCDecoder::DrainOneDeintlBuffer(void *pBufferHeader, uint64 pts)
{
    std::list<BufferInfo *>::iterator it = mDecOutputBufQueue.begin();
    if (!pBufferHeader || (*it) == nullptr) {
        OMX_LOGE("%s, %d, The code stream is empty, return.", __FUNCTION__, __LINE__);
        return;
    }
    while (it != mDecOutputBufQueue.end() && (*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader) {
        ++it;
    }
    if (it == mDecOutputBufQueue.end()) {
        OMX_LOGE("%s, %d, see the bufQueueEnd, return.", __FUNCTION__, __LINE__);
        return;
    }
    BufferInfo *bufferNode = *it;
    OMX_BUFFERHEADERTYPE *outHeader = bufferNode->mHeader;
    bufferNode->deintlWidth = mDecoderInfo.picWidth;
    bufferNode->deintlHeight = mDecoderInfo.picHeight;
    outHeader->nFilledLen = mPictureSize;
    outHeader->nTimeStamp = (OMX_TICKS)pts;
    {
        std::lock_guard<std::mutex> autoLock(mThreadLock);
        mDeinterInputBufQueue.push_back(*it);
        mDecOutputBufQueue.erase(it);
    }
    SignalDeintlThread();
    OMX_LOGI("%s, %d, mDeinterInputBufQueue.size = %d, mNodeId = %d\n",
        __FUNCTION__, __LINE__, mDeinterInputBufQueue.size(), bufferNode->mNodeId);
}

bool SPRDAVCDecoder::DrainAllDecOutputBufferforDeintl()
{
    OMX_BUFFERHEADERTYPE *outHeader;
    BufferInfo *outBufferNode;
    int32_t picId;
    void *pBufferHeader;
    uint64 pts;
    while (!mDecOutputBufQueue.empty() && mEOSStatus != OUTPUT_FRAMES_FLUSHED) {
        if ((*mH264DecGetLastDspFrm)(mHandle, &pBufferHeader, &picId, &pts) == MMDEC_OK) {
            std::list<BufferInfo *>::iterator it = mDecOutputBufQueue.begin();
            while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != mDecOutputBufQueue.end()) {
                ++it;
            }
            if ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader) {
                OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((*it)->mHeader == "
                    "(OMX_BUFFERHEADERTYPE*)pBufferHeader) failed.",
                    __FUNCTION__, FILENAME_ONLY, __LINE__);
                return false;
            }
            outBufferNode = *it;
            outHeader = outBufferNode->mHeader;
            outHeader->nFilledLen = mPictureSize;
            OMX_LOGI("drainAllDecOutputBufferNodes, node id = %d", outBufferNode->mNodeId);
            {
                std::lock_guard<std::mutex> autoLock(mThreadLock);
                mDeinterInputBufQueue.push_back(*it);
                mDecOutputBufQueue.erase(it);
            }
        } else {
            std::list<BufferInfo *>::iterator it = mDecOutputBufQueue.begin();
            outBufferNode = *it;
            outHeader = outBufferNode->mHeader;
            outHeader->nTimeStamp = 0;
            outHeader->nFilledLen = 0;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;
            mEOSStatus = OUTPUT_FRAMES_FLUSHED;
            OMX_LOGI("drainAllDecOutputBufferNodes, the last buffer node id = %d", outBufferNode->mNodeId);
            {
                std::lock_guard<std::mutex> autoLock(mThreadLock);
                mDeinterInputBufQueue.push_back(*it);
                mDecOutputBufQueue.erase(it);
            }
        }
    }
    OMX_LOGI("drainAllDecOutputBufferNodes() mDeinterInputBufQueue.size() = %d", mDeinterInputBufQueue.size());
    return true;
}

void SPRDAVCDecoder::SignalDeintlThread()
{
    std::list<BufferInfo*>& outQueue = getPortQueue(K_OUTPUT_PORT_INDEX);
    if (mDeinterInputBufQueue.size() > MIN_DEINTERLACE_BUFFERS && !outQueue.empty()) {
        OMX_LOGI("%s, %d,  send mDeinterReadyCondition signal\n", __FUNCTION__, __LINE__);
        mDeintl->mDeinterReadyCondition.notify_one();
    }
}

void SPRDAVCDecoder::FlushDeintlBuffer()
{
    OMX_LOGI("%s, %d, mDecOutputBufQueue size: %d, mDeinterInputBufQueue size: %d",
        __FUNCTION__, __LINE__, mDecOutputBufQueue.size(), mDeinterInputBufQueue.size());
    if (mDeinterInputBufQueue.empty()) {
        return;
    }
    for (size_t i = mDeinterInputBufQueue.size(); i-- > 0;) {
        std::list<BufferInfo *>::iterator itBufferNode = mDeinterInputBufQueue.begin();
        mDecOutputBufQueue.push_back(*itBufferNode);
        mDeinterInputBufQueue.erase(itBufferNode);
    }
    mDeintl->mDeintlFrameNum = 0;
}

}  // namespace OMX
}  // namespace OHOS
