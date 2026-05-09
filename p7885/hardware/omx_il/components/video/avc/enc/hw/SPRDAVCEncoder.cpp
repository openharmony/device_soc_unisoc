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
#define LOG_TAG "SPRDAVCEncoder"
#include <securec.h>
#include <utils/omx_log.h>
#include "avc_enc_api.h"
#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#include <dlfcn.h>
#include "utils/omx_log.h"
#include "SPRDAVCEncoder.h"
#include "sprd_omx_typedef.h"
#include "Errors.h"
#include "OMXGraphicBufferMapper.h"
#include "codec_omx_ext.h"
#include "OMXHardwareAPI.h"

#include "Dynamic_buffer.h"
#include <utils/time_util.h>
#include <sys/mman.h>
#include <cstdint>
#include <cstring>
#include <new>
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>
/* Macro definitions for magic numbers */
#define H264ENC_HEADER_BUFFER_SIZE 1024                  /* Size of header buffer for SPS/PPS */
#define H264ENC_START_CODE_PREFIX_SIZE 4                 /* Size of H.264 start code prefix (0x00000001) */
#define H264ENC_START_CODE_BYTE_0 0x00                   /* First byte of start code */
#define H264ENC_START_CODE_BYTE_1 0x00                   /* Second byte of start code */
#define H264ENC_START_CODE_BYTE_2 0x00                   /* Third byte of start code */
#define H264ENC_START_CODE_BYTE_3 0x01                   /* Fourth byte of start code (NAL unit start) */
#define H264ENC_DEFAULT_QP_IVOP_SCENE_0 28               /* Default QP for I-frame in scene mode 0 */
#define H264ENC_DEFAULT_QP_IVOP_SCENE_1 36               /* Default QP for I-frame in scene mode 1 */
#define H264ENC_DEFAULT_QP_PVOP 28                       /* Default QP for P-frame */
#define H264ENC_DEFAULT_VBV_BUF_SIZE_DIVISOR 2           /* Divisor for VBV buffer size calculation */
#define H264ENC_PROFILE_SHIFT_BITS 8                     /* Bit shift for profile in profileAndLevel field */
#define H264ENC_PROFILE_MASK 0xFF                        /* Mask for profile bits in profileAndLevel field */
#define H264ENC_TIME_SCALE_MS 1000                       /* Time scale in milliseconds */
#define H264ENC_DEFAULT_BIT_DEPTH 8                      /* Default bit depth for encoding */
#define H264ENC_STRIDE_ALIGNMENT 16                      /* Alignment for stride calculation */
#define H264ENC_ALIGNMENT_MASK 0xFFFFFFF0                /* Mask for 16-byte alignment (0xFFFFFFF0 = ~15) */
#define H264ENC_BUFFER_SIZE_MULTIPLIATOR 3               /* Multiplier for input buffer size calculation */
#define H264ENC_BUFFER_SIZE_DIVISOR 2                    /* Divisor for input buffer size calculation */
#define H264ENC_OUTPUT_BUFFER_SIZE_DIVISOR 2             /* Divisor for output buffer size calculation */
#define H264ENC_YUV420_SIZE_MULTIPLIATOR 3               /* Multiplier for YUV420 buffer size calculation */
#define H264ENC_YUV420_SIZE_DIVISOR 2                    /* Divisor for YUV420 buffer size calculation */
#define OMX_COLOR_FORMAT_RGBA32 12                       /* OMX color format for RGBA32 */
#define OMX_COLOR_FORMAT_NV12 24                         /* OMX color format for NV12 */
#define OMX_COLOR_FORMAT_NV21 25                         /* OMX color format for NV21 */
#define DMA_BUF_SYNC_START_FLAG (DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW)    /* DMA buffer sync start flag */
#define DMA_BUF_SYNC_END_FLAG (DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW)        /* DMA buffer sync end flag */
#define H264ENC_DEFAULT_FRAME_RATE_DIVISOR 1             /* Divisor for frame rate calculation */
#define H264ENC_PFRAMES_DEFAULT 0                        /* Default number of P-frames between I-frames */
#define H264ENC_LEVEL_DEFAULT 2                          /* Default AVC level */
#define H264ENC_NUM_BUFFERS_DEFAULT 8                    /* Default number of buffers */
#define H264ENC_PRIORITY_DEFAULT (-1)                      /* Default encoder priority */
#define H264ENC_OPERATING_RATE_DEFAULT 0                 /* Default operating rate */
#define H264ENC_CHANNEL_QUALITY_DEFAULT 1                /* Default channel quality */
#define H264ENC_TIMESTAMP_CONVERSION_DIVISOR 1000        /* Divisor for timestamp conversion to milliseconds */
#define H264ENC_TIMESTAMP_ROUNDING_OFFSET 500            /* Offset for timestamp rounding */
#define H264ENC_ENCODE_TIME_DIVISOR 1000000L  /* Divisor for encode time calculation (microseconds to milliseconds) */
/* Frame type definitions */
#define H264ENC_VOP_TYPE_I 0                             /* I-frame type indicator */
#define H264ENC_VOP_TYPE_P 1                             /* P-frame type indicator */
/* Header generation types */
#define H264ENC_GEN_SPS_HEADER 1                         /* Generate SPS header type */
#define H264ENC_GEN_PPS_HEADER 0                         /* Generate PPS header type */
/* Initial frame counter value */
#define H264ENC_INITIAL_NUM_INPUT_FRAMES (-2)              /* Initial value for input frame counter */
/* Default configuration values */
#define H264ENC_IS_H263_DISABLED 0                       /* H.263 encoding disabled */
#define H264ENC_FRAME_WIDTH_DEFAULT 0                    /* Default frame width */
#define H264ENC_FRAME_HEIGHT_DEFAULT 0                   /* Default frame height */
#define H264ENC_CROP_OFFSET_DEFAULT 0                    /* Default crop offset */
/* Codec capability indices */
#define H264ENC_CODEC_CAP_PROFILE_INDEX 0                /* Profile index in codec capabilities */
#define H264ENC_CODEC_CAP_LEVEL_INDEX 1                  /* Level index in codec capabilities */
#define H264ENC_CODEC_CAP_MAX_WIDTH_INDEX 2              /* Max width index in codec capabilities */
#define H264ENC_CODEC_CAP_MAX_HEIGHT_INDEX 3             /* Max height index in codec capabilities */
/* Encode result codes */
#define H264ENC_ENCODE_SUCCESS 0                         /* Encode operation successful */
#define H264ENC_ENCODE_FAILURE (-1)                        /* Encode operation failed */
/* Memory alignment constants */
#define H264ENC_PAGE_SIZE 4096                           /* System page size for memory alignment */
#define H264ENC_MEM_ALIGNMENT 4096                       /* Memory alignment boundary */
/* Frame type identifiers */
#define H264ENC_FRAME_TYPE_P 0                           /* P-frame type */
#define H264ENC_FRAME_TYPE_I 1                           /* I-frame type */
/* Time conversion constants */
#define H264ENC_MS_PER_SECOND 1000                       /* Milliseconds per second */
#define H264ENC_US_PER_MS 1000                           /* Microseconds per millisecond */
/* Frame rate control */
#define H264ENC_DEFAULT_FRAMERATE_NUMERATOR 1            /* Default frame rate numerator */
#define H264ENC_DEFAULT_FRAMERATE_DENOMINATOR 1          /* Default frame rate denominator */
/* Memory management */
#define H264ENC_DYNAMIC_BUFFER_SIZE 1024                 /* Size for dynamic buffer allocation */
/* Hardware capability check */
#define H264ENC_MAX_WIDTH_INDEX 0                        /* Index for maximum width capability */
#define H264ENC_MAX_HEIGHT_INDEX 1                       /* Index for maximum height capability */
/* Byte index constants for array access */
#define H264ENC_BYTE_INDEX_0 0
#define H264ENC_BYTE_INDEX_1 1
#define H264ENC_BYTE_INDEX_2 2
#define H264ENC_BYTE_INDEX_3 3
#define H264ENC_BYTE_INDEX_4 4
#define H264ENC_BYTE_INDEX_5 5
#define H264ENC_BYTE_INDEX_6 6
#define H264ENC_BYTE_INDEX_7 7
#define H264ENC_BYTE_INDEX_8 8
#define H264ENC_BYTE_INDEX_9 9
#define H264ENC_BYTE_INDEX_10 10
#define H264ENC_BYTE_INDEX_11 11
#define H264ENC_BYTE_INDEX_12 12
#define H264ENC_BYTE_INDEX_13 13
#define H264ENC_BYTE_INDEX_14 14
#define H264ENC_BYTE_INDEX_15 15
namespace OHOS {
namespace OMX {
static constexpr int32_t K_DEFERRED_CLOSE_DELAY_SECONDS = 2;
static std::mutex g_avcDeferredCloseMutex;
static std::vector<void *> gAvcDeferredCloseHandles;
static bool g_avcDeferredCloseWorkerRunning = false;

static void CloseDeferredAvcHandlesLocked()
{
    for (void *handle : gAvcDeferredCloseHandles) {
        if (handle != nullptr) {
            OMX_LOGI("deferred dlclose for AVC encoder handle: %p", handle);
            dlclose(handle);
        }
    }
    gAvcDeferredCloseHandles.clear();
}

static void ScheduleDeferredAvcCloseWorkerLocked()
{
    if (g_avcDeferredCloseWorkerRunning) {
        return;
    }
    g_avcDeferredCloseWorkerRunning = true;
    std::thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(K_DEFERRED_CLOSE_DELAY_SECONDS));
        std::lock_guard<std::mutex> lock(g_avcDeferredCloseMutex);
        CloseDeferredAvcHandlesLocked();
        g_avcDeferredCloseWorkerRunning = false;
    }).detach();
}

