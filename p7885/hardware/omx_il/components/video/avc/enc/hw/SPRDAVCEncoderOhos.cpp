/*
 * Copyright 2023 Shenzhen Kaihong DID Co., Ltd.
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
#define LOG_TAG "SPRDAVCEncoderOhos"
#include <securec.h>
#include <utils/omx_log.h>
#include "avc_enc_api.h"
#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <utils/time_util.h>
#include "sprd_omx_typedef.h"
#include "Errors.h"
#include "codec_omx_ext.h"
#include "Dynamic_buffer.h"
#include "SPRDAVCEncoderOhos.h"

/* Color format constants */
#define COLOR_FORMAT_RGBA_8888          12  /* RGBA8888 format */
#define COLOR_FORMAT_N_V12               24  /* NV12 format */
#define COLOR_FORMAT_N_V21               25  /* NV21 format */
/* Time scale constant for video encoding */
#define VIDEO_TIME_SCALE_MS             1000
/* Bit depth for video encoding */
#define VIDEO_BIT_DEPTH_8BIT            8
/* QP values for video encoding */
#define DEFAULT_QP_IVOP_SCENE_0         28  /* Default QP for I-frame in normal scene */
#define DEFAULT_QP_IVOP_SCENE_1         36  /* Default QP for I-frame in other scenes */
#define DEFAULT_QP_PVOP                 28  /* Default QP for P-frame */
/* Frame alignment constant */
#define FRAME_ALIGNMENT_MASK            15
/* YUV buffer size calculation factor */
#define YUV_SIZE_MULTIPLIER_NUMERATOR   3
#define YUV_SIZE_MULTIPLIER_DENOMINATOR 2
/* Stream buffer size calculation factor (half of YUV size) */
#define STREAM_SIZE_DIVISOR             2
/* H.264 NAL unit start code */
#define NAL_START_CODE_PREFIX_SIZE      3
#define NAL_START_CODE                  0x01
/* Initial frame count for encoder initialization */
#define INITIAL_FRAME_COUNT             (-2)
/* Default channel quality for encoding */
#define DEFAULT_CHANNEL_QUALITY         1
/* Maximum header size for SPS/PPS */
#define MAX_HEADER_BYTES_TO_LOG         16
#define H264ENC_HEADER_BUFFER_SIZE      1024
/* DMA buffer synchronization flags */
#define DMA_SYNC_START_FLAGS            (DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW)
#define DMA_SYNC_END_FLAGS              (DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW)
/* Minimum valid stream size check */
#define MIN_VALID_STREAM_SIZE           8
/* Scene mode enumeration */
#define ENC_SCENE_MODE_NORMAL           0
#define ENC_SCENE_MODE_OTHER            1
/* Port index for buffer operations */
#define PORT_INDEX_INPUT                0
#define PORT_INDEX_OUTPUT               1
/* Buffer type constants */
#define BUFFER_TYPE_DYNAMIC_HANDLE      CODEC_BUFFER_TYPE_DYNAMIC_HANDLE
/* NAL start code bytes */
#define NAL_START_CODE_BYTE_0           0x00
#define NAL_START_CODE_BYTE_1           0x00
#define NAL_START_CODE_BYTE_2           0x00
#define NAL_START_CODE_BYTE_3           0x01
/* Profile shift and mask for encoding configuration */
#define PROFILE_SHIFT_BITS              8
#define PROFILE_MASK                    0xFF
/* Timestamp rounding offset */
#define TIMESTAMP_ROUNDING_OFFSET_MS    500
/* Success code for encoder operations */
#define ENCODER_SUCCESS_CODE            0
/* Default operating rate */
#define DEFAULT_OPERATING_RATE          0
/* Default encoder priority */
#define DEFAULT_ENCODER_PRIORITY        (-1)
/* Invalid file descriptor value */
#define INVALID_FD                      (-1)
/* Memory protection flags for mmap */
#define MMAP_PROT_FLAGS                 (PROT_READ | PROT_WRITE)
#define MMAP_MAP_FLAGS                  MAP_SHARED
/* Time unit conversion */
#define NANOSECONDS_PER_MILLISECOND     1000000L
#define MILLISECONDS_PER_SECOND         1000L
/* Success return value */
#define SUCCESS_RETURN_VALUE            0
/* DMA buffer sync error code */
#define DMA_SYNC_ERROR_RETURN           (-1)
/* I-frame VOP type identifier */
#define VOP_TYPE_I                      0
/* First frame index */
#define FIRST_FRAME_INDEX               0
/* Enumerator index for header generation */
#define HEADER_GEN_INDEX_SPS            1
#define HEADER_GEN_INDEX_PPS            0
/* Default H.263 encoding flag */
#define DEFAULT_H263_EN_FLAG            0
/* EOS flag identifier */
#define EOS_FLAG_IDENTIFIER             OMX_BUFFERFLAG_EOS
/* Color format mappings */
#define YUV_FORMAT_RGBA                 MMENC_RGBA32
#define YUV_FORMAT_NV12                 MMENC_YUV420SP_NV12
#define YUV_FORMAT_NV21                 MMENC_YUV420SP_NV21
/* Null pointer value */
#define NULL_PTR_VALUE                  0
/* End of frame flag */
#define END_OF_FRAME_FLAG               OMX_BUFFERFLAG_ENDOFFRAME
/* Codec config flag */
#define CODEC_CONFIG_FLAG               OMX_BUFFERFLAG_CODECCONFIG
/* Sync frame flag */
#define SYNC_FRAME_FLAG                 OMX_BUFFERFLAG_SYNCFRAME
/* Array element index */
#define FIRST_ELEMENT_INDEX             0
/* Memory mapping offset */
#define MMAP_OFFSET                     0
/* Buffer indices for logging */
#define LOG_BUFFER_INDEX_0              0
#define LOG_BUFFER_INDEX_1              1
#define LOG_BUFFER_INDEX_2              2
#define LOG_BUFFER_INDEX_3              3
#define LOG_BUFFER_INDEX_4              4
#define LOG_BUFFER_INDEX_5              5
#define LOG_BUFFER_INDEX_6              6
#define LOG_BUFFER_INDEX_7              7
#define LOG_BUFFER_INDEX_8              8
#define LOG_BUFFER_INDEX_9              9
#define LOG_BUFFER_INDEX_10             10
#define LOG_BUFFER_INDEX_11             11
#define LOG_BUFFER_INDEX_12             12
#define LOG_BUFFER_INDEX_13             13
#define LOG_BUFFER_INDEX_14             14
#define LOG_BUFFER_INDEX_15             15
/* Status check value */
#define STATUS_CHECK_VALUE              0
/* Port definition index */
#define PORT_DEFINITION_INDEX           0
/* Video parameter index */
#define VIDEO_PARAM_INDEX               0
/* Frame count adjustment */
#define FRAME_COUNT_ADJUSTMENT_1        1
/* Buffer pointer offset for UV plane */
#define UV_PLANE_OFFSET_MULTIPLIER      1
/* DMA sync flag initial value */
#define DMA_SYNC_FLAG_INITIAL_VALUE     0
SPRDAVCEncoderOhos::SPRDAVCEncoderOhos(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SPRDAVCEncoder(name, callbacks, appData, component)
{
}

void SPRDAVCEncoderOhos::InitOhosEncInfo()
{
    mEncInfo.frameWidth = mVideoWidth;
    mEncInfo.frameHeight = mVideoHeight;
    mEncInfo.eisMode = false;
#ifdef CONFIG_SPRD_RECORD_EIS
    mEncInfo.eisMode = meisMode;
#endif
    mEncInfo.isH263 = DEFAULT_H263_EN_FLAG;
    mEncInfo.orgWidth = mVideoWidth;
    mEncInfo.orgHeight = mVideoHeight;
    if (mVideoColorFormat == COLOR_FORMAT_N_V21) {
        mEncInfo.yuvFormat = YUV_FORMAT_NV21;
    } else if (mVideoColorFormat == COLOR_FORMAT_N_V12) {
        mEncInfo.yuvFormat = YUV_FORMAT_NV12;
    } else if (mVideoColorFormat == COLOR_FORMAT_RGBA_8888) {
        mEncInfo.yuvFormat = YUV_FORMAT_RGBA;
    }
    mEncInfo.timeScale = VIDEO_TIME_SCALE_MS;
    mEncInfo.bitDepth = VIDEO_BIT_DEPTH_8BIT;
}

OMX_ERRORTYPE SPRDAVCEncoderOhos::allocateOhosStreamBuffers(size_t sizeStream)
{
    unsigned long phyAddr = NULL_PTR_VALUE;
    size_t size = NULL_PTR_VALUE;
    mMFEnum = (*mH264EncGetFnum)(mHandle, mVideoWidth, mVideoHeight, mVideoFrameRate);
    OMX_LOGI("mMFEnum = %d", mMFEnum);
    for (int i = FIRST_ELEMENT_INDEX; i < mMFEnum; i++) {
        mPmemStream[i] = new OHOS::OMX::VideoMemAllocator(sizeStream,
            OHOS::OMX::VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
        int fd = mPmemStream[i]->getHeapID();
        if (fd < INVALID_FD) {
            OMX_LOGE("Failed to alloc stream buffer (%zd), getHeapID failed", sizeStream);
            return OMX_ErrorInsufficientResources;
        }
        int ret = 0;
        if (mIOMMUEnabled) {
            if (!mEncoderSymbolsReady || mH264EncGetIOVA == nullptr || mHandle == nullptr) {
                OMX_LOGE("IOMMU enabled but encoder IOVA callback is unavailable");
                return OMX_ErrorInsufficientResources;
            }
            ret = (*mH264EncGetIOVA)(mHandle, fd, &phyAddr, &size);
        } else {
            ret = mPmemStream[i]->getPhyAddrFromMemAlloc(&phyAddr, &size);
        }
        if (ret < SUCCESS_RETURN_VALUE) {
            OMX_LOGE("Failed to alloc stream buffer, get phy addr failed");
            return OMX_ErrorInsufficientResources;
        }
        mStreamBfr.commonBufferPtr[i] = static_cast<uint8_t*>(mPmemStream[i]->getBase());
        mStreamBfr.commonBufferPtrPhy[i] = phyAddr;
        mStreamBfr.size = size;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoderOhos::configureOhosEncConfig()
{
    mEncConfig->h263En = DEFAULT_H263_EN_FLAG;
    mEncConfig->rateCtrlEnable = mrateCtrlEnable;
    mEncConfig->targetBitRate = mVideoBitRate;
    mEncConfig->frameRate = mVideoFrameRate;
    mEncConfig->pFrames = mPFrames;
    mEncConfig->qpIvop = (mEncSceneMode == ENC_SCENE_MODE_NORMAL) ? DEFAULT_QP_IVOP_SCENE_0 : DEFAULT_QP_IVOP_SCENE_1;
    mEncConfig->qpPvop = DEFAULT_QP_PVOP;
    mEncConfig->vbvBufSize = mVideoBitRate / STREAM_SIZE_DIVISOR;
    mEncConfig->profileAndLevel = ((mAVCEncProfile & PROFILE_MASK) << PROFILE_SHIFT_BITS) | mAVCEncLevel;
    mEncConfig->prependSpsPpsEnable = (mPrependSPSPPS == OMX_FALSE) ? FIRST_ELEMENT_INDEX : FRAME_COUNT_ADJUSTMENT_1;
    mEncConfig->encSceneMode = mEncSceneMode;
    CheckUpdateColorAspects(mEncConfig);
    if ((*mH264EncSetConf)(mHandle, mEncConfig) != 0) {
        OMX_LOGE("Failed to set default encoding parameters");
        return OMX_ErrorUndefined;
    }
    return OMX_ErrorNone;
}

void SPRDAVCEncoderOhos::resetOhosOutputHeader(OMX_BUFFERHEADERTYPE *outHeader) const
{
    outHeader->nTimeStamp = FIRST_ELEMENT_INDEX;
    outHeader->nFlags = FIRST_ELEMENT_INDEX;
    outHeader->nOffset = FIRST_ELEMENT_INDEX;
    outHeader->nFilledLen = FIRST_ELEMENT_INDEX;
}

bool SPRDAVCEncoderOhos::SyncOhosStreamBuffer(int fd, unsigned int flags) const
{
    struct dma_buf_sync sync = { DMA_SYNC_FLAG_INITIAL_VALUE };
    sync.flags = flags;
    int ret = ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
    if (ret == 0) {
        return true;
    }
    OMX_LOGE("%s, line:%d, fd:%d, return:%d, DMA_BUF_IOCTL_SYNC failed.",
        __FUNCTION__, __LINE__, fd, ret);
    return false;
}

bool SPRDAVCEncoderOhos::CopyOhosHeaderToBuffer(const MMEncOut &header, const char *tag, bool log16Bytes) const
{
    uint8_t *p = reinterpret_cast<uint8_t*>(header.pOutBuf[FIRST_ELEMENT_INDEX]);
    p[FIRST_ELEMENT_INDEX] = p[LOG_BUFFER_INDEX_1] = p[LOG_BUFFER_INDEX_2] = NAL_START_CODE_BYTE_0;
    p[NAL_START_CODE_PREFIX_SIZE] = NAL_START_CODE;
    if (log16Bytes) {
        OMX_LOGI("%s: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x",
            tag, p[LOG_BUFFER_INDEX_0], p[LOG_BUFFER_INDEX_1], p[LOG_BUFFER_INDEX_2], p[LOG_BUFFER_INDEX_3],
            p[LOG_BUFFER_INDEX_4], p[LOG_BUFFER_INDEX_5], p[LOG_BUFFER_INDEX_6], p[LOG_BUFFER_INDEX_7],
            p[LOG_BUFFER_INDEX_8], p[LOG_BUFFER_INDEX_9], p[LOG_BUFFER_INDEX_10], p[LOG_BUFFER_INDEX_11],
            p[LOG_BUFFER_INDEX_12], p[LOG_BUFFER_INDEX_13], p[LOG_BUFFER_INDEX_14], p[LOG_BUFFER_INDEX_15]);
    } else {
        OMX_LOGI("%s: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x",
            tag, p[LOG_BUFFER_INDEX_0], p[LOG_BUFFER_INDEX_1], p[LOG_BUFFER_INDEX_2], p[LOG_BUFFER_INDEX_3],
            p[LOG_BUFFER_INDEX_4], p[LOG_BUFFER_INDEX_5], p[LOG_BUFFER_INDEX_6], p[LOG_BUFFER_INDEX_7]);
    }
    errno_t ret = memmove_s(header.pheaderBuf, H264ENC_HEADER_BUFFER_SIZE, p, header.strmSize[FIRST_ELEMENT_INDEX]);
    if (ret == 0) {
        return true;
    }
    OMX_LOGE("memmove_s failed at line %d, ret=%d", __LINE__, ret);
    return false;
}

bool SPRDAVCEncoderOhos::generateOhosCodecHeader(int fd, MMEncOut &header, int32_t headerType,
    const char *tag, bool log16Bytes)
{
    (void)memset_s(&header, sizeof(MMEncOut), FIRST_ELEMENT_INDEX, sizeof(MMEncOut));
    ++mNumInputFrames;
    int ret = (*mH264EncGenHeader)(mHandle, &header, headerType);
    if (ret < ENCODER_SUCCESS_CODE || !SyncOhosStreamBuffer(fd, DMA_SYNC_START_FLAGS)) {
        OMX_LOGE("%s, line:%d, mH264EncGenHeader failed, ret: %d", __FUNCTION__, __LINE__, ret);
        return false;
    }
    OMX_LOGI("%s, %d, %sHeader.strmSize: %d", __FUNCTION__, __LINE__, tag, header.strmSize[FIRST_ELEMENT_INDEX]);
    return CopyOhosHeaderToBuffer(header, tag, log16Bytes);
}

bool SPRDAVCEncoderOhos::appendOhosEncodedData(OMX_BUFFERHEADERTYPE *outHeader, uint8_t *&outPtr,
    void *source, uint32_t sourceSize, uint32_t &dataLength) const
{
    if (sourceSize == FIRST_ELEMENT_INDEX) {
        return true;
    }
    size_t remainingSize = outHeader->nAllocLen - (outPtr - reinterpret_cast<uint8_t*>(outHeader->pBuffer));
    errno_t ret = memmove_s(outPtr, remainingSize, source, sourceSize);
    if (ret != 0) {
        OMX_LOGE("memmove_s failed at line %d, ret=%d", __LINE__, ret);
        return false;
    }
    outPtr += sourceSize;
    dataLength += sourceSize;
    return true;
}

bool SPRDAVCEncoderOhos::emitOhosCodecConfig(EncodeOutputState &output, std::list<BufferInfo*>& outQueue)
{
    output.shouldReturn = false;
    if (mSpsPpsHeaderReceived || mNumInputFrames > FIRST_ELEMENT_INDEX) {
        return true;
    }
    int fd = mPmemStream[FIRST_ELEMENT_INDEX]->getHeapID();
    if (!AppendOhosCodecConfigHeaders(&output, fd)) {
        return false;
    }
    mSpsPpsHeaderReceived = true;
    if (mNumInputFrames != FIRST_ELEMENT_INDEX) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((FIRST_ELEMENT_INDEX) == (mNumInputFrames)) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    return finalizeOhosCodecConfigOutput(output, outQueue);
}

bool SPRDAVCEncoderOhos::AppendOhosCodecConfigHeaders(EncodeOutputState *output, int fd)
{
    MMEncOut spsHeader;
    MMEncOut ppsHeader;
    if (output == nullptr) {
        return false;
    }
    if (!generateOhosCodecHeader(fd, spsHeader, HEADER_GEN_INDEX_SPS, "sps", true) ||
        !appendOhosEncodedData(output->outHeader, output->outPtr, spsHeader.pOutBuf[FIRST_ELEMENT_INDEX],
        spsHeader.strmSize[FIRST_ELEMENT_INDEX], output->dataLength) ||
        !SyncOhosStreamBuffer(fd, DMA_SYNC_END_FLAGS) ||
        !generateOhosCodecHeader(fd, ppsHeader, HEADER_GEN_INDEX_PPS, "pps", false) ||
        !appendOhosEncodedData(output->outHeader, output->outPtr, ppsHeader.pOutBuf[FIRST_ELEMENT_INDEX],
        ppsHeader.strmSize[FIRST_ELEMENT_INDEX], output->dataLength) ||
        !SyncOhosStreamBuffer(fd, DMA_SYNC_END_FLAGS)) {
        return false;
    }
    if (mDumpStrmEnabled) {
        DumpFiles(spsHeader.pOutBuf[FIRST_ELEMENT_INDEX], spsHeader.strmSize[FIRST_ELEMENT_INDEX], DUMP_STREAM);
        DumpFiles(ppsHeader.pOutBuf[FIRST_ELEMENT_INDEX], ppsHeader.strmSize[FIRST_ELEMENT_INDEX], DUMP_STREAM);
    }
    return true;
}

bool SPRDAVCEncoderOhos::finalizeOhosCodecConfigOutput(
    EncodeOutputState &output, std::list<BufferInfo*>& outQueue)
{
    if (OHOS::OMX::SprdOMXUtils::notifyEncodeHeaderWithFirstFrame()) {
        return true;
    }
    output.outHeader->nFlags = CODEC_CONFIG_FLAG;
    output.outHeader->nFilledLen = output.dataLength;
    outQueue.erase(outQueue.begin());
    output.outInfo->mOwnedByUs = false;
    notifyFillBufferDone(output.outHeader);
    output.shouldReturn = true;
    return true;
}

void SPRDAVCEncoderOhos::queueOhosInputBufferInfo(OMX_BUFFERHEADERTYPE *inHeader)
{
    InputBufferInfo info;
    info.mTimeUs = inHeader->nTimeStamp;
    info.mFlags = inHeader->nFlags;
    mInputBufferInfoVec.push_back(info);
}

bool SPRDAVCEncoderOhos::mapOhosGraphicBuffer(
    OMX_BUFFERHEADERTYPE *inHeader, GraphicBufferMapping &mapping) const
{
    OHOS::OMX::DynamicBuffer* buffer = (OHOS::OMX::DynamicBuffer*)inHeader->pBuffer;
    BufferHandle *bufferHandle = buffer->bufferHandle;
    OMX_LOGI("size = %u, fd = %d, mStride = %d, mFrameHeight = %d",
        bufferHandle->size, bufferHandle->fd, mStride, mFrameHeight);
    mapping.bufferSize = bufferHandle->size;
    mapping.py = reinterpret_cast<uint8_t*>(mmap(MMAP_OFFSET, bufferHandle->size,
        MMAP_PROT_FLAGS, MMAP_MAP_FLAGS, bufferHandle->fd, MMAP_OFFSET));
    if (mapping.py == MAP_FAILED) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(py != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        mapping.py = nullptr;
        return false;
    }
    mapping.needUnmap = false;
    mapping.iova = NULL_PTR_VALUE;
    mapping.iovaLen = NULL_PTR_VALUE;
    if (mIOMMUEnabled) {
        if (!mEncoderSymbolsReady || mH264EncGetIOVA == nullptr || mHandle == nullptr) {
            OMX_LOGE("IOMMU enabled but encoder IOVA callback is unavailable");
            munmap(mapping.py, mapping.bufferSize);
            mapping.py = nullptr;
            return false;
        }
        int ret = (*mH264EncGetIOVA)(mHandle, bufferHandle->fd, &mapping.iova, &mapping.iovaLen);
        if (ret != 0) {
            OMX_LOGE("H264DecGetIOVA Failed: %d(%s)", ret, strerror(errno));
            munmap(mapping.py, mapping.bufferSize);
            mapping.py = nullptr;
            return false;
        }
        mapping.needUnmap = true;
    }
    mapping.pyPhy = mIOMMUEnabled ? reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(mapping.iova)) : mapping.py;
    OMX_LOGW("py = %p, pyPhy = %p", mapping.py, mapping.pyPhy);
    return mapping.pyPhy != nullptr;
}

void SPRDAVCEncoderOhos::UnmapOhosGraphicBuffer(const GraphicBufferMapping &mapping) const
{
    if (mapping.needUnmap) {
        if (mEncoderSymbolsReady && mH264EncFreeIOVA != nullptr && mHandle != nullptr) {
            OMX_LOGV("Free_iova, iova: 0x%lx, size: %zd", mapping.iova, mapping.iovaLen);
            (*mH264EncFreeIOVA)(mHandle, mapping.iova, mapping.iovaLen);
        } else {
            OMX_LOGW("skip free IOVA due to incomplete encoder state");
        }
    }
    if (mapping.py == nullptr || mapping.bufferSize == 0) {
        return;
    }
    munmap(mapping.py, mapping.bufferSize);
}

void SPRDAVCEncoderOhos::configureOhosEncodeInput(MMEncIn &vidIn, OMX_BUFFERHEADERTYPE *inHeader,
    uint8_t *py, uint8_t *pyPhy)
{
    (void)memset_s(&vidIn, sizeof(MMEncIn), FIRST_ELEMENT_INDEX, sizeof(MMEncIn));
    if (mVideoColorFormat == COLOR_FORMAT_RGBA_8888) {
        vidIn.yuvFormat = YUV_FORMAT_RGBA;
    } else if (mVideoColorFormat == COLOR_FORMAT_N_V12) {
        vidIn.yuvFormat = YUV_FORMAT_NV12;
    } else if (mVideoColorFormat == COLOR_FORMAT_N_V21) {
        vidIn.yuvFormat = YUV_FORMAT_NV21;
    }
    vidIn.timeStamp = (inHeader->nTimeStamp + TIMESTAMP_ROUNDING_OFFSET_MS) / VIDEO_TIME_SCALE_MS;
    vidIn.channelQuality = DEFAULT_CHANNEL_QUALITY;
    vidIn.bitrate = mBitrate;
    vidIn.isChangeBitrate = mIsChangeBitrate;
    mIsChangeBitrate = false;
    vidIn.needIvoP = mKeyFrameRequested || (mNumInputFrames == FIRST_FRAME_INDEX);
    vidIn.srcYuvAddr[FIRST_ELEMENT_INDEX].pSrcY = py;
    vidIn.srcYuvAddr[FIRST_ELEMENT_INDEX].pSrcU = py + mVideoWidth * mVideoHeight;
    vidIn.srcYuvAddr[FIRST_ELEMENT_INDEX].pSrcV = nullptr;
    vidIn.srcYuvAddr[FIRST_ELEMENT_INDEX].pSrcYPhy = pyPhy;
    vidIn.srcYuvAddr[FIRST_ELEMENT_INDEX].pSrcUPhy = pyPhy + mVideoWidth * mVideoHeight;
    vidIn.srcYuvAddr[FIRST_ELEMENT_INDEX].pSrcVPhy = nullptr;
    vidIn.orgImgWidth = static_cast<int32_t>(mFrameWidth);
    vidIn.orgImgHeight = static_cast<int32_t>(mFrameHeight);
    vidIn.cropX = FIRST_ELEMENT_INDEX;
    vidIn.cropY = FIRST_ELEMENT_INDEX;
}

void SPRDAVCEncoderOhos::SyncOhosStreamBuffers()
{
    for (int i = FIRST_ELEMENT_INDEX; i < mMFEnum; i++) {
        if (mPmemStream[i] != nullptr) {
            mPmemStream[i]->cpuSyncStart();
        }
    }
}

bool SPRDAVCEncoderOhos::PatchOhosEncodedFrame(MMEncOut &vidOut) const
{
    uint8_t *p = reinterpret_cast<uint8_t*>(vidOut.pOutBuf[FIRST_ELEMENT_INDEX]);
    if (p == nullptr || vidOut.strmSize[FIRST_ELEMENT_INDEX] <= MIN_VALID_STREAM_SIZE - FRAME_COUNT_ADJUSTMENT_1) {
        return vidOut.strmSize[FIRST_ELEMENT_INDEX] >= ENCODER_SUCCESS_CODE;
    }
    OMX_LOGV("frame: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x",
        p[LOG_BUFFER_INDEX_0], p[LOG_BUFFER_INDEX_1], p[LOG_BUFFER_INDEX_2], p[LOG_BUFFER_INDEX_3],
        p[LOG_BUFFER_INDEX_4], p[LOG_BUFFER_INDEX_5], p[LOG_BUFFER_INDEX_6], p[LOG_BUFFER_INDEX_7]);
    p[FIRST_ELEMENT_INDEX] = p[LOG_BUFFER_INDEX_1] = p[LOG_BUFFER_INDEX_2] = NAL_START_CODE_BYTE_0;
    p[NAL_START_CODE_PREFIX_SIZE] = NAL_START_CODE;
    return true;
}

bool SPRDAVCEncoderOhos::encodeOhosInputBuffer(OMX_BUFFERHEADERTYPE *inHeader, EncodeOutputState *output)
{
    if (output == nullptr) {
        return false;
    }
    GraphicBufferMapping mapping = { nullptr, nullptr, 0, NULL_PTR_VALUE, NULL_PTR_VALUE, false };
    if (!mapOhosGraphicBuffer(inHeader, mapping)) {
        return false;
    }
    MMEncIn vidIn;
    MMEncOut vidOut;
    configureOhosEncodeInput(vidIn, inHeader, mapping.py, mapping.pyPhy);
    (void)memset_s(&vidOut, sizeof(MMEncOut), FIRST_ELEMENT_INDEX, sizeof(MMEncOut));
    if (mDumpYUVEnabled) {
        DumpFiles(mapping.py, mVideoWidth * mVideoHeight * YUV_SIZE_MULTIPLIER_NUMERATOR /
            YUV_SIZE_MULTIPLIER_DENOMINATOR, DUMP_YUV);
    }
    SyncOhosStreamBuffers();
    int64_t startEncode = OHOS::OMX::systemTime();
    int ret = (*mH264EncStrmEncode)(mHandle, &vidIn, &vidOut);
    int64_t endEncode = OHOS::OMX::systemTime();
    OMX_LOGI("H264EncStrmEncode[%lld] %dms, in { %p-%p, %dx%d, %d}, out {%p-%d, %d}", mNumInputFrames,
        (unsigned int)((endEncode - startEncode) / NANOSECONDS_PER_MILLISECOND), mapping.py, mapping.pyPhy,
        mVideoWidth, mVideoHeight, vidIn.yuvFormat, vidOut.pOutBuf[FIRST_ELEMENT_INDEX],
        vidOut.strmSize[FIRST_ELEMENT_INDEX], vidOut.vopType[FIRST_ELEMENT_INDEX]);
    UnmapOhosGraphicBuffer(mapping);
    if (vidOut.strmSize[FIRST_ELEMENT_INDEX] < ENCODER_SUCCESS_CODE || ret != ENCODER_SUCCESS_CODE ||
        !PatchOhosEncodedFrame(vidOut)) {
        OMX_LOGE("Failed to encode frame %lld, ret=%d", mNumInputFrames, ret);
        return true;
    }
    if (mDumpStrmEnabled) {
        DumpFiles(vidOut.pOutBuf[FIRST_ELEMENT_INDEX], vidOut.strmSize[FIRST_ELEMENT_INDEX], DUMP_STREAM);
    }
    if (!appendOhosEncodedData(output->outHeader, output->outPtr, vidOut.pOutBuf[FIRST_ELEMENT_INDEX],
        vidOut.strmSize[FIRST_ELEMENT_INDEX], output->dataLength)) {
        return false;
    }
    if (vidOut.vopType[FIRST_ELEMENT_INDEX] == VOP_TYPE_I) {
        mKeyFrameRequested = false;
        output->outHeader->nFlags |= SYNC_FRAME_FLAG;
    }
    ++mNumInputFrames;
    return true;
}

bool SPRDAVCEncoderOhos::FinalizeOhosOutputBuffer(EncodeFrameContext &frame)
{
    if ((frame.inHeader->nFlags & EOS_FLAG_IDENTIFIER) && (frame.inHeader->nFilledLen == FIRST_ELEMENT_INDEX)) {
        OMX_LOGI("saw EOS");
        frame.outHeader->nFlags = EOS_FLAG_IDENTIFIER;
    }
    frame.inQueue->erase(frame.inQueue->begin());
    frame.inInfo->mOwnedByUs = false;
    notifyEmptyBufferDone(frame.inHeader);
    if (mVideoWidth > mCapability.maxWidth || mVideoHeight > mCapability.maxHeight) {
        OMX_LOGE("[%d, %d] is out of range [%d, %d], failed to support this format.",
            mVideoWidth, mVideoHeight, mCapability.maxWidth, mCapability.maxHeight);
        notify(OMX_EventError, OMX_ErrorBadParameter, FIRST_ELEMENT_INDEX, nullptr);
        mSignalledError = true;
        return false;
    }
    if (mInputBufferInfoVec.empty()) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(!mInputBufferInfoVec.empty() failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    InputBufferInfo *inputBufInfo = &mInputBufferInfoVec[FIRST_ELEMENT_INDEX];
    if (frame.output.dataLength > FIRST_ELEMENT_INDEX || (frame.inHeader->nFlags & EOS_FLAG_IDENTIFIER)) {
        frame.outQueue->erase(frame.outQueue->begin());
        frame.outHeader->nTimeStamp = inputBufInfo->mTimeUs;
        frame.outHeader->nFlags |= (inputBufInfo->mFlags | END_OF_FRAME_FLAG);
        frame.outHeader->nFilledLen = frame.output.dataLength;
        frame.outInfo->mOwnedByUs = false;
        notifyFillBufferDone(frame.outHeader);
    }
    mInputBufferInfoVec.erase(mInputBufferInfoVec.begin());
    return true;
}

bool SPRDAVCEncoderOhos::processOneOhosInputFrame(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue)
{
    EncodeFrameContext frame = {
        &inQueue,
        &outQueue,
        *inQueue.begin(),
        *outQueue.begin(),
        (*inQueue.begin())->mHeader,
        (*outQueue.begin())->mHeader,
        { *outQueue.begin(), (*outQueue.begin())->mHeader,
            reinterpret_cast<uint8_t*>((*outQueue.begin())->mHeader->pBuffer), FIRST_ELEMENT_INDEX, false }
    };
    resetOhosOutputHeader(frame.outHeader);
    bool inputInfoQueued = false;
    auto failCurrentFrame = [&]() {
        if (frame.inInfo->mOwnedByUs) {
            frame.inQueue->erase(frame.inQueue->begin());
            frame.inInfo->mOwnedByUs = false;
            notifyEmptyBufferDone(frame.inHeader);
        }
        if (frame.outInfo->mOwnedByUs) {
            frame.outQueue->erase(frame.outQueue->begin());
            frame.outHeader->nFilledLen = FIRST_ELEMENT_INDEX;
            frame.outHeader->nOffset = FIRST_ELEMENT_INDEX;
            frame.outHeader->nFlags = FIRST_ELEMENT_INDEX;
            frame.outInfo->mOwnedByUs = false;
            notifyFillBufferDone(frame.outHeader);
        }
        if (inputInfoQueued && !mInputBufferInfoVec.empty()) {
            mInputBufferInfoVec.erase(mInputBufferInfoVec.begin());
        }
        mSignalledError = true;
        notify(OMX_EventError, OMX_ErrorUndefined, FIRST_ELEMENT_INDEX, nullptr);
    };
    OMX_LOGI("outHeader->nAllocLen = %d", frame.outHeader->nAllocLen);
    if (!emitOhosCodecConfig(frame.output, outQueue)) {
        failCurrentFrame();
        return false;
    }
    if (frame.output.shouldReturn) {
        return false;
    }
    queueOhosInputBufferInfo(frame.inHeader);
    inputInfoQueued = true;
    if (frame.inHeader->nFilledLen > FIRST_ELEMENT_INDEX && !encodeOhosInputBuffer(frame.inHeader, &frame.output)) {
        failCurrentFrame();
        return false;
    }
    return FinalizeOhosOutputBuffer(frame);
}

OMX_ERRORTYPE SPRDAVCEncoderOhos::initEncParams()
{
    OMX_ERRORTYPE err = initOhosBufferConfig();
    if (err != OMX_ErrorNone) {
        return err;
    }
    err = initOhosCodecSession();
    if (err != OMX_ErrorNone) {
        return err;
    }
    OMX_LOGI("rateCtrlEnable: %d, targetBitRate: %d, frameRate: %d, pFrames: %d, qpIvop: %d, qpPvop: %d, "
        "vbvBufSize: %u, profileAndLevel: %d, prependSpsPpsEnable: %d, encSceneMode: %d",
        mEncConfig->rateCtrlEnable, mEncConfig->targetBitRate, mEncConfig->frameRate,
        mEncConfig->pFrames, mEncConfig->qpIvop, mEncConfig->qpPvop, mEncConfig->vbvBufSize,
        mEncConfig->profileAndLevel, mEncConfig->prependSpsPpsEnable, mEncConfig->encSceneMode);
    mNumInputFrames = INITIAL_FRAME_COUNT;    // 1st two buffers contain SPS and PPS
    mSpsPpsHeaderReceived = false;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoderOhos::initOhosBufferConfig()
{
    if (mEncConfig == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(mEncConfig != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    (void)memset_s(mEncConfig, sizeof(MMEncConfig), STATUS_CHECK_VALUE, sizeof(MMEncConfig));
    InitOhosEncInfo();
    OMX_LOGD("mEncInfo: frameWidth= %d, frameHeight = %d, eisMode = %d, isH263 = %d",
        mEncInfo.frameWidth, mEncInfo.frameHeight, mEncInfo.eisMode, mEncInfo.isH263);
    OMX_LOGD("orgWidth = %d, orgHeight = %d, yuvFormat = %d, timeScale = %d, bitDepth = %d",
        mEncInfo.orgWidth, mEncInfo.orgHeight, mEncInfo.yuvFormat, mEncInfo.timeScale, mEncInfo.bitDepth);

    int alignedWidth = (mVideoWidth + FRAME_ALIGNMENT_MASK) & (~FRAME_ALIGNMENT_MASK);
    int alignedHeight = (mVideoHeight + FRAME_ALIGNMENT_MASK) & (~FRAME_ALIGNMENT_MASK);
    size_t sizeOfYuv = alignedWidth * alignedHeight * YUV_SIZE_MULTIPLIER_NUMERATOR /
        YUV_SIZE_MULTIPLIER_DENOMINATOR;
    size_t sizeStream = sizeOfYuv / STREAM_SIZE_DIVISOR;
    return allocateOhosStreamBuffers(sizeStream);
}

OMX_ERRORTYPE SPRDAVCEncoderOhos::initOhosCodecSession()
{
    MMEncCodecPriority mEncPriority;
    mEncPriority.priority = DEFAULT_ENCODER_PRIORITY;
    mEncPriority.operatingRate = DEFAULT_OPERATING_RATE;
    if ((*mH264EncInit)(mHandle, &mStreamBfr, &mEncInfo, &mEncPriority)) {
        OMX_LOGE("Failed to init mp4enc");
        return OMX_ErrorUndefined;
    }
    if ((*mH264EncGetConf)(mHandle, mEncConfig)) {
        OMX_LOGE("Failed to get default encoding parameters");
        return OMX_ErrorUndefined;
    }
    return configureOhosEncConfig();
}

OMX_ERRORTYPE SPRDAVCEncoderOhos::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    if (params == nullptr) {
        OMX_LOGE("params is null pointer.");
        return OMX_ErrorBadParameter;
    }

    switch (index) {
        case OMX_IndexParamSupportBufferType: {
            return getOhosSupportBufferType(static_cast<UseBufferType *>(params));
        }
        case OMX_IndexCodecVideoPortFormat: {
            return getOhosCodecVideoPortFormat(static_cast<CodecVideoPortFormatParam *>(params));
        }
        default:
            return SPRDAVCEncoder::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDAVCEncoderOhos::getOhosSupportBufferType(UseBufferType *defParams)
{
    if (CheckParam(defParams, sizeof(UseBufferType))) {
        OMX_LOGE("UseBufferType parameter check failed.");
        return OMX_ErrorUnsupportedIndex;
    }
    if (defParams->portIndex > PORT_INDEX_OUTPUT) {
        OMX_LOGE("port index error, portIndex=%d, max=%d.", defParams->portIndex, PORT_INDEX_OUTPUT);
        return OMX_ErrorUnsupportedIndex;
    }
    defParams->bufferType = CODEC_BUFFER_TYPE_AVSHARE_MEM_FD;
    if (defParams->portIndex == OHOS::OMX::K_INPUT_PORT_INDEX) {
        defParams->bufferType = BUFFER_TYPE_DYNAMIC_HANDLE;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoderOhos::getOhosCodecVideoPortFormat(CodecVideoPortFormatParam *formatParams)
{
    if (CheckParam(formatParams, sizeof(CodecVideoPortFormatParam))) {
        OMX_LOGE("CodecVideoPortFormatParam parameter check failed.");
        return OMX_ErrorUnsupportedIndex;
    }
    PortInfo* port = editPortInfo(formatParams->portIndex);
    if (port == nullptr) {
        OMX_LOGE("port info is null for portIndex=%d.", formatParams->portIndex);
        return OMX_ErrorBadParameter;
    }
    formatParams->codecColorFormat = port->mDef.format.video.eColorFormat;
    formatParams->codecCompressFormat = port->mDef.format.video.eCompressionFormat;
    formatParams->framerate = port->mDef.format.video.xFramerate;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoderOhos::internalSetParameter(OMX_INDEXTYPE index, const OMX_PTR params)
{
    OMX_LOGI("%s, %d, cmd index: 0x%x", __FUNCTION__, __LINE__, index);
    switch (static_cast<int>(index)) {
        case OMX_IndexParamUseBufferType: {
            return SPRDAVCEncoder::internalSetParameter(index, params);
        }
        case OMX_IndexParamPortDefinition: {
            return setOhosPortDefinition(params);
        }
        case OMX_IndexCodecVideoPortFormat: {
            return setOhosCodecVideoPortFormat(params);
        }
        default:
            return SPRDAVCEncoder::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDAVCEncoderOhos::setOhosPortDefinition(const OMX_PTR params)
{
    OMX_PARAM_PORTDEFINITIONTYPE* def =
        static_cast<OMX_PARAM_PORTDEFINITIONTYPE*>(const_cast<OMX_PTR>(params));
    mStride = (def->format.video.nFrameWidth + FRAME_ALIGNMENT_MASK) & (~FRAME_ALIGNMENT_MASK);
    def->format.video.nStride = mStride;
    def->format.video.nSliceHeight = def->format.video.nFrameHeight;
    OMX_LOGI("OMX_IndexParamPortDefinition nFrameWidth : %d, mStride: %u",
        def->format.video.nFrameWidth, mStride);
    return SprdVideoEncoderBase::setParamPortDefinition(OMX_VIDEO_CodingAVC, OMX_IndexParamPortDefinition, params);
}

OMX_ERRORTYPE SPRDAVCEncoderOhos::setOhosCodecVideoPortFormat(const OMX_PTR params)
{
    CodecVideoPortFormatParam* formatParams =
        static_cast<CodecVideoPortFormatParam*>(const_cast<OMX_PTR>(params));
    if (CheckParam(formatParams, sizeof(CodecVideoPortFormatParam))) {
        return OMX_ErrorUnsupportedIndex;
    }

    PortInfo* port = editPortInfo(formatParams->portIndex);
    if (port == nullptr) {
        OMX_LOGE("port info is null for portIndex: %d", formatParams->portIndex);
        return OMX_ErrorBadParameter;
    }
    port->mDef.format.video.eColorFormat = static_cast<OMX_COLOR_FORMATTYPE>(formatParams->codecColorFormat);
    port->mDef.format.video.eCompressionFormat =
        static_cast<OMX_VIDEO_CODINGTYPE>(formatParams->codecCompressFormat);
    port->mDef.format.video.xFramerate = static_cast<OMX_U32>(formatParams->framerate);
    mVideoColorFormat = port->mDef.format.video.eColorFormat;
    OMX_LOGI("internalSetParameter, eCompressionFormat: %d, eColorFormat: 0x%x",
        port->mDef.format.video.eCompressionFormat, port->mDef.format.video.eColorFormat);
    return OMX_ErrorNone;
}
void SPRDAVCEncoderOhos::onQueueFilled(OMX_U32 portIndex)
{
    (void)portIndex;
    if (mSignalledError) {
        return;
    }
    if (!mStarted) {
        if (OMX_ErrorNone != initEncoder()) {
            return;
        }
    }
    std::list<BufferInfo*>& inQueue = getPortQueue(PORT_INDEX_INPUT);
    std::list<BufferInfo*>& outQueue = getPortQueue(PORT_INDEX_OUTPUT);
    while (!inQueue.empty() && !outQueue.empty()) {
        if (!processOneOhosInputFrame(inQueue, outQueue)) {
            return;
        }
    }
}
OHOS::OMX::SprdOMXComponent *createSprdOMXComponent(
    const char *name, const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData, OMX_COMPONENTTYPE **component)
{
    return new SPRDAVCEncoderOhos(name, callbacks, appData, component);
}
