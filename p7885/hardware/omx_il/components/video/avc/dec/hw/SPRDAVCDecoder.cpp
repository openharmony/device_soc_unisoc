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
#include <dlfcn.h>
#include <sys/prctl.h>
#include "avc_utils_sprd.h"
#include "avc_dec_api.h"
#include "utils/omx_log.h"
#include "OMXGraphicBufferMapper.h"
#include <thread>
#include <chrono>
#include <unistd.h>
#include <utils/time_util.h>
#include <sys/mman.h>
#include <cstddef>
#include <cstdint>
#undef LOG_TAG
#define LOG_TAG "SPRDAVCDecoder"
namespace OHOS {
namespace OMX {
struct LevelConversion {
    OMX_U32 omxLevel;
    AVCLevel avcLevel;
};

template<typename T>
static bool LoadDecoderSymbol(void *libHandle, const char *libName, const char *symbolName, T *symbol)
{
    *symbol = reinterpret_cast<T>(dlsym(libHandle, symbolName));
    if (*symbol != nullptr) {
        return true;
    }
    OMX_LOGE("Can't find %s in %s", symbolName, libName);
    return false;
}

constexpr inline size_t Align(size_t x, size_t align)
{
    return (x + (align - 1)) & ~(align - 1);
}

inline constexpr LevelConversion G_CONVERSION_TABLE[] = {
    { OMX_VIDEO_AVCLevel1,  AVC_LEVEL1_B },
    { OMX_VIDEO_AVCLevel1b, AVC_LEVEL1   },
    { OMX_VIDEO_AVCLevel11, AVC_LEVEL1_1 },
    { OMX_VIDEO_AVCLevel12, AVC_LEVEL1_2 },
    { OMX_VIDEO_AVCLevel13, AVC_LEVEL1_3 },
    { OMX_VIDEO_AVCLevel2,  AVC_LEVEL2 },
    { OMX_VIDEO_AVCLevel21, AVC_LEVEL2_1 },
    { OMX_VIDEO_AVCLevel22, AVC_LEVEL2_2 },
    { OMX_VIDEO_AVCLevel3,  AVC_LEVEL3 },
    { OMX_VIDEO_AVCLevel31, AVC_LEVEL3_1 },
    { OMX_VIDEO_AVCLevel32, AVC_LEVEL3_2 },
    { OMX_VIDEO_AVCLevel4,  AVC_LEVEL4 },
    { OMX_VIDEO_AVCLevel41, AVC_LEVEL4_1 },
    { OMX_VIDEO_AVCLevel42, AVC_LEVEL4_2 },
    { OMX_VIDEO_AVCLevel5,  AVC_LEVEL5 },
    { OMX_VIDEO_AVCLevel51, AVC_LEVEL5_1 },
};

static int g_omxCoreMutex = 0;
static int g_blockSizeMask = DEFAULT_BLOCK_SIZE_MASK;
static const char *MEDIA_MIMETYPE_VIDEO_AVC = "video/avc";
static const CodecProfileLevel kProfileLevels[] = {
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51 },
};
static bool CloseDecoderLibraryOnFailure(void **libHandle)
{
    dlclose(*libHandle);
    *libHandle = nullptr;
    return false;
}
SPRDAVCDecoder::SPRDAVCDecoder(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdVideoDecoderBase({
        name,
        "video_decoder.avc",
        OMX_VIDEO_CodingAVC,
        kProfileLevels,
        (sizeof(kProfileLevels) / sizeof((kProfileLevels)[0])),
        callbacks,
        appData,
        component}),
    mHandle(new TagAvcHandle)
{
    OMX_LOGI("Construct SPRDAVCDecoder, this: %p, g_omxCoreMutex: %d", static_cast<void *>(this), g_omxCoreMutex);
    mInitCheck = OMX_ErrorNone;
    if (!ConfigureDecoderInstance(name)) {
        return;
    }
    InitializeDecoderPorts();
    if (!OpenDecoder("libomx_avcdec_hw_sprd.z.so")) {
        OMX_LOGE("Failed to open libomx_avcdec_hw_sprd.z.so, try sw decoder.");
        if (!TryInitSoftwareDecoder()) {
            return;
        }
    } else {
        status_t initRes = initDecoder();
        if (initRes != STATUS_OK) {
            ReleaseDecoder(initRes == OMX_ErrorHardware ? false : true);
            if (!TryInitSoftwareDecoder()) {
                return;
            }
        }
    }
    InitializeCodecConfigBuffers();
    ResetMbInfoBuffers();
    InitDumpFiles(name, "h264");
}

bool SPRDAVCDecoder::ConfigureDecoderInstance(const char *name)
{
    size_t nameLen = strlen(name);
    size_t suffixLen = strlen(".secure");
    if (!strcmp(name + nameLen - suffixLen, ".secure")) {
        mSecureFlag = true;
    }
    if (g_omxCoreMutex >= MAX_INSTANCES || (mSecureFlag && g_omxCoreMutex >= MAX_SECURE_INSTANCES)) {
        mInitCheck = OMX_ErrorInsufficientResources;
        return false;
    }
    ++g_omxCoreMutex;
    return true;
}

void SPRDAVCDecoder::InitializeDecoderPorts() const
{
    SPRDAVCDecoder *self = const_cast<SPRDAVCDecoder *>(this);
    PortConfig portConfig = {
        DEFAULT_NUM_INPUT_BUFFERS,
        DEFAULT_NUM_INPUT_BUFFERS,
        DEFAULT_INPUT_BUFFER_SIZE,
        K_NUM_OUTPUT_BUFFERS,
        K_NUM_OUTPUT_BUFFERS,
        MEDIA_MIMETYPE_VIDEO_AVC,
        mFrameWidth,
        mFrameHeight,
        K_NUM_OUTPUT_BUFFERS,
    };
    self->InitPorts(portConfig);
    self->iUseOhosNativeBuffer[OMX_DirOutput] = OMX_FALSE;
    self->iUseOhosNativeBuffer[OMX_DirInput] = OMX_FALSE;
}

bool SPRDAVCDecoder::TryInitSoftwareDecoder()
{
    if (!OpenDecoder("libomx_avcdec_sw_sprd.z.so")) {
        OMX_LOGE("Failed to open libomx_avcdec_sw_sprd.z.so");
        mInitCheck = OMX_ErrorInsufficientResources;
        return false;
    }
    mDecoderSwFlag = true;
    if (initDecoder() == STATUS_OK) {
        return true;
    }
    mInitCheck = OMX_ErrorInsufficientResources;
    return false;
}

void SPRDAVCDecoder::InitializeCodecConfigBuffers()
{
    mSPSData = reinterpret_cast<uint8_t*>(malloc(H264_HEADER_SIZE));
    mPPSData = reinterpret_cast<uint8_t*>(malloc(H264_HEADER_SIZE));
    if (mSPSData == nullptr || mPPSData == nullptr) {
        mInitCheck = OMX_ErrorInsufficientResources;
    }
}

void SPRDAVCDecoder::ResetMbInfoBuffers()
{
    for (int i = 0; i < MAX_MBINFO_BUFFERS; i++) {
        mPmemMbInfo[i] = nullptr;
        mPmemMbInfoV[i] = nullptr;
        mPmemMbInfoP[i] = 0;
        mPbufMbInfoSize[i] = 0;
    }
}

OMX_ERRORTYPE SPRDAVCDecoder::adjustProfileLevelQuery(OMX_PTR params)
{
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel =
        static_cast<OMX_VIDEO_PARAM_PROFILELEVELTYPE *>(params);
    if (profileLevel->eProfile == OMX_VIDEO_AVCProfileHigh && mCapability.profile < AVC_HIGH) {
        profileLevel->eProfile = OMX_VIDEO_AVCProfileMain;
    }
    if (profileLevel->eProfile == OMX_VIDEO_AVCProfileMain && mCapability.profile < AVC_MAIN) {
        profileLevel->eProfile = OMX_VIDEO_AVCProfileBaseline;
    }
    size_t tableIndex = 1;
    for (; tableIndex < ((sizeof(G_CONVERSION_TABLE) / sizeof(G_CONVERSION_TABLE[0])) - 1); ++tableIndex) {
        if (G_CONVERSION_TABLE[tableIndex].avcLevel > mCapability.level) {
            --tableIndex;
            break;
        }
    }
    if (profileLevel->eLevel > G_CONVERSION_TABLE[tableIndex].omxLevel) {
        profileLevel->eLevel = G_CONVERSION_TABLE[tableIndex].omxLevel;
    }
    return OMX_ErrorNone;
}

void SPRDAVCDecoder::initOmxHeader(
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

OMX_ERRORTYPE SPRDAVCDecoder::initOutputPrivate(
    OMX_BUFFERHEADERTYPE *header, BufferPrivateStruct *bufferPrivate)
{
    header->pOutputPortPrivate = new BufferCtrlStruct;
    if (header->pOutputPortPrivate == nullptr) {
        return OMX_ErrorUndefined;
    }
    BufferCtrlStruct* pBufCtrl = reinterpret_cast<BufferCtrlStruct*>(header->pOutputPortPrivate);
    pBufCtrl->iRefCount = MIN_OUTPUT_BUFFER_COUNT;
    pBufCtrl->id = mIOMMUID;
    pBufCtrl->pMem = nullptr;
    pBufCtrl->phyAddr = 0;
    pBufCtrl->bufferSize = 0;
    pBufCtrl->bufferFd = 0;
    if (mAllocateBuffers && bufferPrivate != nullptr) {
        pBufCtrl->pMem = bufferPrivate->pMem;
        pBufCtrl->phyAddr = bufferPrivate->phyAddr;
        pBufCtrl->bufferSize = bufferPrivate->bufferSize;
        pBufCtrl->bufferFd = bufferPrivate->bufferFd;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoder::initSecureInputPrivate(
    OMX_BUFFERHEADERTYPE *header, BufferPrivateStruct *bufferPrivate)
{
    header->pInputPortPrivate = new BufferCtrlStruct;
    if (header->pInputPortPrivate == nullptr) {
        return OMX_ErrorUndefined;
    }
    BufferCtrlStruct* pBufCtrl = reinterpret_cast<BufferCtrlStruct*>(header->pInputPortPrivate);
    pBufCtrl->pMem = nullptr;
    pBufCtrl->phyAddr = 0;
    pBufCtrl->bufferSize = 0;
    pBufCtrl->bufferFd = 0;
    if (bufferPrivate != nullptr) {
        pBufCtrl->pMem = bufferPrivate->pMem;
        pBufCtrl->phyAddr = bufferPrivate->phyAddr;
        pBufCtrl->bufferSize = bufferPrivate->bufferSize;
    }
    return OMX_ErrorNone;
}

void SPRDAVCDecoder::addBufferToPort(
    OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header, OMX_U8 *ptr, OMX_U32 size)
{
    PortInfo *port = editPortInfo(portIndex);
    port->mBuffers.push_back(BufferInfo());
    BufferInfo *buffer = &port->mBuffers.at(port->mBuffers.size() - 1);
    OMX_LOGI("internalUseBuffer, portIndex= %d, header=%p, pBuffer=%p, size=%d", portIndex, header, ptr, size);
    buffer->mHeader = header;
    buffer->mOwnedByUs = false;
    if (port->mBuffers.size() == port->mDef.nBufferCountActual) {
        port->mDef.bPopulated = OMX_TRUE;
        CheckTransitions();
    }
}

bool SPRDAVCDecoder::LoadRequiredDecoderSymbols(const char *libName) const
{
    SPRDAVCDecoder *self = const_cast<SPRDAVCDecoder *>(this);
    return LoadDecoderSymbol(self->mLibHandle, libName, "H264DecInit", &self->mH264DecInit) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264DecGetInfo", &self->mH264DecGetInfo) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264DecDecode", &self->mH264DecDecode) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264DecRelease", &self->mH264DecRelease) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264Dec_SetCurRecPic", &self->mH264DecSetCurRecPic) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264Dec_GetLastDspFrm", &self->mH264DecGetLastDspFrm) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264Dec_ReleaseRefBuffers", &self->mH264DecReleaseRefBuffers) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264DecMemInit", &self->mH264DecMemInit) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264GetCodecCapability", &self->mH264GetCodecCapability) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264DecGetNALType", &self->mH264DecGetNALType) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264DecSetParameter", &self->mH264DecSetparam) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264Dec_get_iova", &self->mH264DecGetIOVA) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264Dec_free_iova", &self->mH264DecFreeIOVA) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264Dec_get_IOMMU_status", &self->mH264DecGetIOMMUStatus) &&
        LoadDecoderSymbol(self->mLibHandle, libName, "H264Dec_InitStructForNewSeq", &self->mH264DecInitStructForNewSeq);
}
SPRDAVCDecoder::~SPRDAVCDecoder()
{
    OMX_LOGI("Destruct SPRDAVCDecoder, this: %p, g_omxCoreMutex: %d", static_cast<void *>(this), g_omxCoreMutex);
    if (mDeintl) {
        freeDecOutputBuffer();
        mDeintl->mDone = true;
        mDeintl->mDeinterReadyCondition.notify_one();
        delete mDeintl;
        mDeintl = nullptr;
    }
    ReleaseDecoder();

    if (mSPSData != nullptr) {
        free(mSPSData);
        mSPSData = nullptr;
    }
    if (mPPSData != nullptr) {
        free(mPPSData);
        mPPSData = nullptr;
    }
    if (mHandle != nullptr) {
        delete mHandle;
        mHandle = nullptr;
    }
    CloseDumpFiles();
    g_omxCoreMutex--;
}
OMX_ERRORTYPE SPRDAVCDecoder::initCheck() const
{
    OMX_LOGI("%s, mInitCheck: 0x%x", __FUNCTION__, mInitCheck);
    return mInitCheck;
}