static void DeferAvcEncoderDlclose(void *libHandle, const char *reason)
{
    if (libHandle == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_avcDeferredCloseMutex);
    for (void *pending : gAvcDeferredCloseHandles) {
        if (pending == libHandle) {
            return;
        }
    }
    OMX_LOGW("defer dlclose for AVC encoder handle (%s): %p", reason, libHandle);
    gAvcDeferredCloseHandles.push_back(libHandle);
    ScheduleDeferredAvcCloseWorkerLocked();
}

template<typename T>
static bool LoadEncoderSymbol(void *libHandle, const char *libName, const char *symbolName, T *symbol)
{
    *symbol = reinterpret_cast<T>(dlsym(libHandle, symbolName));
    if (*symbol != nullptr) {
        return true;
    }
    OMX_LOGE("Can't find %s in %s", symbolName, libName);
    return false;
}

static bool CloseEncoderLibraryOnFailure(void **libHandle)
{
    if (libHandle != nullptr && *libHandle != nullptr) {
        DeferAvcEncoderDlclose(*libHandle, "load failure");
        *libHandle = nullptr;
    }
    return false;
}

static const char *H264ENC_MIME_TYPE = "video/avc";
static const CodecProfileLevel kProfileLevels[] = {
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51 },
};
typedef struct LevelConversion {
    OMX_U32 omxLevel;
    AVCLevel avcLevel;
} LevelConcersion;
typedef struct ProfileConversion {
    OMX_U32 omxProfile;
    AVCProfile avcProfile;
} ProfileConcersion;
static ProfileConcersion g_conversionTableProfile[] = {
    { OMX_VIDEO_AVCProfileBaseline,  AVC_BASELINE },
    { OMX_VIDEO_AVCProfileMain, AVC_MAIN   },
    { OMX_VIDEO_AVCProfileExtended, AVC_EXTENDED },
    { OMX_VIDEO_AVCProfileHigh, AVC_HIGH },
    { OMX_VIDEO_AVCProfileHigh10, AVC_HIGH10 },
    { OMX_VIDEO_AVCProfileHigh422,  AVC_HIGH422 },
    { OMX_VIDEO_AVCProfileHigh444,  AVC_HIGH444 },
};
static LevelConversion g_conversionTable[] = {
    { OMX_VIDEO_AVCLevel1, AVC_LEVEL1_B },
    { OMX_VIDEO_AVCLevel1b, AVC_LEVEL1   },
    { OMX_VIDEO_AVCLevel11, AVC_LEVEL1_1 },
    { OMX_VIDEO_AVCLevel12, AVC_LEVEL1_2 },
    { OMX_VIDEO_AVCLevel13, AVC_LEVEL1_3 },
    { OMX_VIDEO_AVCLevel2, AVC_LEVEL2 },
    { OMX_VIDEO_AVCLevel21, AVC_LEVEL2_1 },
    { OMX_VIDEO_AVCLevel22, AVC_LEVEL2_2 },
    { OMX_VIDEO_AVCLevel3, AVC_LEVEL3   },
    { OMX_VIDEO_AVCLevel31, AVC_LEVEL3_1 },
    { OMX_VIDEO_AVCLevel32, AVC_LEVEL3_2 },
    { OMX_VIDEO_AVCLevel4, AVC_LEVEL4   },
    { OMX_VIDEO_AVCLevel41, AVC_LEVEL4_1 },
    { OMX_VIDEO_AVCLevel42, AVC_LEVEL4_2 },
    { OMX_VIDEO_AVCLevel5, AVC_LEVEL5   },
    { OMX_VIDEO_AVCLevel51, AVC_LEVEL5_1 },
};
static status_t ConvertOmxAvcProfileToAvcSpecProfile(
    OMX_U32 omxProfile, AVCProfile *avcProfile)
{
    for (size_t i = 0, n = sizeof(g_conversionTableProfile)/sizeof(g_conversionTableProfile[0]); i < n; ++i) {
        if (omxProfile == g_conversionTableProfile[i].omxProfile) {
            *avcProfile = g_conversionTableProfile[i].avcProfile;
            return STATUS_OK;
        }
    }
    OMX_LOGE("ConvertOmxAvcProfileToAvcSpecProfile: %d Profile not supported", static_cast<int32_t>(omxProfile));
    return STATUS_BAD_VALUE;
}
static status_t ConvertAvcSpecProfileToOmxAvcProfile(
    AVCProfile avcProfile, OMX_U32 *omxProfile)
{
    for (size_t i = 0, n = sizeof(g_conversionTableProfile)/sizeof(g_conversionTableProfile[0]);
            i < n; ++i) {
        if (avcProfile == g_conversionTableProfile[i].avcProfile) {
            *omxProfile = g_conversionTableProfile[i].omxProfile;
            return STATUS_OK;
        }
    }
    OMX_LOGE("ConvertAvcSpecProfileToOmxAvcProfile: %d Profile not supported",
        static_cast<int32_t>(avcProfile));
    return STATUS_BAD_VALUE;
}
static status_t ConvertOmxAvcLevelToAvcSpecLevel(
    OMX_U32 omxLevel, AVCLevel *avcLevel)
{
    for (size_t i = 0, n = sizeof(g_conversionTable)/sizeof(g_conversionTable[0]); i < n; ++i) {
        if (omxLevel == g_conversionTable[i].omxLevel) {
            *avcLevel = g_conversionTable[i].avcLevel;
            return STATUS_OK;
        }
    }
    OMX_LOGE("ConvertOmxAvcLevelToAvcSpecLevel: %d level not supported",
        static_cast<int32_t>(omxLevel));
    return STATUS_BAD_VALUE;
}
static status_t ConvertAvcSpecLevelToOmxAvcLevel(
    AVCLevel avcLevel, OMX_U32 *omxLevel)
{
    for (size_t i = 0, n = sizeof(g_conversionTable)/sizeof(g_conversionTable[0]); i < n; ++i) {
        if (avcLevel == g_conversionTable[i].avcLevel) {
            *omxLevel = g_conversionTable[i].omxLevel;
            return STATUS_OK;
        }
    }
    OMX_LOGE("ConvertAvcSpecLevelToOmxAvcLevel: %d level not supported",
        static_cast<int32_t>(avcLevel));
    return STATUS_BAD_VALUE;
}
SPRDAVCEncoder::SPRDAVCEncoder(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdVideoEncoderBase(name, callbacks, appData, component),
      mHandle(new TagAvcHandle),
      mEncConfig(new MMEncConfig)
{
    OMX_LOGI("Construct SPRDAVCEncoder, this: 0x%p", static_cast<void *>(this));
    if (!InitializeHandle() || !InitializeEncoderLibrary(name) || !InitializeEncoderSession()) {
        return;
    }
    InitializeEncoderRuntimeFlags(name);
    mInitCheck = OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoder::initCheck() const
{
    return mInitCheck;
}

bool SPRDAVCEncoder::InitializeHandle()
{
    if (mHandle == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(mHandle != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    (void)memset_s(mHandle, sizeof(TagAvcHandle), 0, sizeof(TagAvcHandle));
    mHandle->videoEncoderData = nullptr;
    mHandle->userData = this;
    mHandle->vspFlushBSCache = FlushCacheWrapper;
    mHandle->vspBeginCPUAccess = BeginCPUAccessWrapper;
    mHandle->vspEndCPUAccess = EndCPUAccessWrapper;
    mHandle->vspExtmemcb = ExtMemAllocWrapper;
    (void)memset_s(&mEncInfo, sizeof(mEncInfo), 0, sizeof(mEncInfo));
    return true;
}

bool SPRDAVCEncoder::InitializeEncoderLibrary(const char *name)
{
    if (!OpenEncoder("libomx_avcenc_hw_sprd.z.so")) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((OpenEncoder(\"libomx_avcenc_hw_sprd.z.so\")) "
            "== (true)) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    OMX_LOGI("%s, line:%d, name: %s", __FUNCTION__, __LINE__, name);
    mDumpYUVEnabled = GetDumpValue("vendor.h264enc.yuv.dump");
    mDumpStrmEnabled = GetDumpValue("vendor.h264enc.strm.dump");
    OMX_LOGI("%s, mDumpYUVEnabled: %d, mDumpStrmEnabled: %d", __FUNCTION__, mDumpYUVEnabled, mDumpStrmEnabled);
    return true;
}

bool SPRDAVCEncoder::InitializeEncoderSession()
{
    MMCodecBuffer interMemBfr;
    uint32_t sizeInter = H264ENC_INTERNAL_BUFFER_SIZE;

    mPbuf_inter = reinterpret_cast<uint8_t*>(malloc(sizeInter));
    if (mPbuf_inter == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(mPbuf_inter != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }

    interMemBfr.commonBufferPtr = mPbuf_inter;
    interMemBfr.commonBufferPtrPhy = 0;
    interMemBfr.size = sizeInter;
    if ((*mH264EncPreInit)(mHandle, &interMemBfr) != MMENC_OK) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((*mH264EncPreInit)(mHandle, &interMemBfr) "
            "== (MMENC_OK)) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    mEncoderPreInitDone = true;
    if ((*mH264EncGetCodecCapability)(mHandle, &mCapability) != MMENC_OK) {
        OMX_LOGE("Failed to get codec capability");
        return false;
    }

    InitPorts();
    OMX_LOGI("Construct SPRDAVCEncoder, Capability: profile %d, level %d, max wh=%d %d",
        mCapability.profile, mCapability.level, mCapability.maxWidth, mCapability.maxHeight);
    return true;
}

void SPRDAVCEncoder::InitializeEncoderRuntimeFlags(const char *name)
{
#ifdef CONFIG_RGB_ENC_SUPPORT
    mSupportRGBEnc = true;
#endif
    int ret = (*mH264EncNeedAlign)(mHandle);
    OMX_LOGI("Enc needAign:%d", ret);
    if (ret == 0) {
        mNeedAlign = false;
    }

    ret = (*mH264EncGetIOMMUStatus)(mHandle);
    OMX_LOGI("Get IOMMU Status: %d(%s)", ret, strerror(errno));
    mIOMMUEnabled = ret >= 0;
    if (mDumpYUVEnabled || mDumpStrmEnabled) {
        InitDumpFiles(name, "h264");
    }
}

void SPRDAVCEncoder::ResetEncoderSymbols()
{
    mH264EncPreInit = nullptr;
    mH264EncGetFnum = nullptr;
    mH264EncInit = nullptr;
    mH264EncSetConf = nullptr;
    mH264EncGetConf = nullptr;
    mH264EncStrmEncode = nullptr;
    mH264EncGenHeader = nullptr;
    mH264EncRelease = nullptr;
    mH264EncGetCodecCapability = nullptr;
    mH264EncGetIOVA = nullptr;
    mH264EncFreeIOVA = nullptr;
    mH264EncGetIOMMUStatus = nullptr;
    mH264EncNeedAlign = nullptr;
    mEncoderSymbolsReady = false;
    mEncoderPreInitDone = false;
}
SPRDAVCEncoder::~SPRDAVCEncoder()
{
    OMX_LOGI("Destruct SPRDAVCEncoder, this: 0x%p", static_cast<void *>(this));
    releaseResource();
    releaseEncoder();
    if (mPorts.size() <= K_OUTPUT_PORT_INDEX) {
        OMX_LOGW("skip queue validation because ports are not initialized, size=%zu", mPorts.size());
        if (mLibHandle) {
            DeferAvcEncoderDlclose(mLibHandle, "destructor no-ports");
            mLibHandle = nullptr;
        }
        if (mDumpYUVEnabled || mDumpStrmEnabled) {
            CloseDumpFiles();
        }
        return;
    }
    std::list<BufferInfo*>& outQueue = getPortQueue(1);
    std::list<BufferInfo*>& inQueue = getPortQueue(0);
    if (!outQueue.empty()) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(outQueue.empty() failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return;
    }
    if (!inQueue.empty()) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(inQueue.empty() failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return;
    }
    if (mLibHandle) {
        DeferAvcEncoderDlclose(mLibHandle, "destructor");
        mLibHandle = nullptr;
    }
    if (mDumpYUVEnabled || mDumpStrmEnabled) {
        CloseDumpFiles();
    }
}
OMX_ERRORTYPE SPRDAVCEncoder::initEncParams()
{
    OMX_ERRORTYPE err = prepareEncoderBuffers();
    if (err != OMX_ErrorNone) {
        return err;
    }
    err = startEncoderSession();
    if (err != OMX_ErrorNone) {
        return err;
    }
    mNumInputFrames = H264ENC_INITIAL_NUM_INPUT_FRAMES;
    mSpsPpsHeaderReceived = false;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoder::prepareEncoderBuffers()
{
    if (mEncConfig == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(mEncConfig != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    (void)memset_s(mEncConfig, sizeof(MMEncConfig), 0, sizeof(MMEncConfig));
    InitEncInfo();
    size_t sizeStream = ((mVideoWidth + H264ENC_STRIDE_ALIGNMENT - 1) & H264ENC_ALIGNMENT_MASK) *
        ((mVideoHeight + H264ENC_STRIDE_ALIGNMENT - 1) & H264ENC_ALIGNMENT_MASK) *
        H264ENC_YUV420_SIZE_MULTIPLIATOR / H264ENC_YUV420_SIZE_DIVISOR;
    return allocateStreamBuffers(sizeStream >> H264ENC_OUTPUT_BUFFER_SIZE_DIVISOR);
}

OMX_ERRORTYPE SPRDAVCEncoder::startEncoderSession()
{
    MMEncCodecPriority mEncPriority;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    mEncPriority.priority = H264ENC_PRIORITY_DEFAULT;
    mEncPriority.operatingRate = H264ENC_OPERATING_RATE_DEFAULT;
    OMX_LOGI("mEncPriority : priority = %d, operatingRate = %d", mEncPriority.priority, mEncPriority.operatingRate);
    if ((*mH264EncInit)(mHandle, &mStreamBfr, &mEncInfo, &mEncPriority) != 0) {
        OMX_LOGE("Failed to init mp4enc");
        return OMX_ErrorUndefined;
    }
    if ((*mH264EncGetConf)(mHandle, mEncConfig) != 0) {
        OMX_LOGE("Failed to get default encoding parameters");
        return OMX_ErrorUndefined;
    }
    err = configureEncConfig();
    if (err != OMX_ErrorNone) {
        return err;
    }
    return OMX_ErrorNone;
}

void SPRDAVCEncoder::InitEncInfo()
{
    mEncInfo.frameWidth = mVideoWidth;         //mStride?
    mEncInfo.frameHeight = mVideoHeight;
    mEncInfo.eisMode = false;
#ifdef CONFIG_SPRD_RECORD_EIS
    mEncInfo.eisMode = meisMode;
#endif
    mEncInfo.isH263 = H264ENC_IS_H263_DISABLED;
    mEncInfo.orgWidth = mVideoWidth;       //mStride
    mEncInfo.orgHeight = mVideoHeight;
    // mVideoColorFormat, 12 rgba, 24 nv12, 25 nv21, 源自graphic模块，暂时用魔法数字
    if (mVideoColorFormat == OMX_COLOR_FORMAT_RGBA32) {
        mEncInfo.yuvFormat = MMENC_RGBA32;
    } else if (mVideoColorFormat == OMX_COLOR_FORMAT_NV12) {
        mEncInfo.yuvFormat = MMENC_YUV420SP_NV12;
    } else if (mVideoColorFormat == OMX_COLOR_FORMAT_NV21) {
        mEncInfo.yuvFormat = MMENC_YUV420SP_NV21;
    }
    mEncInfo.timeScale = H264ENC_TIME_SCALE_MS;
    mEncInfo.bitDepth = H264ENC_DEFAULT_BIT_DEPTH;
    OMX_LOGI("mEncInfo: frameWidth= %d, frameHeight = %d, eisMode = %d, isH263 = %d",
        mEncInfo.frameWidth, mEncInfo.frameHeight, mEncInfo.eisMode, mEncInfo.isH263);
    OMX_LOGI("mEncInfo: orgWidth = %d, orgHeight = %d, yuvFormat = %d, timeScale = %d, bitDepth = %d",
        mEncInfo.orgWidth, mEncInfo.orgHeight, mEncInfo.yuvFormat, mEncInfo.timeScale, mEncInfo.bitDepth);
}

OMX_ERRORTYPE SPRDAVCEncoder::allocateStreamBuffers(size_t sizeStream)
{
    unsigned long phyAddr = 0;
    size_t size = 0;
    int fd = 0;
    int ret = 0;
    mMFEnum = (*mH264EncGetFnum)(mHandle, mVideoWidth, mVideoHeight, mVideoFrameRate);
    OMX_LOGI("mMFEnum = %d", mMFEnum);
    for (int i = 0; i < mMFEnum; i++) {
        mPmemStream[i] = new VideoMemAllocator(sizeStream, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
        fd = mPmemStream[i]->getHeapID();
        if (fd < 0) {
            OMX_LOGE("Failed to alloc stream buffer (%zd), getHeapID failed", sizeStream);
            return OMX_ErrorInsufficientResources;
        }
        if (mIOMMUEnabled) {
            ret = (*mH264EncGetIOVA)(mHandle, fd, &phyAddr, &size);
        } else {
            ret = mPmemStream[i]->getPhyAddrFromMemAlloc(&phyAddr, &size);
        }
        if (ret < 0) {
            OMX_LOGE("Failed to alloc stream buffer, get phy addr failed");
            return OMX_ErrorInsufficientResources;
        }
        mStreamBfr.commonBufferPtr[i] = static_cast<uint8_t*>(mPmemStream[i]->getBase());
        mStreamBfr.commonBufferPtrPhy[i] = phyAddr;
        mStreamBfr.size = size;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoder::configureEncConfig()
{
    mEncConfig->h263En = H264ENC_IS_H263_DISABLED;
    mEncConfig->rateCtrlEnable = mrateCtrlEnable;
    mEncConfig->targetBitRate = mVideoBitRate;
    mEncConfig->frameRate = mVideoFrameRate;
    mEncConfig->pFrames = mPFrames;
    if (mEncSceneMode == 0) {
        mEncConfig->qpIvop = H264ENC_DEFAULT_QP_IVOP_SCENE_0;
    } else {
        mEncConfig->qpIvop = H264ENC_DEFAULT_QP_IVOP_SCENE_1;
    }
    mEncConfig->qpPvop = H264ENC_DEFAULT_QP_PVOP;
    mEncConfig->vbvBufSize = mVideoBitRate/H264ENC_DEFAULT_VBV_BUF_SIZE_DIVISOR;
    uint32_t profilePart = (mAVCEncProfile & H264ENC_PROFILE_MASK) << H264ENC_PROFILE_SHIFT_BITS;
    mEncConfig->profileAndLevel = profilePart | mAVCEncLevel;
    OMX_LOGI("mEncConfig->profileAndLevel %x", mEncConfig->profileAndLevel);
    mEncConfig->prependSpsPpsEnable = ((mPrependSPSPPS == OMX_FALSE) ? 0 : 1);
    mEncConfig->encSceneMode = mEncSceneMode;
    CheckUpdateColorAspects(mEncConfig);
    OMX_LOGI("rateCtrlEnable: %d, targetBitRate: %d, frameRate: %d, pFrames: %d",
        mEncConfig->rateCtrlEnable, mEncConfig->targetBitRate, mEncConfig->frameRate, mEncConfig->pFrames);
    OMX_LOGI("qpIvop: %d, qpPvop: %d, vbvBufSize: %u, profileAndLevel: %d",
        mEncConfig->qpIvop, mEncConfig->qpPvop, mEncConfig->vbvBufSize, mEncConfig->profileAndLevel);
    OMX_LOGI("prependSpsPpsEnable: %d, encSceneMode: %d",
        mEncConfig->prependSpsPpsEnable, mEncConfig->encSceneMode);
    if ((*mH264EncSetConf)(mHandle, mEncConfig)  != 0) {
        OMX_LOGE("Failed to set default encoding parameters");
        return OMX_ErrorUndefined;
    }
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SPRDAVCEncoder::releaseEncoder()
{
    if (mEncoderSymbolsReady && mEncoderPreInitDone && mH264EncRelease != nullptr && mHandle != nullptr) {
        (*mH264EncRelease)(mHandle);
    }
    if (mPbuf_inter != nullptr) {
        free(mPbuf_inter);
        mPbuf_inter = nullptr;
    }
    delete mEncConfig;
    mEncConfig = nullptr;
    delete mHandle;
    mHandle = nullptr;
    ResetEncoderSymbols();
    mStarted = false;
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SPRDAVCEncoder::releaseResource()
{
    if (mPbufExtraV != nullptr) {
        if (mIOMMUEnabled && mEncoderSymbolsReady && mH264EncFreeIOVA != nullptr && mHandle != nullptr) {
            (*mH264EncFreeIOVA)(mHandle, mPbufExtraP, mPbufExtraSize);
        }
        releaseMemIon(&mPmemExtra);
        mPbufExtraV = nullptr;
        mPbufExtraP = 0;
        mPbufExtraSize = 0;
    }
    for (int i = 0; i < mMFEnum; i++) {
        if (mStreamBfr.commonBufferPtr[i] !=  nullptr) {
            if (mIOMMUEnabled && mEncoderSymbolsReady && mH264EncFreeIOVA != nullptr && mHandle != nullptr) {
                (*mH264EncFreeIOVA)(mHandle, mStreamBfr.commonBufferPtrPhy[i], mStreamBfr.size);
            }
            releaseMemIon(&mPmemStream[i]);
            mStreamBfr.commonBufferPtr[i] = nullptr;
            mStreamBfr.commonBufferPtrPhy[i] = 0;
        }
    }
    mStreamBfr.size = 0;
    if (mPbuf_yuv_v != nullptr) {
        if (mIOMMUEnabled && mEncoderSymbolsReady && mH264EncFreeIOVA != nullptr && mHandle != nullptr) {
            (*mH264EncFreeIOVA)(mHandle, mPbuf_yuv_p, mPbuf_yuv_size);
        }
        releaseMemIon(&mYUVInPmemHeap);
        mPbuf_yuv_v = nullptr;
        mPbuf_yuv_p = 0;
        mPbuf_yuv_size = 0;
    }
    return SprdVideoEncoderBase::releaseResource();
}
void SPRDAVCEncoder::InitPorts() const
{
    const size_t kInputBufferSize = (mVideoWidth * mVideoHeight * H264ENC_BUFFER_SIZE_MULTIPLIATOR) >>
        H264ENC_BUFFER_SIZE_DIVISOR;
    const size_t kOutputBufferSize = kInputBufferSize >> H264ENC_OUTPUT_BUFFER_SIZE_DIVISOR;
    SprdVideoEncoderBase::InitPorts(H264ENC_NUM_BUFFERS_DEFAULT,
        kInputBufferSize, kOutputBufferSize, H264ENC_MIME_TYPE, OMX_VIDEO_CodingAVC);
}
OMX_ERRORTYPE SPRDAVCEncoder::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    switch (index) {
        case OMX_IndexParamVideoPortFormat: {
            return SprdVideoEncoderBase::getParamVideoPortFormat(OMX_VIDEO_CodingAVC, params);
        }
        case OMX_IndexParamSupportBufferType: {
            return getSupportBufferType(static_cast<UseBufferType *>(params));
        }
        case OMX_IndexParamVideoAvc: {
            return getVideoAvc(static_cast<OMX_VIDEO_PARAM_AVCTYPE *>(params));
        }
        case OMX_IndexCodecVideoPortFormat: {
            return getCodecVideoPortFormat(static_cast<CodecVideoPortFormatParam *>(params));
        }
        case OMX_IndexParamVideoProfileLevelQuerySupported: {
            return getProfileLevelQuerySupported(static_cast<OMX_VIDEO_PARAM_PROFILELEVELTYPE *>(params));
        }
        default:
            return SprdVideoEncoderBase::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDAVCEncoder::getSupportBufferType(UseBufferType *defParams)
{
    if (CheckParam(defParams, sizeof(UseBufferType)) || defParams->portIndex > 1) {
        OMX_LOGE("port index error.");
        return OMX_ErrorUnsupportedIndex;
    }
    defParams->bufferType = (defParams->portIndex == K_INPUT_PORT_INDEX) ?
        CODEC_BUFFER_TYPE_DYNAMIC_HANDLE : CODEC_BUFFER_TYPE_AVSHARE_MEM_FD;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoder::getVideoAvc(OMX_VIDEO_PARAM_AVCTYPE *avcParams)
{
    if (avcParams->nPortIndex != 1) {
        return OMX_ErrorUndefined;
    }
    avcParams->eProfile = OMX_VIDEO_AVCProfileBaseline;
    OMX_U32 omxLevel = H264ENC_LEVEL_DEFAULT;
    if (ConvertAvcSpecLevelToOmxAvcLevel(mAVCEncLevel, &omxLevel) != OMX_ErrorNone) {
        return OMX_ErrorUndefined;
    }
    avcParams->eLevel = static_cast<OMX_VIDEO_AVCLEVELTYPE>(omxLevel);
    avcParams->nRefFrames = 1;
    avcParams->nBFrames = 0;
    avcParams->bUseHadamard = OMX_TRUE;
    avcParams->nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
    avcParams->nRefIdx10ActiveMinus1 = 0;
    avcParams->nRefIdx11ActiveMinus1 = 0;
    avcParams->bWeightedPPrediction = OMX_FALSE;
    avcParams->bEntropyCodingCABAC = OMX_FALSE;
    avcParams->bconstIpred = OMX_FALSE;
    avcParams->bDirect8x8Inference = OMX_FALSE;
    avcParams->bDirectSpatialTemporal = OMX_FALSE;
    avcParams->nCabacInitIdc = 0;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoder::getCodecVideoPortFormat(CodecVideoPortFormatParam *formatParams)
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

OMX_ERRORTYPE SPRDAVCEncoder::getProfileLevelQuerySupported(OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel)
{
    if (profileLevel->nPortIndex != 1) {
        return OMX_ErrorUndefined;
    }
    size_t index = profileLevel->nProfileIndex;
    size_t nProfileLevels = sizeof(kProfileLevels) / sizeof(kProfileLevels[0]);
    if (index >= nProfileLevels) {
        return OMX_ErrorNoMore;
    }
    profileLevel->eProfile = kProfileLevels[index].mProfile;
    profileLevel->eLevel = kProfileLevels[index].mLevel;
    OMX_U32 maxOmxProfile = OMX_VIDEO_AVCProfileHigh;
    if (ConvertAvcSpecProfileToOmxAvcProfile(mCapability.profile, &maxOmxProfile) == STATUS_OK &&
        profileLevel->eProfile > maxOmxProfile) {
        profileLevel->eProfile = maxOmxProfile;
    }
    OMX_U32 maxOmxLevel = OMX_VIDEO_AVCLevel51;
    if (ConvertAvcSpecLevelToOmxAvcLevel(mCapability.level, &maxOmxLevel) == STATUS_OK &&
        profileLevel->eLevel > maxOmxLevel) {
        profileLevel->eLevel = maxOmxLevel;
    }
    OMX_LOGV("index = %zd, eProfile = %d, eLevel = %d", index, profileLevel->eProfile, profileLevel->eLevel);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoder::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params)
{
    OMX_LOGI("%s, %d, cmd index: 0x%x", __FUNCTION__, __LINE__, index);
    switch (static_cast<int>(index)) {
        case OMX_IndexParamUseBufferType: {
            return setUseBufferType(static_cast<const UseBufferType *>(params));
        }
        case OMX_INDEX_STORE_META_DATA_IN_BUFFERS: {
            const StoreMetaDataInBuffersParams *metadataParams =
                static_cast<const StoreMetaDataInBuffersParams *>(params);
            if (metadataParams == nullptr ||
                metadataParams->nPortIndex != OHOS::OMX::K_INPUT_PORT_INDEX) {
                OMX_LOGE("invalid StoreMetaDataInBuffersParams");
                return OMX_ErrorBadPortIndex;
            }
            mStoreMetaData = metadataParams->bStoreMetaData;
            iUseOhosNativeBuffer[OHOS::OMX::K_INPUT_PORT_INDEX] =
                (metadataParams->bStoreMetaData == OMX_TRUE) ? OMX_TRUE : OMX_FALSE;
            OMX_LOGI("set metadata mode: port=%u, enable=%d",
                metadataParams->nPortIndex, metadataParams->bStoreMetaData);
            return OMX_ErrorNone;
        }
        case OMX_IndexParamProcessName: {
            return setProcessName(static_cast<const ProcessNameParam *>(params));
        }
        case OMX_IndexParamVideoBitrate: {
            return setVideoBitrate(static_cast<const OMX_VIDEO_PARAM_BITRATETYPE *>(params));
        }
        case OMX_IndexParamPortDefinition: {
            return setPortDefinition(params);
        }
        case OMX_IndexCodecVideoPortFormat: {
            return setCodecVideoPortFormat(static_cast<const CodecVideoPortFormatParam *>(params));
        }
        case OMX_IndexParamStandardComponentRole: {
            return SprdVideoEncoderBase::setParamStandardComponentRole("video_encoder.avc", params);
        }
        case OMX_IndexParamVideoPortFormat: {
            return SprdVideoEncoderBase::setParamVideoPortFormat(OMX_VIDEO_CodingAVC, params);
        }
        case OMX_IndexParamVideoAvc: {
            return setVideoAvc(static_cast<const OMX_VIDEO_PARAM_AVCTYPE *>(params));
        }
        default:
            return SprdVideoEncoderBase::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDAVCEncoder::setPortDefinition(const OMX_PTR params)
{
    OMX_PARAM_PORTDEFINITIONTYPE *def = (OMX_PARAM_PORTDEFINITIONTYPE *)params;
    mStride = (def->format.video.nFrameWidth + H264ENC_STRIDE_ALIGNMENT - 1) & H264ENC_ALIGNMENT_MASK;
    def->format.video.nStride = mStride;
    def->format.video.nSliceHeight = def->format.video.nFrameHeight;
    OMX_LOGI("OMX_IndexParamPortDefinition nFrameWidth : %d, mStride: %u",
        def->format.video.nFrameWidth, mStride);
    return SprdVideoEncoderBase::setParamPortDefinition(OMX_VIDEO_CodingAVC, OMX_IndexParamPortDefinition, params);
}

OMX_ERRORTYPE SPRDAVCEncoder::setUseBufferType(const UseBufferType *defParams)
{
    iUseOhosNativeBuffer[defParams->portIndex] = OMX_TRUE;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoder::setProcessName(const ProcessNameParam *nameParams) const
{
    if (nameParams == nullptr) {
        OMX_LOGE("ProcessNameParam is nullptr");
        return OMX_ErrorBadParameter;
    }
    OMX_LOGI("OMX_IndexParamProcessName : %s", nameParams->processName);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoder::setVideoBitrate(const OMX_VIDEO_PARAM_BITRATETYPE *bitRate)
{
    if (bitRate->nPortIndex != 1 ||
        (bitRate->eControlRate != OMX_Video_ControlRateDisable &&
        bitRate->eControlRate != OMX_Video_ControlRateVariable &&
        bitRate->eControlRate != OMX_Video_ControlRateConstant)) {
        OMX_LOGI("OMX_ErrorUndefined bitRate->eControlRate %d", bitRate->eControlRate);
        return OMX_ErrorUndefined;
    }
    mVideoBitRate = bitRate->nTargetBitrate;
    mrateCtrlEnable = (bitRate->eControlRate == OMX_Video_ControlRateDisable) ? 0 : 1;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoder::setCodecVideoPortFormat(const CodecVideoPortFormatParam *formatParams)
{
    if (CheckParam(const_cast<CodecVideoPortFormatParam *>(formatParams), sizeof(CodecVideoPortFormatParam))) {
        return OMX_ErrorUnsupportedIndex;
    }
    PortInfo *port = editPortInfo(formatParams->portIndex);
    port->mDef.format.video.eColorFormat = static_cast<OMX_COLOR_FORMATTYPE>(formatParams->codecColorFormat);
    port->mDef.format.video.eCompressionFormat = static_cast<OMX_VIDEO_CODINGTYPE>(formatParams->codecCompressFormat);
    port->mDef.format.video.xFramerate = static_cast<OMX_U32>(formatParams->framerate);
    mVideoColorFormat = port->mDef.format.video.eColorFormat;
    OMX_LOGI("internalSetParameter, eCompressionFormat: %d, eColorFormat: 0x%x",
        port->mDef.format.video.eCompressionFormat, port->mDef.format.video.eColorFormat);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCEncoder::setVideoAvc(const OMX_VIDEO_PARAM_AVCTYPE *avcType)
{
    if (avcType->nPortIndex != 1) {
        return OMX_ErrorUndefined;
    }
    mPFrames = avcType->nPFrames;
    OMX_LOGI("%s, mPFrames: %d", __FUNCTION__, mPFrames);
    if (ConvertOmxAvcProfileToAvcSpecProfile(avcType->eProfile, &mAVCEncProfile) != STATUS_OK) {
        OMX_LOGE("ConvertOmxAvcProfileToAvcSpecProfile error");
        return OMX_ErrorUndefined;
    }
    if (ConvertOmxAvcLevelToAvcSpecLevel(avcType->eLevel, &mAVCEncLevel) != STATUS_OK) {
        OMX_LOGE("ConvertOmxAvcLevelToAvcSpecLevel error");
        return OMX_ErrorUndefined;
    }
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SPRDAVCEncoder::getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index)
{
    if (strcmp(name, "OMX.index.prependSPSPPSToIDRFrames") == 0) {
        *index = (OMX_INDEXTYPE) OMX_INDEX_PREPEND_SPSPPS_TO_IDR;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.sprd.index.RecordMode") == 0) {
        *index = (OMX_INDEXTYPE) OMX_INDEX_CONFIG_VIDEO_RECORD_MODE;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.sprd.index.EncSceneMode") == 0) {
        *index = (OMX_INDEXTYPE) OMX_INDEX_CONFIG_ENC_SCENE_MODE;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.index.describeColorAspects") == 0) {
        *index = (OMX_INDEXTYPE) OMX_INDEX_DESCRIBE_COLOR_ASPECTS;
        return OMX_ErrorNone;
    }
    return SprdVideoEncoderBase::getExtensionIndex(name, index);
}
// static
void SPRDAVCEncoder::FlushCacheWrapper(const void *aUserData)
{
    static_cast<const SPRDAVCEncoder *>(aUserData)->FlushCacheforBsBuf();
    return;
}
void SPRDAVCEncoder::FlushCacheforBsBuf() const
{
    for (int i = 0; i < mMFEnum; i++) {
        mPmemStream[i]->cpuSyncStart();
    }
    return;
}
void SPRDAVCEncoder::BeginCPUAccessWrapper(const void *aUserData)
{
    static_cast<const SPRDAVCEncoder *>(aUserData)->BeginCpuAccessforBsBuf();
    return;
}
void SPRDAVCEncoder::BeginCpuAccessforBsBuf() const
{
    int fd = 0;
    struct dma_buf_sync sync = { 0 };
    for (int i = 0; i < mMFEnum; i++) {
        fd = mPmemStream[i]->getHeapID();
        sync.flags = DMA_BUF_SYNC_START_FLAG;
        ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
    }
    return;
}
void SPRDAVCEncoder::EndCPUAccessWrapper(const void *aUserData)
{
    static_cast<const SPRDAVCEncoder *>(aUserData)->EndCpuAccessforBsBuf();
    return;
}
void SPRDAVCEncoder::EndCpuAccessforBsBuf() const
{
    int fd;
    struct dma_buf_sync sync = { 0 };
    for (int i = 0; i < mMFEnum; i++) {
        fd = mPmemStream[i]->getHeapID();
        sync.flags = DMA_BUF_SYNC_END_FLAG;
        ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
    }
    return;
}

void SPRDAVCEncoder::resetOutputHeader(OMX_BUFFERHEADERTYPE *outHeader)
{
    outHeader->nTimeStamp = 0;
    outHeader->nFlags = 0;
    outHeader->nOffset = 0;
    outHeader->nFilledLen = 0;
}

bool SPRDAVCEncoder::appendEncodedData(OMX_BUFFERHEADERTYPE *outHeader, uint8_t *&outPtr,
    void *srcBuffer, int32_t srcSize, uint32_t &dataLength)
{
    if (srcBuffer == nullptr || srcSize <= 0) {
        return false;
    }
    size_t remainingSize = outHeader->nAllocLen -
        (outPtr - reinterpret_cast<uint8_t*>(outHeader->pBuffer));
    errno_t ret = memmove_s(outPtr, remainingSize, srcBuffer, static_cast<size_t>(srcSize));
    if (ret != 0) {
        OMX_LOGE("memmove_s failed at line %d, ret=%d", __LINE__, ret);
        return false;
    }
    outPtr += srcSize;
    dataLength += srcSize;
    return true;
}

bool SPRDAVCEncoder::SyncStreamBuffer(int fd, unsigned int flags)
{
    struct dma_buf_sync sync = { 0 };
    sync.flags = flags;
    int ret = ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
    if (ret == 0) {
        return true;
    }
    OMX_LOGE("%s, line:%d, fd:%d, return:%d, DMA_BUF_IOCTL_SYNC failed.",
        __FUNCTION__, __LINE__, fd, ret);
    return false;
}

void SPRDAVCEncoder::patchStartCodeAndLog(uint8_t *buffer, const char *tag, bool log16Bytes)
{
    if (buffer == nullptr) {
        return;
    }
    OMX_LOGI("%s: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x", tag,
        buffer[H264ENC_BYTE_INDEX_0], buffer[H264ENC_BYTE_INDEX_1], buffer[H264ENC_BYTE_INDEX_2],
        buffer[H264ENC_BYTE_INDEX_3], buffer[H264ENC_BYTE_INDEX_4], buffer[H264ENC_BYTE_INDEX_5],
        buffer[H264ENC_BYTE_INDEX_6], buffer[H264ENC_BYTE_INDEX_7]);
    if (log16Bytes) {
        OMX_LOGI("%s: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x", tag,
            buffer[H264ENC_BYTE_INDEX_8], buffer[H264ENC_BYTE_INDEX_9], buffer[H264ENC_BYTE_INDEX_10],
            buffer[H264ENC_BYTE_INDEX_11], buffer[H264ENC_BYTE_INDEX_12], buffer[H264ENC_BYTE_INDEX_13],
            buffer[H264ENC_BYTE_INDEX_14], buffer[H264ENC_BYTE_INDEX_15]);
    }
    buffer[H264ENC_BYTE_INDEX_0] = H264ENC_START_CODE_BYTE_0;
    buffer[H264ENC_BYTE_INDEX_1] = H264ENC_START_CODE_BYTE_0;
    buffer[H264ENC_BYTE_INDEX_2] = H264ENC_START_CODE_BYTE_0;
    buffer[H264ENC_BYTE_INDEX_3] = H264ENC_START_CODE_BYTE_3;
}

bool SPRDAVCEncoder::CopyHeaderToBuffer(MMEncOut &header, const char *tag, bool log16Bytes)
{
    uint8_t *buffer = reinterpret_cast<uint8_t*>(header.pOutBuf[0]);
    if (buffer == nullptr || header.pheaderBuf == nullptr || header.strmSize[0] <= 0) {
        OMX_LOGE("invalid %s header buffer", tag);
        return false;
    }
    patchStartCodeAndLog(buffer, tag, log16Bytes);
    size_t copySize = static_cast<size_t>(header.strmSize[0]);
    if (copySize > H264ENC_HEADER_BUFFER_SIZE) {
        OMX_LOGE("header copy overflow for %s, size=%zu", tag, copySize);
        return false;
    }
    errno_t ret = memmove_s(header.pheaderBuf, H264ENC_HEADER_BUFFER_SIZE, buffer, copySize);
    if (ret == 0) {
        return true;
    }
    OMX_LOGE("memmove_s error (%s header), ret=%d, copySize=%zu", tag, ret, copySize);
    return false;
}

bool SPRDAVCEncoder::generateCodecHeader(int fd, MMEncOut &header, int32_t headerType,
    const char *tag, bool log16Bytes)
{
    (void)memset_s(&header, sizeof(MMEncOut), 0, sizeof(MMEncOut));
    ++mNumInputFrames;
    int ret = (*mH264EncGenHeader)(mHandle, &header, headerType);
    if (ret < 0) {
        OMX_LOGE("%s, line:%d, mH264EncGenHeader failed, ret: %d", __FUNCTION__, __LINE__, ret);
        return false;
    }
    if (!SyncStreamBuffer(fd, DMA_BUF_SYNC_START_FLAG)) {
        return false;
    }
    OMX_LOGI("%s, %d, %sHeader.strmSize: %d", __FUNCTION__, __LINE__, tag, header.strmSize[0]);
    return CopyHeaderToBuffer(header, tag, log16Bytes);
}

bool SPRDAVCEncoder::emitCodecConfig(EncodeOutputState &output, std::list<BufferInfo*>& outQueue)
{
    output.shouldReturn = false;
    if (mSpsPpsHeaderReceived || mNumInputFrames > 0) {
        return true;
    }
    int fd = mPmemStream[0]->getHeapID();
    if (!AppendCodecConfigHeaders(&output, fd)) {
        return false;
    }
    mSpsPpsHeaderReceived = true;
    if (mNumInputFrames != 0) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((0) == (mNumInputFrames)) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    return finalizeCodecConfigOutput(output, outQueue);
}

bool SPRDAVCEncoder::AppendCodecConfigHeaders(EncodeOutputState *output, int fd)
{
    MMEncOut spsHeader;
    MMEncOut ppsHeader;
    if (output == nullptr) {
        return false;
    }
    if (!generateCodecHeader(fd, spsHeader, H264ENC_GEN_SPS_HEADER, "sps", true) ||
        !appendEncodedData(output->outHeader, output->outPtr, spsHeader.pOutBuf[0],
        spsHeader.strmSize[0], output->dataLength) ||
        !SyncStreamBuffer(fd, DMA_BUF_SYNC_END_FLAG) ||
        !generateCodecHeader(fd, ppsHeader, H264ENC_GEN_PPS_HEADER, "pps", false) ||
        !appendEncodedData(output->outHeader, output->outPtr, ppsHeader.pOutBuf[0],
        ppsHeader.strmSize[0], output->dataLength) ||
        !SyncStreamBuffer(fd, DMA_BUF_SYNC_END_FLAG)) {
        return false;
    }
    if (mDumpStrmEnabled) {
        DumpFiles(spsHeader.pOutBuf[0], spsHeader.strmSize[0], DUMP_STREAM);
        DumpFiles(ppsHeader.pOutBuf[0], ppsHeader.strmSize[0], DUMP_STREAM);
    }
    return true;
}

bool SPRDAVCEncoder::finalizeCodecConfigOutput(EncodeOutputState &output, std::list<BufferInfo*>& outQueue)
{
    if (SprdOMXUtils::notifyEncodeHeaderWithFirstFrame()) {
        return true;
    }
    output.outHeader->nFlags = OMX_BUFFERFLAG_CODECCONFIG;
    output.outHeader->nFilledLen = output.dataLength;
    outQueue.erase(outQueue.begin());
    output.outInfo->mOwnedByUs = false;
    notifyFillBufferDone(output.outHeader);
    OMX_LOGV("notifyFillBufferDone  sps/pps");
    output.shouldReturn = true;
    return true;
}

void SPRDAVCEncoder::queueInputBufferInfo(OMX_BUFFERHEADERTYPE *inHeader)
{
    InputBufferInfo info;
    info.mTimeUs = inHeader->nTimeStamp;
    info.mFlags = inHeader->nFlags;
    mInputBufferInfoVec.push_back(info);
}

bool SPRDAVCEncoder::mapGraphicBuffer(OMX_BUFFERHEADERTYPE *inHeader, GraphicBufferMapping &mapping)
{
    DynamicBuffer* buffer = reinterpret_cast<DynamicBuffer*>(inHeader->pBuffer);
    BufferHandle *bufferHandle = buffer->bufferHandle;
    OMX_LOGI("size = %u, fd = %d, mStride = %d, mFrameHeight = %d",
        bufferHandle->size, bufferHandle->fd, mStride, mFrameHeight);
    mapping.bufferSize = bufferHandle->size;
    mapping.py = reinterpret_cast<uint8_t*>(mmap(0, mapping.bufferSize, PROT_READ | PROT_WRITE,
        MAP_SHARED, bufferHandle->fd, 0));
    if (mapping.py == MAP_FAILED) {
        mapping.py = nullptr;
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] mmap failed.", __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    mapping.needUnmap = false;
    mapping.iova = 0;
    mapping.iovaLen = 0;
    if (mIOMMUEnabled) {
        if (!mEncoderSymbolsReady || mH264EncGetIOVA == nullptr || mHandle == nullptr) {
            OMX_LOGE("IOMMU enabled but encoder IOVA callback is unavailable");
            munmap(mapping.py, mapping.bufferSize);
            mapping.py = nullptr;
            return false;
        }
        if ((*mH264EncGetIOVA)(mHandle, bufferHandle->fd, &mapping.iova, &mapping.iovaLen) != 0) {
            OMX_LOGE("H264DecGetIOVA Failed: %d(%s)", errno, strerror(errno));
            munmap(mapping.py, mapping.bufferSize);
            mapping.py = nullptr;
            return false;
        }
    }
    mapping.needUnmap = mIOMMUEnabled;
    mapping.pyPhy = mIOMMUEnabled ? reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(mapping.iova)) : mapping.py;
    OMX_LOGW("py = %p, pyPhy = %p", mapping.py, mapping.pyPhy);
    return mapping.pyPhy != nullptr;
}

void SPRDAVCEncoder::UnmapGraphicBuffer(const GraphicBufferMapping &mapping)
{
    if (mapping.needUnmap) {
        if (mEncoderSymbolsReady && mH264EncFreeIOVA != nullptr && mHandle != nullptr) {
            OMX_LOGV("Free_iova, iova: 0x%lx, size: %zu", mapping.iova, mapping.iovaLen);
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

void SPRDAVCEncoder::configureEncodeInput(MMEncIn &vidIn, OMX_BUFFERHEADERTYPE *inHeader,
    uint8_t *py, uint8_t *pyPhy)
{
    (void)memset_s(&vidIn, sizeof(MMEncIn), 0, sizeof(MMEncIn));
    switch (mVideoColorFormat) {
        case OMX_COLOR_FORMAT_RGBA32:
            vidIn.yuvFormat = MMENC_RGBA32;
            break;
        case OMX_COLOR_FORMAT_NV12:
            vidIn.yuvFormat = MMENC_YUV420SP_NV12;
            break;
        case OMX_COLOR_FORMAT_NV21:
            vidIn.yuvFormat = MMENC_YUV420SP_NV21;
            break;
        default:
            break;
    }
    vidIn.timeStamp = (inHeader->nTimeStamp + H264ENC_TIMESTAMP_ROUNDING_OFFSET) /
        H264ENC_TIMESTAMP_CONVERSION_DIVISOR;
    vidIn.channelQuality = H264ENC_CHANNEL_QUALITY_DEFAULT;
    vidIn.bitrate = mBitrate;
    vidIn.isChangeBitrate = mIsChangeBitrate;
    mIsChangeBitrate = false;
    vidIn.needIvoP = mKeyFrameRequested || (mNumInputFrames == 0);
    vidIn.srcYuvAddr[0].pSrcY = py;
    vidIn.srcYuvAddr[0].pSrcU = py + mVideoWidth * mVideoHeight;
    vidIn.srcYuvAddr[0].pSrcV = nullptr;
    vidIn.srcYuvAddr[0].pSrcYPhy = pyPhy;
    vidIn.srcYuvAddr[0].pSrcUPhy = pyPhy + mVideoWidth * mVideoHeight;
    vidIn.srcYuvAddr[0].pSrcVPhy = nullptr;
    vidIn.orgImgWidth = static_cast<int32_t>(mFrameWidth);
    vidIn.orgImgHeight = static_cast<int32_t>(mFrameHeight);
    vidIn.cropX = H264ENC_CROP_OFFSET_DEFAULT;
    vidIn.cropY = H264ENC_CROP_OFFSET_DEFAULT;
}

void SPRDAVCEncoder::SyncAllStreamBuffers()
{
    for (int i = 0; i < mMFEnum; i++) {
        if (mPmemStream[i] != nullptr) {
            mPmemStream[i]->cpuSyncStart();
        }
    }
}

bool SPRDAVCEncoder::PatchEncodedFrame(MMEncOut &vidOut)
{
    uint8_t *buffer = reinterpret_cast<uint8_t*>(vidOut.pOutBuf[0]);
    if (buffer == nullptr || vidOut.strmSize[0] <= H264ENC_START_CODE_PREFIX_SIZE - 1) {
        return vidOut.strmSize[0] >= 0;
    }
    OMX_LOGV("frame: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, ",
        buffer[H264ENC_BYTE_INDEX_0], buffer[H264ENC_BYTE_INDEX_1], buffer[H264ENC_BYTE_INDEX_2],
        buffer[H264ENC_BYTE_INDEX_3], buffer[H264ENC_BYTE_INDEX_4], buffer[H264ENC_BYTE_INDEX_5],
        buffer[H264ENC_BYTE_INDEX_6], buffer[H264ENC_BYTE_INDEX_7]);
    buffer[H264ENC_BYTE_INDEX_0] = H264ENC_START_CODE_BYTE_0;
    buffer[H264ENC_BYTE_INDEX_1] = H264ENC_START_CODE_BYTE_0;
    buffer[H264ENC_BYTE_INDEX_2] = H264ENC_START_CODE_BYTE_0;
    buffer[H264ENC_BYTE_INDEX_3] = H264ENC_START_CODE_BYTE_3;
    return true;
}

bool SPRDAVCEncoder::encodeInputBuffer(OMX_BUFFERHEADERTYPE *inHeader, EncodeOutputState &output)
{
    GraphicBufferMapping mapping = { nullptr, nullptr, 0, 0, 0, false };
    if (!mapGraphicBuffer(inHeader, mapping)) {
        return false;
    }
    MMEncOut vidOut;
    int ret = MMENC_OK;
    if (!runEncode(mapping, inHeader, vidOut, ret)) {
        UnmapGraphicBuffer(mapping);
        return false;
    }
    UnmapGraphicBuffer(mapping);
    if (vidOut.strmSize[0] < 0 || ret != MMENC_OK || !PatchEncodedFrame(vidOut)) {
        OMX_LOGE("Failed to encode frame %lld, ret=%d", mNumInputFrames, ret);
        return true;
    }
    return AppendEncodedFrame(vidOut, &output);
}

bool SPRDAVCEncoder::runEncode(
    const GraphicBufferMapping &mapping, OMX_BUFFERHEADERTYPE *inHeader, MMEncOut &vidOut, int &ret)
{
    MMEncIn vidIn;
    configureEncodeInput(vidIn, inHeader, mapping.py, mapping.pyPhy);
    (void)memset_s(&vidOut, sizeof(MMEncOut), 0, sizeof(MMEncOut));
    if (mDumpYUVEnabled) {
        size_t yuvSize = mVideoWidth * mVideoHeight * H264ENC_YUV420_SIZE_MULTIPLIATOR /
            H264ENC_YUV420_SIZE_DIVISOR;
        DumpFiles(mapping.py, yuvSize, DUMP_YUV);
    }
    SyncAllStreamBuffers();
    int64_t startEncode = systemTime();
    ret = (*mH264EncStrmEncode)(mHandle, &vidIn, &vidOut);
    int64_t endEncode = systemTime();
    OMX_LOGI("H264EncStrmEncode[%lld] %dms, in { %p-%p, %dx%d, %d}", mNumInputFrames,
        (unsigned int)((endEncode - startEncode) / H264ENC_ENCODE_TIME_DIVISOR),
        mapping.py, mapping.pyPhy, mVideoWidth, mVideoHeight, vidIn.yuvFormat);
    OMX_LOGI("out {%p-%d, %d}", vidOut.pOutBuf[0], vidOut.strmSize[0], vidOut.vopType[0]);
    return ret == MMENC_OK;
}

bool SPRDAVCEncoder::AppendEncodedFrame(MMEncOut &vidOut, EncodeOutputState *output)
{
    if (output == nullptr) {
        return false;
    }
    if (mDumpStrmEnabled) {
        DumpFiles(vidOut.pOutBuf[0], vidOut.strmSize[0], DUMP_STREAM);
    }
    if (!appendEncodedData(output->outHeader, output->outPtr, vidOut.pOutBuf[0],
        vidOut.strmSize[0], output->dataLength)) {
        return true;
    }
    if (vidOut.vopType[0] == H264ENC_VOP_TYPE_I) {
        mKeyFrameRequested = false;
        output->outHeader->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
    }
    ++mNumInputFrames;
    return true;
}

bool SPRDAVCEncoder::FinalizeOutputBuffer(EncodeFrameContext &frame)
{
    if ((frame.inHeader->nFlags & OMX_BUFFERFLAG_EOS) && (frame.inHeader->nFilledLen == 0)) {
        OMX_LOGI("saw EOS");
        frame.outHeader->nFlags = OMX_BUFFERFLAG_EOS;
    }
    frame.inQueue->erase(frame.inQueue->begin());
    frame.inInfo->mOwnedByUs = false;
    notifyEmptyBufferDone(frame.inHeader);
    if (!ValidateOutputDimensions()) {
        return false;
    }
    if (mInputBufferInfoVec.empty()) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(!mInputBufferInfoVec.empty() failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    InputBufferInfo inputBufInfo = mInputBufferInfoVec[0];
    if (!submitEncodedOutput(frame, inputBufInfo)) {
        return false;
    }
    mInputBufferInfoVec.erase(mInputBufferInfoVec.begin());
    return true;
}

bool SPRDAVCEncoder::ValidateOutputDimensions()
{
    if (mVideoWidth <= mCapability.maxWidth && mVideoHeight <= mCapability.maxHeight) {
        return true;
    }
    OMX_LOGE("[%d, %d] is out of range [%d, %d], failed to support this format.",
        mVideoWidth, mVideoHeight, mCapability.maxWidth, mCapability.maxHeight);
    notify(OMX_EventError, OMX_ErrorBadParameter, 0, nullptr);
    mSignalledError = true;
    return false;
}

bool SPRDAVCEncoder::submitEncodedOutput(EncodeFrameContext &frame, const InputBufferInfo &inputBufInfo)
{
    if (!(frame.output.dataLength > 0 || (frame.inHeader->nFlags & OMX_BUFFERFLAG_EOS))) {
        return true;
    }
    frame.outQueue->erase(frame.outQueue->begin());
    frame.outHeader->nTimeStamp = inputBufInfo.mTimeUs;
    frame.outHeader->nFlags |= (inputBufInfo.mFlags | OMX_BUFFERFLAG_ENDOFFRAME);
    frame.outHeader->nFilledLen = frame.output.dataLength;
    frame.outInfo->mOwnedByUs = false;
    notifyFillBufferDone(frame.outHeader);
    return true;
}

SPRDAVCEncoder::EncodeFrameContext SPRDAVCEncoder::BuildFrameContext(
    std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue)
{
    BufferInfo *inInfo = *inQueue.begin();
    BufferInfo *outInfo = *outQueue.begin();
    return { &inQueue, &outQueue, inInfo, outInfo, inInfo->mHeader, outInfo->mHeader,
        { outInfo, outInfo->mHeader, reinterpret_cast<uint8_t*>(outInfo->mHeader->pBuffer), 0, false } };
}

void SPRDAVCEncoder::FailCurrentFrame(EncodeFrameContext &frame, bool inputInfoQueued)
{
    if (frame.inInfo->mOwnedByUs) {
        frame.inQueue->erase(frame.inQueue->begin());
        frame.inInfo->mOwnedByUs = false;
        notifyEmptyBufferDone(frame.inHeader);
    }
    if (frame.outInfo->mOwnedByUs) {
        frame.outQueue->erase(frame.outQueue->begin());
        frame.outHeader->nFilledLen = 0;
        frame.outHeader->nOffset = 0;
        frame.outHeader->nFlags = 0;
        frame.outInfo->mOwnedByUs = false;
        notifyFillBufferDone(frame.outHeader);
    }
    if (inputInfoQueued && !mInputBufferInfoVec.empty()) {
        mInputBufferInfoVec.erase(mInputBufferInfoVec.begin());
    }
    mSignalledError = true;
    notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
}

bool SPRDAVCEncoder::processOneInputFrame(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue)
{
    EncodeFrameContext frame = BuildFrameContext(inQueue, outQueue);
    resetOutputHeader(frame.outHeader);
    bool inputInfoQueued = false;
    OMX_LOGI("outHeader->nAllocLen = %d", frame.outHeader->nAllocLen);
    if (!emitCodecConfig(frame.output, outQueue)) {
        FailCurrentFrame(frame, inputInfoQueued);
        return false;
    }
    if (frame.output.shouldReturn) {
        return false;
    }
    OMX_LOGV("%s, line:%d, inHeader->nFilledLen: %d, mStoreMetaData: %d",
        __FUNCTION__, __LINE__, frame.inHeader->nFilledLen, mStoreMetaData);
    OMX_LOGV("mVideoColorFormat: 0x%x, inHeader->nOffset = %d, inHeader->nAllocLen = %d",
        mVideoColorFormat, frame.inHeader->nOffset, frame.inHeader->nAllocLen);
    queueInputBufferInfo(frame.inHeader);
    inputInfoQueued = true;
    if (frame.inHeader->nFilledLen > 0 && !encodeInputBuffer(frame.inHeader, frame.output)) {
        FailCurrentFrame(frame, inputInfoQueued);
        return false;
    }
    return FinalizeOutputBuffer(frame);
}

void SPRDAVCEncoder::onQueueFilled(OMX_U32 portIndex)
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
    std::list<BufferInfo*>& inQueue = getPortQueue(0);
    std::list<BufferInfo*>& outQueue = getPortQueue(1);
    while (!inQueue.empty() && !outQueue.empty()) {
        if (!processOneInputFrame(inQueue, outQueue)) {
            return;
        }
    }
}
bool SPRDAVCEncoder::OpenEncoder(const char *libName)
{
    if (mLibHandle) {
        DeferAvcEncoderDlclose(mLibHandle, "reopen");
        mLibHandle = nullptr;
    }
    ResetEncoderSymbols();
    OMX_LOGI("OpenEncoder, lib: %s", libName);
    mLibHandle = dlopen(libName, RTLD_NOW);
    if (mLibHandle == nullptr) {
        OMX_LOGE("OpenEncoder, can't open lib: %s", libName);
        return false;
    }
    if (!LoadEncoderSymbols(libName)) {
        ResetEncoderSymbols();
        return CloseEncoderLibraryOnFailure(&mLibHandle);
    }
    mEncoderSymbolsReady = true;
    return true;
}

bool SPRDAVCEncoder::LoadEncoderSymbols(const char *libName)
{
    return LoadEncoderSymbol(mLibHandle, libName, "H264EncPreInit", &mH264EncPreInit) &&
        LoadEncoderSymbol(mLibHandle, libName, "H264EncGetFnum", &mH264EncGetFnum) &&
        LoadEncoderSymbol(mLibHandle, libName, "H264EncInit", &mH264EncInit) &&
        LoadEncoderSymbol(mLibHandle, libName, "H264EncSetConf", &mH264EncSetConf) &&
        LoadEncoderSymbol(mLibHandle, libName, "H264EncGetConf", &mH264EncGetConf) &&
        LoadEncoderSymbol(mLibHandle, libName, "H264EncStrmEncode", &mH264EncStrmEncode) &&
        LoadEncoderSymbol(mLibHandle, libName, "H264EncGenHeader", &mH264EncGenHeader) &&
        LoadEncoderSymbol(mLibHandle, libName, "H264EncRelease", &mH264EncRelease) &&
        LoadEncoderSymbol(mLibHandle, libName, "H264EncGetCodecCapability", &mH264EncGetCodecCapability) &&
        LoadEncoderSymbol(mLibHandle, libName, "H264Enc_get_iova", &mH264EncGetIOVA) &&
        LoadEncoderSymbol(mLibHandle, libName, "H264Enc_free_iova", &mH264EncFreeIOVA) &&
        LoadEncoderSymbol(mLibHandle, libName, "H264Enc_get_IOMMU_status", &mH264EncGetIOMMUStatus) &&
        LoadEncoderSymbol(mLibHandle, libName, "H264Enc_NeedAlign", &mH264EncNeedAlign);
}
int32_t SPRDAVCEncoder::ExtMemAllocWrapper(
    void *aUserData, unsigned int sizeExtra, MMCodecBuffer *extraMemBfr)
{
    return static_cast<SPRDAVCEncoder *>(aUserData)->VspMallocCb(sizeExtra, extraMemBfr);
}
int32 SPRDAVCEncoder::VspMallocCb(uint32 sizeExtra, MMCodecBuffer *extraMemBfr)
{
    int fd;
    unsigned long phyAddr;
    size_t size;
    if (sizeExtra == 0 || extraMemBfr == nullptr || mHandle == nullptr) {
        OMX_LOGE("Invalid VspMallocCb params, sizeExtra: %u, extraMemBfr: %p, mHandle: %p",
            sizeExtra, static_cast<void *>(extraMemBfr), static_cast<void *>(mHandle));
        return -1;
    }
    ReleaseExtraBuffer();
    mPmemExtra = new VideoMemAllocator(sizeExtra, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    if ((fd = mPmemExtra->getHeapID()) < 0) {
        OMX_LOGE("Failed to alloc extra memory\n");
        return -1;
    }
    if (mIOMMUEnabled) {
        if (!mEncoderSymbolsReady || mH264EncGetIOVA == nullptr) {
            OMX_LOGE("IOMMU enabled but encoder IOVA callback is unavailable");
            return -1;
        }
        (*mH264EncGetIOVA)(reinterpret_cast<AVCHandle*>(mHandle), fd, &phyAddr, &size);
    } else {
        mPmemExtra->getPhyAddrFromMemAlloc(&phyAddr, &size);
    }
    mPbufExtraV = reinterpret_cast<uint8*>(mPmemExtra->getBase());
    mPbufExtraP = phyAddr;
    mPbufExtraSize = size;
    extraMemBfr->commonBufferPtr = mPbufExtraV;
    extraMemBfr->commonBufferPtrPhy = mPbufExtraP;
    extraMemBfr->size = mPbufExtraSize;
    return 0;
}

void SPRDAVCEncoder::ReleaseExtraBuffer()
{
    if (mPmemExtra == nullptr) {
        return;
    }
    if (mIOMMUEnabled && mEncoderSymbolsReady && mH264EncFreeIOVA != nullptr && mHandle != nullptr) {
        (*mH264EncFreeIOVA)(reinterpret_cast<AVCHandle*>(mHandle), mPbufExtraP, mPbufExtraSize);
    } else if (mIOMMUEnabled) {
        OMX_LOGW("skip free extra IOVA due to incomplete encoder state");
    }
    releaseMemIon(&mPmemExtra);
    mPmemExtra = nullptr;
    mPbufExtraP = 0;
    mPbufExtraSize = 0;
    OMX_LOGI("Free extra buffer\n");
}
SprdOMXComponent *createSprdOMXComponent(
    const char *name, const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData, OMX_COMPONENTTYPE **component)
{
    return new SPRDAVCEncoder(name, callbacks, appData, component);
}
}
}
