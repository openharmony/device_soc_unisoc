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
#define LOG_TAG "SPRDHEVCDecoder"
#include <securec.h>
#include "utils/omx_log.h"
#include "SPRDHEVCDecoder.h"
#include "OMX_VideoExt.h"
#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#include <dlfcn.h>
#include "hevc_dec_api.h"

#include <utils/time_util.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include "display_type.h"
// == == == == == == == == == == = Magic Number Macros == == == == == == == == == == =
// Instance limits
/* Maximum number of decoder g_omxCoreMutex */
#define MAX_INSTANCES 20
/* Maximum number of secure decoder g_omxCoreMutex */
#define MAX_SECURE_INSTANCES 1
/* Memory alignment for start code */
#define START_CODE_LENGTH 4
/* Memory alignment boundary for 32 bytes */
#define ALIGN_32_BOUNDARY 31
/* Memory alignment boundary for 16 bytes */
#define ALIGN_16_BOUNDARY 15
/* Memory alignment boundary for 4KB */
#define ALIGN_4KB_BOUNDARY 4095
/* Memory alignment boundary for 1KB */
#define ALIGN_1KB_BOUNDARY 1023
/* Default frame width */
#define DEFAULT_FRAME_WIDTH 320
/* Default frame height */
#define DEFAULT_FRAME_HEIGHT 240
/* YUV420 frame size multiplier (width * height * 1.5) */
#define YUV420_MULTIPLIER 3
#define YUV420_DIVISOR 2
/* High 10-bit format multiplier */
#define HIGH10_MULTIPLIER 2
/* AFBC header size calculation divisor */
#define AFBC_HEADER_DIVISOR 16
/* Maximum supported frame depth */
#define MAX_FRAME_DEPTH 8
/* Additional buffer count for timeout avoidance */
#define ADDITIONAL_TIMEOUT_BUFFER 1
/* Additional buffer for reconstructed frame */
#define ADDITIONAL_RECONSTRUCTED_BUFFER 1
/* Reserved buffers by SurfaceFlinger */
#define SURFACE_FLINGER_RESERVED_BUFFERS 4
/* MB info buffer count */
#define MB_INFO_BUFFER_COUNT 17
/* Stream buffer additional padding */
#define STREAM_BUFFER_PADDING 4
/* H.265 decoder internal buffer size */
#define H265_DECODER_INTERNAL_BUFFER_SIZE (0x100000)
/* H.265 decoder stream buffer size */
#define H265_DECODER_STREAM_BUFFER_SIZE (1024*1024*3)
/* Start code pattern */
#define START_CODE_PREFIX 0x00000001
#define START_CODE_PREFIX_BYTE_0 0x00
#define START_CODE_PREFIX_BYTE_1 0x00
#define START_CODE_PREFIX_BYTE_2 0x00
#define START_CODE_PREFIX_BYTE_3 0x01
/* Common array indices */
#define ARRAY_INDEX_0 0
#define ARRAY_INDEX_1 1
#define ARRAY_INDEX_2 2
#define ARRAY_INDEX_3 3
#define ARRAY_INDEX_4 4
/* Input buffer size for 4K resolution */
#define INPUT_BUFFER_4K_SIZE 4096
#define INPUT_BUFFER_4K_HEIGHT 2160
/* Min compression ratio for buffer size calculation */
#define MIN_COMPRESSION_RATIO_DEFAULT 8
/* Decoder thread count */
#define DECODER_THREAD_COUNT 4
/* Codec priority default values */
#define CODEC_PRIORITY_DEFAULT (-1)
#define OPERATING_RATE_DEFAULT 0
/* Initial reference count */
#define INITIAL_REF_COUNT 1
/* Invalid physical address value */
#define INVALID_PHY_ADDR 0
/* Output port index */
#define OUTPUT_PORT_INDEX 1
/* Maximum port index */
#define MAX_PORT_INDEX 1
/* Initial picture ID */
#define INITIAL_PIC_ID 0
/* Zero value for various uses */
#define ZERO_VALUE 0
/* Minimum value for loop conditions */
#define MIN_LOOP_VALUE 0
/* CheckParam return value for unsupported index */
#define CHECK_PARAM_UNSUPPORTED_INDEX 1
/* Size of META_DATA_T structure */
#define META_DATA_T_SIZE sizeof(META_DATA_T)
// == == == == == == == == == == = End Magic Number Macros == == == == == == == == == == =
namespace OHOS {
namespace OMX {
template<typename T>
static bool LoadHevcDecoderSymbol(void *libHandle, const char *libName, const char *symbolName, T *symbol)
{
    *symbol = reinterpret_cast<T>(dlsym(libHandle, symbolName));
    if (*symbol != nullptr) {
        return true;
    }
    OMX_LOGE("Can't find %s in %s", symbolName, libName);
    return false;
}

static bool CloseHevcDecoderLibraryOnFailure(void **libHandle)
{
    dlclose(*libHandle);
    *libHandle = nullptr;
    return false;
}

static int g_omxCoreMutex = 0;
static const char *MEDIA_MIMETYPE_VIDEO_HEVC = "video/hevc";
static const CodecProfileLevel kProfileLevels[] = {
    { OMX_VIDEO_HEVC_PROFILE_MAIN, OMX_VIDEO_HEVC_MAIN_TIER_LEVEL1  },
    { OMX_VIDEO_HEVC_PROFILE_MAIN, OMX_VIDEO_HEVC_MAIN_TIER_LEVEL2  },
    { OMX_VIDEO_HEVC_PROFILE_MAIN, OMX_VIDEO_HEVC_MAIN_TIER_LEVEL21 },
    { OMX_VIDEO_HEVC_PROFILE_MAIN, OMX_VIDEO_HEVC_MAIN_TIER_LEVEL3  },
    { OMX_VIDEO_HEVC_PROFILE_MAIN, OMX_VIDEO_HEVC_MAIN_TIER_LEVEL31 },
    { OMX_VIDEO_HEVC_PROFILE_MAIN, OMX_VIDEO_HEVC_MAIN_TIER_LEVEL4  },
    { OMX_VIDEO_HEVC_PROFILE_MAIN, OMX_VIDEO_HEVC_MAIN_TIER_LEVEL41 },
    { OMX_VIDEO_HEVC_PROFILE_MAIN, OMX_VIDEO_HEVC_MAIN_TIER_LEVEL5  },
    { OMX_VIDEO_HEVC_PROFILE_MAIN, OMX_VIDEO_HEVC_MAIN_TIER_LEVEL51 },
};
OMX_ERRORTYPE SPRDHEVCDecoder::allocateStreamBuffer(OMX_U32 bufferSize)
{
    unsigned long phyAddr = 0;
    size_t size = 0;
    OMX_LOGD("SPRDHEVCDecoder::allocateStreamBuffer, bufferSize = %u", bufferSize);
    if (bufferSize == 0 || bufferSize > UINT32_MAX) {
        OMX_LOGE("allocateStreamBuffer invalid bufferSize: %u", bufferSize);
        return OMX_ErrorBadParameter;
    }
    if (mDecoderSwFlag) {
        mPbufStreamV = static_cast<uint8_t*>(malloc(bufferSize * sizeof(unsigned char)));
        mPbufStreamP = 0;
        mPbufStreamSize = bufferSize;
    } else {
        mPmemStream = new VideoMemAllocator(bufferSize, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
        int fd = mPmemStream->getHeapID();
        if (fd < 0) {
            OMX_LOGE("allocateStreamBuffer Failed to alloc bitstream pmem buffer, getHeapID failed");
            return OMX_ErrorInsufficientResources;
        } else {
            int ret;
            if (mIOMMUEnabled) {
                ret = (*mH265DecGetIOVA)(mHandle, fd, &phyAddr, &size, true);
            } else {
                ret = mPmemStream->getPhyAddrFromMemAlloc(&phyAddr, &size);
            }
            if (ret < 0) {
                OMX_LOGE("allocateStreamBuffer Failed to alloc bitstream pmem buffer, get phy addr failed");
                return OMX_ErrorInsufficientResources;
            } else {
                mPbufStreamV = static_cast<uint8_t*>(mPmemStream->getBase());
                mPbufStreamP = phyAddr;
                mPbufStreamSize = size;
            }
        }
    }
    OMX_LOGI("%s, 0x%lx - %p - %zd", __FUNCTION__, mPbufStreamP,
        mPbufStreamV, mPbufStreamSize);
    return OMX_ErrorNone;
}
void SPRDHEVCDecoder::ReleaseStreamBuffer()
{
    OMX_LOGI("%s, 0x%lx - %p - %zd", __FUNCTION__, mPbufStreamP,
        mPbufStreamV, mPbufStreamSize);
    if (mPbufStreamV != nullptr) {
        if (mDecoderSwFlag) {
            free(mPbufStreamV);
            mPbufStreamV = nullptr;
        } else {
            if (mIOMMUEnabled) {
                (*mH265DecFreeIOVA)(mHandle, mPbufStreamP, mPbufStreamSize, true);
            }
            releaseMemIon(&mPmemStream);
            mPbufStreamV = nullptr;
            mPbufStreamP = 0;
            mPbufStreamSize = 0;
        }
    }
}
bool SPRDHEVCDecoder::outputBuffersNotEnough(const H265SwDecInfo *info, OMX_U32 bufferCountMin,
                                             OMX_U32 bufferCountActual, OMX_BOOL useNativeBuffer)
{
    if (useNativeBuffer) {
        if (info->numFrames + ADDITIONAL_TIMEOUT_BUFFER > bufferCountMin) {
            return true;
        }
    } else {
        int totalRequired = info->numFrames + ADDITIONAL_TIMEOUT_BUFFER +
            ADDITIONAL_RECONSTRUCTED_BUFFER + SURFACE_FLINGER_RESERVED_BUFFERS;
        if ((OMX_U32)totalRequired > bufferCountActual) {
            return true;
        }
    }
    return false;
}
SPRDHEVCDecoder::SPRDHEVCDecoder(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdVideoDecoderBase({
        name,
        "video_decoder.hevc",
        (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingHEVC,
        kProfileLevels,
        (sizeof(kProfileLevels) / sizeof((kProfileLevels)[0])),
        callbacks,
        appData,
        component}),
      mHandle(new TagHevcHandle)
{
    OMX_LOGI("Construct SPRDHEVCDecoder, this: %p, g_omxCoreMutex: %d", static_cast<void *>(this), g_omxCoreMutex);
    InitSecureMode(name);
    InitPortsAndStatus();
    g_omxCoreMutex++;
    if (g_omxCoreMutex > (mSecureFlag ? MAX_SECURE_INSTANCES : MAX_INSTANCES)) {
        OMX_LOGE("g_omxCoreMutex(%d) are too much, return OMX_ErrorInsufficientResources", g_omxCoreMutex);
        mInitCheck = OMX_ErrorInsufficientResources;
        if (mHandle != nullptr) {
            delete mHandle;
            mHandle = nullptr;
        }
        return;
    }
    if (!OpenAndInitDecoder()) {
        mInitCheck = OMX_ErrorInsufficientResources;
    }
    mDumpYUVEnabled = GetDumpValue("vendor.h265dec.yuv.dump");
    mDumpStrmEnabled = GetDumpValue("vendor.h265dec.strm.dump");
    OMX_LOGI("%s, mDumpYUVEnabled: %d, mDumpStrmEnabled: %d", __FUNCTION__, mDumpYUVEnabled, mDumpStrmEnabled);
    InitMbInfoState();
    InitDumpFiles(name, "h265");
}

void SPRDHEVCDecoder::InitSecureMode(const char *name)
{
    size_t nameLen = strlen(name);
    size_t suffixLen = strlen(".secure");
    if (!strcmp(name + nameLen - suffixLen, ".secure")) {
        mSecureFlag = true;
    }
}

void SPRDHEVCDecoder::InitPortsAndStatus() const
{
    SPRDHEVCDecoder *self = const_cast<SPRDHEVCDecoder *>(this);
    self->mInitCheck = OMX_ErrorNone;
    size_t inputBufferSize = INPUT_BUFFER_4K_SIZE * INPUT_BUFFER_4K_HEIGHT *
        YUV420_MULTIPLIER / YUV420_DIVISOR / MAX_FRAME_DEPTH;
    PortConfig portConfig = {
        K_NUM_INPUT_BUFFERS,
        K_NUM_INPUT_BUFFERS,
        static_cast<OMX_U32>(inputBufferSize),
        K_NUM_OUTPUT_BUFFERS,
        K_NUM_OUTPUT_BUFFERS,
        MEDIA_MIMETYPE_VIDEO_HEVC,
        mFrameWidth,
        mFrameHeight,
        MAX_FRAME_DEPTH,
    };
    self->InitPorts(portConfig);
}

bool SPRDHEVCDecoder::OpenAndInitDecoder()
{
    if (!OpenDecoder(mSecureFlag ? "libomx_hevcdec_hw_sprd_secure.so" : "libomx_hevcdec_hw_sprd.z.so")) {
        return false;
    }
    return initDecoder() == OMX_ErrorNone;
}

void SPRDHEVCDecoder::InitMbInfoState()
{
    for (int i = 0; i < MB_INFO_BUFFER_COUNT; ++i) {
        mPmemMbInfo[i] = nullptr;
        mPmemMbInfoV[i] = nullptr;
        mPmemMbInfoP[i] = 0;
        mPbufMbInfoSize[i] = 0;
    }
}
SPRDHEVCDecoder::~SPRDHEVCDecoder()
{
    OMX_LOGD("Destruct SPRDHEVCDecoder, this: %p, g_omxCoreMutex: %d", static_cast<void *>(this), g_omxCoreMutex);
    ReleaseDecoder();
    if (mHandle != nullptr) {
        delete mHandle;
        mHandle = nullptr;
    }
    if (mPorts.size() <= K_OUTPUT_PORT_INDEX) {
        OMX_LOGW("skip queue validation because ports are not initialized, size=%zu", mPorts.size());
        CloseDumpFiles();
        g_omxCoreMutex--;
        return;
    }
    std::list<BufferInfo*>& outQueue = getPortQueue(K_OUTPUT_PORT_INDEX);
    std::list<BufferInfo*>& inQueue = getPortQueue(K_INPUT_PORT_INDEX);
    if (!outQueue.empty()) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(outQueue.empty() failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return;
    };
    if (!inQueue.empty()) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(inQueue.empty() failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return;
    };
    CloseDumpFiles();
    g_omxCoreMutex--;
}
OMX_ERRORTYPE SPRDHEVCDecoder::initCheck() const
{
    OMX_LOGI("%s, mInitCheck: 0x%x", __FUNCTION__, mInitCheck);
    return mInitCheck;
}
status_t SPRDHEVCDecoder::initDecoder()
{
    InitDecoderHandle();
    uint32_t sizeInter = H265_DECODER_INTERNAL_BUFFER_SIZE;
    mCodecInterBuffer = reinterpret_cast<uint8_t*>(malloc(sizeInter));
    if (mCodecInterBuffer == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(mCodecInterBuffer != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    OMX_ERRORTYPE err = initDecoderInstance();
    if (err != OMX_ErrorNone) {
        return err;
    }
    UpdateIommuStatus();
    return allocateStreamBuffer(H265_DECODER_STREAM_BUFFER_SIZE);
}

void SPRDHEVCDecoder::InitDecoderHandle()
{
    (void)memset_s(mHandle, sizeof(TagHevcHandle), 0, sizeof(TagHevcHandle));
    mHandle->userdata = static_cast<void *>(this);
    mHandle->vspBindcb = BindFrameWrapper;
    mHandle->vspUnbindcb = UnbindFrameWrapper;
    mHandle->vspExtmemcb = ExtMemAllocWrapper;
    mHandle->vspCtuinfomemcb = CtuInfoMemAllocWrapper;
}

OMX_ERRORTYPE SPRDHEVCDecoder::initDecoderInstance()
{
    uint32_t sizeInter = H265_DECODER_INTERNAL_BUFFER_SIZE;
    MMCodecBuffer codecBuf;
    MMDecVideoFormat videoFormat;
    MMDecCodecPriority codecPriority;
    codecBuf.commonBufferPtr = mCodecInterBuffer;
    codecBuf.commonBufferPtrPhy = 0;
    codecBuf.size = sizeInter;
    codecBuf.intBufferPtr = nullptr;
    codecBuf.intSize = 0;
    videoFormat.videoStd = H265;
    videoFormat.frameWidth = 0;
    videoFormat.frameHeight = 0;
    videoFormat.pExtra = nullptr;
    videoFormat.pExtraPhy = 0;
    videoFormat.iExtra = 0;
    videoFormat.yuvFormat = YUV420SP_NV12;
    videoFormat.useThreadCnt = DECODER_THREAD_COUNT;
    codecPriority.priority = CODEC_PRIORITY_DEFAULT;
    codecPriority.operatingRate = OPERATING_RATE_DEFAULT;
    if ((*mH265DecInit)(mHandle, &codecBuf, &videoFormat, &codecPriority) != MMDEC_OK) {
        OMX_LOGE("SPRDHEVCDecoder::initDecoder Failed to init HEVCDEC");
        return OMX_ErrorUndefined;
    }
    if ((*mH265GetCodecCapability)(mHandle, &mCapability) != MMDEC_OK) {
        OMX_LOGE("SPRDHEVCDecoder::initDecoder Failed to mH265GetCodecCapability");
    }
    return OMX_ErrorNone;
}

void SPRDHEVCDecoder::UpdateIommuStatus() const
{
    int ret = (*mH265DecGetIOMMUStatus)(mHandle);
    OMX_LOGD("SPRDHEVCDecoder::initDecoder Get IOMMU Status: %d(%s)",
        ret, strerror(errno));
    OMX_LOGD("mSecureFlag: %d", mSecureFlag);
    if (ret < 0) {
        mIOMMUEnabled = false;
    } else {
        mIOMMUEnabled = mSecureFlag ? false : true;
    }
}

void SPRDHEVCDecoder::ReleaseExtraBuffer()
{
    if (mPbufExtraV == nullptr) {
        return;
    }
    if (mIOMMUEnabled) {
        (*mH265DecFreeIOVA)(mHandle, mPbufExtraP, mPbufExtraSize, true);
    }
    releaseMemIon(&mPmemExtra);
    mPbufExtraV = nullptr;
    mPbufExtraP = 0;
    mPbufExtraSize = 0;
}

void SPRDHEVCDecoder::ReleaseMbInfoBuffers()
{
    for (int i = 0; i < MB_INFO_BUFFER_COUNT; i++) {
        if (!mPmemMbInfoV[i]) {
            continue;
        }
        if (mIOMMUEnabled) {
            (*mH265DecFreeIOVA)(mHandle, mPmemMbInfoP[i], mPbufMbInfoSize[i], true);
        }
        releaseMemIon(&mPmemMbInfo[i]);
        mPmemMbInfoV[i] = nullptr;
        mPmemMbInfoP[i] = 0;
        mPbufMbInfoSize[i] = 0;
    }
    mPbufMbInfoIdx = 0;
}

void SPRDHEVCDecoder::ReleaseCodecResources()
{
    if (mH265DecRelease != nullptr) {
        (*mH265DecRelease)(mHandle);
    }
    if (mCodecInterBuffer != nullptr) {
        free(mCodecInterBuffer);
        mCodecInterBuffer = nullptr;
    }
    if (mCodecExtraBuffer != nullptr) {
        free(mCodecExtraBuffer);
        mCodecExtraBuffer = nullptr;
    }
    if (mLibHandle == nullptr) {
        return;
    }
    dlclose(mLibHandle);
    mLibHandle = nullptr;
    mH265DecReleaseRefBuffers = nullptr;
    mH265DecRelease = nullptr;
}

void SPRDHEVCDecoder::ReleaseDecoder()
{
    ReleaseStreamBuffer();
    ReleaseExtraBuffer();
    ReleaseMbInfoBuffers();
    ReleaseCodecResources();
}

OMX_ERRORTYPE SPRDHEVCDecoder::getSupportBufferType(UseBufferType *defParams)
{
    if (defParams->portIndex > MAX_PORT_INDEX) {
        OMX_LOGE("port index error.");
        return OMX_ErrorUnsupportedIndex;
    }
    defParams->bufferType = CODEC_BUFFER_TYPE_AVSHARE_MEM_FD;
    if (defParams->portIndex == K_OUTPUT_PORT_INDEX) {
        defParams->bufferType = CODEC_BUFFER_TYPE_HANDLE;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCDecoder::getUseBufferType(UseBufferType *defParams)
{
    if (defParams->portIndex > MAX_PORT_INDEX) {
        return OMX_ErrorBadPortIndex;
    }
    defParams->bufferType = iUseOhosNativeBuffer[defParams->portIndex];
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCDecoder::getBufferHandleUsage(GetBufferHandleUsageParams *defParams)
{
    if (defParams->size != sizeof(struct GetBufferHandleUsageParams)) {
        OMX_LOGE("param size check fail.");
        return OMX_ErrorBadParameter;
    }
    defParams->usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCDecoder::getPortDefinitionParam(OMX_PARAM_PORTDEFINITIONTYPE *defParams)
{
    if (defParams->nPortIndex > MAX_PORT_INDEX || defParams->nSize != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
        return OMX_ErrorUndefined;
    }
    PortInfo *port = editPortInfo(defParams->nPortIndex);
    AutoMutex autoLock(mLock);
    errno_t ret = memmove_s(defParams, sizeof(port->mDef), &port->mDef, sizeof(port->mDef));
    if (ret != 0) {
        OMX_LOGE("memmove_s failed at line %d, ret=%d", __LINE__, ret);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCDecoder::getCodecVideoPortFormat(CodecVideoPortFormatParam *formatParams)
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

OMX_ERRORTYPE SPRDHEVCDecoder::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_IndexParamSupportBufferType: {
            return getSupportBufferType((struct UseBufferType *)params);
        }
        case OMX_IndexParamUseBufferType: {
            return getUseBufferType((struct UseBufferType *)params);
        }
        case OMX_IndexParamGetBufferHandleUsage: {
            return getBufferHandleUsage((struct GetBufferHandleUsageParams *)params);
        }
        case OMX_IndexParamPortDefinition: {
            return getPortDefinitionParam((OMX_PARAM_PORTDEFINITIONTYPE *)params);
        }
        case OMX_IndexCodecVideoPortFormat: {
            return getCodecVideoPortFormat((CodecVideoPortFormatParam *)params);
        }
        default:
            return SprdVideoDecoderBase::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDHEVCDecoder::setUseBufferType(const UseBufferType *defParams)
{
    if (defParams->portIndex > MAX_PORT_INDEX) {
        return OMX_ErrorBadPortIndex;
    }
    iUseOhosNativeBuffer[defParams->portIndex] = OMX_TRUE;
    OMX_LOGI("OMX_IndexParamUseBufferType : %x, index: %d, bufferType: %d, "
        "iUseOhosNativeBuffer[defParams->portIndex]:%d",
        OMX_IndexParamUseBufferType, defParams->portIndex, defParams->bufferType,
        iUseOhosNativeBuffer[defParams->portIndex]);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCDecoder::setProcessName(const ProcessNameParam *nameParams)
{
    OMX_LOGI("OMX_IndexParamProcessName : %s", nameParams->processName);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCDecoder::setPortDefinitionParam(const OMX_PARAM_PORTDEFINITIONTYPE *defParams)
{
    if (defParams->nPortIndex > MAX_PORT_INDEX) {
        return OMX_ErrorBadPortIndex;
    }
    if (defParams->nSize != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
        return OMX_ErrorUnsupportedSetting;
    }
    PortInfo *port = editPortInfo(defParams->nPortIndex);
    if (defParams->nBufferSize > port->mDef.nBufferSize) {
        port->mDef.nBufferSize = defParams->nBufferSize;
    }
    return updatePortDefinition(const_cast<OMX_PARAM_PORTDEFINITIONTYPE *>(defParams), port);
}

OMX_ERRORTYPE SPRDHEVCDecoder::setCodecVideoPortFormat(const CodecVideoPortFormatParam *formatParams)
{
    if (CheckParam(const_cast<CodecVideoPortFormatParam *>(formatParams), sizeof(CodecVideoPortFormatParam))) {
        return OMX_ErrorUnsupportedIndex;
    }
    PortInfo *port = editPortInfo(formatParams->portIndex);
    port->mDef.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)formatParams->codecColorFormat;
    port->mDef.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)formatParams->codecCompressFormat;
    port->mDef.format.video.xFramerate = (OMX_U32)formatParams->framerate;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCDecoder::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_IndexParamUseBufferType: {
            return setUseBufferType((const UseBufferType *)params);
        }
        case OMX_IndexParamProcessName: {
            return setProcessName((const ProcessNameParam *)params);
        }
        case OMX_IndexParamPortDefinition: {
            return setPortDefinitionParam((const OMX_PARAM_PORTDEFINITIONTYPE *)params);
        }
        case OMX_IndexCodecVideoPortFormat: {
            return setCodecVideoPortFormat((const CodecVideoPortFormatParam *)params);
        }
        default:
            return SprdVideoDecoderBase::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDHEVCDecoder::updatePortDefinition(OMX_PARAM_PORTDEFINITIONTYPE *defParams, PortInfo *port)
{
    if (defParams->nBufferCountActual < port->mDef.nBufferCountMin) {
        OMX_LOGW("component requires at least %u buffers (%u requested)",
            port->mDef.nBufferCountMin, defParams->nBufferCountActual);
        return OMX_ErrorUnsupportedSetting;
    }
    if (port->mDef.nBufferCountActual < defParams->nBufferCountActual) {
        port->mDef.nBufferCountActual = defParams->nBufferCountActual;
    }
    uint32_t oldWidth = port->mDef.format.video.nFrameWidth;
    uint32_t oldHeight = port->mDef.format.video.nFrameHeight;
    uint32_t newWidth = defParams->format.video.nFrameWidth;
    uint32_t newHeight = defParams->format.video.nFrameHeight;
    OMX_LOGI("%s, port:%d, old wh:%d %d, new wh:%d %d", __FUNCTION__, defParams->nPortIndex,
        oldWidth, oldHeight, newWidth, newHeight);
    if (!((newWidth <= mCapability.maxWidth && newHeight <= mCapability.maxHeight) ||
        (newWidth <= mCapability.maxHeight && newHeight <= mCapability.maxWidth))) {
        OMX_LOGE("%s %d, [%d, %d] is out of range [%d, %d], failed to support this format.",
            __FUNCTION__, __LINE__, newWidth, newHeight, mCapability.maxWidth, mCapability.maxHeight);
        return OMX_ErrorUnsupportedSetting;
    }
    errno_t ret = memmove_s(&port->mDef.format.video, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE),
        &defParams->format.video, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
    if (ret != 0) {
        OMX_LOGE("memmove_s failed at line %d, ret=%d", __LINE__, ret);
    }
    if (oldWidth != newWidth || oldHeight != newHeight) {
        applyPortGeometryChange(defParams->nPortIndex, newWidth, newHeight);
    }
    EnsureStreamBufferCapacity();
    return OMX_ErrorNone;
}

void SPRDHEVCDecoder::applyPortGeometryChange(OMX_U32 portIndex, uint32_t newWidth, uint32_t newHeight)
{
    if (portIndex != K_OUTPUT_PORT_INDEX) {
        PortInfo *port = editPortInfo(portIndex);
        port->mDef.format.video.nFrameWidth = newWidth;
        port->mDef.format.video.nFrameHeight = newHeight;
        return;
    }
    mFrameWidth = newWidth;
    mFrameHeight = newHeight;
    mStride = ((newWidth + ALIGN_32_BOUNDARY) & ~ALIGN_32_BOUNDARY);
    mSliceHeight = ((newHeight + ALIGN_32_BOUNDARY) & ~ALIGN_32_BOUNDARY);
    mPictureSize = mStride * mSliceHeight * YUV420_MULTIPLIER / YUV420_DIVISOR;
    if (mFbcMode == AFBC) {
        int headerSize = (mStride * mSliceHeight / AFBC_HEADER_DIVISOR + ALIGN_1KB_BOUNDARY) &
            (~ALIGN_1KB_BOUNDARY);
        mPictureSize += headerSize;
    }
    OMX_LOGI("%s, mFrameWidth %d, mFrameHeight %d, mStride %d, mSliceHeight %d", __FUNCTION__,
        mFrameWidth, mFrameHeight, mStride, mSliceHeight);
    UpdatePortDefinitions(true, true);
}

void SPRDHEVCDecoder::EnsureStreamBufferCapacity()
{
    PortInfo *inPort = editPortInfo(K_INPUT_PORT_INDEX);
    if (inPort->mDef.nBufferSize > mPbufStreamSize) {
        ReleaseStreamBuffer();
        allocateStreamBuffer(inPort->mDef.nBufferSize + STREAM_BUFFER_PADDING);
    }
}
OMX_ERRORTYPE SPRDHEVCDecoder::internalUseBuffer(
    OMX_BUFFERHEADERTYPE **header, const UseBufferParams &params)
{
    *header = new OMX_BUFFERHEADERTYPE;
    initOmxHeader(*header, params.portIndex, params.appPrivate, params.size, params.ptr);
    if (params.portIndex == OMX_DirOutput) {
        OMX_ERRORTYPE err = initOutputPrivate(*header, params.ptr, params.bufferPrivate);
        if (err != OMX_ErrorNone) {
            return err;
        }
    } else if (params.portIndex == OMX_DirInput) {
        if (mSecureFlag) {
            OMX_ERRORTYPE err = initSecureInputPrivate(*header, params.bufferPrivate);
            if (err != OMX_ErrorNone) {
                return err;
            }
        }
    }
    addBufferToPort(params.portIndex, *header, params.ptr, params.size);
    return OMX_ErrorNone;
}

void SPRDHEVCDecoder::initOmxHeader(
    OMX_BUFFERHEADERTYPE *header, OMX_U32 portIndex, OMX_PTR appPrivate, OMX_U32 size, OMX_U8 *ptr)
{
    header->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    header->nVersion.s.nVersionMajor = 1;
    header->nVersion.s.nVersionMinor = 0;
    header->nVersion.s.nRevision = 0;
    header->nVersion.s.nStep = 0;
    header->pBuffer = ptr;
    header->nAllocLen = size;
    header->nFilledLen = 0;
    header->nOffset = 0;
    header->pAppPrivate = appPrivate;
    header->pPlatformPrivate = nullptr;
    header->pInputPortPrivate = nullptr;
    header->pOutputPortPrivate = nullptr;
    header->hMarkTargetComponent = nullptr;
    header->pMarkData = nullptr;
    header->nTickCount = 0;
    header->nTimeStamp = 0;
    header->nFlags = 0;
    header->nOutputPortIndex = portIndex;
    header->nInputPortIndex = portIndex;
}

OMX_ERRORTYPE SPRDHEVCDecoder::initOutputPrivate(
    OMX_BUFFERHEADERTYPE *header, OMX_U8 *ptr, BufferPrivateStruct *bufferPrivate)
{
    header->pOutputPortPrivate = new BufferCtrlStruct;
    if (header->pOutputPortPrivate == nullptr) {
        return OMX_ErrorUndefined;
    }
    BufferCtrlStruct* pBufCtrl = reinterpret_cast<BufferCtrlStruct*>(header->pOutputPortPrivate);
    pBufCtrl->iRefCount = INITIAL_REF_COUNT;
    pBufCtrl->id = mIOMMUID;
    mStride16 = ((mFrameWidth + ALIGN_16_BOUNDARY) & ~ALIGN_16_BOUNDARY);
    initAllocatedOutputPrivate(pBufCtrl, nullptr);
    if (mAllocateBuffers) {
        initAllocatedOutputPrivate(pBufCtrl, bufferPrivate);
        return OMX_ErrorNone;
    }
    return initNativeOutputPrivate(header, ptr, pBufCtrl);
}

void SPRDHEVCDecoder::initAllocatedOutputPrivate(BufferCtrlStruct *bufferCtrl, BufferPrivateStruct *bufferPrivate)
{
    bufferCtrl->pMem = nullptr;
    bufferCtrl->bufferFd = 0;
    bufferCtrl->phyAddr = 0;
    bufferCtrl->bufferSize = 0;
    if (bufferPrivate != nullptr) {
        bufferCtrl->pMem = bufferPrivate->pMem;
        bufferCtrl->bufferFd = bufferPrivate->bufferFd;
        bufferCtrl->phyAddr = bufferPrivate->phyAddr;
        bufferCtrl->bufferSize = bufferPrivate->bufferSize;
    }
}

OMX_ERRORTYPE SPRDHEVCDecoder::initNativeOutputPrivate(
    OMX_BUFFERHEADERTYPE *header, OMX_U8 *ptr, BufferCtrlStruct *bufferCtrl)
{
    BufferHandle* bufferHandle = reinterpret_cast<BufferHandle*>(ptr);
    if (bufferHandle->fd < 0) {
        return OMX_ErrorInsufficientResources;
    }
    header->pPlatformPrivate = mmap(0, bufferHandle->size, PROT_READ | PROT_WRITE, MAP_SHARED, bufferHandle->fd, 0);
    VideoMemAllocator *vm = new VideoMemAllocator(mPictureSize, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    int fd = vm->getHeapID();
    size_t bufferSize = 0;
    if (fd < 0) {
        return OMX_ErrorInsufficientResources;
    }
    int ret = mIOMMUEnabled ? (*mH265DecGetIOVA)(mHandle, fd, &bufferCtrl->phyAddr, &bufferSize, true) :
        VideoMemAllocator::GetPhyAddrFromMemAlloc(fd, &bufferCtrl->phyAddr, &bufferSize);
    if (ret != 0) {
        return OMX_ErrorInsufficientResources;
    }
    bufferCtrl->pMem = vm;
    bufferCtrl->bufferFd = fd;
    bufferCtrl->bufferSize = bufferHandle->size;
    header->pBuffer = reinterpret_cast<OMX_U8 *>(vm->getBase());
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCDecoder::initSecureInputPrivate(
    OMX_BUFFERHEADERTYPE *header, BufferPrivateStruct *bufferPrivate)
{
    header->pInputPortPrivate = new BufferCtrlStruct;
    if (header->pInputPortPrivate == nullptr) {
        return OMX_ErrorUndefined;
    }
    BufferCtrlStruct* pBufCtrl = reinterpret_cast<BufferCtrlStruct*>(header->pInputPortPrivate);
    pBufCtrl->pMem = nullptr;
    pBufCtrl->bufferFd = 0;
    pBufCtrl->phyAddr = 0;
    pBufCtrl->bufferSize = 0;
    if (bufferPrivate != nullptr) {
        pBufCtrl->pMem = bufferPrivate->pMem;
        pBufCtrl->bufferFd = bufferPrivate->bufferFd;
        pBufCtrl->phyAddr = bufferPrivate->phyAddr;
        pBufCtrl->bufferSize = bufferPrivate->bufferSize;
    }
    return OMX_ErrorNone;
}

void SPRDHEVCDecoder::addBufferToPort(OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header, OMX_U8 *ptr, OMX_U32 size)
{
    PortInfo *port = editPortInfo(portIndex);
    port->mBuffers.push_back(BufferInfo());
    BufferInfo *buffer = &port->mBuffers.at(port->mBuffers.size() - 1);
    OMX_LOGI("internalUseBuffer, header=%p, pBuffer=%p, size=%d", header, ptr, size);
    buffer->mHeader = header;
    buffer->mOwnedByUs = false;
    if (port->mBuffers.size() == port->mDef.nBufferCountActual) {
        port->mDef.bPopulated = OMX_TRUE;
        CheckTransitions();
    }
}
OMX_ERRORTYPE SPRDHEVCDecoder::allocateBuffer(
    OMX_BUFFERHEADERTYPE **header,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    OMX_U32 size)
{
    switch (portIndex) {
        case OMX_DirInput: {
            return mSecureFlag ? allocateSecureInputBuffer(header, appPrivate, size) :
                SprdSimpleOMXComponent::allocateBuffer(header, portIndex, appPrivate, size);
        }
        case OMX_DirOutput: {
            return allocateOutputBufferInternal(header, appPrivate, size);
        }
        default:
            return OMX_ErrorUnsupportedIndex;
    }
}

OMX_ERRORTYPE SPRDHEVCDecoder::allocateSecureInputBuffer(
    OMX_BUFFERHEADERTYPE **header, OMX_PTR appPrivate, OMX_U32 size)
{
    size_t alignedSize = (size + ALIGN_4KB_BOUNDARY) & ~ALIGN_4KB_BOUNDARY;
    unsigned long phyAddr = 0;
    size_t bufferSize = 0;
    MemIon* pMem = new VideoMemAllocator(alignedSize, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    int fd = pMem->getHeapID();
    if (fd < 0 || pMem->getPhyAddrFromMemAlloc(&phyAddr, &bufferSize)) {
        return OMX_ErrorInsufficientResources;
    }
    BufferPrivateStruct* bufferPrivate = new BufferPrivateStruct();
    bufferPrivate->pMem = pMem;
    bufferPrivate->bufferFd = fd;
    bufferPrivate->phyAddr = phyAddr;
    bufferPrivate->bufferSize = bufferSize;
    UseBufferParams params = {
        OMX_DirInput,
        appPrivate,
        static_cast<OMX_U32>(bufferSize),
        reinterpret_cast<OMX_U8 *>(phyAddr),
        bufferPrivate,
    };
    SprdSimpleOMXComponent::useBuffer(header, params);
    delete bufferPrivate;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCDecoder::allocateOutputBufferInternal(
    OMX_BUFFERHEADERTYPE **header, OMX_PTR appPrivate, OMX_U32 size)
{
    mAllocateBuffers = true;
    if (mDecoderSwFlag || mChangeToSwDec) {
        return SprdSimpleOMXComponent::allocateBuffer(header, OMX_DirOutput, appPrivate, size);
    }
    size_t alignedSize = (size + ALIGN_4KB_BOUNDARY) & ~ALIGN_4KB_BOUNDARY;
    unsigned long phyAddr = 0;
    size_t bufferSize = 0;
    MemIon* pMem = new VideoMemAllocator(alignedSize, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    int fd = pMem->getHeapID();
    if (fd < 0) {
        return OMX_ErrorInsufficientResources;
    }
    int ret = mIOMMUEnabled ? (*mH265DecGetIOVA)(mHandle, fd, &phyAddr, &bufferSize, true) :
        pMem->getPhyAddrFromMemAlloc(&phyAddr, &bufferSize);
    if (ret != 0) {
        return OMX_ErrorInsufficientResources;
    }
    BufferPrivateStruct* bufferPrivate = new BufferPrivateStruct();
    bufferPrivate->pMem = pMem;
    bufferPrivate->bufferFd = fd;
    bufferPrivate->phyAddr = phyAddr;
    bufferPrivate->bufferSize = bufferSize;
    UseBufferParams params = {
        OMX_DirOutput,
        appPrivate,
        static_cast<OMX_U32>(alignedSize),
        reinterpret_cast<OMX_U8 *>(pMem->getBase()),
        bufferPrivate,
    };
    SprdSimpleOMXComponent::useBuffer(header, params);
    delete bufferPrivate;
    return OMX_ErrorNone;
}
void SPRDHEVCDecoder::freePlatform(OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header)
{
    OMX_LOGI("SPRDHEVCDecoder::freePlatform");
    PortInfo *port = &mPorts.at(portIndex);
    BufferCtrlStruct* pBufCtrl = (BufferCtrlStruct*)(header->pOutputPortPrivate);
    for (size_t i = 0; i < port->mBuffers.size(); ++i) {
        BufferInfo *buffer = &port->mBuffers.at(i);
        if (buffer->mHeader == header) {
            releasePlatformBuffer(header, pBufCtrl);
            break;
        }
    }
}

void SPRDHEVCDecoder::releasePlatformBuffer(OMX_BUFFERHEADERTYPE *header, BufferCtrlStruct *bufferCtrl) const
{
    if (header->pPlatformPrivate == nullptr || bufferCtrl == nullptr) {
        return;
    }
    if (munmap(header->pPlatformPrivate, bufferCtrl->bufferSize) != 0) {
        OMX_LOGE("Unmap failed");
        return;
    }
    header->pPlatformPrivate = nullptr;
    OMX_LOGI("header->pPlatformPrivate munmap success");
}
OMX_ERRORTYPE SPRDHEVCDecoder::freeBuffer(
    OMX_U32 portIndex,
    OMX_BUFFERHEADERTYPE *header)
{
    switch (portIndex) {
        case OMX_DirInput: {
            if (mSecureFlag) {
                BufferCtrlStruct* pBufCtrl = (BufferCtrlStruct*)(header->pInputPortPrivate);
                if (pBufCtrl != nullptr && pBufCtrl->pMem != nullptr) {
                    OMX_LOGI("freeBuffer, intput phyAddr: 0x%lx, pBuffer: %p", pBufCtrl->phyAddr, header->pBuffer);
                    releaseMemIon(&pBufCtrl->pMem);
                } else {
                    OMX_LOGE("freeBuffer, intput pBufCtrl == nullptr or pBufCtrl->pMem == nullptr");
                }
            }
            return SprdSimpleOMXComponent::freeBuffer(portIndex, header);
        }
        case OMX_DirOutput: {
            BufferCtrlStruct* pBufCtrl = (BufferCtrlStruct*)(header->pOutputPortPrivate);
            if (pBufCtrl != nullptr) {
                OMX_LOGV("freeBuffer, phyAddr: 0x%lx", pBufCtrl->phyAddr);
                if (mIOMMUEnabled && pBufCtrl->phyAddr > INVALID_PHY_ADDR) {
                    (*mH265DecFreeIOVA)(mHandle, pBufCtrl->phyAddr, mPictureSize, true);
                }
                if (pBufCtrl->pMem != nullptr) {
                    //pbuffer
                    releaseMemIon(&pBufCtrl->pMem);
                }
                freePlatform(portIndex, header);
                return SprdSimpleOMXComponent::freeBuffer(portIndex, header);
            } else {
                OMX_LOGE("freeBuffer, pBufCtrl == nullptr");
                return OMX_ErrorUndefined;
            }
        }
        default:
            return OMX_ErrorUnsupportedIndex;
    }
}
OMX_ERRORTYPE SPRDHEVCDecoder::getConfig(OMX_INDEXTYPE index, OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_INDEX_DESCRIBE_COLOR_ASPECTS: {
            OMX_LOGE("SPRDHEVCDecoder, DO NOT USE OMX_INDEX_DESCRIBE_COLOR_ASPECTS");
            return OMX_ErrorUnsupportedIndex;
        }
        case OMX_IndexConfigCommonOutputCrop: {
            OMX_CONFIG_RECTTYPE *rectParams = (OMX_CONFIG_RECTTYPE *)params;
            if (rectParams->nPortIndex != OUTPUT_PORT_INDEX) {
                return OMX_ErrorUndefined;
            }
            rectParams->nLeft = 0;
            rectParams->nTop = 0;
            {
                OMX_LOGI("%s, mCropWidth:%d, mCropHeight:%d",
                    __FUNCTION__, mCropWidth, mCropHeight);
                AutoMutex autoLock(mLock);
                rectParams->nWidth = mCropWidth;
                rectParams->nHeight = mCropHeight;
            }
            return OMX_ErrorNone;
        }
        default:
            return SprdVideoDecoderBase::getConfig(index, params);
    }
}
OMX_ERRORTYPE SPRDHEVCDecoder::internalSetConfig(OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig)
{
    switch (static_cast<int>(index)) {
        case OMX_INDEX_DESCRIBE_COLOR_ASPECTS: {
            OMX_LOGE("setConfig DescribeColorAspects is not supported");
            return OMX_ErrorNotImplemented;
        }
        case OMX_INDEX_CONFIG_THUMBNAIL_MODE: {
            OMX_BOOL *pEnable = (OMX_BOOL *)params;
            if (*pEnable == OMX_TRUE) {
                mThumbnailMode = OMX_TRUE;
            }
            OMX_LOGI("setConfig, mThumbnailMode = %d", mThumbnailMode);
            return OMX_ErrorNone;
        }
        default:
            return SprdVideoDecoderBase::internalSetConfig(index, params, frameConfig);
    }
}
bool SPRDHEVCDecoder::ShouldSkipQueueFill()
{
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return true;
    }
    if (mEOSStatus == OUTPUT_FRAMES_FLUSHED) {
        return true;
    }
    return false;
}

bool SPRDHEVCDecoder::shouldContinueDecodeLoop(
    const std::list<BufferInfo*>& inQueue, const std::list<BufferInfo*>& outQueue)
{
    return !mStopDecode && (mEOSStatus != INPUT_DATA_AVAILABLE || !inQueue.empty()) &&
        outQueue.size() != MIN_LOOP_VALUE;
}

bool SPRDHEVCDecoder::processQueuedFrames(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue)
{
    while (shouldContinueDecodeLoop(inQueue, outQueue)) {
        if (mEOSStatus == INPUT_EOS_SEEN) {
            OMX_LOGI("%s, %d, EOS had seen, break loop.", __FUNCTION__, __LINE__);
            break;
        }
        bool shouldBreak = false;
        if (!processOneQueuedFrame(inQueue, outQueue, shouldBreak)) {
            return false;
        }
        if (shouldBreak) {
            break;
        }
    }
    return true;
}

void SPRDHEVCDecoder::onQueueFilled(OMX_U32 portIndex)
{
    (void)portIndex;
    if (ShouldSkipQueueFill()) {
        return;
    }
    std::list<BufferInfo*>& inQueue = getPortQueue(K_INPUT_PORT_INDEX);
    std::list<BufferInfo*>& outQueue = getPortQueue(K_OUTPUT_PORT_INDEX);
    if (!processQueuedFrames(inQueue, outQueue)) {
        return;
    }
    OMX_LOGI("%s, %d, mEOSStatus=%d", __FUNCTION__, __LINE__, mEOSStatus);
    if (mEOSStatus == INPUT_EOS_SEEN) {
        DrainAllOutputBuffers();
        return;
    }
}

bool SPRDHEVCDecoder::processOneQueuedFrame(
    std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue, bool &shouldBreak)
{
    BufferInfo *inInfo = *inQueue.begin();
    OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;
    ++picId;
    shouldBreak = false;
    if (!prepareInputHeader(inQueue, inInfo, inHeader)) {
        return true;
    }
    OMX_BUFFERHEADERTYPE *outHeader = nullptr;
    BufferCtrlStruct *pBufCtrl = nullptr;
    if (!acquireAvailableOutputHeader(outQueue, outHeader, pBufCtrl)) {
        return false;
    }
    MMDecInput decIn;
    MMDecOutput decOut;
    uint32_t addStartcodeLen = 0;
    prepareDecodeInput(inHeader, decIn, decOut, addStartcodeLen);
    MMDecRet decRet = decodeFrame(inHeader, outHeader, pBufCtrl, decIn, decOut);
    if (!HandleDecodeResult(decRet)) {
        return false;
    }
    if (!ProcessDecoderInfo()) {
        return false;
    }
    if (!FinalizeInputFrame(decIn, inQueue, inInfo, inHeader)) {
        return false;
    }
    drainDecodedFrames(outQueue, decOut);
    return true;
}

bool SPRDHEVCDecoder::acquireAvailableOutputHeader(
    std::list<BufferInfo*>& outQueue, OMX_BUFFERHEADERTYPE *&outHeader, BufferCtrlStruct *&bufferCtrl)
{
    auto itBuffer = outQueue.begin();
    size_t count = 0;
    while (count < outQueue.size()) {
        outHeader = (*itBuffer)->mHeader;
        bufferCtrl = reinterpret_cast<BufferCtrlStruct*>(outHeader->pOutputPortPrivate);
        if (bufferCtrl == nullptr) {
            notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
            mSignalledError = true;
            return false;
        }
        ++itBuffer;
        ++count;
        if (bufferCtrl->iRefCount <= MIN_LOOP_VALUE) {
            return true;
        }
    }
    return false;
}

bool SPRDHEVCDecoder::prepareInputHeader(
    std::list<BufferInfo*>& inQueue, BufferInfo *&inInfo, OMX_BUFFERHEADERTYPE *&inHeader)
{
    if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
        mEOSStatus = INPUT_EOS_SEEN;
    }
    if (inHeader->nFilledLen != ZERO_VALUE) {
        return true;
    }
    releaseCurrentInputBuffer(inQueue, inInfo, inHeader);
    return false;
}

void SPRDHEVCDecoder::releaseCurrentInputBuffer(
    std::list<BufferInfo*>& inQueue, BufferInfo *&inInfo, OMX_BUFFERHEADERTYPE *&inHeader)
{
    inInfo->mOwnedByUs = false;
    inQueue.erase(inQueue.begin());
    notifyEmptyBufferDone(inHeader);
    inInfo = nullptr;
    inHeader = nullptr;
}

void SPRDHEVCDecoder::initDecoderInputBuffer(MMDecInput &decIn, OMX_BUFFERHEADERTYPE *inHeader)
{
    if (!mSecureFlag) {
        decIn.pStream = mPbufStreamV;
        decIn.pStreamPhy = mPbufStreamP;
    } else {
        BufferCtrlStruct *bufferCtrl = reinterpret_cast<BufferCtrlStruct*>(inHeader->pInputPortPrivate);
        mPbufStreamV = reinterpret_cast<uint8*>(bufferCtrl->pMem->getBase());
        decIn.pStream = mPbufStreamV;
        decIn.pStreamPhy = reinterpret_cast<unsigned long>(bufferCtrl->phyAddr);
    }
    decIn.beLastFrame = 0;
    decIn.expectedIVOP = mNeedIVOP;
    decIn.beDisplayed = 1;
    decIn.errPktNum = 0;
    decIn.fbcMode = mFbcMode;
}

void SPRDHEVCDecoder::copyInputDataToDecoder(MMDecInput &decIn, OMX_BUFFERHEADERTYPE *inHeader,
                                             uint32_t &addStartcodeLen)
{
    uint8_t *bitstream = inHeader->pBuffer + inHeader->nOffset;
    uint32_t bufferSize = inHeader->nFilledLen;
    if (mSecureFlag) {
        decIn.dataLength = bufferSize;
        return;
    }
    uint32_t copyLen = (bufferSize <= mPbufStreamSize) ? bufferSize : mPbufStreamSize;
    if (mThumbnailMode) {
        if ((bitstream[0] != START_CODE_PREFIX_BYTE_0) ||
            (bitstream[1] != START_CODE_PREFIX_BYTE_1) ||
            (bitstream[ARRAY_INDEX_2] != START_CODE_PREFIX_BYTE_2) ||
            (bitstream[ARRAY_INDEX_3] != START_CODE_PREFIX_BYTE_3)) {
            reinterpret_cast<uint8_t*>(mPbufStreamV)[ARRAY_INDEX_0] = START_CODE_PREFIX_BYTE_0;
            reinterpret_cast<uint8_t*>(mPbufStreamV)[ARRAY_INDEX_1] = START_CODE_PREFIX_BYTE_1;
            reinterpret_cast<uint8_t*>(mPbufStreamV)[ARRAY_INDEX_2] = START_CODE_PREFIX_BYTE_2;
            reinterpret_cast<uint8_t*>(mPbufStreamV)[ARRAY_INDEX_3] = START_CODE_PREFIX_BYTE_3;
            addStartcodeLen = START_CODE_LENGTH;
        }
        copyLen = (bufferSize <= mPbufStreamSize - addStartcodeLen) ? bufferSize :
            (mPbufStreamSize - addStartcodeLen);
    }
    if (mPbufStreamV != nullptr) {
        errno_t ret = memmove_s(mPbufStreamV + addStartcodeLen,
                                mPbufStreamSize - addStartcodeLen,
                                bitstream, copyLen);
        if (ret != 0) {
            OMX_LOGE("memmove_s failed in line %d, ret=%d", __LINE__, ret);
        }
    }
    decIn.dataLength = copyLen + addStartcodeLen;
}

void SPRDHEVCDecoder::prepareDecodeInput(
    OMX_BUFFERHEADERTYPE *inHeader, MMDecInput &decIn, MMDecOutput &decOut, uint32_t &addStartcodeLen)
{
    initDecoderInputBuffer(decIn, inHeader);
    decOut.frameEffective = 0;
    decOut.hasPic = 0;
    copyInputDataToDecoder(decIn, inHeader, addStartcodeLen);
}

bool SPRDHEVCDecoder::prepareOutputFrameBuffer(
    OMX_BUFFERHEADERTYPE *outHeader, BufferCtrlStruct *bufferCtrl, OMX_BUFFERHEADERTYPE *inHeader, int &fd)
{
    outHeader->nTimeStamp = inHeader->nTimeStamp;
    outHeader->nFlags = inHeader->nFlags & (~OMX_BUFFERFLAG_EOS);
    unsigned long picPhyAddr = 0;
    fd = 0;
    if (mAllocateBuffers) {
        fd = bufferCtrl->pMem->getHeapID();
    }
    if (bufferCtrl->phyAddr != INVALID_PHY_ADDR) {
        picPhyAddr = bufferCtrl->phyAddr;
    } else if (mIOMMUEnabled) {
        notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
        mSignalledError = true;
        return false;
    }
    uint8 *yuv = reinterpret_cast<uint8 *>(outHeader->pBuffer + outHeader->nOffset);
    (*mH265DecSetCurRecPic)(mHandle, yuv, reinterpret_cast<uint8*>(picPhyAddr), 0,
        reinterpret_cast<void*>(outHeader), picId, bufferCtrl->bufferFd);
    return true;
}

void SPRDHEVCDecoder::SyncOutputBuffer(int fd, unsigned long flags)
{
    if (!mAllocateBuffers) {
        return;
    }
    struct dma_buf_sync sync;
    sync.flags = flags;
    ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
}

MMDecRet SPRDHEVCDecoder::decodeFrame(OMX_BUFFERHEADERTYPE *inHeader, OMX_BUFFERHEADERTYPE *outHeader,
    BufferCtrlStruct *bufferCtrl, MMDecInput &decIn, MMDecOutput &decOut)
{
    int fd = 0;
    if (!prepareOutputFrameBuffer(outHeader, bufferCtrl, inHeader, fd)) {
        return MMDEC_MEMORY_ERROR;
    }
    if (mDumpStrmEnabled && inHeader->nOffset == ZERO_VALUE) {
        DumpFiles(mPbufStreamV, decIn.dataLength, DUMP_STREAM);
    }
    SyncOutputBuffer(fd, DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW);
    int64_t startDecode = systemTime();
    MMDecRet decRet = (*mH265DecDecode)(mHandle, &decIn, &decOut);
    int64_t endDecode = systemTime();
    OMX_LOGI("%s, %d, decRet: %d, %dms, decOut.frameEffective: %d, needIvoP: %d", __FUNCTION__, __LINE__, decRet,
        (unsigned int)((endDecode - startDecode) / 1000000L), decOut.frameEffective, mNeedIVOP);
    SyncOutputBuffer(fd, DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW);
    return decRet;
}

bool SPRDHEVCDecoder::HandleDecodeResult(MMDecRet decRet)
{
    if (decRet == MMDEC_OK) {
        mNeedIVOP = false;
        return true;
    }
    mNeedIVOP = true;
    if (decRet == MMDEC_MEMORY_ERROR) {
        notify(OMX_EventError, OMX_ErrorInsufficientResources, 0, nullptr);
        mSignalledError = true;
        return false;
    }
    if (decRet == MMDEC_NOT_SUPPORTED) {
        notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, nullptr);
        mSignalledError = true;
        return false;
    }
    return true;
}

bool SPRDHEVCDecoder::ProcessDecoderInfo()
{
    H265SwDecInfo decoderInfo;
    (void)memset_s(&decoderInfo, sizeof(H265SwDecInfo), 0, sizeof(H265SwDecInfo));
    MMDecRet ret = (*mH265DecGetInfo)(mHandle, &decoderInfo);
    GetVuiParams(&decoderInfo);
    if (ret != MMDEC_OK || decoderInfo.picWidth == ZERO_VALUE) {
        OMX_LOGE("failed to get decoder information.");
        return true;
    }
    if (!(decoderInfo.picWidth <= mCapability.maxWidth && decoderInfo.picHeight <= mCapability.maxHeight)) {
        notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, nullptr);
        mSignalledError = true;
        return false;
    }
    if (HandlePortSettingChangeEvent(&decoderInfo)) {
        return false;
    }
    return true;
}

bool SPRDHEVCDecoder::FinalizeInputFrame(
    MMDecInput &decIn, std::list<BufferInfo*>& inQueue, BufferInfo *&inInfo, OMX_BUFFERHEADERTYPE *&inHeader)
{
    uint32_t bufferSize = decIn.dataLength;
    if (bufferSize > inHeader->nFilledLen) {
        return false;
    }
    inHeader->nOffset += bufferSize;
    inHeader->nFilledLen -= bufferSize;
    if (inHeader->nFilledLen <= MIN_LOOP_VALUE) {
        inHeader->nOffset = 0;
        releaseCurrentInputBuffer(inQueue, inInfo, inHeader);
    }
    return true;
}

void SPRDHEVCDecoder::drainDecodedFrames(std::list<BufferInfo*>& outQueue, MMDecOutput &decOut)
{
    while (!outQueue.empty() && mHeadersDecoded && decOut.frameEffective) {
        if (mDumpYUVEnabled) {
            DumpFiles(decOut.pOutFrameY, mhigh10En ? mPictureSize - META_DATA_T_SIZE : mPictureSize, DUMP_YUV);
        }
        drainOneOutputBuffer(decOut.picId, decOut.pBufferHeader);
        decOut.frameEffective = false;
        if (mThumbnailMode) {
            mStopDecode = true;
        }
    }
}
bool SPRDHEVCDecoder::GetVuiParams(H265SwDecInfo *mDecoderInfo)
{
    return true;
}

bool SPRDHEVCDecoder::ShouldDisableAfbc(const H265SwDecInfo *info)
{
    return ((info->picWidth & ALIGN_32_BOUNDARY) && (mFbcMode == AFBC));
}

void SPRDHEVCDecoder::DrainPendingDecoderFrames()
{
    int32_t lastPicId;
    void *pBufferHeader = nullptr;
    while ((*mH265DecGetLastDspFrm)(mHandle, &pBufferHeader, &lastPicId) == MMDEC_OK) {
        drainOneOutputBuffer(lastPicId, pBufferHeader);
    }
}

void SPRDHEVCDecoder::UpdateDecoderPictureState(const H265SwDecInfo *info)
{
    mFrameWidth = info->cropParams.cropOutWidth;
    mFrameHeight = info->cropParams.cropOutHeight;
    mhigh10En = info->high10En;
    mStride = info->picWidth;
    mSliceHeight = info->picHeight;
    int high10Factor = (info->high10En != 0) ? HIGH10_MULTIPLIER : 1;
    mPictureSize = mStride * mSliceHeight * YUV420_MULTIPLIER / YUV420_DIVISOR * high10Factor;
    if (mFbcMode == AFBC) {
        int headerSize = (mStride * mSliceHeight / AFBC_HEADER_DIVISOR * high10Factor + ALIGN_1KB_BOUNDARY) &
            (~ALIGN_1KB_BOUNDARY);
        mPictureSize += headerSize;
    }
    if (mhigh10En) {
        mPictureSize += META_DATA_T_SIZE;
    }
}

void SPRDHEVCDecoder::UpdateOutputBufferCount(
    OMX_PARAM_PORTDEFINITIONTYPE *def, const H265SwDecInfo *info, OMX_BOOL useNativeBuffer)
{
    if (useNativeBuffer) {
        def->nBufferCountMin = info->numFrames + ADDITIONAL_TIMEOUT_BUFFER + ADDITIONAL_TIMEOUT_BUFFER;
        return;
    }
    def->nBufferCountActual = info->numFrames + ADDITIONAL_TIMEOUT_BUFFER +
        ADDITIONAL_RECONSTRUCTED_BUFFER + SURFACE_FLINGER_RESERVED_BUFFERS;
    def->bPopulated = OMX_FALSE;
}

bool SPRDHEVCDecoder::HandlePortSettingChangeEvent(const H265SwDecInfo *info)
{
    OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(K_OUTPUT_PORT_INDEX)->mDef;
    OMX_BOOL useNativeBuffer = iUseOMXNativeBuffer[OMX_DirOutput];
    bool needFlushBuffer = outputBuffersNotEnough(info,
        def->nBufferCountMin, def->nBufferCountActual, useNativeBuffer);
    bool cropChanged = HandleCropRectEvent(&info->cropParams);
    bool bitdepChanged = HandleBitdepthChangeEvent(&info->high10En);
    if (DisableAfbcIfNeeded(info)) {
        return true;
    }
    if (ShouldReconfigurePort(info, cropChanged, bitdepChanged, needFlushBuffer)) {
        return ApplyPortReconfiguration(info, def, useNativeBuffer, needFlushBuffer);
    }
    if (mUpdateColorAspects) {
        OMX_LOGI("skip color-aspects settings changed event on OHOS HEVC decoder");
        mUpdateColorAspects = false;
    }
    return false;
}

bool SPRDHEVCDecoder::ShouldReconfigurePort(
    const H265SwDecInfo *info, bool cropChanged, bool bitdepChanged, bool needFlushBuffer) const
{
    return (mStride != info->picWidth) || (mSliceHeight != info->picHeight) || bitdepChanged || cropChanged ||
        (!mThumbnailMode && needFlushBuffer);
}

bool SPRDHEVCDecoder::DisableAfbcIfNeeded(const H265SwDecInfo *info)
{
    if (!ShouldDisableAfbc(info)) {
        return false;
    }
    mFbcMode = FBC_NONE;
    notify(OMX_EventPortSettingsChanged, OUTPUT_PORT_INDEX, OMX_IndexParamPortDefinition, nullptr);
    mOutputPortSettingsChange = AWAITING_DISABLED;
    OMX_LOGI("width %d, disable AFBC\n", info->picWidth);
    return true;
}

bool SPRDHEVCDecoder::ApplyPortReconfiguration(
    const H265SwDecInfo *info, OMX_PARAM_PORTDEFINITIONTYPE *def, OMX_BOOL useNativeBuffer, bool needFlushBuffer)
{
    AutoMutex autoLock(mLock);
    DrainPendingDecoderFrames();
    UpdateDecoderPictureState(info);
    if (!mThumbnailMode && needFlushBuffer) {
        UpdateOutputBufferCount(def, info, useNativeBuffer);
    }
    UpdatePortDefinitions(true, true);
    (*mH265DecReleaseRefBuffers)(mHandle);
    notify(OMX_EventPortSettingsChanged, OUTPUT_PORT_INDEX, OMX_IndexParamPortDefinition, nullptr);
    mOutputPortSettingsChange = AWAITING_DISABLED;
    mPbufMbInfoIdx = 0;
    return true;
}
bool SPRDHEVCDecoder::HandleCropRectEvent(const CropParams *crop)
{
    if (mCropWidth != crop->cropOutWidth || mCropHeight != crop->cropOutHeight) {
        OMX_LOGI("%s, crop w h: %d %d", __FUNCTION__, crop->cropOutWidth, crop->cropOutHeight);
        return true;
    }
    return false;
}
bool SPRDHEVCDecoder::HandleBitdepthChangeEvent(const unsigned char *high10En)
{
    if (mhigh10En != *high10En) {
        OMX_LOGI("%s, high10En %d", __FUNCTION__, *high10En);
        return true;
    }
    return false;
}
void SPRDHEVCDecoder::notifyFillBufferDone(OMX_BUFFERHEADERTYPE *header)
{
    NV12Crop(header, mStride, mSliceHeight, mStride16, mFrameHeight);
    (*mCallbacks->FillBufferDone)(
        mComponent, mComponent->pApplicationPrivate, header);
}
void SPRDHEVCDecoder::drainOneOutputBuffer(int32_t picId, const void *pBufferHeader)
{
    (void)picId;
    std::list<BufferInfo*>& outQueue = getPortQueue(K_OUTPUT_PORT_INDEX);
    std::list<BufferInfo *>::iterator it = outQueue.begin();
    while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != outQueue.end()) {
        ++it;
    }
    if ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((*it)->mHeader == "
            "(OMX_BUFFERHEADERTYPE*)pBufferHeader) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return;
    }
    BufferInfo *outInfo = *it;
    OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;
    outHeader->nFilledLen = mPictureSize;
    OMX_LOGI("%s, %d, outHeader: %p, outHeader->pBuffer: %p, outHeader->nOffset: %d, "
        "outHeader->nFlags: %d, outHeader->nTimeStamp: %lld", __FUNCTION__, __LINE__,
        outHeader, outHeader->pBuffer, outHeader->nOffset, outHeader->nFlags, outHeader->nTimeStamp);
    outInfo->mOwnedByUs = false;
    outQueue.erase(it);
    outInfo = nullptr;
    BufferCtrlStruct* pOutBufCtrl = (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
    pOutBufCtrl->iRefCount++;
    notifyFillBufferDone(outHeader);
}
bool SPRDHEVCDecoder::DrainAllOutputBuffers()
{
    std::list<BufferInfo*>& outQueue = getPortQueue(K_OUTPUT_PORT_INDEX);
    while (!outQueue.empty() && mEOSStatus != OUTPUT_FRAMES_FLUSHED) {
        BufferInfo *outInfo = nullptr;
        OMX_BUFFERHEADERTYPE *outHeader = nullptr;
        if (!prepareDrainOutput(outInfo, outHeader)) {
            return false;
        }
        submitDrainOutput(outQueue, outInfo, outHeader);
    }
    return true;
}

bool SPRDHEVCDecoder::prepareDrainOutput(BufferInfo *&outInfo, OMX_BUFFERHEADERTYPE *&outHeader)
{
    std::list<BufferInfo*>& outQueue = getPortQueue(K_OUTPUT_PORT_INDEX);
    if (mHeadersDecoded) {
        int32_t picId = 0;
        void *pBufferHeader = nullptr;
        if ((*mH265DecGetLastDspFrm)(mHandle, &pBufferHeader, &picId) == MMDEC_OK) {
            std::list<BufferInfo *>::iterator it = outQueue.begin();
            while (it != outQueue.end() && (*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader) {
                ++it;
            }
            if (it == outQueue.end()) {
                OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((*it)->mHeader == "
                    "(OMX_BUFFERHEADERTYPE*)pBufferHeader) failed.",
                    __FUNCTION__, FILENAME_ONLY, __LINE__);
                return false;
            }
            outInfo = *it;
            outQueue.erase(it);
            outHeader = outInfo->mHeader;
            outHeader->nFilledLen = mPictureSize;
            OMX_LOGI("%s, %d, outHeader: %p, outHeader->pBuffer: %p, outHeader->nOffset: %d, "
                "outHeader->nFlags: %d, outHeader->nTimeStamp: %lld, mEOSStatus: %d",
                __FUNCTION__, __LINE__, outHeader, outHeader->pBuffer, outHeader->nOffset,
                outHeader->nFlags, outHeader->nTimeStamp, mEOSStatus);
            return true;
        }
    }
    outInfo = *outQueue.begin();
    outQueue.erase(outQueue.begin());
    outHeader = outInfo->mHeader;
    outHeader->nTimeStamp = 0;
    outHeader->nFilledLen = 0;
    outHeader->nFlags = OMX_BUFFERFLAG_EOS;
    mEOSStatus = OUTPUT_FRAMES_FLUSHED;
    OMX_LOGI("%s, %d, set EOS flag", __FUNCTION__, __LINE__);
    return true;
}

void SPRDHEVCDecoder::submitDrainOutput(std::list<BufferInfo*>& outQueue, BufferInfo *outInfo,
    OMX_BUFFERHEADERTYPE *outHeader)
{
    (void)outQueue;
    outInfo->mOwnedByUs = false;
    BufferCtrlStruct* pOutBufCtrl = (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
    pOutBufCtrl->iRefCount++;
    notifyFillBufferDone(outHeader);
}
void SPRDHEVCDecoder::onPortFlushCompleted(OMX_U32 portIndex)
{
    if (portIndex == K_INPUT_PORT_INDEX) {
        mEOSStatus = INPUT_DATA_AVAILABLE;
        mNeedIVOP = true;
    }
}
void SPRDHEVCDecoder::onPortFlushPrepare(OMX_U32 portIndex)
{
    if (portIndex == OMX_DirOutput) {
        if (mH265DecReleaseRefBuffers != nullptr) {
            (*mH265DecReleaseRefBuffers)(mHandle);
        }
            
        mPbufMbInfoIdx = 0;
    }
}
void SPRDHEVCDecoder::UpdatePortDefinitions(bool updateCrop, bool updateInputSize)
{
    OMX_PARAM_PORTDEFINITIONTYPE *outDef = &editPortInfo(K_OUTPUT_PORT_INDEX)->mDef;
    if (updateCrop) {
        mCropWidth = mFrameWidth;
        mCropHeight = mFrameHeight;
    }
    outDef->format.video.nFrameWidth = mStride;
    outDef->format.video.nFrameHeight = mSliceHeight;
    outDef->format.video.nStride = mStride;
    outDef->format.video.nSliceHeight = mSliceHeight;
    outDef->nBufferSize = mPictureSize;
    OMX_LOGI("%s, %d %d %d %d", __FUNCTION__, outDef->format.video.nFrameWidth,
        outDef->format.video.nFrameHeight,
        outDef->format.video.nStride,
        outDef->format.video.nSliceHeight);
    OMX_PARAM_PORTDEFINITIONTYPE *inDef = &editPortInfo(K_INPUT_PORT_INDEX)->mDef;
    inDef->format.video.nFrameWidth = mFrameWidth;
    inDef->format.video.nFrameHeight = mFrameHeight;
    // input port is compressed, hence it has no stride
    inDef->format.video.nStride = 0;
    inDef->format.video.nSliceHeight = 0;
    // when output format changes, input buffer size does not actually change
    if (updateInputSize) {
        inDef->nBufferSize = std::max(outDef->nBufferSize / MIN_COMPRESSION_RATIO_DEFAULT, inDef->nBufferSize);
    }
}
// static
int32_t SPRDHEVCDecoder::ExtMemAllocWrapper(
    void *aUserData, unsigned int sizeExtra)
{
    return static_cast<SPRDHEVCDecoder *>(aUserData)->VspMallocCb(sizeExtra);
}
// static
int32_t SPRDHEVCDecoder::CtuInfoMemAllocWrapper(
    void *aUserData, unsigned int sizeMbinfo, unsigned long *pPhyAddr)
{
    return static_cast<SPRDHEVCDecoder *>(aUserData)->VspMallocCtuinfoCb(sizeMbinfo, pPhyAddr);
}
// static
int32_t SPRDHEVCDecoder::BindFrameWrapper(void *aUserData, void *pHeader)
{
    return static_cast<SPRDHEVCDecoder *>(aUserData)->VspBindCb(pHeader);
}
// static
int32_t SPRDHEVCDecoder::UnbindFrameWrapper(void *aUserData, void *pHeader)
{
    return static_cast<SPRDHEVCDecoder *>(aUserData)->VspUnbindCb(pHeader);
}

int SPRDHEVCDecoder::AllocateSoftwareExtraBuffer(unsigned int sizeExtra, MMCodecBuffer extraMem[])
{
    if (mCodecExtraBuffer != nullptr) {
        free(mCodecExtraBuffer);
        mCodecExtraBuffer = nullptr;
    }
    if (sizeExtra == ZERO_VALUE || sizeExtra > UINT32_MAX) {
        OMX_LOGE("Invalid sizeExtra: %zu, malloc failed", sizeExtra);
        return -1;
    }
    mCodecExtraBuffer = reinterpret_cast<uint8_t*>(malloc(sizeExtra));
    if (mCodecExtraBuffer == nullptr) {
        return -1;
    }
    extraMem[SW_CACHABLE].commonBufferPtr = mCodecExtraBuffer;
    extraMem[SW_CACHABLE].commonBufferPtrPhy = 0;
    extraMem[SW_CACHABLE].size = sizeExtra;
    return 0;
}

int SPRDHEVCDecoder::AllocateHardwareExtraBuffer(unsigned int sizeExtra, MMCodecBuffer extraMem[])
{
    mPbufMbInfoIdx = 0;
    if (mPbufExtraV != nullptr) {
        if (mIOMMUEnabled) {
            (*mH265DecFreeIOVA)(mHandle, mPbufExtraP, mPbufExtraSize, false);
        }
        releaseMemIon(&mPmemExtra);
        mPbufExtraV = nullptr;
        mPbufExtraP = 0;
        mPbufExtraSize = 0;
    }
    mPmemExtra = new VideoMemAllocator(sizeExtra, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    int fd = mPmemExtra->getHeapID();
    if (fd < 0) {
        OMX_LOGE("mPmemExtra: getHeapID fail %d", fd);
        return -1;
    }
    unsigned long phyAddr = 0;
    size_t bufferSize = 0;
    int ret = mIOMMUEnabled ? (*mH265DecGetIOVA)(mHandle, fd, &phyAddr, &bufferSize, false) :
        mPmemExtra->getPhyAddrFromMemAlloc(&phyAddr, &bufferSize);
    if (ret < 0) {
        OMX_LOGE("mPmemExtra: get phy addr fail %d(%s)", ret, strerror(errno));
        return -1;
    }
    mPbufExtraP = phyAddr;
    mPbufExtraSize = bufferSize;
    mPbufExtraV = reinterpret_cast<uint8_t*>(mPmemExtra->getBase());
    OMX_LOGI("pmem 0x%lx - %p - %zd", mPbufExtraP, mPbufExtraV, mPbufExtraSize);
    extraMem[HW_NO_CACHABLE].commonBufferPtr = mPbufExtraV;
    extraMem[HW_NO_CACHABLE].commonBufferPtrPhy = mPbufExtraP;
    extraMem[HW_NO_CACHABLE].size = sizeExtra;
    return 0;
}

int SPRDHEVCDecoder::VspMallocCb(unsigned int sizeExtra)
{
    OMX_LOGI("%s, %d, mDecoderSwFlag: %d, mPictureSize: %d, sizeExtra: %d",
        __FUNCTION__, __LINE__, mDecoderSwFlag, mPictureSize, sizeExtra);
    MMCodecBuffer extraMem[MAX_MEM_TYPE];
    int ret = mDecoderSwFlag ? AllocateSoftwareExtraBuffer(sizeExtra, extraMem) :
        AllocateHardwareExtraBuffer(sizeExtra, extraMem);
    if (ret != 0) {
        return -1;
    }
    (*mH265DecMemInit)(reinterpret_cast<SPRDHEVCDecoder*>(this)->mHandle, extraMem);
    mHeadersDecoded = true;
    return 0;
}
int SPRDHEVCDecoder::VspMallocCtuinfoCb(unsigned int sizeMbinfo, unsigned long *pPhyAddr)
{
    int idx = mPbufMbInfoIdx;
    OMX_LOGI("%s, %d, idx: %d, sizeMbinfo: %d", __FUNCTION__, __LINE__, idx, sizeMbinfo);
    if (mPmemMbInfoV[idx] != nullptr) {
        if (mIOMMUEnabled) {
            (*mH265DecFreeIOVA)(mHandle, mPmemMbInfoP[idx], mPbufMbInfoSize[idx], false);
        }
        releaseMemIon(&mPmemMbInfo[idx]);
        mPmemMbInfoV[idx] = nullptr;
        mPmemMbInfoP[idx] = 0;
        mPbufMbInfoSize[idx] = 0;
    }
    mPmemMbInfo[idx] = new VideoMemAllocator(sizeMbinfo, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    int fd = mPmemMbInfo[idx]->getHeapID();
    if (fd >= 0) {
        int ret;
        unsigned long phyAddr;
        size_t bufferSize;
        if (mIOMMUEnabled) {
            ret = (*mH265DecGetIOVA)(mHandle, fd, &phyAddr, &bufferSize, false);
        } else {
            ret = mPmemMbInfo[idx]->getPhyAddrFromMemAlloc(&phyAddr, &bufferSize);
        }
        if (ret < 0) {
            OMX_LOGE("mPmemMbInfo[%d]: get phy addr fail %d", idx, ret);
            return -1;
        }
        mPmemMbInfoP[idx] = phyAddr;
        mPbufMbInfoSize[idx] = bufferSize;
        mPmemMbInfoV[idx] = reinterpret_cast<uint8_t*>(mPmemMbInfo[idx]->getBase());
        OMX_LOGI("pmem 0x%lx - %p - %zd", mPmemMbInfoP[idx], mPmemMbInfoV[idx], mPbufMbInfoSize[idx]);
        *pPhyAddr = phyAddr;
    } else {
        OMX_LOGE("mPmemMbInfo[%d]: getHeapID fail %d", idx, fd);
        return -1;
    }
    mPbufMbInfoIdx++;
    return 0;
}
int SPRDHEVCDecoder::VspBindCb(void *pHeader)
{
    BufferCtrlStruct *pBufCtrl = reinterpret_cast<BufferCtrlStruct*>
                                 (((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);
    pBufCtrl->iRefCount++;
    OMX_LOGI("VspBindCb, pBuffer: %p, pHeader: %p; phyAddr=0x%lx, iRefCount=%d",
        ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader, pBufCtrl->phyAddr, pBufCtrl->iRefCount);
    return 0;
}
int SPRDHEVCDecoder::VspUnbindCb(void *pHeader)
{
    BufferCtrlStruct *pBufCtrl = reinterpret_cast<BufferCtrlStruct*>
                                 (((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);
    OMX_LOGI("VspUnbindCb, pBuffer: %p, pHeader: %p; phyAddr=0x%lx, iRefCount=%d",
        ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader, pBufCtrl->phyAddr, pBufCtrl->iRefCount);
    if (pBufCtrl->iRefCount  > MIN_LOOP_VALUE) {
        pBufCtrl->iRefCount--;
    }
    return 0;
}
OMX_ERRORTYPE SPRDHEVCDecoder::getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index)
{
    if (strcmp(name, "OMX.sprd.index.ThumbnailMode") == 0) {
        OMX_LOGI("getExtensionIndex: OMX.sprd.index.ThumbnailMode");
        *(int32_t*)index = OMX_INDEX_CONFIG_THUMBNAIL_MODE;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.index.allocateNativeHandle") == 0) {
        OMX_LOGI("getExtensionIndex: OMX.index.allocateNativeHandle");
        *(int32_t*)index = OMX_INDEX_ALLOCATE_NATIVE_HANDLE;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.index.describeColorAspects") == 0) {
        OMX_LOGI("getExtensionIndex: describeColorAspects is not supported");
        return OMX_ErrorUnsupportedIndex;
    }
    return SprdVideoDecoderBase::getExtensionIndex(name, index);
}
bool SPRDHEVCDecoder::OpenDecoder(const char *libName)
{
    if (mLibHandle) {
        dlclose(mLibHandle);
    }
    OMX_LOGI("OpenDecoder, lib: %s", libName);
    mLibHandle = dlopen(libName, RTLD_NOW);
    if (mLibHandle == nullptr) {
        OMX_LOGE("OpenDecoder, can't open lib: %s", libName);
        return false;
    }
    if (!LoadHevcDecoderSymbol(mLibHandle, libName, "H265DecInit", &mH265DecInit) ||
        !LoadHevcDecoderSymbol(mLibHandle, libName, "H265DecGetInfo", &mH265DecGetInfo) ||
        !LoadHevcDecoderSymbol(mLibHandle, libName, "H265DecDecode", &mH265DecDecode) ||
        !LoadHevcDecoderSymbol(mLibHandle, libName, "H265DecRelease", &mH265DecRelease) ||
        !LoadHevcDecoderSymbol(mLibHandle, libName, "H265Dec_SetCurRecPic", &mH265DecSetCurRecPic) ||
        !LoadHevcDecoderSymbol(mLibHandle, libName, "H265Dec_GetLastDspFrm", &mH265DecGetLastDspFrm) ||
        !LoadHevcDecoderSymbol(mLibHandle, libName, "H265Dec_ReleaseRefBuffers", &mH265DecReleaseRefBuffers) ||
        !LoadHevcDecoderSymbol(mLibHandle, libName, "H265DecMemInit", &mH265DecMemInit) ||
        !LoadHevcDecoderSymbol(mLibHandle, libName, "H265GetCodecCapability", &mH265GetCodecCapability) ||
        !LoadHevcDecoderSymbol(mLibHandle, libName, "H265DecSetParameter", &mH265DecSetparam) ||
        !LoadHevcDecoderSymbol(mLibHandle, libName, "H265Dec_get_iova", &mH265DecGetIOVA) ||
        !LoadHevcDecoderSymbol(mLibHandle, libName, "H265Dec_free_iova", &mH265DecFreeIOVA) ||
        !LoadHevcDecoderSymbol(mLibHandle, libName, "H265Dec_get_IOMMU_status", &mH265DecGetIOMMUStatus)) {
        return CloseHevcDecoderLibraryOnFailure(&mLibHandle);
    }
    return true;
}
int SPRDHEVCDecoder::GetColorAspectPreference()
{
    return K_NOT_SUPPORTED;
}
SprdOMXComponent *createSprdOMXComponent(
    const char *name, const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData, OMX_COMPONENTTYPE **component)
{
    return new SPRDHEVCDecoder(name, callbacks, appData, component);
}
}
}
