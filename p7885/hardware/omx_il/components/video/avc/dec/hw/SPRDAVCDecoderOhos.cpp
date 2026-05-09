/*
 * Copyright 2023 Shenzhen Kaihong DID Co., Ltd.
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
#include "SPRDAVCDecoderOhos.h"
#include "OMX_VideoExt.h"
#include <sys/mman.h>
#include <algorithm>
#undef LOG_TAG
#define LOG_TAG "SPRDAVCDecoderOhos"
namespace OHOS {
namespace OMX {
// Memory alignment size for buffer allocation (4KB)
#define ALIGNMENT_SIZE_4KB 4096
// Default initial reference count for buffer control
#define DEFAULT_INITIAL_REF_COUNT 1
// Color component offset for YUV data (chroma offset calculation)
#define YUV_CHROMA_OFFSET_DIVIDER 2
// Port index validation boundary
#define MAX_PORT_INDEX 1
// Number of OMX component ports (input and output)
#define OMX_PORT_COUNT 2
// Starting port number for OMX component
#define OMX_START_PORT_NUMBER 0
// Memory protection flags for mmap (read and write access)
#define MMAP_PROT_FLAGS (PROT_READ | PROT_WRITE)
// Memory mapping flags for mmap (shared mapping)
#define MMAP_MAP_FLAGS MAP_SHARED
// Invalid file descriptor value
#define INVALID_FILE_DESCRIPTOR (-1)
// Memory mapping failure return value
#define MMAP_FAILURE_VALUE ((void *)-1)
// Port buffer comparison tolerance threshold
#define PORT_BUFFER_SIZE_TOLERANCE 0
// Reference count increment value
#define REF_COUNT_INCREMENT 1
// Buffer header offset reset value
#define BUFFER_HEADER_OFFSET_RESET 0
// Buffer header flag reset value
#define BUFFER_HEADER_FLAG_RESET 0
// Video port format start index
#define VIDEO_PORT_FORMAT_START_INDEX 0
// Unspecified buffer supplier value
#define BUFFER_SUPPLIER_UNSPECIFIED 0
// Zero tick count value
#define ZERO_TICK_COUNT 0
// Zero time stamp value
#define ZERO_TIME_STAMP 0
// Memory zero address
#define MEMORY_ZERO_ADDRESS 0
// Buffer size zero value
#define BUFFER_SIZE_ZERO 0
// Port index zero value
#define PORT_INDEX_ZERO 0
// Default port buffer count adjustment threshold
#define DEFAULT_PORT_BUFFER_COUNT_ADJUSTMENT 0
// Memory address zero value
#define MEMORY_ADDRESS_ZERO 0
// Decoder buffer queue empty size
#define DECODER_BUFFER_QUEUE_EMPTY_SIZE 0
// Buffer flag for end of stream
#define BUFFER_FLAG_EOS OMX_BUFFERFLAG_EOS
// Memory allocation size zero value
#define MEMORY_ALLOC_SIZE_ZERO 0
// Memory offset zero value
#define MEMORY_OFFSET_ZERO 0
SPRDAVCDecoderOhos::SPRDAVCDecoderOhos(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SPRDAVCDecoder(name, callbacks, appData, component)
{
        iUseOhosNativeBuffer[OMX_DirInput] = OMX_FALSE;
        iUseOhosNativeBuffer[OMX_DirOutput] = OMX_FALSE;
}
OMX_ERRORTYPE SPRDAVCDecoderOhos::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    OMX_LOGI("SPRDAVCDecoderOhos::internalGetParameter cmd index: 0x%x", index);
    switch (static_cast<int>(index)) {
        case OMX_IndexCodecExtEnableNativeBuffer: {
            return setOhosNativeBufferEnabled(static_cast<CodecEnableNativeBufferParams *>(params));
        }
        case OMX_IndexParamSupportBufferType: {
            return getSupportBufferType(static_cast<UseBufferType *>(params));
        }
        case OMX_IndexParamGetBufferHandleUsage: {
            return getBufferHandleUsage(static_cast<GetBufferHandleUsageParams *>(params));
        }
        case OMX_INDEX_ENABLE_ANB: {
            OMX_LOGE("internalGetParameter, DO NOT USE OMX NATIVE BUFFER");
            return OMX_ErrorUnsupportedIndex;
        }
        case OMX_IndexParamCompBufferSupplier: {
            return getBufferSupplier(static_cast<OMX_PARAM_BUFFERSUPPLIERTYPE *>(params));
        }
        case OMX_IndexParamVideoInit: {
            return initVideoPort(static_cast<OMX_PORT_PARAM_TYPE *>(params));
        }
        case OMX_IndexCodecVideoPortFormat: {
            return getCodecVideoPortFormat(static_cast<CodecVideoPortFormatParam *>(params));
        }
        default:
            return SPRDAVCDecoder::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::setOhosNativeBufferEnabled(const CodecEnableNativeBufferParams *formatParams)
{
    iUseOhosNativeBuffer[formatParams->portIndex] = formatParams->enable ? OMX_TRUE : OMX_FALSE;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::initVideoPort(OMX_PORT_PARAM_TYPE *portParams) const
{
    portParams->nPorts = OMX_PORT_COUNT;
    portParams->nStartPortNumber = OMX_START_PORT_NUMBER;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::getSupportBufferType(UseBufferType *defParams)
{
    if (CheckParam(defParams, sizeof(UseBufferType)) || defParams->portIndex > MAX_PORT_INDEX) {
        OMX_LOGE("port index error.");
        return OMX_ErrorUnsupportedIndex;
    }
    defParams->bufferType = (defParams->portIndex == K_OUTPUT_PORT_INDEX) ?
        CODEC_BUFFER_TYPE_HANDLE : CODEC_BUFFER_TYPE_AVSHARE_MEM_FD;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::getBufferHandleUsage(GetBufferHandleUsageParams *defParams) const
{
    if (defParams->size != sizeof(GetBufferHandleUsageParams)) {
        OMX_LOGE("param size check fail.");
        return OMX_ErrorBadParameter;
    }
    defParams->usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::getBufferSupplier(OMX_PARAM_BUFFERSUPPLIERTYPE *defParams)
{
    if (CheckParam(defParams, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE))) {
        return OMX_ErrorUnsupportedIndex;
    }
    const PortInfo *port = &mPorts[defParams->nPortIndex];
    if (port->mDef.eDir == OMX_DirInput) {
        defParams->eBufferSupplier = OMX_BufferSupplyInput;
        return OMX_ErrorNone;
    }
    if (port->mDef.eDir == OMX_DirOutput) {
        defParams->eBufferSupplier = OMX_BufferSupplyOutput;
        return OMX_ErrorNone;
    }
    OMX_LOGE("index OMX_IndexParamCompBufferSupplier error");
    return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::getCodecVideoPortFormat(CodecVideoPortFormatParam *formatParams)
{
    if (CheckParam(formatParams, sizeof(CodecVideoPortFormatParam))) {
        return OMX_ErrorUnsupportedIndex;
    }
    PortInfo *port = editPortInfo(formatParams->portIndex);
    formatParams->codecColorFormat = port->mDef.format.video.eColorFormat;
    formatParams->codecCompressFormat = port->mDef.format.video.eCompressionFormat;
    formatParams->framerate = port->mDef.format.video.xFramerate;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params)
{
    OMX_LOGI("cmd index: 0x%x", index);
    switch (static_cast<int>(index)) {
        case OMX_IndexParamUseBufferType: {
            return setUseBufferType(static_cast<const UseBufferType *>(params));
        }
        case OMX_INDEX_ENABLE_ANB: {
            OMX_LOGE("internalSetParameter, DO NOT USE OMX NATIVE BUFFER");
            return OMX_ErrorUnsupportedIndex;
        }
        // video format info
        case OMX_IndexCodecVideoPortFormat: {
            return setCodecVideoPortFormat(static_cast<const CodecVideoPortFormatParam *>(params));
        }
        // set video port format
        case OMX_IndexParamVideoPortFormat: {
            return setVideoPortFormat(static_cast<const OMX_VIDEO_PARAM_PORTFORMATTYPE *>(params));
        }
        case OMX_INDEX_PREPARE_FOR_ADAPTIVE_PLAYBACK: {
            OMX_LOGE("internalSetParameter, DO NOT USE OMX_INDEX_PREPARE_FOR_ADAPTIVE_PLAYBACK");
            return OMX_ErrorUnsupportedIndex;
        }
        // set use native buffer
        case OMX_IndexCodecExtEnableNativeBuffer: {
            return setOhosNativeBufferEnabled(static_cast<const CodecEnableNativeBufferParams *>(params));
        }
        case OMX_IndexParamCompBufferSupplier: {
            return setBufferSupplier(static_cast<const OMX_PARAM_BUFFERSUPPLIERTYPE *>(params));
        }
        case OMX_IndexParamProcessName: {
            return setProcessName(static_cast<const ProcessNameParam *>(params));
        }
        default:
            return SPRDAVCDecoder::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::setUseBufferType(const UseBufferType *defParams)
{
    iUseOhosNativeBuffer[defParams->portIndex] = OMX_TRUE;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::setCodecVideoPortFormat(const CodecVideoPortFormatParam *formatParams)
{
    if (CheckParam(const_cast<CodecVideoPortFormatParam *>(formatParams), sizeof(CodecVideoPortFormatParam))) {
        return OMX_ErrorUnsupportedIndex;
    }
    PortInfo *port = editPortInfo(formatParams->portIndex);
    port->mDef.format.video.eColorFormat = static_cast<OMX_COLOR_FORMATTYPE>(formatParams->codecColorFormat);
    port->mDef.format.video.eCompressionFormat = static_cast<OMX_VIDEO_CODINGTYPE>(formatParams->codecCompressFormat);
    port->mDef.format.video.xFramerate = static_cast<OMX_U32>(formatParams->framerate);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::setVideoPortFormat(const OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams)
{
    if (CheckParam(const_cast<OMX_VIDEO_PARAM_PORTFORMATTYPE *>(formatParams),
        sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE))) {
        return OMX_ErrorUnsupportedIndex;
    }
    PortInfo *port = editPortInfo(formatParams->nPortIndex);
    port->mDef.format.video.eColorFormat = formatParams->eColorFormat;
    port->mDef.format.video.eCompressionFormat = formatParams->eCompressionFormat;
    port->mDef.format.video.xFramerate = formatParams->xFramerate;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::setBufferSupplier(const OMX_PARAM_BUFFERSUPPLIERTYPE *defParams)
{
    if (defParams->nPortIndex >= mPorts.size() || defParams->nSize != sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)) {
        return OMX_ErrorUndefined;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::setProcessName(const ProcessNameParam *nameParams) const
{
    OMX_LOGI("OMX_IndexParamProcessName : %s", nameParams->processName);
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SPRDAVCDecoderOhos::internalSetConfig(
    OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig)
{
    switch (index) {
        case OMX_INDEX_CONFIG_THUMBNAIL_MODE: {
            OMX_LOGE("setConfig  ThumbnailMode is not supported");
            return OMX_ErrorNotImplemented;
        }
        case OMX_INDEX_DESCRIBE_COLOR_ASPECTS: {
            OMX_LOGE("setConfig  DescribeColorAspects is not supported");
            return OMX_ErrorNotImplemented;
        }
        case OMX_INDEX_CONFIG_DEC_SCENE_MODE: {
            OMX_LOGE("setConfig  DecSceneMode is not supported");
            return OMX_ErrorNotImplemented;
        }
        //bitrate
        case OMX_IndexConfigVideoBitrate: {
            OMX_VIDEO_CONFIG_BITRATETYPE* configParam = (OMX_VIDEO_CONFIG_BITRATETYPE*)params;
            if (CheckParam(configParam, sizeof(OMX_VIDEO_CONFIG_BITRATETYPE))) {
                return OMX_ErrorUnsupportedIndex;
            }
            if (configParam->nPortIndex != K_OUTPUT_PORT_INDEX) {
                OMX_LOGE("only support  BitRate from Output port");
                return OMX_ErrorUnsupportedIndex;
            }
            return OMX_ErrorNone;
        }
        default:
            return SPRDAVCDecoder::internalSetConfig(index, params, frameConfig);
    }
}
OMX_ERRORTYPE SPRDAVCDecoderOhos::getConfig(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    OMX_LOGI("cmd index: 0x%x", index);
    switch (index) {
        case OMX_INDEX_DESCRIBE_COLOR_ASPECTS: {
            OMX_LOGE("SPRDAVCDecoderOhos, DO NOT USE OMX_INDEX_DESCRIBE_COLOR_ASPECTS");
            return OMX_ErrorUnsupportedIndex;
        }
        case OMX_IndexConfigVideoBitrate: {
            OMX_VIDEO_CONFIG_BITRATETYPE* configParam = (OMX_VIDEO_CONFIG_BITRATETYPE*)params;
            if (CheckParam(configParam, sizeof(OMX_VIDEO_CONFIG_BITRATETYPE))) {
                OMX_LOGE("yuanwei ERROR PARAM");
                return OMX_ErrorUnsupportedIndex;
            }
            OMX_LOGE("Not return");
            if (configParam->nPortIndex != K_OUTPUT_PORT_INDEX) {
                OMX_LOGE("only support Get BitRate from Output port");
                return OMX_ErrorUnsupportedIndex;
            }
            PortInfo *port = editPortInfo(K_OUTPUT_PORT_INDEX);
            configParam->nEncodeBitrate = port->mDef.format.video.nBitrate;
            return OMX_ErrorNone;
        }
        default:
            return SPRDAVCDecoder::getConfig(index, params);
    }
}
SprdOMXComponent *createSprdOMXComponent(
    const char *name, const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData, OMX_COMPONENTTYPE **component)
{
    return new SPRDAVCDecoderOhos(name, callbacks, appData, component);
}
bool SPRDAVCDecoderOhos::GetVuiParams(H264SwDecInfo *mDecoderInfo)
{
    return true;
}
OMX_ERRORTYPE SPRDAVCDecoderOhos::useOhosBufferHandle(
    OMX_BUFFERHEADERTYPE **header,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    OMX_U32 size,
    OMX_U8 *ptr)
{
    (void)portIndex;
    (void)appPrivate;
    OhosOutputBufferConfig config;
    BufferHandle *bufferHandle = nullptr;
    OMX_ERRORTYPE err = buildOhosOutputBufferConfig(normalizeOhosOutputBufferSize(size), ptr, config, bufferHandle);
    if (err != OMX_ErrorNone) {
        return err;
    }
    return configureOhosOutputBuffer(header, config);
}

OMX_U32 SPRDAVCDecoderOhos::normalizeOhosOutputBufferSize(OMX_U32 size) const
{
    PortInfo *port = const_cast<SPRDAVCDecoderOhos *>(this)->editPortInfo(K_OUTPUT_PORT_INDEX);
    if (size != port->mDef.nBufferSize + PORT_BUFFER_SIZE_TOLERANCE) {
        OMX_LOGE("PLEASE CHECK--size %d doesnt match Outputport nBufferSize %d", size, port->mDef.nBufferSize);
        return port->mDef.nBufferSize;
    }
    return size;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::buildOhosOutputBufferConfig(
    OMX_U32 size, OMX_U8 *ptr, OhosOutputBufferConfig &config, BufferHandle *&bufferHandle) const
{
    unsigned long phyAddr = MEMORY_ADDRESS_ZERO;
    size_t bufferSize = BUFFER_SIZE_ZERO;
    OMX_U8 *platformBuffer = nullptr;
    VideoMemAllocator *vm = nullptr;
    bufferHandle = reinterpret_cast<BufferHandle *>(ptr);
    OMX_ERRORTYPE err = mapPlatformBuffer(bufferHandle, platformBuffer);
    if (err != OMX_ErrorNone) {
        return err;
    }
    bufferHandle->virAddr = platformBuffer;
    err = allocateOhosOutputMemory(vm, phyAddr, bufferSize, size);
    if (err != OMX_ErrorNone) {
        return err;
    }
    config.size = size;
    config.buffer = reinterpret_cast<OMX_U8 *>(vm->getBase());
    config.memory = vm;
    config.phyAddr = phyAddr;
    config.bufferSize = bufferSize;
    config.platformPrivate = ptr;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::mapPlatformBuffer(BufferHandle *bufferhandle, OMX_U8 *&pBufferPlatform) const
{
    if (bufferhandle->fd < PORT_INDEX_ZERO) {
        OMX_LOGE("Failed to use outport buffer");
        return OMX_ErrorInsufficientResources;
    }
    pBufferPlatform = reinterpret_cast<OMX_U8 *>(mmap(MEMORY_ZERO_ADDRESS, bufferhandle->size, MMAP_PROT_FLAGS,
        MMAP_MAP_FLAGS, bufferhandle->fd, MEMORY_OFFSET_ZERO));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::allocateOhosOutputMemory(
    VideoMemAllocator *&vm, unsigned long &phyAddr, size_t &bufferSize, OMX_U32 size) const
{
    vm = new VideoMemAllocator(size, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    int fd = vm->getHeapID();
    if (fd < PORT_INDEX_ZERO) {
        OMX_LOGE("Failed to alloc outport pmem buffer");
        return OMX_ErrorInsufficientResources;
    }
    if (mIOMMUEnabled) {
        int ret = (*mH264DecGetIOVA)(mHandle, fd, &phyAddr, &bufferSize, true);
        if (ret != 0) {
            OMX_LOGE("H264DecGetIOVA Failed: %d(%s)", ret, strerror(errno));
            return OMX_ErrorInsufficientResources;
        }
        return OMX_ErrorNone;
    }
    if (VideoMemAllocator::GetPhyAddrFromMemAlloc(fd, &phyAddr, &bufferSize)) {
        OMX_LOGE("getPhyAddrFromMemAlloc fail");
        return OMX_ErrorInsufficientResources;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::configureOhosOutputBuffer(
    OMX_BUFFERHEADERTYPE **header, const OhosOutputBufferConfig &config)
{
    OMX_LOGI("useOhosBufferHandle, allocate buffer from pmem, pBuffer: %p, phyAddr: 0x%lx, size: %zd, "
        "mIOMMUEnabled = %d", config.buffer, config.phyAddr, config.bufferSize, mIOMMUEnabled);
    if ((*header)->pOutputPortPrivate == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((*header)->pOutputPortPrivate != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    BufferCtrlStruct *pBufCtrl = reinterpret_cast<BufferCtrlStruct *>((*header)->pOutputPortPrivate);
    pBufCtrl->iRefCount = DEFAULT_INITIAL_REF_COUNT;
    pBufCtrl->id = mIOMMUID;
    pBufCtrl->pMem = config.memory;
    pBufCtrl->phyAddr = config.phyAddr;
    pBufCtrl->bufferSize = config.bufferSize;
    pBufCtrl->bufferFd = config.memory->getHeapID();
    (*header)->pBuffer = config.buffer;
    (*header)->nAllocLen = config.size;
    (*header)->pPlatformPrivate = config.platformPrivate;
    return OMX_ErrorNone;
}

void SPRDAVCDecoderOhos::freePlatform(OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header)
{
    OMX_LOGI("SPRDAVCDecoderOhos::freePlatform");
    PortInfo *port = &mPorts.at(portIndex);
    BufferCtrlStruct* pBufCtrl = (BufferCtrlStruct*)(header->pOutputPortPrivate);
    for (size_t i = VIDEO_PORT_FORMAT_START_INDEX; i < port->mBuffers.size(); ++i) {
        BufferInfo *buffer = &port->mBuffers.at(i);
        if (buffer->mHeader == header) {
            releasePlatformBuffer(header, pBufCtrl);
            return;
        }
    }
}

void SPRDAVCDecoderOhos::releasePlatformBuffer(OMX_BUFFERHEADERTYPE *header, BufferCtrlStruct *bufferCtrl) const
{
    (void)bufferCtrl;
    if (header->pPlatformPrivate == nullptr) {
        return;
    }
    BufferHandle* bufferhandle = reinterpret_cast<BufferHandle*>(header->pPlatformPrivate);
    if (munmap(bufferhandle->virAddr, bufferhandle->size) != 0) {
        OMX_LOGE("Unmap failed");
    }
    bufferhandle->virAddr = nullptr;
    header->pPlatformPrivate = nullptr;
    OMX_LOGI("header->pPlatformPrivate munmap success");
}

void SPRDAVCDecoderOhos::dispatchFillBufferDone(OMX_BUFFERHEADERTYPE *header) const
{
    (*mCallbacks->FillBufferDone)(mComponent, mComponent->pApplicationPrivate, header);
}

bool SPRDAVCDecoderOhos::prepareCopyParams(OMX_BUFFERHEADERTYPE *header, BufferHandle *&bufferhandle,
    uint32_t &copyWidth, uint32_t &copyHeight) const
{
    if (header == nullptr || header->pBuffer == nullptr || header->pPlatformPrivate == nullptr) {
        return false;
    }
    if (mOutputPortSettingsChange != NONE) {
        OMX_LOGW("skip output copy while port settings are changing, state=%d", mOutputPortSettingsChange);
        dispatchFillBufferDone(header);
        return false;
    }
    bufferhandle = reinterpret_cast<BufferHandle *>(header->pPlatformPrivate);
    if (bufferhandle->virAddr == nullptr || bufferhandle->stride <= 0 || bufferhandle->height <= 0 ||
        bufferhandle->size == 0 || mStride == 0 || mSliceHeight == 0) {
        OMX_LOGE("invalid buffer layout, skip copy: dst(%p, stride=%d, height=%d), src(stride=%u, slice=%u)",
            bufferhandle->virAddr, bufferhandle->stride, bufferhandle->height, mStride, mSliceHeight);
        dispatchFillBufferDone(header);
        return false;
    }

    const uint32_t dstStride = static_cast<uint32_t>(bufferhandle->stride);
    const uint32_t dstHeight = static_cast<uint32_t>(bufferhandle->height);
    const uint32_t dstCapacity = static_cast<uint32_t>(bufferhandle->size);
    copyHeight = (dstHeight < mSliceHeight) ? dstHeight : mSliceHeight;
    copyWidth = (dstStride < mStride) ? dstStride : mStride;
    if (copyHeight == 0 || copyWidth == 0) {
        dispatchFillBufferDone(header);
        return false;
    }

    const uint64_t dstRequired = static_cast<uint64_t>(dstStride) * copyHeight *
        (YUV_CHROMA_OFFSET_DIVIDER + 1) / YUV_CHROMA_OFFSET_DIVIDER;
    const uint64_t srcRequired = static_cast<uint64_t>(mStride) * mSliceHeight +
        static_cast<uint64_t>(mStride) * (copyHeight / YUV_CHROMA_OFFSET_DIVIDER);
    const uint64_t srcCapacity = static_cast<uint64_t>(static_cast<uint32_t>(header->nAllocLen));
    if (dstRequired > dstCapacity || srcRequired > srcCapacity) {
        OMX_LOGE("buffer size mismatch, skip copy: dstNeed=%llu dstCap=%u srcNeed=%llu srcCap=%u copyWidth=%u "
            "copyHeight=%u", dstRequired, dstCapacity, srcRequired, header->nAllocLen, copyWidth, copyHeight);
        dispatchFillBufferDone(header);
        return false;
    }
    return true;
}

void SPRDAVCDecoderOhos::notifyFillBufferDone(OMX_BUFFERHEADERTYPE *header)
{
    OMX_LOGD("notifyFillBufferDone enter");
    BufferHandle *bufferhandle = nullptr;
    uint32_t copyWidth = 0;
    uint32_t copyHeight = 0;
    if (!prepareCopyParams(header, bufferhandle, copyWidth, copyHeight)) {
        return;
    }
    OMX_LOGD("notifyFillBufferDone mStride=%u, mSliceHeight=%u, dstStride=%d, dstHeight=%d, copyWidth=%u, "
        "copyHeight=%u", mStride, mSliceHeight, bufferhandle->stride, bufferhandle->height, copyWidth, copyHeight);
    OMX_U8 *buffInfo = reinterpret_cast<OMX_U8 *>(header->pBuffer);
    OMX_U8 *dstInfo = reinterpret_cast<OMX_U8 *>(bufferhandle->virAddr);
    copyOhosLumaPlane(dstInfo, buffInfo, bufferhandle);
    copyOhosChromaPlane(dstInfo, buffInfo, bufferhandle);
    dispatchFillBufferDone(header);
}

void SPRDAVCDecoderOhos::copyOhosLumaPlane(OMX_U8 *dstInfo, OMX_U8 *buffInfo, const BufferHandle *bufferhandle) const
{
    if (bufferhandle->stride <= 0 || bufferhandle->height <= 0) {
        return;
    }
    const uint32_t dstStride = static_cast<uint32_t>(bufferhandle->stride);
    const uint32_t dstHeight = static_cast<uint32_t>(bufferhandle->height);
    const uint32_t copyHeight = (dstHeight < mSliceHeight) ? dstHeight : mSliceHeight;
    const uint32_t copyWidth = (dstStride < mStride) ? dstStride : mStride;
    for (uint32_t i = VIDEO_PORT_FORMAT_START_INDEX; i < copyHeight; i++) {
        errno_t ret = memmove_s(&dstInfo[i * dstStride], dstStride,
            &buffInfo[i * mStride], copyWidth);
        if (ret != 0) {
            OMX_LOGE("memmove_s failed at line %d, ret=%d", __LINE__, ret);
        }
    }
}

void SPRDAVCDecoderOhos::copyOhosChromaPlane(
    OMX_U8 *dstInfo, OMX_U8 *buffInfo, const BufferHandle *bufferhandle) const
{
    if (bufferhandle->stride <= 0 || bufferhandle->height <= 0) {
        return;
    }
    const uint32_t dstStride = static_cast<uint32_t>(bufferhandle->stride);
    const uint32_t dstHeight = static_cast<uint32_t>(bufferhandle->height);
    const uint32_t copyHeight = (dstHeight < mSliceHeight) ? dstHeight : mSliceHeight;
    const uint32_t copyWidth = (dstStride < mStride) ? dstStride : mStride;
    uint32_t dstSize = copyHeight * dstStride;
    uint32_t srcSize = mSliceHeight * mStride;
    for (uint32_t i = VIDEO_PORT_FORMAT_START_INDEX; i < copyHeight / YUV_CHROMA_OFFSET_DIVIDER; i++) {
        errno_t ret = memmove_s(&dstInfo[dstSize + i * dstStride], dstStride,
            &buffInfo[srcSize + i * mStride], copyWidth);
        if (ret != 0) {
            OMX_LOGE("memmove_s failed at line %d, ret=%d", __LINE__, ret);
        }
    }
}
void SPRDAVCDecoderOhos::drainOneOutputBuffer(int32_t picId, const void *pBufferHeader, OMX_U64 pts)
{
    (void)picId;
    std::list<BufferInfo*>& outQueue = getPortQueue(K_OUTPUT_PORT_INDEX);
    std::list<BufferInfo *>::iterator it = outQueue.begin();
    if (!findOutputBufferByHeader(outQueue, pBufferHeader, it)) {
        return;
    }
    BufferInfo *outInfo = *it;
    OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;
    outHeader->nFilledLen = mPictureSize;
    outHeader->nTimeStamp = (OMX_TICKS)pts;
    OMX_LOGD("%s, %d, outHeader: %p, outHeader->pBuffer: %p, outHeader->nOffset: %d, outHeader->nFlags: %d, "
        "outHeader->nTimeStamp: %lld", __FUNCTION__, __LINE__, outHeader, outHeader->pBuffer,
        outHeader->nOffset, outHeader->nFlags, outHeader->nTimeStamp);
    outInfo->mOwnedByUs = false;
    outQueue.erase(it);
    outInfo = nullptr;
    BufferCtrlStruct* pOutBufCtrl = (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
    pOutBufCtrl->iRefCount++;
    notifyFillBufferDone(outHeader);
}

bool SPRDAVCDecoderOhos::findOutputBufferByHeader(
    std::list<BufferInfo*>& outQueue, const void *bufferHeader, std::list<BufferInfo *>::iterator &it) const
{
    while (it != outQueue.end() && (*it) != nullptr && (*it)->mHeader != (OMX_BUFFERHEADERTYPE*)bufferHeader) {
        ++it;
    }
    if (it == outQueue.end() || (*it) == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(it != outQueue.end() && (*it) != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    if ((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)bufferHeader) {
        return true;
    }
    OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((*it)->mHeader == "
        "(OMX_BUFFERHEADERTYPE*)bufferHeader) failed.", __FUNCTION__, FILENAME_ONLY, __LINE__);
    return false;
}

bool SPRDAVCDecoderOhos::validateDrainQueues(std::list<BufferInfo*>& outQueue) const
{
    if (mDeintl && mDecOutputBufQueue.size() == DECODER_BUFFER_QUEUE_EMPTY_SIZE) {
        OMX_LOGW("%s, %d, There is no more  decoder output buffer\n", __FUNCTION__, __LINE__);
        return false;
    }
    if (outQueue.size() == DECODER_BUFFER_QUEUE_EMPTY_SIZE) {
        OMX_LOGW("%s, %d, There is no more  display buffer\n", __FUNCTION__, __LINE__);
        return false;
    }
    return true;
}

void SPRDAVCDecoderOhos::selectDrainQueue(std::list<BufferInfo*>& outQueue,
    std::list<BufferInfo *>::iterator &it, std::list<BufferInfo *>::iterator &itEnd)
{
    if (mDeintl) {
        it = mDecOutputBufQueue.begin();
        itEnd = mDecOutputBufQueue.end();
        return;
    }
    it = outQueue.begin();
    itEnd = outQueue.end();
}

bool SPRDAVCDecoderOhos::findDrainBuffer(std::list<BufferInfo *>::iterator &it,
    std::list<BufferInfo *>::iterator itEnd, void *bufferHeader) const
{
    while (it != itEnd && (*it) != nullptr && (*it)->mHeader != (OMX_BUFFERHEADERTYPE*)bufferHeader) {
        ++it;
    }
    if (it == itEnd || (*it) == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(it != itEnd && (*it) != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    if ((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)bufferHeader) {
        return true;
    }
    OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((*it)->mHeader == "
        "(OMX_BUFFERHEADERTYPE*)bufferHeader) failed.", __FUNCTION__, FILENAME_ONLY, __LINE__);
    return false;
}

void SPRDAVCDecoderOhos::prepareDrainHeader(BufferInfo *&outInfo, OMX_BUFFERHEADERTYPE *&outHeader,
    std::list<BufferInfo *>::iterator it, OMX_U64 pts, bool hasFrame)
{
    outInfo = *it;
    outHeader = outInfo->mHeader;
    if (hasFrame) {
        outHeader->nFilledLen = mPictureSize;
        outHeader->nTimeStamp = (OMX_TICKS)pts;
        return;
    }
    outHeader->nTimeStamp = ZERO_TIME_STAMP;
    outHeader->nFilledLen = BUFFER_SIZE_ZERO;
    outHeader->nFlags = BUFFER_FLAG_EOS;
    mEOSStatus = OUTPUT_FRAMES_FLUSHED;
}

void SPRDAVCDecoderOhos::dispatchDrainHeader(std::list<BufferInfo*>& outQueue,
    std::list<BufferInfo *>::iterator it, BufferInfo *outInfo, OMX_BUFFERHEADERTYPE *outHeader)
{
    if (mDeintl) {
        std::lock_guard<std::mutex> autoLock(mThreadLock);
        if (mDeinterInputBufQueue.size() == DECODER_BUFFER_QUEUE_EMPTY_SIZE) {
            OMX_LOGI("mDeinterInputBufQueue.size is 0");
            notifyFillBufferDone(outHeader);
            return;
        }
        mDeinterInputBufQueue.push_back(*it);
        mDecOutputBufQueue.erase(it);
        return;
    }
    outQueue.erase(it);
    outInfo->mOwnedByUs = false;
    BufferCtrlStruct* pOutBufCtrl = (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
    pOutBufCtrl->iRefCount++;
    notifyFillBufferDone(outHeader);
}

bool SPRDAVCDecoderOhos::DrainAllOutputBuffers()
{
    OMX_LOGD("%s, %d", __FUNCTION__, __LINE__);
    std::list<BufferInfo*>& outQueue = getPortQueue(K_OUTPUT_PORT_INDEX);
    while (mEOSStatus != OUTPUT_FRAMES_FLUSHED) {
        if (!drainOneOutputFrame(outQueue)) {
            return false;
        }
    }
    if (mDeintl) {
        SignalDeintlThread();
    }
    return true;
}

bool SPRDAVCDecoderOhos::drainOneOutputFrame(std::list<BufferInfo*>& outQueue)
{
    BufferInfo *outInfo = nullptr;
    std::list<BufferInfo *>::iterator it;
    std::list<BufferInfo *>::iterator itEnd;
    OMX_BUFFERHEADERTYPE *outHeader = nullptr;
    int32_t picId = 0;
    void *pBufferHeader = nullptr;
    uint64 pts = ZERO_TIME_STAMP;

    if (!validateDrainQueues(outQueue)) {
        return false;
    }
    selectDrainQueue(outQueue, it, itEnd);
    bool hasFrame = mHeadersDecoded && (*mH264DecGetLastDspFrm)(mHandle, &pBufferHeader, &picId, &pts) == MMDEC_OK;
    if (hasFrame && !findDrainBuffer(it, itEnd, pBufferHeader)) {
        return false;
    }
    prepareDrainHeader(outInfo, outHeader, it, pts, hasFrame);
    OMX_LOGI("%s, %d, outHeader: %p, outHeader->pBuffer: %p, outHeader->nOffset: %d, outHeader->nFlags: %d, "
        "outHeader->nTimeStamp: %llu, mEOSStatus: %d", __FUNCTION__, __LINE__, outHeader, outHeader->pBuffer,
        outHeader->nOffset, outHeader->nFlags, outHeader->nTimeStamp, mEOSStatus);
    dispatchDrainHeader(outQueue, it, outInfo, outHeader);
    return true;
}
OMX_ERRORTYPE SPRDAVCDecoderOhos::internalUseBuffer(
    OMX_BUFFERHEADERTYPE **header, const UseBufferParams &params)
{
    *header = new OMX_BUFFERHEADERTYPE;
    initOhosHeader(*header, params);
    if (params.portIndex == OMX_DirOutput) {
        OMX_ERRORTYPE err = initOhosOutputPrivate(header, params);
        if (err != OMX_ErrorNone) {
            return err;
        }
    } else if (params.portIndex == OMX_DirInput && mSecureFlag) {
        OMX_ERRORTYPE err = initOhosInputPrivate(*header, params.bufferPrivate);
        if (err != OMX_ErrorNone) {
            return err;
        }
    }
    addOhosBufferToPort(params, *header);
    return OMX_ErrorNone;
}

void SPRDAVCDecoderOhos::initOhosHeader(OMX_BUFFERHEADERTYPE *header, const UseBufferParams &params) const
{
    header->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    header->nVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    header->nVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    header->nVersion.s.nRevision = REVISION_NUMBER;
    header->nVersion.s.nStep = STEP_NUMBER;
    header->pBuffer = params.ptr;
    header->nAllocLen = params.size;
    header->nFilledLen = BUFFER_SIZE_ZERO;
    header->nOffset = BUFFER_HEADER_OFFSET_RESET;
    header->pAppPrivate = params.appPrivate;
    header->pPlatformPrivate = nullptr;
    header->pInputPortPrivate = nullptr;
    header->pOutputPortPrivate = nullptr;
    header->hMarkTargetComponent = nullptr;
    header->pMarkData = nullptr;
    header->nTickCount = ZERO_TICK_COUNT;
    header->nTimeStamp = ZERO_TIME_STAMP;
    header->nFlags = BUFFER_HEADER_FLAG_RESET;
    header->nOutputPortIndex = params.portIndex;
    header->nInputPortIndex = params.portIndex;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::initOhosOutputPrivate(
    OMX_BUFFERHEADERTYPE **header, const UseBufferParams &params)
{
    OMX_LOGD("OMX_DirOutput internalUseBuffer");
    (*header)->pOutputPortPrivate = new BufferCtrlStruct;
    if ((*header)->pOutputPortPrivate == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((*header)->pOutputPortPrivate != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    BufferCtrlStruct* pBufCtrl = (BufferCtrlStruct*)((*header)->pOutputPortPrivate);
    pBufCtrl->iRefCount = DEFAULT_INITIAL_REF_COUNT;
    pBufCtrl->id = mIOMMUID;
    if (mAllocateBuffers) {
        initDefaultOutputBufferCtrl(pBufCtrl, params.bufferPrivate);
        return OMX_ErrorNone;
    }
    if (!iUseOhosNativeBuffer[OMX_DirOutput]) {
        initDefaultOutputBufferCtrl(pBufCtrl, nullptr);
        return OMX_ErrorNone;
    }
    OMX_LOGD("Output port use OhosBufferHandle");
    OMX_ERRORTYPE err = useOhosBufferHandle(header, params.portIndex, params.appPrivate, params.size, params.ptr);
    if (err != OMX_ErrorNone) {
        OMX_LOGE("use ohos buffer handle fail.");
        return OMX_ErrorUnsupportedSetting;
    }
    return OMX_ErrorNone;
}

void SPRDAVCDecoderOhos::initDefaultOutputBufferCtrl(
    BufferCtrlStruct *bufferCtrl, BufferPrivateStruct *bufferPrivate) const
{
    if (bufferPrivate != nullptr) {
        bufferCtrl->pMem = bufferPrivate->pMem;
        bufferCtrl->phyAddr = bufferPrivate->phyAddr;
        bufferCtrl->bufferSize = bufferPrivate->bufferSize;
        bufferCtrl->bufferFd = bufferPrivate->bufferFd;
        return;
    }
    bufferCtrl->pMem = nullptr;
    bufferCtrl->phyAddr = MEMORY_ADDRESS_ZERO;
    bufferCtrl->bufferSize = BUFFER_SIZE_ZERO;
    bufferCtrl->bufferFd = BUFFER_SIZE_ZERO;
}

OMX_ERRORTYPE SPRDAVCDecoderOhos::initOhosInputPrivate(
    OMX_BUFFERHEADERTYPE *header, BufferPrivateStruct *bufferPrivate)
{
    OMX_LOGD("OMX_DirInput internalUseBuffer");
    header->pInputPortPrivate = new BufferCtrlStruct;
    if (header->pInputPortPrivate == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(header->pInputPortPrivate != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    BufferCtrlStruct* pBufCtrl = (BufferCtrlStruct*)(header->pInputPortPrivate);
    pBufCtrl->pMem = bufferPrivate != nullptr ? bufferPrivate->pMem : nullptr;
    pBufCtrl->phyAddr = bufferPrivate != nullptr ? bufferPrivate->phyAddr : MEMORY_ADDRESS_ZERO;
    pBufCtrl->bufferSize = bufferPrivate != nullptr ? bufferPrivate->bufferSize : BUFFER_SIZE_ZERO;
    pBufCtrl->bufferFd = BUFFER_SIZE_ZERO;
    return OMX_ErrorNone;
}

void SPRDAVCDecoderOhos::addOhosBufferToPort(const UseBufferParams &params, OMX_BUFFERHEADERTYPE *header)
{
    PortInfo *port = editPortInfo(params.portIndex);
    BufferInfo buffer;
    buffer.mHeader = header;
    buffer.mOwnedByUs = false;
    port->mBuffers.push_back(buffer);
    OMX_LOGI("internalUseBuffer, header=0x%p, pBuffer=0x%p, size=%d mBuffers.size=%zu",
        header, params.ptr, params.size, port->mBuffers.size());
    if (port->mBuffers.size() == port->mDef.nBufferCountActual + DEFAULT_PORT_BUFFER_COUNT_ADJUSTMENT) {
        port->mDef.bPopulated = OMX_TRUE;
        CheckTransitions();
    }
}
bool SPRDAVCDecoderOhos::SupportsDescribeColorAspects()
{
    OMX_LOGI("OHOS NOT IMPLEMENT SprdColorAspects");
    return false;
}
void SPRDAVCDecoderOhos::onBufferConfig(OMX_BUFFERHEADERTYPE *header)
{
    if (header == nullptr) {
        OMX_LOGE("buffer not initialized");
        return;
    } else {
        OMX_LOGD("onBufferConfig");
    }
}
}  // namespace OMX
}  // namespace OHOS