OMX_ERRORTYPE SPRDAVCDecoder::allocateStreamBuffer(OMX_U32 bufferSize)
{
    unsigned long phyAddr = 0;
    size_t size = 0;

    if (bufferSize == 0 || bufferSize > UINT32_MAX) {
        OMX_LOGE("allocateStreamBuffer invalid bufferSize: %u", bufferSize);
        return OMX_ErrorInsufficientResources;
    }

    if (mDecoderSwFlag) {
        OMX_LOGE("WARNING---Softwware Codec is NOT expected");
        mPbufStreamV = static_cast<uint8_t*>(malloc(bufferSize * sizeof(unsigned char)));
        mPbufStreamP = 0;
        mPbufStreamSize = bufferSize;
    } else {
        mPmemStream = new VideoMemAllocator(bufferSize, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
        int fd = mPmemStream->getHeapID();
        if (fd < STATUS_CODE_OK) {
            OMX_LOGE("Failed to alloc bitstream pmem buffer, getHeapID failed");
            return OMX_ErrorInsufficientResources;
        }
        int ret = mIOMMUEnabled ? (*mH264DecGetIOVA)(mHandle, fd, &phyAddr, &size, true) :
            mPmemStream->getPhyAddrFromMemAlloc(&phyAddr, &size);
        if (ret < STATUS_CODE_OK) {
            OMX_LOGE("Failed to alloc bitstream pmem buffer, get phy addr failed: %d(%s)", ret, strerror(errno));
            return OMX_ErrorInsufficientResources;
        }
        mPbufStreamV = static_cast<uint8_t*>(mPmemStream->getBase());
        mPbufStreamP = phyAddr;
        mPbufStreamSize = size;
    }
    OMX_LOGI("%s, 0x%lx - %p - %zd", __FUNCTION__, mPbufStreamP,
        mPbufStreamV, mPbufStreamSize);
    return OMX_ErrorNone;
}
void SPRDAVCDecoder::ReleaseStreamBuffer()
{
    OMX_LOGI("%s, 0x%lx - %p - %zd", __FUNCTION__, mPbufStreamP,
        mPbufStreamV, mPbufStreamSize);
    if (mPbufStreamV != nullptr) {
        if (mDecoderSwFlag) {
            free(mPbufStreamV);
            mPbufStreamV = nullptr;
        } else {
            if (mIOMMUEnabled) {
                (*mH264DecFreeIOVA)(mHandle, mPbufStreamP, mPbufStreamSize, true);
            }
            releaseMemIon(&mPmemStream);
            mPbufStreamV = nullptr;
            mPbufStreamP = 0;
            mPbufStreamSize = 0;
        }
    }
}
bool SPRDAVCDecoder::outputBuffersNotEnough(const H264SwDecInfo *info, OMX_U32 bufferCountMin,
                                            OMX_U32 bufferCountActual, OMX_BOOL useNativeBuffer)
{
    if (mNeedDeinterlace && !mIsInterlacedSequence) {
        return false;
    }
    if (useNativeBuffer) {
        if (info->numRefFrames + info->hasBFrames+ MIN_OUTPUT_BUFFER_COUNT > bufferCountMin) {
            return true;
        }
    } else {
        if (info->numRefFrames + info->hasBFrames + MIN_OUTPUT_BUFFER_COUNT +
            ADDITIONAL_OUTPUT_BUFFERS > bufferCountActual) {
            return true;
        }
    }
    return false;
}
status_t SPRDAVCDecoder::initDecoder()
{
    OMX_LOGI(" SPRDAVCDecoder::initDecoder() start");
    InitDecoderHandle();
    uint32_t sizeInter = H264_DECODER_INTERNAL_BUFFER_SIZE;
    mCodecInterBuffer = reinterpret_cast<uint8_t*>(malloc(sizeInter));
    if (mCodecInterBuffer == nullptr) {
        OMX_LOGE("mCodecInterBuffer is nullptr");
    }
    OMX_ERRORTYPE err = initDecoderInstance();
    if (err != OMX_ErrorNone) {
        return err;
    }
    UpdateIommuStatus();
    return allocateStreamBuffer(H264_DECODER_STREAM_BUFFER_SIZE);
}

void SPRDAVCDecoder::InitDecoderHandle()
{
    (void)memset_s(mHandle, sizeof(TagAvcHandle), 0, sizeof(TagAvcHandle));
    mHandle->userdata = static_cast<void*>(this);
    mHandle->vspBindcb = BindFrameWrapper;
    mHandle->vspUnbindcb = UnbindFrameWrapper;
    mHandle->vspExtmemcb = ExtMemAllocWrapper;
    mHandle->vspMbinfomemcb = MbinfoMemAllocWrapper;
}

OMX_ERRORTYPE SPRDAVCDecoder::initDecoderInstance()
{
    MMCodecBuffer camelBack;
    MMDecVideoFormat videoFormat;
    MMDecCodecPriority codecPriority;
    InitDecoderInitParams(camelBack, videoFormat, codecPriority);
    OMX_LOGI("start to use mH264DecInit");
    int myRes = TryInitDecoderInstance(camelBack, videoFormat, codecPriority);
    if (myRes != MMDEC_OK) {
        OMX_LOGE("Failed to init AVCDEC, %d", myRes);
        return OMX_ErrorHardware;
    }
    if ((*mH264GetCodecCapability)(mHandle, &mCapability) != MMDEC_OK) {
        OMX_LOGE("Failed to mH264GetCodecCapability");
        return OMX_ErrorUndefined;
    }
    OMX_LOGI("initDecoder, capability: profile %d, level %d, max wh=%d %d",
        mCapability.profile, mCapability.level, mCapability.maxWidth, mCapability.maxHeight);
    return OMX_ErrorNone;
}

void SPRDAVCDecoder::InitDecoderInitParams(
    MMCodecBuffer &camelBack, MMDecVideoFormat &videoFormat, MMDecCodecPriority &codecPriority) const
{
    uint32_t sizeInter = H264_DECODER_INTERNAL_BUFFER_SIZE;
    camelBack.commonBufferPtr = mCodecInterBuffer;
    camelBack.commonBufferPtrPhy = 0;
    camelBack.size = sizeInter;
    camelBack.intBufferPtr = nullptr;
    camelBack.intSize = 0;
    videoFormat.videoStd = H264;
    videoFormat.frameWidth = 0;
    videoFormat.frameHeight = 0;
    videoFormat.pExtra = nullptr;
    videoFormat.pExtraPhy = 0;
    videoFormat.iExtra = 0;
    videoFormat.yuvFormat = YUV420SP_NV12;
    codecPriority.priority = ERROR_CODE_INVALID;
    codecPriority.operatingRate = STATUS_CODE_OK;
}

int SPRDAVCDecoder::TryInitDecoderInstance(
    MMCodecBuffer &camelBack, MMDecVideoFormat &videoFormat, MMDecCodecPriority &codecPriority) const
{
    int myRes = MMDEC_ERROR;
    int tryTimes = STATUS_CODE_OK;
    while (myRes != MMDEC_OK && tryTimes < DECODER_INIT_MAX_RETRIES) {
        myRes = (*mH264DecInit)(mHandle, &camelBack, &videoFormat, &codecPriority);
        if (myRes != MMDEC_OK) {
            usleep(DECODER_INIT_RETRY_DELAY_US);
        }
        OMX_LOGE("tryTimes = %d", tryTimes);
        ++tryTimes;
    }
    return myRes;
}

void SPRDAVCDecoder::ReleaseDecoder(bool isNeedRelease)
{
    ReleaseStreamBuffer();
    if (mPbufExtraV != nullptr) {
        if (mIOMMUEnabled && isNeedRelease) {
            (*mH264DecFreeIOVA)(mHandle, mPbufExtraP, mPbufExtraSize, true);
        }
        releaseMemIon(&mPmemExtra);
        mPbufExtraV = nullptr;
        mPbufExtraP = 0;
        mPbufExtraSize = 0;
    }
    for (int i = 0; i < MAX_MBINFO_BUFFERS; ++i) {
        if (mPmemMbInfoV[i] == nullptr) {
            continue;
        }
        if (mIOMMUEnabled && isNeedRelease) {
            (*mH264DecFreeIOVA)(mHandle, mPmemMbInfoP[i], mPbufMbInfoSize[i], true);
        }
        releaseMemIon(&mPmemMbInfo[i]);
        mPmemMbInfoV[i] = nullptr;
        mPmemMbInfoP[i] = 0;
        mPbufMbInfoSize[i] = 0;
    }
    mPbufMbInfoIdx = 0;
    if (mH264DecRelease != nullptr && isNeedRelease) {
        (*mH264DecRelease)(mHandle);
    }
    if (mCodecInterBuffer != nullptr) {
        free(mCodecInterBuffer);
        mCodecInterBuffer = nullptr;
    }
    if (mCodecExtraBuffer != nullptr) {
        free(mCodecExtraBuffer);
        mCodecExtraBuffer = nullptr;
    }
    if (mLibHandle != nullptr) {
        dlclose(mLibHandle);
        mLibHandle = nullptr;
        mH264DecReleaseRefBuffers = nullptr;
        mH264DecRelease = nullptr;
    }
}

OMX_ERRORTYPE SPRDAVCDecoder::internalGetParameter(OMX_INDEXTYPE index, OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_IndexParamVideoProfileLevelQuerySupported: {
            OMX_ERRORTYPE err = SprdVideoDecoderBase::internalGetParameter(index, params);
            return err == OMX_ErrorNone ? adjustProfileLevelQuery(params) : err;
        }
        case OMX_IndexParamPortDefinition: {
            OMX_PARAM_PORTDEFINITIONTYPE *defParams =
                static_cast<OMX_PARAM_PORTDEFINITIONTYPE *>(params);
            if (defParams->nPortIndex > 1 || defParams->nSize != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
                return OMX_ErrorUndefined;
            }
            PortInfo *port = editPortInfo(defParams->nPortIndex);
            {
                std::lock_guard<std::mutex> autoLock(mLock);
                errno_t ret = memmove_s(defParams, sizeof(OMX_PARAM_PORTDEFINITIONTYPE),
                                        &port->mDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                if (ret != 0) {
                    OMX_LOGE("memmove_s failed at line %d, ret=%d", __LINE__, ret);
                    return OMX_ErrorUndefined;
                }
            }
            return OMX_ErrorNone;
        }
        default:
            return SprdVideoDecoderBase::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDAVCDecoder::updatePortDefinition(OMX_PARAM_PORTDEFINITIONTYPE *defParams, PortInfo *port)
{
    if (defParams->nBufferCountActual == MIN_OUTPUT_BUFFER_COUNT) {
        mThumbnailMode = OMX_TRUE;
    } else if (defParams->nBufferCountActual < port->mDef.nBufferCountMin) {
        OMX_LOGW("component requires at least %u buffers (%u requested)",
            port->mDef.nBufferCountMin, defParams->nBufferCountActual);
        return OMX_ErrorUnsupportedSetting;
    } else if (port->mDef.nBufferCountActual < defParams->nBufferCountActual) {
        port->mDef.nBufferCountActual = defParams->nBufferCountActual;
    }
    uint32_t oldWidth = port->mDef.format.video.nFrameWidth;
    uint32_t oldHeight = port->mDef.format.video.nFrameHeight;
    uint32_t newWidth = defParams->format.video.nFrameWidth;
    uint32_t newHeight = defParams->format.video.nFrameHeight;
    if (!(newWidth <= mCapability.maxWidth && newHeight <= mCapability.maxHeight) &&
        !(newWidth <= mCapability.maxHeight && newHeight <= mCapability.maxWidth)) {
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

OMX_ERRORTYPE SPRDAVCDecoder::internalSetParameter(OMX_INDEXTYPE index, const OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_IndexParamPortDefinition: {
            OMX_PARAM_PORTDEFINITIONTYPE *defParams =
                (OMX_PARAM_PORTDEFINITIONTYPE *)params;
            if (defParams->nPortIndex > 1) {
                return OMX_ErrorBadPortIndex;
            }
            if (defParams->nSize != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
                return OMX_ErrorUnsupportedSetting;
            }
            PortInfo *port = editPortInfo(defParams->nPortIndex);
            if (defParams->nBufferSize > port->mDef.nBufferSize) {
                port->mDef.nBufferSize = defParams->nBufferSize;
            }
            return updatePortDefinition(defParams, port);
        }
        default:
            return SprdVideoDecoderBase::internalSetParameter(index, params);
    }
}

void SPRDAVCDecoder::applyPortGeometryChange(OMX_U32 portIndex, uint32_t newWidth, uint32_t newHeight)
{
    if (portIndex != K_OUTPUT_PORT_INDEX) {
        PortInfo *port = editPortInfo(portIndex);
        port->mDef.format.video.nFrameWidth = newWidth;
        port->mDef.format.video.nFrameHeight = newHeight;
        return;
    }
    mFrameWidth = newWidth;
    mFrameHeight = newHeight;
    mStride = (newWidth >= UHD_WIDTH_THRESHOLD || newHeight >= UHD_HEIGHT_THRESHOLD) ?
        Align(newWidth, MEMORY_ALIGNMENT_128) : Align(newWidth, MEMORY_ALIGNMENT_16);
    mSliceHeight = (newWidth >= UHD_WIDTH_THRESHOLD || newHeight >= UHD_HEIGHT_THRESHOLD) ?
        Align(newHeight, MEMORY_ALIGNMENT_32) : Align(newHeight, MEMORY_ALIGNMENT_16);
    mPictureSize = mStride * mSliceHeight * YUV420_FRAME_SIZE_MULTIPLIER / YUV420_FRAME_SIZE_DIVISOR;
    if (mFbcMode == AFBC) {
        int headerSize = (mStride * mSliceHeight / MEMORY_ALIGNMENT_16 + KBYTE_ALIGNMENT_1024 - 1) &
            (~(KBYTE_ALIGNMENT_1024 - 1));
        mPictureSize += headerSize;
    }
    UpdatePortDefinitions(true, true);
}

void SPRDAVCDecoder::EnsureStreamBufferCapacity()
{
    PortInfo *inPort = editPortInfo(K_INPUT_PORT_INDEX);
    if (inPort->mDef.nBufferSize > mPbufStreamSize) {
        ReleaseStreamBuffer();
        allocateStreamBuffer(inPort->mDef.nBufferSize + H264_START_CODE_SIZE);
    }
}
OMX_ERRORTYPE SPRDAVCDecoder::internalUseBuffer(
    OMX_BUFFERHEADERTYPE **header, const UseBufferParams &params)
{
    *header = new OMX_BUFFERHEADERTYPE;
    initOmxHeader(*header, params.portIndex, params.appPrivate, params.size, params.ptr);
    if (params.portIndex == OMX_DirOutput) {
        OMX_ERRORTYPE err = initOutputPrivate(*header, params.bufferPrivate);
        if (err != OMX_ErrorNone) {
            return err;
        }
    } else if (params.portIndex == OMX_DirInput && mSecureFlag) {
        OMX_ERRORTYPE err = initSecureInputPrivate(*header, params.bufferPrivate);
        if (err != OMX_ErrorNone) {
            return err;
        }
    }
    addBufferToPort(params.portIndex, *header, params.ptr, params.size);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoder::allocateBuffer(
    OMX_BUFFERHEADERTYPE **header,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    OMX_U32 size)
{
    OMX_LOGI("SPRDAVCDecoder::%s, portIndex: %d, size: %d", __FUNCTION__, portIndex, size);
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

OMX_ERRORTYPE SPRDAVCDecoder::allocateSecureInputBuffer(
    OMX_BUFFERHEADERTYPE **header, OMX_PTR appPrivate, OMX_U32 size)
{
    size_t alignedSize = (size + KBYTE_ALIGNMENT_4 - 1) & ~(KBYTE_ALIGNMENT_4 - 1);
    size_t bufferSize = 0;
    unsigned long phyAddr = 0;
    MemIon* pMem = new VideoMemAllocator(alignedSize, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    int fd = pMem->getHeapID();
    if (fd < STATUS_CODE_OK || pMem->getPhyAddrFromMemAlloc(&phyAddr, &bufferSize)) {
        return OMX_ErrorInsufficientResources;
    }
    BufferPrivateStruct* bufferPrivate = new BufferPrivateStruct();
    bufferPrivate->pMem = pMem;
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

OMX_ERRORTYPE SPRDAVCDecoder::allocateOutputBufferInternal(
    OMX_BUFFERHEADERTYPE **header, OMX_PTR appPrivate, OMX_U32 size)
{
    mAllocateBuffers = true;
    if (mDecoderSwFlag || mChangeToSwDec) {
        return SprdSimpleOMXComponent::allocateBuffer(header, OMX_DirOutput, appPrivate, size);
    }
    size_t alignedSize = (size + KBYTE_ALIGNMENT_4 - 1) & ~(KBYTE_ALIGNMENT_4 - 1);
    size_t bufferSize = 0;
    unsigned long phyAddr = 0;
    MemIon* pMem = nullptr;
    int fd = STATUS_CODE_ERROR;
    OMX_ERRORTYPE err = allocateOutputMemory(alignedSize, pMem, fd, phyAddr, bufferSize);
    if (err != OMX_ErrorNone) {
        return err;
    }
    BufferPrivateStruct* bufferPrivate = new BufferPrivateStruct();
    initBufferPrivate(bufferPrivate, pMem, phyAddr, bufferSize, fd);
    UseBufferParams params = {
        OMX_DirOutput,
        appPrivate,
        static_cast<OMX_U32>(bufferSize),
        reinterpret_cast<OMX_U8 *>(pMem->getBase()),
        bufferPrivate,
    };
    SprdSimpleOMXComponent::useBuffer(header, params);
    delete bufferPrivate;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoder::allocateOutputMemory(
    size_t alignedSize, MemIon *&memory, int &fd, unsigned long &phyAddr, size_t &bufferSize)
{
    memory = new VideoMemAllocator(alignedSize, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    fd = memory->getHeapID();
    if (fd < STATUS_CODE_OK) {
        return OMX_ErrorInsufficientResources;
    }
    int ret = mIOMMUEnabled ? (*mH264DecGetIOVA)(mHandle, fd, &phyAddr, &bufferSize, true) :
        memory->getPhyAddrFromMemAlloc(&phyAddr, &bufferSize);
    return ret == 0 ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
}

void SPRDAVCDecoder::initBufferPrivate(
    BufferPrivateStruct *bufferPrivate, MemIon *memory, unsigned long phyAddr, size_t bufferSize, int fd) const
{
    bufferPrivate->pMem = memory;
    bufferPrivate->phyAddr = phyAddr;
    bufferPrivate->bufferSize = bufferSize;
    bufferPrivate->bufferFd = fd;
}
OMX_ERRORTYPE SPRDAVCDecoder::freeBuffer(
    OMX_U32 portIndex,
    OMX_BUFFERHEADERTYPE *header)
{
    switch (portIndex) {
        case OMX_DirInput: {
            return mSecureFlag ? freeSecureInputBufferInternal(header) :
                SprdSimpleOMXComponent::freeBuffer(portIndex, header);
        }
        case OMX_DirOutput: {
            return freeOutputBufferInternal(portIndex, header);
        }
        default:
            return OMX_ErrorUnsupportedIndex;
    }
}

OMX_ERRORTYPE SPRDAVCDecoder::freeSecureInputBufferInternal(OMX_BUFFERHEADERTYPE *header)
{
    BufferCtrlStruct* bufferCtrl = reinterpret_cast<BufferCtrlStruct*>(header->pInputPortPrivate);
    if (bufferCtrl == nullptr || bufferCtrl->pMem == nullptr) {
        OMX_LOGE("freeBuffer, intput pBufCtrl == nullptr or pBufCtrl->pMem == nullptr");
        return OMX_ErrorUndefined;
    }
    OMX_LOGI("freeBuffer, intput phyAddr: 0x%lx, pBuffer: %p", bufferCtrl->phyAddr, header->pBuffer);
    releaseMemIon(&bufferCtrl->pMem);
    return SprdSimpleOMXComponent::freeBuffer(OMX_DirInput, header);
}

void SPRDAVCDecoder::releaseOutputBufferResources(
    BufferCtrlStruct *bufferCtrl, OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header)
{
    if (mIOMMUEnabled && bufferCtrl->phyAddr > 0) {
        OMX_LOGI("freeBuffer, phyAddr: 0x%lx", bufferCtrl->phyAddr);
        if (mDeintl && mDecoderSwFlag) {
            mDeintl->VspFreeIova(bufferCtrl->phyAddr, bufferCtrl->bufferSize);
        } else {
            (*mH264DecFreeIOVA)(mHandle, bufferCtrl->phyAddr, bufferCtrl->bufferSize, true);
        }
    }
    if (bufferCtrl->pMem != nullptr) {
        releaseMemIon(&bufferCtrl->pMem);
        header->pBuffer = nullptr;
    }
    if (iUseOhosNativeBuffer[OMX_DirOutput]) {
        freePlatform(portIndex, header);
    }
}

OMX_ERRORTYPE SPRDAVCDecoder::freeOutputBufferInternal(OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header)
{
    BufferCtrlStruct* bufferCtrl = reinterpret_cast<BufferCtrlStruct*>(header->pOutputPortPrivate);
    if (bufferCtrl == nullptr) {
        OMX_LOGE("freeBuffer, pBufCtrl == nullptr");
        return OMX_ErrorUndefined;
    }
    releaseOutputBufferResources(bufferCtrl, portIndex, header);
    return SprdSimpleOMXComponent::freeBuffer(portIndex, header);
}
OMX_ERRORTYPE SPRDAVCDecoder::getConfig(OMX_INDEXTYPE index, OMX_PTR params)
{
    switch (index) {
        case OMX_IndexConfigCommonOutputCrop: {
            OMX_CONFIG_RECTTYPE *rectParams = (OMX_CONFIG_RECTTYPE *)params;
            if (rectParams->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }
            rectParams->nLeft = 0;
            rectParams->nTop = 0;
            {
                OMX_LOGI("%s, mCropWidth:%d, mCropHeight:%d",
                    __FUNCTION__, mCropWidth, mCropHeight);
                std::lock_guard<std::mutex> autoLock(mLock);
                rectParams->nWidth = mCropWidth;
                rectParams->nHeight = mCropHeight;
            }
            return OMX_ErrorNone;
        }
        default:
            return SprdVideoDecoderBase::getConfig(index, params);
    }
}
OMX_ERRORTYPE SPRDAVCDecoder::internalSetConfig(
    OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig)
{
    switch (static_cast<int>(index)) {
        case OMX_INDEX_CONFIG_THUMBNAIL_MODE: {
            OMX_BOOL *pEnable = (OMX_BOOL *)params;
            if (*pEnable == OMX_TRUE) {
                mThumbnailMode = OMX_TRUE;
            }
            OMX_LOGI("setConfig, mThumbnailMode = %d", mThumbnailMode);
            if (mThumbnailMode) {
                PortInfo *pInPort = editPortInfo(OMX_DirInput);
                PortInfo *pOutPort = editPortInfo(OMX_DirOutput);
                pInPort->mDef.nBufferCountActual = DEFAULT_NUM_OUTPUT_BUFFERS;
                pOutPort->mDef.nBufferCountActual = DEFAULT_NUM_OUTPUT_BUFFERS;
                MMDecVideoFormat videoFormat;
                videoFormat.thumbnail = MIN_OUTPUT_BUFFER_COUNT;
                (*mH264DecSetparam)(mHandle, &videoFormat);
            }
            return OMX_ErrorNone;
        }
        case OMX_INDEX_CONFIG_DEC_SCENE_MODE: {
            int *pDecSceneMode = reinterpret_cast<int*>(params);
            mDecSceneMode = *pDecSceneMode;
            if (mDecSceneMode == DEC_SCENE_MODE_VOLTE)
                mChangeToSwDec = true;
            SprdVideoDecoderBase::internalSetConfig(index, params, frameConfig);
            return OMX_ErrorNone;
        }
        default:
            return SprdVideoDecoderBase::internalSetConfig(index, params, frameConfig);
    }
}
void SPRDAVCDecoder::onDecodePrepare(OMX_BUFFERHEADERTYPE *header)
{
    if ((!mSecureFlag) && (header->nFlags & OMX_BUFFERFLAG_CODECCONFIG)) {
        findCodecConfigData(header);
    }
}

enum class DecodeLoopAction {
    PROCEED,
    CONTINUE,
    BREAK,
    RETURN,
};

bool SPRDAVCDecoder::HandleSwDecoderSwitch()
{
    if (!mChangeToSwDec) {
        return true;
    }
    mChangeToSwDec = false;
    OMX_LOGD("%s, %d, change to sw decoder, mThumbnailMode: %d", __FUNCTION__, __LINE__, mThumbnailMode);
    if (mIOMMUEnabled && (!mIsInterlacedSequence || !mNeedDeinterlace || mThumbnailMode)) {
        FreeOutputBufferIova();
    }
    if (mDeintl != nullptr) {
        if (!mDeinterInputBufQueue.empty()) {
            return false;
        }
        freeDecOutputBuffer();
        mDeintl->mDone = true;
        mDeintl->mDeinterReadyCondition.notify_one();
        delete mDeintl;
        mDeintl = nullptr;
    }
    ReleaseDecoder();
    const char *libName = (DEC_SCENE_MODE_VOLTE == mDecSceneMode) ?
        "libomx_avcdec_vt_sprd.z.so" : "libomx_avcdec_sw_sprd.z.so";
    if (!OpenDecoder(libName) || initDecoder() != STATUS_OK) {
        notify(OMX_EventError, OMX_ErrorDynamicResourcesUnavailable, 0, nullptr);
        mSignalledError = true;
        mDecoderSwFlag = false;
        return false;
    }
    mDecoderSwFlag = true;
    mDecSceneMode = 0;
    if (mThumbnailMode) {
        MMDecVideoFormat videoFormat;
        videoFormat.thumbnail = MIN_OUTPUT_BUFFER_COUNT;
        (*mH264DecSetparam)(mHandle, &videoFormat);
    }
    return true;
}

bool SPRDAVCDecoder::acquireAvailableOutputHeader(
    std::list<BufferInfo*>& outQueue, OMX_BUFFERHEADERTYPE *&outHeader, BufferCtrlStruct *&bufferCtrl)
{
    std::lock_guard<std::mutex> autoLock(mThreadLock);
    auto itBuffer = mDeintl ? mDecOutputBufQueue.begin() : outQueue.begin();
    size_t count = 0;
    while (count < (mDeintl ? mDecOutputBufQueue.size() : outQueue.size())) {
        outHeader = (*itBuffer)->mHeader;
        bufferCtrl = reinterpret_cast<BufferCtrlStruct*>(outHeader->pOutputPortPrivate);
        if (bufferCtrl == nullptr) {
            notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
            mSignalledError = true;
            return false;
        }
        ++itBuffer;
        ++count;
        if (bufferCtrl->iRefCount <= 0) {
            return true;
        }
    }
    return false;
}

bool SPRDAVCDecoder::prepareInputHeader(
    std::list<BufferInfo*>& inQueue, BufferInfo *&inInfo, OMX_BUFFERHEADERTYPE *&inHeader)
{
    if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
        mEOSStatus = INPUT_EOS_SEEN;
        if (mDeintl) {
            mDeintlNeedRestart = true;
        }
    }
    if (inHeader->nFilledLen != 0) {
        return true;
    }
    mFrameDecoded = true;
    releaseCurrentInputBuffer(inQueue, inInfo, inHeader);
    return false;
}

void SPRDAVCDecoder::releaseCurrentInputBuffer(
    std::list<BufferInfo*>& inQueue, BufferInfo *&inInfo, OMX_BUFFERHEADERTYPE *&inHeader)
{
    inInfo->mOwnedByUs = false;
    inQueue.erase(inQueue.begin());
    notifyEmptyBufferDone(inHeader);
    inInfo = nullptr;
    inHeader = nullptr;
}

void SPRDAVCDecoder::selectInputBitstreamSource(
    OMX_BUFFERHEADERTYPE *inHeader, uint8_t *&bitstream, uint32_t &bufferSize)
{
    bitstream = inHeader->pBuffer + inHeader->nOffset;
    bufferSize = inHeader->nFilledLen;
    if (mSecureFlag || (inHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG)) {
        return;
    }
    if (!mDecoderSawSPS && mSPSDataSize > 0) {
        bitstream = mSPSData;
        bufferSize = mSPSDataSize;
        mIsResume = true;
    } else if (!mDecoderSawPPS && mPPSDataSize > 0) {
        bitstream = mPPSData;
        bufferSize = mPPSDataSize;
        mIsResume = true;
    }
}

void SPRDAVCDecoder::initDecoderInputBuffer(MMDecInput &decIn, OMX_BUFFERHEADERTYPE *inHeader)
{
    if (!mSecureFlag) {
        decIn.pStream = mPbufStreamV;
        decIn.pStreamPhy = mPbufStreamP;
    } else {
        BufferCtrlStruct* bufCtrl = reinterpret_cast<BufferCtrlStruct*>(inHeader->pInputPortPrivate);
        mPbufStreamV = reinterpret_cast<uint8*>(bufCtrl->pMem->getBase());
        decIn.pStream = mPbufStreamV;
        decIn.pStreamPhy = reinterpret_cast<unsigned long>(bufCtrl->phyAddr);
    }
    decIn.beLastFrame = 0;
    decIn.expectedIVOP = mNeedIVOP;
    decIn.beDisplayed = MIN_OUTPUT_BUFFER_COUNT;
    decIn.errPktNum = 0;
    decIn.nTimeStamp = static_cast<uint64>(inHeader->nTimeStamp);
    decIn.fbcMode = mFbcMode;
}

void SPRDAVCDecoder::copyInputDataToDecoder(
    MMDecInput &decIn, OMX_BUFFERHEADERTYPE *inHeader, uint8_t *bitstream, uint32_t bufferSize,
    uint32_t &addStartcodeLen)
{
    if (mSecureFlag) {
        decIn.dataLength = bufferSize;
        return;
    }
    uint32_t copyLen = (bufferSize <= mPbufStreamSize) ? bufferSize : mPbufStreamSize;
    if (mThumbnailMode) {
        uint32_t iIndex = 0;
        uint32_t remained = inHeader->nFilledLen - inHeader->nOffset;
        while (iIndex < remained && bitstream[iIndex] == H264_START_CODE_BYTE_0) {
            iIndex++;
        }
        if (iIndex < H264_START_CODE_MIN_INDEX || bitstream[iIndex] != H264_START_CODE_BYTE_3) {
            reinterpret_cast<uint8_t*>(mPbufStreamV)[ARRAY_INDEX_0] = H264_START_CODE_BYTE_0;
            reinterpret_cast<uint8_t*>(mPbufStreamV)[ARRAY_INDEX_1] = H264_START_CODE_BYTE_1;
            reinterpret_cast<uint8_t*>(mPbufStreamV)[ARRAY_INDEX_2] = H264_START_CODE_BYTE_2;
            reinterpret_cast<uint8_t*>(mPbufStreamV)[ARRAY_INDEX_3] = H264_START_CODE_BYTE_3;
            addStartcodeLen = H264_START_CODE_SIZE;
        }
        copyLen = (bufferSize <= mPbufStreamSize - addStartcodeLen) ? bufferSize :
            (mPbufStreamSize - addStartcodeLen);
    }
    if (mPbufStreamV != nullptr) {
        errno_t ret = memmove_s(mPbufStreamV + addStartcodeLen, mPbufStreamSize - addStartcodeLen,
                                bitstream, copyLen);
        if (ret != 0) {
            OMX_LOGE("memmove_s failed in line %d, ret=%d", __LINE__, ret);
        }
    }
    decIn.dataLength = copyLen + addStartcodeLen;
}

void SPRDAVCDecoder::prepareDecodeInput(
    OMX_BUFFERHEADERTYPE *inHeader, MMDecInput &decIn, MMDecOutput &decOut, uint32_t &addStartcodeLen)
{
    (void)memset_s(&decOut, sizeof(MMDecOutput), 0, sizeof(MMDecOutput));
    uint8_t *bitstream = nullptr;
    uint32_t bufferSize = 0;
    selectInputBitstreamSource(inHeader, bitstream, bufferSize);
    initDecoderInputBuffer(decIn, inHeader);
    decOut.frameEffective = 0;
    decOut.hasPic = 0;
    copyInputDataToDecoder(decIn, inHeader, bitstream, bufferSize, addStartcodeLen);
}

bool SPRDAVCDecoder::prepareOutputFrameBuffer(
    OMX_BUFFERHEADERTYPE *outHeader, BufferCtrlStruct *bufferCtrl, OMX_BUFFERHEADERTYPE *inHeader)
{
    outHeader->nTimeStamp = inHeader->nTimeStamp;
    outHeader->nFlags = inHeader->nFlags & (~OMX_BUFFERFLAG_EOS);
    unsigned long picPhyAddr = 0;
    if (!mDecoderSwFlag) {
        picPhyAddr = bufferCtrl->phyAddr;
        if (picPhyAddr == 0 && mIOMMUEnabled) {
            notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
            mSignalledError = true;
            return false;
        }
        if (mAllocateBuffers) {
            bufferCtrl->pMem->cpuSyncStart();
        }
    }
    if (mDeintl) {
        (*mH264DecSetCurRecPic)(mHandle, reinterpret_cast<uint8*>(outHeader->pBuffer),
            reinterpret_cast<uint8*>(picPhyAddr), reinterpret_cast<void*>(outHeader), picId, 0);
        return true;
    }
    uint8 *yuv = reinterpret_cast<uint8 *>(outHeader->pBuffer + outHeader->nOffset);
    (*mH264DecSetCurRecPic)(mHandle, yuv, reinterpret_cast<uint8*>(picPhyAddr),
        reinterpret_cast<void*>(outHeader), picId, 0);
    return true;
}

MMDecRet SPRDAVCDecoder::decodeFrame(OMX_BUFFERHEADERTYPE *inHeader, OMX_BUFFERHEADERTYPE *outHeader,
    BufferCtrlStruct *bufferCtrl, MMDecInput &decIn, MMDecOutput &decOut)
{
    if (!prepareOutputFrameBuffer(outHeader, bufferCtrl, inHeader)) {
        return MMDEC_MEMORY_ERROR;
    }
    if (mDumpStrmEnabled && inHeader->nOffset == 0) {
        DumpFiles(mPbufStreamV, decIn.dataLength, DUMP_STREAM);
    }
    int64_t startDecode = systemTime();
    MMDecRet decRet = (*mH264DecDecode)(mHandle, &decIn, &decOut);
    int64_t endDecode = systemTime();
    OMX_LOGD("%s, %d, decRet: %d, %dms, decOut.frameEffective: %d, needIvoP: %d, in { %p, 0x%lx}, "
        "consume byte: %u, flag:0x%x, SPS:%d, PPS:%d, pts:%lld",
        __FUNCTION__, __LINE__, decRet, (unsigned int)((endDecode - startDecode) / (USEC_PER_MSEC * USEC_PER_MSEC)),
        decOut.frameEffective, mNeedIVOP, decIn.pStream, decIn.pStreamPhy, decIn.dataLength, inHeader->nFlags,
        decOut.sawSPS, decOut.sawPPS, decOut.pts);
    mDecoderSawSPS = decOut.sawSPS;
    mDecoderSawPPS = decOut.sawPPS;
    return decRet;
}

int SPRDAVCDecoder::HandleDecodeResult(
    MMDecRet decRet, std::list<BufferInfo*>& inQueue, BufferInfo *&inInfo, OMX_BUFFERHEADERTYPE *&inHeader)
{
    if (decRet == MMDEC_OK) {
        mNeedIVOP = false;
        return static_cast<int>(DecodeLoopAction::PROCEED);
    }
    mNeedIVOP = true;
    if (decRet == MMDEC_MEMORY_ERROR) {
        if (mDecoderSwFlag) {
            notify(OMX_EventError, OMX_ErrorInsufficientResources, 0, nullptr);
            mSignalledError = true;
        } else {
            mChangeToSwDec = true;
            mDecoderSawSPS = false;
            mDecoderSawPPS = false;
            mIsResume = false;
        }
        return static_cast<int>(DecodeLoopAction::RETURN);
    }
    if (decRet == MMDEC_NOT_SUPPORTED) {
        notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, nullptr);
        mSignalledError = true;
        return static_cast<int>(DecodeLoopAction::RETURN);
    }
    if (decRet == MMDEC_STREAM_ERROR && mThumbnailMode) {
        mFrameDecoded = true;
        releaseCurrentInputBuffer(inQueue, inInfo, inHeader);
        return static_cast<int>(DecodeLoopAction::BREAK);
    }
    return static_cast<int>(DecodeLoopAction::PROCEED);
}

bool SPRDAVCDecoder::ProcessDecoderInfo(std::list<BufferInfo*>& outQueue)
{
    (void)memset_s(&mDecoderInfo, sizeof(H264SwDecInfo), 0, sizeof(H264SwDecInfo));
    MMDecRet ret = (*mH264DecGetInfo)(mHandle, &mDecoderInfo);
    GetVuiParams(&mDecoderInfo);
    if (ret != MMDEC_OK) {
        OMX_LOGE("failed to get decoder information.");
        return true;
    }
    if (!ValidateDecoderDimensions()) {
        return false;
    }
#ifndef VSP_NO_DEINT
    mNeedDeinterlace = (mDecoderInfo.needDeinterlace != 0);
#else
    mNeedDeinterlace = false;
#endif
    mNeedDeinterlace = false;
    initDeinterlaceIfNeeded(outQueue);
    if (HandlePortSettingChangeEvent(&mDecoderInfo) || mChangeToSwDec) {
        return false;
    }
    return true;
}

bool SPRDAVCDecoder::ValidateDecoderDimensions()
{
    bool withinPortrait = (mDecoderInfo.picWidth <= mCapability.maxWidth &&
        mDecoderInfo.picHeight <= mCapability.maxHeight);
    bool withinLandscape = (mDecoderInfo.picWidth <= mCapability.maxHeight &&
        mDecoderInfo.picHeight <= mCapability.maxWidth);
    if (withinPortrait || withinLandscape) {
        return true;
    }
    notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, nullptr);
    mSignalledError = true;
    return false;
}

void SPRDAVCDecoder::initDeinterlaceIfNeeded(std::list<BufferInfo*>& outQueue)
{
    if (mDeintInitialized || mThumbnailMode || !mNeedDeinterlace) {
        return;
    }
    mDeintInitialized = true;
    DeinterlaceInfo info;
    info.width = mStride;
    info.height = mSliceHeight;
    info.decoder = this;
    info.callback = DrainOneOutputBuffercallback;
    info.threadLock = &mThreadLock;
    mDeintl = new SPRDDeinterlace((void*)&mDecOutputBufQueue, (void*)&mDeinterInputBufQueue, (void*)&outQueue, info);
    mDeintl->mUseNativeBuffer = iUseOMXNativeBuffer[OMX_DirOutput];
    mDeintl->StartDeinterlaceThread();
    allocateDecOutputBuffer(&mDecoderInfo);
}

void SPRDAVCDecoder::FinalizeInputFrame(FinalizeInputContext &context)
{
    if (mIsResume) {
        if ((context.decRet != MMDEC_OK && context.decRet != MMDEC_FRAME_SEEK_IVOP) || context.infoRet != MMDEC_OK) {
            mSPSDataSize = 0;
            mPPSDataSize = 0;
        }
        mIsResume = false;
        return;
    }
    if (context.decIn->dataLength > (context.inHeader->nFilledLen + context.addStartcodeLen)) {
        return;
    }
    uint32_t bufferSize = context.decIn->dataLength;
    context.inHeader->nOffset += bufferSize;
    int32 nFilledRemained = context.inHeader->nFilledLen - bufferSize;
    context.inHeader->nFilledLen -= bufferSize;
    if (nFilledRemained <= 0) {
        mFrameDecoded = true;
        context.inHeader->nOffset = 0;
        releaseCurrentInputBuffer(*context.inQueue, context.inInfo, context.inHeader);
    }
}

void SPRDAVCDecoder::drainDecodedFrames(std::list<BufferInfo*>& outQueue, MMDecOutput &decOut)
{
    if (mDeintl) {
        while (!mDecOutputBufQueue.empty() && mHeadersDecoded && decOut.frameEffective) {
            DrainOneDeintlBuffer(decOut.pBufferHeader, decOut.pts);
            decOut.frameEffective = false;
        }
        return;
    }
    while (!outQueue.empty() && mHeadersDecoded && decOut.frameEffective) {
        drainOneOutputBuffer(decOut.picId, decOut.pBufferHeader, decOut.pts);
        if (mDumpYUVEnabled) {
            DumpFiles(decOut.pOutFrameY, mPictureSize, DUMP_YUV);
        }
        decOut.frameEffective = false;
        if (mThumbnailMode) {
            mStopDecode = true;
        }
    }
}

bool SPRDAVCDecoder::shouldContinueDecodeLoop(
    const std::list<BufferInfo*>& inQueue, const std::list<BufferInfo*>& outQueue)
{
    return !mStopDecode && (mEOSStatus != INPUT_DATA_AVAILABLE || !inQueue.empty()) &&
        (!outQueue.empty() || (mDeintl && !mDecOutputBufQueue.empty()));
}

bool SPRDAVCDecoder::processQueuedFrames(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue)
{
    while (shouldContinueDecodeLoop(inQueue, outQueue)) {
        if (mEOSStatus == INPUT_EOS_SEEN && mFrameDecoded) {
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

void SPRDAVCDecoder::onQueueFilled(OMX_U32 portIndex)
{
    if (ShouldSkipQueueFill(portIndex)) {
        return;
    }
    std::list<BufferInfo*>& inQueue = getPortQueue(K_INPUT_PORT_INDEX);
    std::list<BufferInfo*>& outQueue = getPortQueue(K_OUTPUT_PORT_INDEX);
    if (!processQueuedFrames(inQueue, outQueue)) {
        return;
    }
    if (mEOSStatus == INPUT_EOS_SEEN) {
        DrainAllOutputBuffers();
        return;
    }
}

bool SPRDAVCDecoder::processOneQueuedFrame(
    std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue, bool &shouldBreak)
{
    RestartDeinterlaceIfNeeded();
    BufferInfo *inInfo = *inQueue.begin();
    OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;
    ++picId;
    mFrameDecoded = false;
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
    DecodeLoopAction action = static_cast<DecodeLoopAction>(HandleDecodeResult(decRet, inQueue, inInfo, inHeader));
    if (action == DecodeLoopAction::RETURN) {
        return false;
    }
    if (action == DecodeLoopAction::BREAK) {
        shouldBreak = true;
        return true;
    }
    MMDecRet infoRet = (*mH264DecGetInfo)(mHandle, &mDecoderInfo);
    GetVuiParams(&mDecoderInfo);
    if (infoRet == MMDEC_OK && !ProcessDecoderInfo(outQueue)) {
        return false;
    }
    FinalizeInputContext finalizeContext = { decRet, infoRet, &decIn, addStartcodeLen, &inQueue, inInfo, inHeader };
    FinalizeInputFrame(finalizeContext);
    drainDecodedFrames(outQueue, decOut);
    return true;
}
bool SPRDAVCDecoder::HandlePortSettingChangeEvent(const H264SwDecInfo *info)
{
    OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(K_OUTPUT_PORT_INDEX)->mDef;
    OMX_BOOL useNativeBuffer = iUseOMXNativeBuffer[OMX_DirOutput];
    bool needFlushBuffer = outputBuffersNotEnough(info,
        def->nBufferCountMin, def->nBufferCountActual, useNativeBuffer);
    bool cropChanged = HandleCropRectEvent(&info->cropParams);
    bool bitdepChanged = HandleBitdepthChangeEvent(&info->high10En);
    UpdateSwDecoderSuggestion();
    if (ShouldDisableAfbc(info)) {
        mFbcMode = FBC_NONE;
        notify(OMX_EventPortSettingsChanged, MIN_OUTPUT_BUFFER_COUNT, OMX_IndexParamPortDefinition, nullptr);
        mOutputPortSettingsChange = AWAITING_DISABLED;
        OMX_LOGI("width %d, change2sw %d, swflag %d, frameMBOnly %d, disable AFBC\n",
            info->picWidth, mChangeToSwDec, mDecoderSwFlag, info->frameMBOnly);
        return true;
    }
    if (NeedsPortReconfigure(info, cropChanged, bitdepChanged, needFlushBuffer)) {
        return handlePortReconfiguration(info, def, useNativeBuffer, needFlushBuffer);
    }
    if (mUpdateColorAspects) {
        notify(OMX_EventPortSettingsChanged, K_OUTPUT_PORT_INDEX,
            OMX_INDEX_DESCRIBE_COLOR_ASPECTS, nullptr);
        mUpdateColorAspects = false;
    }
    return false;
}

void SPRDAVCDecoder::UpdateSwDecoderSuggestion()
{
    if (!mDecoderSwFlag && mH264DecNeedToSwitchToSwDecoder != nullptr &&
        mH264DecNeedToSwitchToSwDecoder(mHandle) != 0) {
        OMX_LOGE("ChangeToSwDec, is NOT suggested");
        mChangeToSwDec = true;
    }
}

bool SPRDAVCDecoder::NeedsPortReconfigure(
    const H264SwDecInfo *info, bool cropChanged, bool bitdepChanged, bool needFlushBuffer)
{
    return (mStride != info->picWidth) || (mSliceHeight != info->picHeight) ||
        cropChanged || bitdepChanged || (!mThumbnailMode && needFlushBuffer);
}

bool SPRDAVCDecoder::handlePortReconfiguration(
    const H264SwDecInfo *info, OMX_PARAM_PORTDEFINITIONTYPE *def, OMX_BOOL useNativeBuffer, bool needFlushBuffer)
{
    std::lock_guard<std::mutex> autoLock(mLock);
    DrainPendingDecoderFrames();
    UpdateDecoderPictureState(info);
    if (!mThumbnailMode && needFlushBuffer) {
        UpdateOutputBufferCount(def, info, useNativeBuffer);
    }
    UpdatePortDefinitions(true, true);
    (*mH264DecReleaseRefBuffers)(mHandle);
    notify(OMX_EventPortSettingsChanged, MIN_OUTPUT_BUFFER_COUNT, OMX_IndexParamPortDefinition, nullptr);
    mOutputPortSettingsChange = AWAITING_DISABLED;
    return true;
}

bool SPRDAVCDecoder::ShouldDisableAfbc(const H264SwDecInfo *info)
{
    return ((((info->picWidth & g_blockSizeMask) || (!info->frameMBOnly)) ||
        mChangeToSwDec || mDecoderSwFlag) && (mFbcMode == AFBC));
}

void SPRDAVCDecoder::DrainPendingDecoderFrames()
{
    int32_t lastPicId;
    void *pBufferHeader;
    uint64 pts;
    while ((*mH264DecGetLastDspFrm)(mHandle, &pBufferHeader, &lastPicId, &pts) == MMDEC_OK) {
        if (mDeintl) {
            DrainOneDeintlBuffer(pBufferHeader, pts);
        } else {
            drainOneOutputBuffer(lastPicId, pBufferHeader, pts);
        }
    }
}

void SPRDAVCDecoder::UpdateDecoderPictureState(const H264SwDecInfo *info)
{
    mFrameWidth = info->cropParams.cropOutWidth;
    mFrameHeight = info->cropParams.cropOutHeight;
    mhigh10En = info->high10En;
    mStride = info->picWidth;
    mSliceHeight = info->picHeight;
    int sizeMultiplier = (info->high10En != 0) ? DEFAULT_NUM_OUTPUT_BUFFERS : MIN_OUTPUT_BUFFER_COUNT;
    mPictureSize = mStride * mSliceHeight * YUV420_FRAME_SIZE_MULTIPLIER / YUV420_FRAME_SIZE_DIVISOR * sizeMultiplier;
    if (mFbcMode == AFBC) {
        int headerMultiplier = (info->high10En != 0) ? DEFAULT_NUM_OUTPUT_BUFFERS : MIN_OUTPUT_BUFFER_COUNT;
        int headerSize = (mStride * mSliceHeight / MEMORY_ALIGNMENT_16 * headerMultiplier +
            KBYTE_ALIGNMENT_1024 - 1) & (~(KBYTE_ALIGNMENT_1024 - 1));
        mPictureSize += headerSize;
    }
}

void SPRDAVCDecoder::UpdateOutputBufferCount(
    OMX_PARAM_PORTDEFINITIONTYPE *def, const H264SwDecInfo *info, OMX_BOOL useNativeBuffer)
{
    if (useNativeBuffer) {
        def->nBufferCountMin = info->numRefFrames + info->hasBFrames + MIN_OUTPUT_BUFFER_COUNT +
            ADDITIONAL_NATIVE_BUFFER;
        return;
    }
    def->nBufferCountActual = info->numRefFrames + info->hasBFrames + MIN_OUTPUT_BUFFER_COUNT +
        ADDITIONAL_OUTPUT_BUFFER_FOR_TIMEOUT + ADDITIONAL_OUTPUT_BUFFERS;
    def->bPopulated = OMX_FALSE;
}
bool SPRDAVCDecoder::HandleCropRectEvent(const CropParams *crop)
{
    mCropHeight = (mCropHeight+MEMORY_ALIGNMENT_16 - 1) & (~(MEMORY_ALIGNMENT_16 - 1));
    if (mCropWidth != crop->cropOutWidth ||
            mCropHeight != ((crop->cropOutHeight+MEMORY_ALIGNMENT_16 - 1)&(~(MEMORY_ALIGNMENT_16 - 1)))) {
        OMX_LOGI("%s, crop w h: %d %d", __FUNCTION__, crop->cropOutWidth, crop->cropOutHeight);
        return true;
    }
    return false;
}
bool SPRDAVCDecoder::HandleBitdepthChangeEvent(const unsigned char *high10En)
{
    if (mhigh10En != *high10En) {
        OMX_LOGI("%s, high10En %d", __FUNCTION__, *high10En);
        return true;
    }
    return false;
}
void SPRDAVCDecoder::drainOneOutputBuffer(int32_t picId, const void *pBufferHeader, OMX_U64 pts)
{
    (void)picId;
    std::list<BufferInfo*>& outQueue = getPortQueue(K_OUTPUT_PORT_INDEX);
    std::list<BufferInfo *>::iterator it = outQueue.begin();
    while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != outQueue.end()) {
        ++it;
    }
    if ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((*it)->mHeader "
            "== (OMX_BUFFERHEADERTYPE*)pBufferHeader) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return;
    }
    BufferInfo *outInfo = *it;
    OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;
    outHeader->nFilledLen = mPictureSize;
    outHeader->nTimeStamp = (OMX_TICKS)pts;
    OMX_LOGD("%s, %d, outHeader: %p, outHeader->pBuffer: %p, outHeader->nOffset: %d, "
        "outHeader->nFlags: %d, outHeader->nTimeStamp: %lld",
        __FUNCTION__, __LINE__, outHeader, outHeader->pBuffer,
        outHeader->nOffset, outHeader->nFlags, outHeader->nTimeStamp);
    outInfo->mOwnedByUs = false;
    outQueue.erase(it);
    outInfo = nullptr;
    BufferCtrlStruct* pOutBufCtrl = (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
    pOutBufCtrl->iRefCount++;
    notifyFillBufferDone(outHeader);
}

std::list<SprdSimpleOMXComponent::BufferInfo*> *SPRDAVCDecoder::selectDrainQueue(
    std::list<SprdSimpleOMXComponent::BufferInfo*>& outQueue)
{
    if (mDeintl) {
        if (mDecOutputBufQueue.empty()) {
            OMX_LOGW("%s, %d, There is no more  decoder output buffer\n", __FUNCTION__, __LINE__);
            return nullptr;
        }
        return &mDecOutputBufQueue;
    }
    if (outQueue.empty()) {
        OMX_LOGW("%s, %d, There is no more  display buffer\n", __FUNCTION__, __LINE__);
        return nullptr;
    }
    return &outQueue;
}

bool SPRDAVCDecoder::prepareDrainOutputHeader(std::list<SprdSimpleOMXComponent::BufferInfo*>& drainQueue,
    SprdSimpleOMXComponent::BufferInfo *&outInfo, OMX_BUFFERHEADERTYPE *&outHeader,
    std::list<SprdSimpleOMXComponent::BufferInfo*>::iterator &it)
{
    it = drainQueue.begin();
    if (mHeadersDecoded) {
        void *pBufferHeader = nullptr;
        int32_t picId = 0;
        uint64 pts = 0;
        if ((*mH264DecGetLastDspFrm)(mHandle, &pBufferHeader, &picId, &pts) == MMDEC_OK) {
            for (; it != drainQueue.end() && (*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader; ++it) {}
            if (it == drainQueue.end()) {
                OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((*it)->mHeader == "
                    "(OMX_BUFFERHEADERTYPE*)pBufferHeader) failed.",
                    __FUNCTION__, FILENAME_ONLY, __LINE__);
                return false;
            }
            outInfo = *it;
            outHeader = outInfo->mHeader;
            outHeader->nFilledLen = mPictureSize;
            outHeader->nTimeStamp = (OMX_TICKS)pts;
            return true;
        }
    }
    outInfo = *it;
    outHeader = outInfo->mHeader;
    outHeader->nTimeStamp = 0;
    outHeader->nFilledLen = 0;
    outHeader->nFlags = OMX_BUFFERFLAG_EOS;
    mEOSStatus = OUTPUT_FRAMES_FLUSHED;
    return true;
}

void SPRDAVCDecoder::submitDrainOutput(std::list<SprdSimpleOMXComponent::BufferInfo*>& outQueue,
    std::list<SprdSimpleOMXComponent::BufferInfo*>::iterator it,
    SprdSimpleOMXComponent::BufferInfo *outInfo, OMX_BUFFERHEADERTYPE *outHeader)
{
    if (mDeintl) {
        std::lock_guard<std::mutex> autoLock(mThreadLock);
        if (mDeinterInputBufQueue.empty()) {
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

bool SPRDAVCDecoder::DrainAllOutputBuffers()
{
    OMX_LOGI("%s, %d", __FUNCTION__, __LINE__);
    std::list<BufferInfo*>& outQueue = getPortQueue(K_OUTPUT_PORT_INDEX);
    while (mEOSStatus != OUTPUT_FRAMES_FLUSHED) {
        std::list<BufferInfo*> *drainQueue = selectDrainQueue(outQueue);
        if (drainQueue == nullptr) {
            return false;
        }
        BufferInfo *outInfo = nullptr;
        OMX_BUFFERHEADERTYPE *outHeader = nullptr;
        std::list<BufferInfo*>::iterator it;
        if (!prepareDrainOutputHeader(*drainQueue, outInfo, outHeader, it)) {
            return false;
        }
        OMX_LOGI("%s, %d, outHeader: %p, outHeader->pBuffer: %p, outHeader->nOffset: %d",
            __FUNCTION__, __LINE__, outHeader, outHeader->pBuffer, outHeader->nOffset);
        OMX_LOGI("outHeader->nFlags: %d, outHeader->nTimeStamp: %llu, mEOSStatus: %d",
            outHeader->nFlags, outHeader->nTimeStamp, mEOSStatus);
        submitDrainOutput(outQueue, it, outInfo, outHeader);
    }
    if (mDeintl) {
        SignalDeintlThread();
    }
    return true;
}
void SPRDAVCDecoder::onPortFlushCompleted(OMX_U32 portIndex)
{
    if (portIndex == K_INPUT_PORT_INDEX) {
        mEOSStatus = INPUT_DATA_AVAILABLE;
        mNeedIVOP = true;
    }
    if (portIndex == K_OUTPUT_PORT_INDEX && mDeintl) {
        FlushDeintlBuffer();
    }
}
void SPRDAVCDecoder::onPortFlushPrepare(OMX_U32 portIndex)
{
    if (portIndex == OMX_DirInput) {
        OMX_LOGI("%s", __FUNCTION__);
    }
        
    if (portIndex == OMX_DirOutput) {
        if (mH264DecReleaseRefBuffers != nullptr) {
            (*mH264DecReleaseRefBuffers)(mHandle);
        }
            
        if (mH264DecInitStructForNewSeq != nullptr) {
            (*mH264DecInitStructForNewSeq)(mHandle);
        }
            
        mNeedIVOP = true;
    }
}
void SPRDAVCDecoder::OnReset()
{
    SprdVideoDecoderBase::OnReset();
    mDecoderSawSPS = false;
    mDecoderSawPPS = false;
    mSPSDataSize = 0;
    mPPSDataSize = 0;
    mIsResume = false;
}
void SPRDAVCDecoder::UpdatePortDefinitions(bool updateCrop, bool updateInputSize)
{
    OMX_PARAM_PORTDEFINITIONTYPE *outDef = &editPortInfo(K_OUTPUT_PORT_INDEX)->mDef;
    if (updateCrop) {
        mCropWidth = mFrameWidth;
        mCropHeight = mFrameHeight;
    }
    OMX_LOGI("mStride:%d, mSliceHeight:%d, mFrameWidth:%d, mFrameHeight:%d",
        mStride, mSliceHeight, mFrameWidth, mFrameHeight);
    outDef->format.video.nFrameWidth = mStride;
    outDef->format.video.nFrameHeight = mSliceHeight;
    outDef->format.video.nStride = mStride;
    outDef->format.video.nSliceHeight = mSliceHeight;
    outDef->nBufferSize = mPictureSize;
    if (mhigh10En) {
//        outDef->format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)MALI_GRALLOC_FORMAT_INTERNAL_P010;
    }
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
        inDef->nBufferSize = std::max(outDef->nBufferSize / mMinCompressionRatio,
                                      inDef->nBufferSize);
    }
}
// static
int32_t SPRDAVCDecoder::ExtMemAllocWrapper(
    void *aUserData, unsigned int sizeExtra)
{
    return static_cast<SPRDAVCDecoder *>(aUserData)->VspMallocCb(sizeExtra);
}
// static
int32_t SPRDAVCDecoder::MbinfoMemAllocWrapper(
    void *aUserData, unsigned int sizeMbinfo, unsigned long *pPhyAddr)
{
    return static_cast<SPRDAVCDecoder *>(aUserData)->VspMallocMbinfoCb(sizeMbinfo, pPhyAddr);
}
// static
int32_t SPRDAVCDecoder::BindFrameWrapper(void *aUserData, void *pHeader)
{
    return static_cast<SPRDAVCDecoder *>(aUserData)->VspBindCb(pHeader);
}
// static
int32_t SPRDAVCDecoder::UnbindFrameWrapper(void *aUserData, void *pHeader)
{
    return static_cast<SPRDAVCDecoder *>(aUserData)->VspUnbindCb(pHeader);
}
// static
void SPRDAVCDecoder::DrainOneOutputBuffercallback(void *decoder, int32_t picId, void *pBufferHeader,
                                                  unsigned long long pts)
{
    return const_cast<SPRDAVCDecoder*>(static_cast<const SPRDAVCDecoder*>(decoder))->
        drainOneOutputBuffer(picId, pBufferHeader, pts);
}
int SPRDAVCDecoder::VspMallocCb(unsigned int sizeExtra)
{
    OMX_LOGI("%s, %d, mDecoderSwFlag: %d, mPictureSize: %d, sizeExtra: %d",
        __FUNCTION__, __LINE__, mDecoderSwFlag, mPictureSize, sizeExtra);
    DrainPendingDecoderFrames();
    MMCodecBuffer extraMem[MAX_MEM_TYPES];
    int ret = mDecoderSwFlag ? AllocateSoftwareExtraBuffer(sizeExtra, extraMem) :
        AllocateHardwareExtraBuffer(sizeExtra, extraMem);
    if (ret != ERROR_CODE_SUCCESS) {
        return ret;
    }
    (*mH264DecMemInit)(reinterpret_cast<SPRDAVCDecoder*>(this)->mHandle, extraMem);
    mHeadersDecoded = true;
    return ERROR_CODE_SUCCESS;
}

int SPRDAVCDecoder::AllocateSoftwareExtraBuffer(unsigned int sizeExtra, MMCodecBuffer extraMem[])
{
    if (mCodecExtraBuffer != nullptr) {
        free(mCodecExtraBuffer);
        mCodecExtraBuffer = nullptr;
    }
    if (sizeExtra == 0 || sizeExtra > UINT32_MAX) {
        OMX_LOGE("Invalid sizeExtra: %zu, malloc failed", sizeExtra);
        return ERROR_CODE_INVALID;
    }
    mCodecExtraBuffer = reinterpret_cast<uint8_t*>(malloc(sizeExtra));
    if (mCodecExtraBuffer == nullptr) {
        return ERROR_CODE_INVALID;
    }
    extraMem[MEM_TYPE_SW_CACHABLE].commonBufferPtr = mCodecExtraBuffer;
    extraMem[MEM_TYPE_SW_CACHABLE].commonBufferPtrPhy = 0;
    extraMem[MEM_TYPE_SW_CACHABLE].size = sizeExtra;
    return ERROR_CODE_SUCCESS;
}

int SPRDAVCDecoder::AllocateHardwareExtraBuffer(unsigned int sizeExtra, MMCodecBuffer extraMem[])
{
    mPbufMbInfoIdx = 0;
    if (mPbufExtraV != nullptr) {
        if (mIOMMUEnabled) {
            (*mH264DecFreeIOVA)(mHandle, mPbufExtraP, mPbufExtraSize, false);
        }
        releaseMemIon(&mPmemExtra);
        mPbufExtraV = nullptr;
        mPbufExtraP = 0;
        mPbufExtraSize = 0;
    }
    mPmemExtra = new VideoMemAllocator(sizeExtra, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    int fd = mPmemExtra->getHeapID();
    if (fd < STATUS_CODE_OK) {
        OMX_LOGE("mPmemExtra: getHeapID fail %d", fd);
        return ERROR_CODE_INVALID;
    }
    unsigned long phyAddr = 0;
    size_t bufferSize = 0;
    int ret = mIOMMUEnabled ? (*mH264DecGetIOVA)(mHandle, fd, &phyAddr, &bufferSize, false) :
        mPmemExtra->getPhyAddrFromMemAlloc(&phyAddr, &bufferSize);
    if (ret < STATUS_CODE_OK) {
        OMX_LOGE("mPmemExtra: get phy addr fail %d(%s)", ret, strerror(errno));
        return ERROR_CODE_INVALID;
    }
    mPbufExtraP = phyAddr;
    mPbufExtraSize = bufferSize;
    mPbufExtraV = reinterpret_cast<uint8_t*>(mPmemExtra->getBase());
    extraMem[MEM_TYPE_HW_NO_CACHE].commonBufferPtr = mPbufExtraV;
    extraMem[MEM_TYPE_HW_NO_CACHE].commonBufferPtrPhy = mPbufExtraP;
    extraMem[MEM_TYPE_HW_NO_CACHE].size = sizeExtra;
    return ERROR_CODE_SUCCESS;
}
int SPRDAVCDecoder::VspMallocMbinfoCb(unsigned int sizeMbinfo, unsigned long *pPhyAddr)
{
    int idx = mPbufMbInfoIdx % MAX_MBINFO_BUFFERS;
    OMX_LOGI("%s, %d, idx: %d, sizeMbinfo: %d", __FUNCTION__, __LINE__, idx, sizeMbinfo);
    if (mPmemMbInfoV[idx] != nullptr && mPmemMbInfo[idx] != nullptr) {
        if (mIOMMUEnabled) {
            (*mH264DecFreeIOVA)(mHandle, mPmemMbInfoP[idx], mPbufMbInfoSize[idx], true);
        }
        mPmemMbInfo[idx].clear();
        OMX_LOGD("mPmemMbInfo[idx].clear() passed");
        mPmemMbInfoV[idx] = nullptr;
        mPmemMbInfoP[idx] = 0;
        mPbufMbInfoSize[idx] = 0;
        OMX_LOGD("-");
    }
    mPmemMbInfo[idx] = new VideoMemAllocator(sizeMbinfo, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    int fd = mPmemMbInfo[idx]->getHeapID();
    if (fd >= STATUS_CODE_OK) {
        int ret;
        unsigned long phyAddr;
        size_t bufferSize;
        if (mIOMMUEnabled) {
            ret = (*mH264DecGetIOVA)(mHandle, fd, &phyAddr, &bufferSize, false);
        } else {
            ret = mPmemMbInfo[idx]->getPhyAddrFromMemAlloc(&phyAddr, &bufferSize);
        }
        if (ret < STATUS_CODE_OK) {
            OMX_LOGE("mPmemMbInfo[%d]: get phy addr fail %d(%s)", idx, ret, strerror(errno));
            return ERROR_CODE_INVALID;
        }
        mPmemMbInfoP[idx] = phyAddr;
        mPbufMbInfoSize[idx] = bufferSize;
        mPmemMbInfoV[idx] = reinterpret_cast<uint8_t*>(mPmemMbInfo[idx]->getBase());
        OMX_LOGI("pmem 0x%lx - %p - %zd", mPmemMbInfoP[idx], mPmemMbInfoV[idx], mPbufMbInfoSize[idx]);
        *pPhyAddr = phyAddr;
    } else {
        OMX_LOGE("mPmemMbInfo[%d]: getHeapID fail %d", idx, fd);
        return ERROR_CODE_INVALID;
    }
    mPbufMbInfoIdx++;
    return ERROR_CODE_SUCCESS;
}
int SPRDAVCDecoder::VspBindCb(void *pHeader)
{
    OMX_BUFFERHEADERTYPE* omxHeader = reinterpret_cast<OMX_BUFFERHEADERTYPE*>(pHeader);
    BufferCtrlStruct *pBufCtrl = reinterpret_cast<BufferCtrlStruct*>(
        omxHeader->pOutputPortPrivate);
    OMX_LOGD("VspBindCb, pBuffer: %p, pHeader: %p; iRefCount=%d",
        ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader, pBufCtrl->iRefCount);
    pBufCtrl->iRefCount++;
    return ERROR_CODE_SUCCESS;
}
int SPRDAVCDecoder::VspUnbindCb(void *pHeader)
{
    BufferCtrlStruct *pBufCtrl = reinterpret_cast<BufferCtrlStruct*>
                                 (reinterpret_cast<OMX_BUFFERHEADERTYPE*>(pHeader)->pOutputPortPrivate);
    OMX_LOGD("VspUnbindCb, pBuffer: %p, pHeader: %p; iRefCount=%d",
        ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader, pBufCtrl->iRefCount);
    if (pBufCtrl->iRefCount  > 0) {
        pBufCtrl->iRefCount--;
    }
    return ERROR_CODE_SUCCESS;
}
OMX_ERRORTYPE SPRDAVCDecoder::getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index)
{
    if (strcmp(name, "OMX.sprd.index.ThumbnailMode") == 0) {
        OMX_LOGD("getExtensionIndex: OMX.sprd.index.ThumbnailMode");
        *(int32_t*)index = OMX_INDEX_CONFIG_THUMBNAIL_MODE;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.index.allocateNativeHandle") == 0) {
        OMX_LOGD("getExtensionIndex: OMX.index.allocateNativeHandle");
        *(int32_t*)index = OMX_INDEX_ALLOCATE_NATIVE_HANDLE;
        return OMX_ErrorNone;
    }
    return SprdVideoDecoderBase::getExtensionIndex(name, index);
}

bool SPRDAVCDecoder::LoadSwSwitchSymbolIfNeeded(const char *libName)
{
    const char *hardwareLib = mSecureFlag ? "libomx_avcdec_hw_sprd_secure.so" : "libomx_avcdec_hw_sprd.so";
    if (strcmp(libName, hardwareLib) != 0) {
        return true;
    }
    mH264DecNeedToSwitchToSwDecoder = reinterpret_cast<FT_H264DecNeedToSwitchToSwDecoder>(
        dlsym(mLibHandle, "H264DecNeedToSwitchToSwDecoder"));
    if (mH264DecNeedToSwitchToSwDecoder != nullptr) {
        return true;
    }
    OMX_LOGE("Can't find %s in %s", "H264DecNeedToSwitchToSwDecoder", libName);
    return false;
}

bool SPRDAVCDecoder::OpenDecoder(const char *libName)
{
    if (mLibHandle) {
        dlclose(mLibHandle);
    }
    OMX_LOGI("try to OpenDecoder, lib: %s", libName);
    mLibHandle = dlopen(libName, RTLD_NOW);
    if (mLibHandle == nullptr) {
        OMX_LOGE("OpenDecoder, can't open lib: %s", libName);
        return false;
    }
    if (!LoadRequiredDecoderSymbols(libName) || !LoadSwSwitchSymbolIfNeeded(libName)) {
        return CloseDecoderLibraryOnFailure(&mLibHandle);
    }
    OMX_LOGI("open decoder success, no handle is nullptr");
    return true;
}

void SPRDAVCDecoder::findCodecConfigData(OMX_BUFFERHEADERTYPE *header)
{
    OMX_LOGD("findCodecConfigData");
    int nalType = 0;
    int nalRefIdc = 0;
    uint8 *p = header->pBuffer + header->nOffset;
    MMDecRet decRet = (*mH264DecGetNALType)(mHandle, p, header->nFilledLen, &nalType, &nalRefIdc);
    OMX_LOGI("%s, header:%p, nalType:%d, nalRefIdc:%d, mSPSDataSize:%u, mPPSDataSize:%u",
        __FUNCTION__, header, nalType, nalRefIdc, mSPSDataSize, mPPSDataSize);
    if (decRet != MMDEC_OK || header->nFilledLen > H264_HEADER_SIZE) {
        return;
    }
    if (nalType == H264_NAL_TYPE_SPS) {
        cacheSpsData(p, header->nFilledLen);
        UpdateInterlaceSequenceState(IsInterlacedSequence(mSPSData, mSPSDataSize));
        HandleNewSequence();
        return;
    }
    if (nalType == H264_NAL_TYPE_PPS) {
        cachePpsData(p, header->nFilledLen);
    }
}
void SPRDAVCDecoder::FreeOutputBufferIova()
{
    PortInfo *port = editPortInfo(OMX_DirOutput);
    for (size_t i = 0; i < port->mBuffers.size(); ++i) {
        BufferInfo *buffer = &port->mBuffers.at(i);
        OMX_BUFFERHEADERTYPE *header = buffer->mHeader;
        if (header->pOutputPortPrivate != nullptr) {
            BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct*)(header->pOutputPortPrivate);
            if (pBufCtrl->phyAddr > 0) {
                OMX_LOGI("%s, fd: %d, iova: 0x%lx", __FUNCTION__, pBufCtrl->bufferFd, pBufCtrl->phyAddr);
                (*mH264DecFreeIOVA)(mHandle, pBufCtrl->phyAddr, pBufCtrl->bufferSize, true);
                pBufCtrl->bufferFd = 0;
                pBufCtrl->phyAddr = 0;
            }
        }
    }
}

int SPRDAVCDecoder::GetColorAspectPreference()
{
    return COLOR_ASPECT_PREFER_BITSTREAM;
}
}  // namespace OMX
}  // namespace OHOS