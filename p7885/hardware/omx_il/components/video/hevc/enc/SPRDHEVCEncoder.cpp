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
#define LOG_TAG "SPRDHEVCEncoder"
#include <securec.h>
#include <utils/omx_log.h>
#include "hevc_enc_api.h"
#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#include <dlfcn.h>
#include "SPRDHEVCEncoder.h"
#include <OMX_IndexExt.h>
#include "sprd_omx_typedef.h"
#include "OMXGraphicBufferMapper.h"
#include "OMXMetadataBufferType.h"
#include "codec_omx_ext.h"
#include "Dynamic_buffer.h"
#include <cstdint>
#include <cinttypes>
#include <utils/time_util.h>
#include <sys/mman.h>
#include <unistd.h>
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>
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
#define FRAME_ALIGNMENT_32              31  /* Alignment to 32 pixels (for width and height) */
/* YUV buffer size calculation factor */
#define YUV_SIZE_MULTIPLIER_NUMERATOR   3
#define YUV_SIZE_MULTIPLIER_DENOMINATOR 2
/* Stream buffer size calculation factors */
#define STREAM_SIZE_DIVISOR_1080P       3   /* Divisor for stream buffer size for resolutions > 1080p */
#define STREAM_SIZE_DIVISOR_OTHER       2   /* Divisor for stream buffer size for other resolutions */
/* H.265 NAL unit start code */
#define NAL_START_CODE_PREFIX_SIZE      3
#define NAL_START_CODE                  0x01
/* Initial frame count for encoder initialization */
#define INITIAL_FRAME_COUNT             (-3)  /* First three buffers contain VPS, SPS and PPS */
/* Default channel quality for encoding */
#define DEFAULT_CHANNEL_QUALITY         1
/* Maximum header size for VPS/SPS/PPS */
#define MAX_HEADER_BYTES_TO_LOG         16
/* DMA buffer synchronization flags */
#define DMA_SYNC_START_FLAGS            (DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW)
#define DMA_SYNC_END_FLAGS              (DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW)
/* Scene mode enumeration */
#define ENC_SCENE_MODE_NORMAL           0
#define ENC_SCENE_MODE_OTHER            1
/* Port index for buffer operations */
#define PORT_INDEX_INPUT                0
#define PORT_INDEX_OUTPUT               1
/* Buffer type constants */
#define BUFFER_TYPE_DYNAMIC_HANDLE      CODEC_BUFFER_TYPE_DYNAMIC_HANDLE
/* Video resolution threshold for buffer size calculation */
#define RESOLUTION_1080P_WIDTH          1920
#define RESOLUTION_1080P_HEIGHT         1080
/* DDR frequency settings for optimization */
#define DDR_FREQ_LOW_RES                "200000"
#define DDR_FREQ_HIGH_RES               "300000"
#define DDR_FREQ_DEFAULT                "0"
/* Time conversion constant (microseconds to milliseconds) */
#define US_TO_MS_CONVERSION             1000
#define HALF_MS_ROUNDING                500
/* Internal buffer size for encoder */
#define INTERNAL_BUFFER_SIZE_UNKNOWN    0  /* H265ENC_INTERNAL_BUFFER_SIZE is defined elsewhere */
/* Profile and level configuration */
#define DEFAULT_HEVC_PROFILE_LEVEL      1
/* CTU size slice configuration */
#define DEFAULT_CTU_SIZE_SLICE          0
/* Default frame rate multiplier */
#define FRAME_RATE_MULTIPLIER           1000
/* Port index constants */
#define PORT_INDEX_PORTDEF_INPUT        0
#define PORT_INDEX_PORTDEF_OUTPUT       1
/* EIS warp matrix valid marker */
#define EIS_WARP_MATRIX_MARKER          16171225U
/* Number of buffers for port initialization */
#define K_NUM_BUFFERS                   4
/* Color format enumeration for MMENC */
#define MMENC_RGBA32                    12
#define MMENC_YUV420SP_NV12             24
#define MMENC_YUV420SP_NV21             25
/* Success code for MMENC operations */
#define MMENC_OK                        0
/* VOP type constants */
#define VOP_TYPE_I_FRAME                0
/* I/O buffer count for multi-frame encoding */
#define MULTI_FRAME_ENUM_DEFAULT        1
/* Heap ID error indicator */
#define HEAP_ID_ERROR                   (-1)
/* Memory allocation failure code */
#define MEM_ALLOC_FAILURE               (-1)
/* Cache type for memory allocation */
#define MEM_ALLOC_NO_CACHE              0
/* Frame count for header generation */
#define HEADER_GEN_VPS_FRAME            0
#define HEADER_GEN_SPS_FRAME            1
#define HEADER_GEN_PPS_FRAME            2
/* Buffer handle access flags */
#define PROT_READ_WRITE                 (PROT_READ | PROT_WRITE)
#define MAP_SHARED_FLAGS                MAP_SHARED
/* Port index constants for get/set parameters */
#define K_INPUT_PORT_INDEX              0
/* Control rate type constants */
#define CONTROL_RATE_VARIABLE           0
#define CONTROL_RATE_CONSTANT           1
/* Default number of reference frames */
#define DEFAULT_NUM_REF_FRAMES          1
/* Default number of slice groups */
#define DEFAULT_NUM_SLICE_GROUPS        1
/* Video coding type for HEVC */
#define CODEC_OMX_VIDEO_CODING_HEVC      6
/* Buffer type for AV shared memory */
#define CODEC_BUFFER_TYPE_AVSHARE_MEM_FD 2
/* Default rate control setting */
#define RATE_CONTROL_ENABLED            1
/* Default initialization QP value */
#define INIT_QP_DEFAULT                 0
/* Default CBP removal delay */
#define INIT_CBP_REMOVAL_DELAY          1600
/* Default intra refresh value */
#define INTRAMB_REFRESH_DEFAULT         0
/* Default POC type */
#define POC_TYPE_DEFAULT                2
/* Default log2 max POC LSB minus 4 */
#define LOG2_MAX_POC_LSB_MINUS4_DEFAULT 12
/* Default delta POC zero flag */
#define DELTA_POC_ZERO_FLAG_DEFAULT     0
/* Default offset POC non-ref */
#define OFFSET_POC_NON_REF_DEFAULT      0
/* Default offset top bottom */
#define OFFSET_TOP_BOTTOM_DEFAULT       0
/* Default number of reference in cycle */
#define NUM_REF_IN_CYCLE_DEFAULT        0
/* Default FMO type */
#define FMO_TYPE_DEFAULT                0
/* Default disable DB IDC */
#define DISABLE_DB_IDC_DEFAULT          0
/* Default alpha offset */
#define ALPHA_OFFSET_DEFAULT            0
/* Default beta offset */
#define BETA_OFFSET_DEFAULT             0
/* Default search range */
#define SEARCH_RANGE_DEFAULT            16
/* Default priority for encoding */
#define ENC_PRIORITY_DEFAULT            (-1)
/* Default operating rate */
#define OPERATING_RATE_DEFAULT          0
/* H.263 encoding flag */
#define H263_ENCODING_DISABLED          0
/* Prepend SPS/PPS enable flag */
#define PREPEND_SPSPPS_DISABLED         0
/* Buffer sync flags for memory operations */
#define CPU_SYNC_START                  0
/* MMAP offset for buffer mapping */
#define MMAP_OFFSET_DEFAULT             0
/* Matrix dimension for EIS */
#define MATRIX_DIMENSION                3
/* Default matrix values for identity transformation */
#define MATRIX_IDENTITY_DIAGONAL        1.0
#define MATRIX_IDENTITY_OFF_DIAGONAL    0.0
/* Time unit conversion constants */
#define SYSTEM_TIME_TO_MS_DIVISOR       1000000L
/* Error return codes */
#define ERROR_RETURN_CODE               (-1)
/* Success return code */
#define SUCCESS_RETURN_CODE             0
/* EIS matrix indices */
#define MATRIX_ROW_0                    0
#define MATRIX_ROW_1                    1
#define MATRIX_ROW_2                    2
#define MATRIX_COL_0                    0
#define MATRIX_COL_1                    1
#define MATRIX_COL_2                    2
/* YUV plane offset calculation */
#define YUV_PLANE_OFFSET_MULTIPLIER     1
/* Default frame rate for IDR frame request */
#define DEFAULT_FRAME_RATE_FOR_IDR      0
/* Stream buffer index for single buffer operations */
#define STREAM_BUFFER_INDEX_0           0
/* Log array indices */
#define LOG_ARRAY_INDEX_0               0
#define LOG_ARRAY_INDEX_1               1
#define LOG_ARRAY_INDEX_2               2
#define LOG_ARRAY_INDEX_3               3
#define LOG_ARRAY_INDEX_4               4
#define LOG_ARRAY_INDEX_5               5
#define LOG_ARRAY_INDEX_6               6
#define LOG_ARRAY_INDEX_7               7
#define LOG_ARRAY_INDEX_8               8
#define LOG_ARRAY_INDEX_9               9
#define LOG_ARRAY_INDEX_10              10
#define LOG_ARRAY_INDEX_11              11
#define LOG_ARRAY_INDEX_12              12
#define LOG_ARRAY_INDEX_13              13
#define LOG_ARRAY_INDEX_14              14
#define LOG_ARRAY_INDEX_15              15
/* Log format for matrix elements */
#define LOG_MATRIX_ELEMENTS_COUNT       9
/* Output buffer size magic number */
#define K_OUTPUT_BUFFER_MAGIC_SIZE      31584
/* Resolution thresholds for DDR frequency setting */
#define LOW_RESOLUTION_MAX_WIDTH        720
#define LOW_RESOLUTION_MAX_HEIGHT       480
namespace OHOS {
namespace OMX {
static constexpr int32_t K_DEFERRED_CLOSE_DELAY_SECONDS = 2;
static std::mutex g_hevcDeferredCloseMutex;
static std::vector<void *> gHevcDeferredCloseHandles;
static bool g_hevcDeferredCloseWorkerRunning = false;

static void CloseDeferredHevcHandlesLocked()
{
    for (void *handle : gHevcDeferredCloseHandles) {
        if (handle != nullptr) {
            OMX_LOGI("deferred dlclose for HEVC encoder handle: %p", handle);
            dlclose(handle);
        }
    }
    gHevcDeferredCloseHandles.clear();
}

static void ScheduleDeferredHevcCloseWorkerLocked()
{
    if (g_hevcDeferredCloseWorkerRunning) {
        return;
    }
    g_hevcDeferredCloseWorkerRunning = true;
    std::thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(K_DEFERRED_CLOSE_DELAY_SECONDS));
        std::lock_guard<std::mutex> lock(g_hevcDeferredCloseMutex);
        CloseDeferredHevcHandlesLocked();
        g_hevcDeferredCloseWorkerRunning = false;
    }).detach();
}

static void DeferHevcEncoderDlclose(void *libHandle, const char *reason)
{
    if (libHandle == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_hevcDeferredCloseMutex);
    for (void *pending : gHevcDeferredCloseHandles) {
        if (pending == libHandle) {
            return;
        }
    }
    OMX_LOGW("defer dlclose for HEVC encoder handle (%s): %p", reason, libHandle);
    gHevcDeferredCloseHandles.push_back(libHandle);
    ScheduleDeferredHevcCloseWorkerLocked();
}

template<typename T>
static bool LoadHevcEncoderSymbol(void *libHandle, const char *libName, const char *symbolName, T *symbol)
{
    *symbol = reinterpret_cast<T>(dlsym(libHandle, symbolName));
    if (*symbol != nullptr) {
        return true;
    }
    OMX_LOGE("Can't find %s in %s", symbolName, libName);
    return false;
}

static bool CloseHevcEncoderLibraryOnFailure(void **libHandle)
{
    if (libHandle != nullptr && *libHandle != nullptr) {
        DeferHevcEncoderDlclose(*libHandle, "load failure");
        *libHandle = nullptr;
    }
    return false;
}
static const char *H265ENC_MIME_TYPE = "video/hevc";
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
SPRDHEVCEncoder::SPRDHEVCEncoder(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdVideoEncoderBase(name, callbacks, appData, component),
    mHandle(new TagHevcHandle),
    mEncParams(new TagHevcEncParam),
    mEncConfig(new MMEncConfig)
{
    OMX_LOGI("Construct SPRDHEVCEncoder, this: 0x%p", static_cast<void *>(this));
    if (!InitializeEncoderHandle(name)) {
        return;
    }
    if (!InitializeEncoderRuntime(name)) {
        return;
    }
    mInitCheck = OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCEncoder::initCheck() const
{
    return mInitCheck;
}

bool SPRDHEVCEncoder::InitializeEncoderHandle(const char *name)
{
    if (mHandle == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(mHandle != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    (void)memset_s(mHandle, sizeof(TagHevcHandle), 0, sizeof(TagHevcHandle));
    mHandle->videoEncoderData = nullptr;
    mHandle->userData = this;
    mHandle->vspFlushBSCache = FlushCacheWrapper;
    mHandle->vspBeginCPUAccess = BeginCPUAccessWrapper;
    mHandle->vspEndCPUAccess = EndCPUAccessWrapper;
    mHandle->vspExtmemcb = ExtMemAllocWrapper;
    (void)memset_s(&mEncInfo, sizeof(mEncInfo), 0, sizeof(mEncInfo));
    if (!OpenEncoder("libomx_hevcenc_hw_sprd.z.so")) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OpenEncoder failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    OMX_LOGI("%s, line:%d, name: %s", __FUNCTION__, __LINE__, name);
    mDumpYUVEnabled = GetDumpValue("vendor.h265enc.yuv.dump");
    mDumpStrmEnabled = GetDumpValue("vendor.h265enc.strm.dump");
    OMX_LOGI("%s, mDumpYUVEnabled: %d, mDumpStrmEnabled: %d", __FUNCTION__, mDumpYUVEnabled, mDumpStrmEnabled);
    return true;
}

bool SPRDHEVCEncoder::InitializeEncoderRuntime(const char *name)
{
    MMCodecBuffer interMemBfr;
    uint32_t sizeInter = H265ENC_INTERNAL_BUFFER_SIZE;
    mPbuf_inter = reinterpret_cast<uint8_t*>(malloc(sizeInter));
    if (mPbuf_inter == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(mPbuf_inter != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    interMemBfr.commonBufferPtr = mPbuf_inter;
    interMemBfr.commonBufferPtrPhy = 0;
    interMemBfr.size = sizeInter;
    if ((*mH265EncPreInit)(mHandle, &interMemBfr) != MMENC_OK) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] (*mH265EncPreInit)(mHandle, &interMemBfr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    mEncoderPreInitDone = true;
    if ((*mH265EncGetCodecCapability)(mHandle, &mCapability) != MMENC_OK) {
        OMX_LOGE("%s, %d, Get codec capability failed", __FUNCTION__, __LINE__);
    }
#ifdef CONFIG_RGB_ENC_SUPPORT
    mSupportRGBEnc = true;
#endif
    int ret = (*mH265EncNeedAlign)(mHandle);
    OMX_LOGI("Enc needAign:%d", ret);
    if (ret == 0) {
        mNeedAlign = false;
    }
    ret = (*mH265EncGetIOMMUStatus)(mHandle);
    OMX_LOGI("Get IOMMU Status: %d(%s)", ret, strerror(errno));
    mIOMMUEnabled = ret >= 0;
    InitPorts();
    OMX_LOGI("Construct SPRDHEVCEncoder, Capability: profile %d, level %d, max wh=%d %d",
        mCapability.profile, mCapability.level, mCapability.maxWidth, mCapability.maxHeight);
    if (mDumpYUVEnabled || mDumpStrmEnabled) {
        InitDumpFiles(name, "h265");
    }
    return true;
}

void SPRDHEVCEncoder::ResetEncoderSymbols()
{
    mH265EncPreInit = nullptr;
    mH265EncGetFnum = nullptr;
    mH265EncInit = nullptr;
    mH265EncSetConf = nullptr;
    mH265EncGetConf = nullptr;
    mH265EncStrmEncode = nullptr;
    mH265EncGenHeader = nullptr;
    mH265EncRelease = nullptr;
    mH265EncGetCodecCapability = nullptr;
    mH265EncGetIOVA = nullptr;
    mH265EncFreeIOVA = nullptr;
    mH265EncGetIOMMUStatus = nullptr;
    mH265EncNeedAlign = nullptr;
    mEncoderSymbolsReady = false;
    mEncoderPreInitDone = false;
}

SPRDHEVCEncoder::~SPRDHEVCEncoder()
{
    OMX_LOGI("Destruct SPRDHEVCEncoder, this: 0x%p", static_cast<void *>(this));
    releaseResource();
    releaseEncoder();
    if (mPorts.size() <= K_OUTPUT_PORT_INDEX) {
        OMX_LOGW("skip queue validation because ports are not initialized, size=%zu", mPorts.size());
        if (mLibHandle) {
            DeferHevcEncoderDlclose(mLibHandle, "destructor no-ports");
            mLibHandle = nullptr;
        }
        if (mDumpYUVEnabled || mDumpStrmEnabled) {
            CloseDumpFiles();
        }
        return;
    }
    std::list<BufferInfo*>& outQueue = getPortQueue(PORT_INDEX_OUTPUT);
    std::list<BufferInfo*>& inQueue = getPortQueue(PORT_INDEX_INPUT);
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
        DeferHevcEncoderDlclose(mLibHandle, "destructor");
        mLibHandle = nullptr;
    }
    if (mDumpYUVEnabled || mDumpStrmEnabled) {
        CloseDumpFiles();
    }
}

OMX_ERRORTYPE SPRDHEVCEncoder::validateEncParamPointers()
{
    if (mEncConfig == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(mEncConfig != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    if (mEncParams == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(mEncParams != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    return OMX_ErrorNone;
}

void SPRDHEVCEncoder::InitEncParamDefaults(MMEncCodecPriority &encPriority)
{
    (void)memset_s(mEncConfig, sizeof(MMEncConfig), 0, sizeof(MMEncConfig));
    (void)memset_s(mEncParams, sizeof(TagHevcEncParam), 0, sizeof(TagHevcEncParam));
    mEncParams->rateControl = HEVC_ON;
    mEncParams->initQP = INIT_QP_DEFAULT;
    mEncParams->initCBPRemovalDelay = INIT_CBP_REMOVAL_DELAY;
    mEncParams->intrambRefresh = INTRAMB_REFRESH_DEFAULT;
    mEncParams->autoScd = HEVC_ON;
    mEncParams->outOfBandParamSet = HEVC_ON;
    mEncParams->pocType = POC_TYPE_DEFAULT;
    mEncParams->log2MaxPocLsbMinus4 = LOG2_MAX_POC_LSB_MINUS4_DEFAULT;
    mEncParams->deltaPocZeroFlag = DELTA_POC_ZERO_FLAG_DEFAULT;
    mEncParams->offsetPocNonRef = OFFSET_POC_NON_REF_DEFAULT;
    mEncParams->offsetTopBottom = OFFSET_TOP_BOTTOM_DEFAULT;
    mEncParams->numRefInCycle = NUM_REF_IN_CYCLE_DEFAULT;
    mEncParams->offsetPocRef = nullptr;
    mEncParams->numRefFrame = DEFAULT_NUM_REF_FRAMES;
    mEncParams->numSliceGroup = DEFAULT_NUM_SLICE_GROUPS;
    mEncParams->fmoType = FMO_TYPE_DEFAULT;
    mEncParams->dbFilter = HEVC_ON;
    mEncParams->disableDbIdc = DISABLE_DB_IDC_DEFAULT;
    mEncParams->alphaOffset = ALPHA_OFFSET_DEFAULT;
    mEncParams->betaOffset = BETA_OFFSET_DEFAULT;
    mEncParams->constrainedIntraPred = HEVC_OFF;
    mEncParams->dataPar = HEVC_OFF;
    mEncParams->fullsearch = HEVC_OFF;
    mEncParams->searchRange = SEARCH_RANGE_DEFAULT;
    mEncParams->subPel = HEVC_OFF;
    mEncParams->submbPred = HEVC_OFF;
    mEncParams->rdoptMode = HEVC_OFF;
    mEncParams->bidirPred = HEVC_OFF;
    mEncParams->useOverrunBuffer = HEVC_OFF;
    encPriority.priority = ENC_PRIORITY_DEFAULT;
    encPriority.operatingRate = OPERATING_RATE_DEFAULT;
}

void SPRDHEVCEncoder::ConfigureDdrFrequency() const
{
#ifdef VIDEOENC_CURRENT_OPT
    bool lowResolution = ((mFrameWidth <= LOW_RESOLUTION_MAX_WIDTH) &&
        (mFrameHeight <= LOW_RESOLUTION_MAX_HEIGHT)) ||
        ((mFrameWidth <= LOW_RESOLUTION_MAX_HEIGHT) &&
        (mFrameHeight <= LOW_RESOLUTION_MAX_WIDTH));
    SetDdrFreq(lowResolution ? DDR_FREQ_LOW_RES : DDR_FREQ_HIGH_RES);
#endif
}

size_t SPRDHEVCEncoder::CalculateStreamBufferSize() const
{
    size_t sizeOfYuv = ((mVideoWidth + FRAME_ALIGNMENT_32) & (~FRAME_ALIGNMENT_32)) *
        ((mVideoHeight + FRAME_ALIGNMENT_32) & (~FRAME_ALIGNMENT_32)) *
        YUV_SIZE_MULTIPLIER_NUMERATOR / YUV_SIZE_MULTIPLIER_DENOMINATOR;
    if (mVideoWidth * mVideoHeight <= RESOLUTION_1080P_WIDTH * RESOLUTION_1080P_HEIGHT) {
        return sizeOfYuv;
    }
    return sizeOfYuv / STREAM_SIZE_DIVISOR_1080P;
}

OMX_ERRORTYPE SPRDHEVCEncoder::allocateStreamBuffers(size_t sizeStream)
{
    unsigned long phyAddr = 0;
    size_t size = 0;
    mMFEnum = (*mH265EncGetFnum)(mHandle, mFrameWidth, mFrameHeight, mVideoFrameRate);
    for (int i = 0; i < mMFEnum; i++) {
        mPmemStream[i] = new VideoMemAllocator(sizeStream, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
        int fd = mPmemStream[i]->getHeapID();
        if (fd < HEAP_ID_ERROR) {
            OMX_LOGE("Failed to alloc stream buffer (%zd), getHeapID failed", sizeStream);
            return OMX_ErrorInsufficientResources;
        }
        int ret = 0;
        if (mIOMMUEnabled) {
            if (!mEncoderSymbolsReady || mH265EncGetIOVA == nullptr || mHandle == nullptr) {
                OMX_LOGE("IOMMU enabled but encoder IOVA callback is unavailable");
                return OMX_ErrorInsufficientResources;
            }
            ret = (*mH265EncGetIOVA)(mHandle, fd, &phyAddr, &size);
        } else {
            ret = mPmemStream[i]->getPhyAddrFromMemAlloc(&phyAddr, &size);
        }
        if (ret < SUCCESS_RETURN_CODE) {
            OMX_LOGE("Failed to alloc stream buffer, get phy addr failed");
            return OMX_ErrorInsufficientResources;
        }
        mStreamBfr.commonBufferPtr[i] = static_cast<uint8_t*>(mPmemStream[i]->getBase());
        mStreamBfr.commonBufferPtrPhy[i] = phyAddr;
        mStreamBfr.size = size;
    }
    return OMX_ErrorNone;
}

void SPRDHEVCEncoder::InitVideoInfo()
{
    mEncInfo.frameWidth = mFrameWidth;
    mEncInfo.frameHeight = mFrameHeight;
    mEncInfo.eisMode = false;
#ifdef CONFIG_SPRD_RECORD_EIS
    mEncInfo.eisMode = meisMode;
#endif
    mEncInfo.isH263 = H263_ENCODING_DISABLED;
    mEncInfo.orgWidth = mFrameWidth;
    mEncInfo.orgHeight = mFrameHeight;
    mEncInfo.bitDepth = VIDEO_BIT_DEPTH_8BIT;
    switch (mVideoColorFormat) {
        case COLOR_FORMAT_RGBA_8888:
            mEncInfo.yuvFormat = MMENC_RGBA32;
            break;
        case COLOR_FORMAT_N_V12:
            mEncInfo.yuvFormat = MMENC_YUV420SP_NV12;
            break;
        case COLOR_FORMAT_N_V21:
            mEncInfo.yuvFormat = MMENC_YUV420SP_NV21;
            break;
        default:
            break;
    }
    mEncInfo.timeScale = VIDEO_TIME_SCALE_MS;
}

OMX_ERRORTYPE SPRDHEVCEncoder::applyEncoderConfig(MMEncCodecPriority &encPriority)
{
    if ((*mH265EncInit)(mHandle, &mStreamBfr, &mEncInfo, &encPriority) != 0) {
        OMX_LOGE("Failed to init h265enc");
        return OMX_ErrorUndefined;
    }
    if ((*mH265EncGetConf)(mHandle, mEncConfig) != 0) {
        OMX_LOGE("Failed to get default encoding parameters");
        return OMX_ErrorUndefined;
    }
    mEncConfig->h263En = H263_ENCODING_DISABLED;
    mEncConfig->rateCtrlEnable = RATE_CONTROL_ENABLED;
    mEncConfig->targetBitRate = mVideoBitRate;
    mEncConfig->frameRate = mVideoFrameRate;
    mEncConfig->pFrames = mPFrames;
    mEncConfig->qpIvop = (mEncSceneMode == ENC_SCENE_MODE_NORMAL) ?
        DEFAULT_QP_IVOP_SCENE_0 : DEFAULT_QP_IVOP_SCENE_1;
    mEncConfig->qpPvop = DEFAULT_QP_PVOP;
    mEncConfig->vbvBufSize = mVideoBitRate / STREAM_SIZE_DIVISOR_OTHER;
    mEncConfig->profileAndLevel = DEFAULT_HEVC_PROFILE_LEVEL;
    mEncConfig->prependSpsPpsEnable = (mPrependSPSPPS == OMX_FALSE) ? PREPEND_SPSPPS_DISABLED : 1;
    mEncConfig->encSceneMode = mEncSceneMode;
    mEncConfig->ctuSizeSlice = DEFAULT_CTU_SIZE_SLICE;
    CheckUpdateColorAspects(mEncConfig);
    if ((*mH265EncSetConf)(mHandle, mEncConfig) == 0) {
        return OMX_ErrorNone;
    }
    OMX_LOGE("Failed to set default encoding parameters");
    return OMX_ErrorUndefined;
}

void SPRDHEVCEncoder::FinalizeEncParams()
{
    mEncParams->width = mFrameWidth;
    mEncParams->height = mFrameHeight;
    mEncParams->bitrate = mVideoBitRate;
    mEncParams->frameRate = FRAME_RATE_MULTIPLIER * mVideoFrameRate;
    mEncParams->cpbSize = static_cast<uint32_t>(mVideoBitRate >> 1);
    mEncParams->profile = mHEVCEncProfile;
    mEncParams->level = mHEVCEncLevel;
    mNumInputFrames = INITIAL_FRAME_COUNT;
    mSpsPpsHeaderReceived = false;
}

OMX_ERRORTYPE SPRDHEVCEncoder::initEncParams()
{
    MMEncCodecPriority mEncPriority;
    OMX_ERRORTYPE err = validateEncParamPointers();
    if (err != OMX_ErrorNone) {
        return err;
    }
    InitEncParamDefaults(mEncPriority);
    ConfigureDdrFrequency();
    mSetFreqCount++;
    err = allocateStreamBuffers(CalculateStreamBufferSize());
    if (err != OMX_ErrorNone) {
        return err;
    }
    InitVideoInfo();
    err = applyEncoderConfig(mEncPriority);
    if (err != OMX_ErrorNone) {
        return err;
    }
    FinalizeEncParams();
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SPRDHEVCEncoder::releaseEncoder()
{
    if (mEncoderSymbolsReady && mEncoderPreInitDone && mH265EncRelease != nullptr && mHandle != nullptr) {
        (*mH265EncRelease)(mHandle);
    }
    if (mPbuf_inter != nullptr) {
        free(mPbuf_inter);
        mPbuf_inter = nullptr;
    }
    delete mEncParams;
    mEncParams = nullptr;
    delete mEncConfig;
    mEncConfig = nullptr;
    delete mHandle;
    mHandle = nullptr;
    ResetEncoderSymbols();
    mStarted = false;
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SPRDHEVCEncoder::releaseResource()
{
    if (mPbufExtraV != nullptr) {
        if (mIOMMUEnabled && mEncoderSymbolsReady && mH265EncFreeIOVA != nullptr && mHandle != nullptr) {
            (*mH265EncFreeIOVA)(mHandle, mPbufExtraP, mPbufExtraSize);
        }
        releaseMemIon(&mPmemExtra);
        mPbufExtraV = nullptr;
        mPbufExtraP = 0;
        mPbufExtraSize = 0;
    }
    for (int i = 0; i < mMFEnum; i++) {
        if (mStreamBfr.commonBufferPtr[i] !=  nullptr) {
            if (mIOMMUEnabled && mEncoderSymbolsReady && mH265EncFreeIOVA != nullptr && mHandle != nullptr) {
                (*mH265EncFreeIOVA)(mHandle, mStreamBfr.commonBufferPtrPhy[i], mStreamBfr.size);
            }
            releaseMemIon(&mPmemStream[i]);
            mStreamBfr.commonBufferPtr[i] = nullptr;
            mStreamBfr.commonBufferPtrPhy[i] = 0;
        }
    }
    mStreamBfr.size = 0;
    if (mPbuf_yuv_v != nullptr) {
        if (mIOMMUEnabled && mEncoderSymbolsReady && mH265EncFreeIOVA != nullptr && mHandle != nullptr) {
            (*mH265EncFreeIOVA)(mHandle, mPbuf_yuv_p, mPbuf_yuv_size);
        }
        releaseMemIon(&mYUVInPmemHeap);
        mPbuf_yuv_v = nullptr;
        mPbuf_yuv_p = 0;
        mPbuf_yuv_size = 0;
    }
#ifdef VIDEOENC_CURRENT_OPT
    while (mSetFreqCount > 0) {
        SetDdrFreq(DDR_FREQ_DEFAULT);
        mSetFreqCount --;
    }
#endif
    return SprdVideoEncoderBase::releaseResource();
}
void SPRDHEVCEncoder::InitPorts() const
{
    const size_t kInputBufferSize = (mFrameWidth * mFrameHeight * YUV_SIZE_MULTIPLIER_NUMERATOR) >> 1;
    // 31584 is PV's magic number.  Not sure why.
    const size_t kOutputBufferSize =
        (kInputBufferSize > K_OUTPUT_BUFFER_MAGIC_SIZE) ? kInputBufferSize : K_OUTPUT_BUFFER_MAGIC_SIZE;
    SprdVideoEncoderBase::InitPorts(K_NUM_BUFFERS,
                                    kInputBufferSize, kOutputBufferSize, const_cast<char *>(H265ENC_MIME_TYPE),
                                    (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CODING_HEVC);
}
OMX_ERRORTYPE SPRDHEVCEncoder::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_IndexParamVideoPortFormat: {
            return SprdVideoEncoderBase::getParamVideoPortFormat(
                (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CODING_HEVC, params);
        }
        case OMX_IndexParamSupportBufferType: {
            return getSupportBufferType(static_cast<UseBufferType *>(params));
        }
        case OMX_IndexParamVideoHevc: {
            return getVideoHevc(static_cast<OMX_VIDEO_PARAM_HEVCTYPE *>(params));
        }
        case OMX_IndexParamVideoProfileLevelQuerySupported: {
            return getProfileLevelQuerySupported(static_cast<OMX_VIDEO_PARAM_PROFILELEVELTYPE *>(params));
        }
        case OMX_IndexCodecVideoPortFormat: {
            return getCodecVideoPortFormat(static_cast<CodecVideoPortFormatParam *>(params));
        }
        default:
            return SprdVideoEncoderBase::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDHEVCEncoder::getSupportBufferType(UseBufferType *defParams)
{
    if (CheckParam(defParams, sizeof(UseBufferType)) || defParams->portIndex > PORT_INDEX_OUTPUT) {
        OMX_LOGE("port index error.");
        return OMX_ErrorUnsupportedIndex;
    }
    defParams->bufferType = (defParams->portIndex == K_INPUT_PORT_INDEX) ?
        BUFFER_TYPE_DYNAMIC_HANDLE : CODEC_BUFFER_TYPE_AVSHARE_MEM_FD;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCEncoder::getVideoHevc(OMX_VIDEO_PARAM_HEVCTYPE *hevcParams)
{
    if (hevcParams->nPortIndex != PORT_INDEX_OUTPUT) {
        return OMX_ErrorUndefined;
    }
    hevcParams->eProfile = OMX_VIDEO_HEVC_PROFILE_MAIN;
    hevcParams->eLevel = OMX_VIDEO_HEVC_MAIN_TIER_LEVEL5;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCEncoder::getProfileLevelQuerySupported(OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel)
{
    if (profileLevel->nPortIndex != PORT_INDEX_OUTPUT) {
        return OMX_ErrorUndefined;
    }
    size_t index = profileLevel->nProfileIndex;
    size_t nProfileLevels = sizeof(kProfileLevels) / sizeof(kProfileLevels[0]);
    if (index >= nProfileLevels) {
        return OMX_ErrorNoMore;
    }
    profileLevel->eProfile = kProfileLevels[index].mProfile;
    profileLevel->eLevel = kProfileLevels[index].mLevel;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCEncoder::getCodecVideoPortFormat(CodecVideoPortFormatParam *formatParams)
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

OMX_ERRORTYPE SPRDHEVCEncoder::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_IndexParamUseBufferType: {
            return OMX_ErrorNone;
        }
        case OMX_IndexParamVideoBitrate: {
            return setVideoBitrate(static_cast<const OMX_VIDEO_PARAM_BITRATETYPE *>(params));
        }
        case OMX_IndexParamPortDefinition: {
            return SprdVideoEncoderBase::setParamPortDefinition(
                (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CODING_HEVC, index, params);
        }
        case OMX_IndexParamStandardComponentRole: {
            return SprdVideoEncoderBase::setParamStandardComponentRole("video_encoder.hevc", params);
        }
        case OMX_IndexParamVideoPortFormat: {
            return SprdVideoEncoderBase::setParamVideoPortFormat(
                (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CODING_HEVC, params);
        }
        case OMX_IndexParamVideoHevc: {
            return setVideoHevc(static_cast<const OMX_VIDEO_PARAM_HEVCTYPE *>(params));
        }
        case OMX_IndexCodecVideoPortFormat: {
            return setCodecVideoPortFormat(static_cast<const CodecVideoPortFormatParam *>(params));
        }
        default:
            return SprdVideoEncoderBase::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDHEVCEncoder::setVideoBitrate(const OMX_VIDEO_PARAM_BITRATETYPE *bitRate)
{
    if (bitRate->nPortIndex != PORT_INDEX_OUTPUT ||
        (bitRate->eControlRate != OMX_Video_ControlRateVariable &&
        bitRate->eControlRate != OMX_Video_ControlRateConstant)) {
        return OMX_ErrorUndefined;
    }
    mVideoBitRate = bitRate->nTargetBitrate;
    if (bitRate->eControlRate == OMX_Video_ControlRateConstant && !mSetEncMode) {
        mEncSceneMode = ENC_SCENE_MODE_OTHER;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCEncoder::setVideoHevc(const OMX_VIDEO_PARAM_HEVCTYPE *hevcType)
{
    if (hevcType->nPortIndex != PORT_INDEX_OUTPUT || OMX_VIDEO_HEVC_PROFILE_MAIN != hevcType->eProfile) {
        return OMX_ErrorUndefined;
    }
    setPFrames(&mPFrames, const_cast<OMX_VIDEO_PARAM_HEVCTYPE *>(hevcType));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCEncoder::setCodecVideoPortFormat(const CodecVideoPortFormatParam *formatParams)
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
OMX_ERRORTYPE SPRDHEVCEncoder::getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index)
{
    if (strcmp(name, "OMX.index.prependSPSPPSToIDRFrames") == 0) {
        *index = (OMX_INDEXTYPE) OMX_INDEX_PREPEND_SPSPPS_TO_IDR;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.sprd.index.RecordMode") == 0) {
        *index = (OMX_INDEXTYPE) OMX_INDEX_CONFIG_VIDEO_RECORD_MODE;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.sprd.index.encSceneMode") == 0) {
        *index = (OMX_INDEXTYPE) OMX_INDEX_CONFIG_ENC_SCENE_MODE;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.index.describeColorAspects") == 0) {
        *index = (OMX_INDEXTYPE) OMX_INDEX_DESCRIBE_COLOR_ASPECTS;
        return OMX_ErrorNone;
    }
    return SprdVideoEncoderBase::getExtensionIndex(name, index);
}
// static
void SPRDHEVCEncoder::FlushCacheWrapper(const void *aUserData)
{
    static_cast<const SPRDHEVCEncoder *>(aUserData)->FlushCacheforBsBuf();
}
void SPRDHEVCEncoder::FlushCacheforBsBuf() const
{
    for (int i = 0; i < mMFEnum; i++) {
        mPmemStream[i]->cpuSyncStart();
    }
}
void SPRDHEVCEncoder::BeginCPUAccessWrapper(const void *aUserData)
{
    static_cast<const SPRDHEVCEncoder *>(aUserData)->BeginCpuAccessforBsBuf();
    return;
}
void SPRDHEVCEncoder::BeginCpuAccessforBsBuf() const
{
    int fd;
    struct dma_buf_sync sync = { 0 };
    for (int i = 0; i < mMFEnum; i++) {
        fd = mPmemStream[i]->getHeapID();
        sync.flags = DMA_SYNC_START_FLAGS;
        ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
    }
}
void SPRDHEVCEncoder::EndCPUAccessWrapper(const void *aUserData)
{
    static_cast<const SPRDHEVCEncoder *>(aUserData)->EndCpuAccessforBsBuf();
}
void SPRDHEVCEncoder::EndCpuAccessforBsBuf() const
{
    int fd;
    struct dma_buf_sync sync = { 0 };
    for (int i = 0; i < mMFEnum; i++) {
        fd = mPmemStream[i]->getHeapID();
        sync.flags = DMA_SYNC_END_FLAGS;
        ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
    }
    return;
}
void SPRDHEVCEncoder::resetOutputHeader(OMX_BUFFERHEADERTYPE *outHeader)
{
    outHeader->nTimeStamp = 0;
    outHeader->nFlags = 0;
    outHeader->nOffset = 0;
    outHeader->nFilledLen = 0;
}

bool SPRDHEVCEncoder::appendEncodedData(OMX_BUFFERHEADERTYPE *outHeader, uint8_t *&outPtr,
    uint8_t *srcBuffer, int32_t srcSize, uint32_t &dataLength)
{
    if (srcBuffer == nullptr || srcSize <= 0) {
        return false;
    }
    dataLength += srcSize;
    size_t remainingSize = outHeader->nAllocLen - (outPtr - reinterpret_cast<uint8_t*>(outHeader->pBuffer));
    errno_t ret = memmove_s(outPtr, remainingSize, srcBuffer, srcSize);
    if (ret != 0) {
        OMX_LOGE("memmove_s failed at line %d, ret=%d", __LINE__, ret);
        return false;
    }
    outPtr += srcSize;
    return true;
}

void SPRDHEVCEncoder::patchStartCodeAndLog(uint8_t *buffer, const char *tag, bool log16Bytes)
{
    if (buffer == nullptr) {
        return;
    }
    OMX_LOGI("%s: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x",
        tag, buffer[LOG_ARRAY_INDEX_0], buffer[LOG_ARRAY_INDEX_1], buffer[LOG_ARRAY_INDEX_2],
        buffer[LOG_ARRAY_INDEX_3], buffer[LOG_ARRAY_INDEX_4], buffer[LOG_ARRAY_INDEX_5],
        buffer[LOG_ARRAY_INDEX_6], buffer[LOG_ARRAY_INDEX_7]);
    if (log16Bytes) {
        OMX_LOGI("%s: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x",
            tag, buffer[LOG_ARRAY_INDEX_8], buffer[LOG_ARRAY_INDEX_9], buffer[LOG_ARRAY_INDEX_10],
            buffer[LOG_ARRAY_INDEX_11], buffer[LOG_ARRAY_INDEX_12], buffer[LOG_ARRAY_INDEX_13],
            buffer[LOG_ARRAY_INDEX_14], buffer[LOG_ARRAY_INDEX_15]);
    }
    buffer[LOG_ARRAY_INDEX_0] = buffer[LOG_ARRAY_INDEX_1] = buffer[LOG_ARRAY_INDEX_2] = 0x0;
    buffer[LOG_ARRAY_INDEX_3] = NAL_START_CODE;
}

bool SPRDHEVCEncoder::generateAndAppendCodecHeader(int32_t headerType, const char *tag, bool log16Bytes,
    EncodeOutputState *output)
{
    if (output == nullptr) {
        return false;
    }
    MMEncOut header;
    (void)memset_s(&header, sizeof(MMEncOut), 0, sizeof(MMEncOut));
    mPmemStream[STREAM_BUFFER_INDEX_0]->cpuSyncStart();
    ++mNumInputFrames;
    (*mH265EncGenHeader)(mHandle, &header, headerType);
    OMX_LOGI("%s, %d, %sHeader.strmSize: %d", __FUNCTION__, __LINE__, tag, header.strmSize[STREAM_BUFFER_INDEX_0]);
    patchStartCodeAndLog(reinterpret_cast<uint8_t*>(header.pOutBuf[STREAM_BUFFER_INDEX_0]), tag, log16Bytes);
    if (mDumpStrmEnabled) {
        DumpFiles(header.pOutBuf[STREAM_BUFFER_INDEX_0], header.strmSize[STREAM_BUFFER_INDEX_0], DUMP_STREAM);
    }
    return appendEncodedData(output->outHeader, output->outPtr, header.pOutBuf[STREAM_BUFFER_INDEX_0],
        header.strmSize[STREAM_BUFFER_INDEX_0], output->dataLength);
}

bool SPRDHEVCEncoder::emitCodecConfig(EncodeOutputState &output, std::list<BufferInfo*>& outQueue)
{
    if (mSpsPpsHeaderReceived || mNumInputFrames > 0) {
        return true;
    }
    if (!generateAndAppendCodecHeader(HEADER_GEN_VPS_FRAME, "vps", true, &output) ||
        !generateAndAppendCodecHeader(HEADER_GEN_SPS_FRAME, "sps", true, &output) ||
        !generateAndAppendCodecHeader(HEADER_GEN_PPS_FRAME, "pps", false, &output)) {
        return false;
    }
    mSpsPpsHeaderReceived = true;
    if ((0) != (mNumInputFrames)) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((0) == (mNumInputFrames)) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    if (!SprdOMXUtils::notifyEncodeHeaderWithFirstFrame()) {
        output.outHeader->nFlags = OMX_BUFFERFLAG_CODECCONFIG;
        output.outHeader->nFilledLen = output.dataLength;
        outQueue.erase(outQueue.begin());
        output.outInfo->mOwnedByUs = false;
        notifyFillBufferDone(output.outHeader);
        return false;
    }
    return true;
}

bool SPRDHEVCEncoder::mapInputGraphicBuffer(OMX_BUFFERHEADERTYPE *inHeader, MappedInputBuffer &mappedBuffer)
{
    DynamicBuffer* buffer = (DynamicBuffer*)inHeader->pBuffer;
    BufferHandle *bufferHandle = buffer->bufferHandle;
    mappedBuffer.bufferHandle = bufferHandle;
    mappedBuffer.width = 0;
    mappedBuffer.height = 0;
    mappedBuffer.cropX = 0;
    mappedBuffer.cropY = 0;
    mappedBuffer.needUnmap = false;
    mappedBuffer.iova = 0;
    mappedBuffer.iovaLen = 0;
    mappedBuffer.virtualAddr = reinterpret_cast<uint8_t*>(mmap(MMAP_OFFSET_DEFAULT, bufferHandle->size,
        PROT_READ_WRITE, MAP_SHARED_FLAGS, bufferHandle->fd, 0));
    if (mappedBuffer.virtualAddr == MAP_FAILED) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(py != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        mappedBuffer.virtualAddr = nullptr;
        return false;
    }
    if (mIOMMUEnabled) {
        if (!mEncoderSymbolsReady || mH265EncGetIOVA == nullptr || mHandle == nullptr) {
            OMX_LOGE("IOMMU enabled but encoder IOVA callback is unavailable");
            munmap(mappedBuffer.virtualAddr, bufferHandle->size);
            mappedBuffer.virtualAddr = nullptr;
            return false;
        }
        if ((*mH265EncGetIOVA)(mHandle, bufferHandle->fd,
            &mappedBuffer.iova, &mappedBuffer.iovaLen) != 0) {
            OMX_LOGE("H264DecGetIOVA Failed: %d(%s)", errno, strerror(errno));
            munmap(mappedBuffer.virtualAddr, bufferHandle->size);
            mappedBuffer.virtualAddr = nullptr;
            return false;
        }
    }
    mappedBuffer.needUnmap = mIOMMUEnabled;
    mappedBuffer.physicalAddr = mIOMMUEnabled ?
        reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(mappedBuffer.iova)) : mappedBuffer.virtualAddr;
    OMX_LOGW("py = %p, pyPhy = %p", mappedBuffer.virtualAddr, mappedBuffer.physicalAddr);
    return mappedBuffer.physicalAddr != nullptr;
}

void SPRDHEVCEncoder::UnmapInputGraphicBuffer(const MappedInputBuffer &mappedBuffer) const
{
    BufferHandle *bufferHandle = static_cast<BufferHandle *>(mappedBuffer.bufferHandle);
    if (mappedBuffer.needUnmap) {
        if (mEncoderSymbolsReady && mH265EncFreeIOVA != nullptr && mHandle != nullptr) {
            OMX_LOGV("Free_iova, fd: %d, iova: 0x%lx, size: %zd",
                bufferHandle->fd, mappedBuffer.iova, mappedBuffer.iovaLen);
            (*mH265EncFreeIOVA)(mHandle, mappedBuffer.iova, mappedBuffer.iovaLen);
        } else {
            OMX_LOGW("skip free IOVA due to incomplete encoder state");
        }
    }
    if (mappedBuffer.virtualAddr == nullptr || bufferHandle == nullptr || bufferHandle->size == 0) {
        return;
    }
    munmap(mappedBuffer.virtualAddr, bufferHandle->size);
}

void SPRDHEVCEncoder::configureEncodeInput(MMEncIn &vidIn, OMX_BUFFERHEADERTYPE *inHeader,
    const MappedInputBuffer &mappedBuffer)
{
    (void)memset_s(&vidIn, sizeof(MMEncIn), 0, sizeof(MMEncIn));
    switch (mVideoColorFormat) {
        case COLOR_FORMAT_RGBA_8888:
            vidIn.yuvFormat = MMENC_RGBA32;
            break;
        case COLOR_FORMAT_N_V12:
            vidIn.yuvFormat = MMENC_YUV420SP_NV12;
            break;
        case COLOR_FORMAT_N_V21:
            vidIn.yuvFormat = MMENC_YUV420SP_NV21;
            break;
        default:
            break;
    }
    vidIn.timeStamp = (inHeader->nTimeStamp + HALF_MS_ROUNDING) / US_TO_MS_CONVERSION;
    vidIn.channelQuality = DEFAULT_CHANNEL_QUALITY;
    vidIn.bitrate = mBitrate;
    vidIn.isChangeBitrate = mIsChangeBitrate;
    mIsChangeBitrate = false;
    vidIn.needIvoP = mKeyFrameRequested || (mNumInputFrames == DEFAULT_FRAME_RATE_FOR_IDR);
    vidIn.srcYuvAddr[STREAM_BUFFER_INDEX_0].pSrcY = mappedBuffer.virtualAddr;
    vidIn.srcYuvAddr[STREAM_BUFFER_INDEX_0].pSrcV = nullptr;
    vidIn.srcYuvAddr[STREAM_BUFFER_INDEX_0].pSrcYPhy = mappedBuffer.physicalAddr;
    vidIn.srcYuvAddr[STREAM_BUFFER_INDEX_0].pSrcVPhy = nullptr;
    uint32_t planeOffset = mappedBuffer.width != 0 && mappedBuffer.height != 0 ?
        mappedBuffer.width * mappedBuffer.height * YUV_PLANE_OFFSET_MULTIPLIER :
        mFrameWidth * mFrameHeight * YUV_PLANE_OFFSET_MULTIPLIER;
    vidIn.srcYuvAddr[STREAM_BUFFER_INDEX_0].pSrcU = mappedBuffer.virtualAddr + planeOffset;
    vidIn.srcYuvAddr[STREAM_BUFFER_INDEX_0].pSrcUPhy = mappedBuffer.physicalAddr + planeOffset;
    vidIn.orgImgWidth = static_cast<int32_t>(mappedBuffer.width);
    vidIn.orgImgHeight = static_cast<int32_t>(mappedBuffer.height);
    vidIn.cropX = static_cast<int32_t>(mappedBuffer.cropX);
    vidIn.cropY = static_cast<int32_t>(mappedBuffer.cropY);
}

void SPRDHEVCEncoder::SyncInputStreamBuffers() const
{
    for (int i = 0; i < mMFEnum; i++) {
        mPmemStream[i]->cpuSyncStart();
    }
}

bool SPRDHEVCEncoder::FinalizeEncodedFrame(MMEncOut &vidOut, EncodeOutputState &output)
{
    if ((vidOut.strmSize[STREAM_BUFFER_INDEX_0] < SUCCESS_RETURN_CODE)) {
        return true;
    }
    OMX_LOGI("%s, %d, out_streamPtr: %p", __FUNCTION__, __LINE__, output.outPtr);
    patchStartCodeAndLog(reinterpret_cast<uint8_t*>(vidOut.pOutBuf[STREAM_BUFFER_INDEX_0]), "frame", false);
    if (mDumpStrmEnabled) {
        DumpFiles(vidOut.pOutBuf[STREAM_BUFFER_INDEX_0], vidOut.strmSize[STREAM_BUFFER_INDEX_0], DUMP_STREAM);
    }
    if (vidOut.strmSize[STREAM_BUFFER_INDEX_0] <= 0) {
        output.dataLength = 0;
        return true;
    }
    if (!appendEncodedData(output.outHeader, output.outPtr, vidOut.pOutBuf[STREAM_BUFFER_INDEX_0],
        vidOut.strmSize[STREAM_BUFFER_INDEX_0], output.dataLength)) {
        return false;
    }
    if (vidOut.vopType[STREAM_BUFFER_INDEX_0] == VOP_TYPE_I_FRAME) {
        mKeyFrameRequested = false;
        output.outHeader->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
    }
    ++mNumInputFrames;
    return true;
}

bool SPRDHEVCEncoder::encodeInputBuffer(OMX_BUFFERHEADERTYPE *inHeader, EncodeOutputState &output)
{
    if (inHeader->nFilledLen <= 0) {
        output.dataLength = 0;
        return true;
    }
    MappedInputBuffer mappedBuffer;
    if (!mapInputGraphicBuffer(inHeader, mappedBuffer)) {
        return false;
    }
    MMEncIn vidIn;
    MMEncOut vidOut;
    (void)memset_s(&vidOut, sizeof(MMEncOut), 0, sizeof(MMEncOut));
    configureEncodeInput(vidIn, inHeader, mappedBuffer);
    if (mDumpYUVEnabled) {
        size_t yuvSize = mFrameWidth * mFrameHeight * YUV_SIZE_MULTIPLIER_NUMERATOR / YUV_SIZE_MULTIPLIER_DENOMINATOR;
        DumpFiles(mappedBuffer.virtualAddr, yuvSize, DUMP_YUV);
    }
    SyncInputStreamBuffers();
    int64_t startEncode = systemTime();
    int ret = (*mH265EncStrmEncode)(mHandle, &vidIn, &vidOut);
    int64_t endEncode = systemTime();
    OMX_LOGI("H265EncStrmEncode[%lld] %dms, in { %p-%p, %dx%d, %d}",
        mNumInputFrames, static_cast<unsigned int>((endEncode-startEncode) / SYSTEM_TIME_TO_MS_DIVISOR),
        mappedBuffer.virtualAddr, mappedBuffer.physicalAddr, mFrameWidth, mFrameHeight, vidIn.yuvFormat);
    OMX_LOGI("out {%p-%d, %d}, wh {%d, %d}, xy{%d, %d}",
        vidOut.pOutBuf[STREAM_BUFFER_INDEX_0], vidOut.strmSize[STREAM_BUFFER_INDEX_0],
        vidOut.vopType[STREAM_BUFFER_INDEX_0], mappedBuffer.width, mappedBuffer.height,
        mappedBuffer.cropX, mappedBuffer.cropY);
    UnmapInputGraphicBuffer(mappedBuffer);
    if ((vidOut.strmSize[STREAM_BUFFER_INDEX_0] < SUCCESS_RETURN_CODE) || (ret != MMENC_OK)) {
        OMX_LOGE("Failed to encode frame %lld, ret=%d", mNumInputFrames, ret);
        return true;
    }
    return FinalizeEncodedFrame(vidOut, output);
}

bool SPRDHEVCEncoder::FinalizeOutputBuffer(EncodeFrameContext &frame)
{
    if ((frame.inHeader->nFlags & OMX_BUFFERFLAG_EOS) && (frame.inHeader->nFilledLen == 0)) {
        OMX_LOGI("saw EOS");
        frame.outHeader->nFlags = OMX_BUFFERFLAG_EOS;
    }
    frame.inQueue->erase(frame.inQueue->begin());
    frame.inInfo->mOwnedByUs = false;
    notifyEmptyBufferDone(frame.inHeader);
    if (!(mFrameWidth <= mCapability.maxWidth && mFrameHeight <= mCapability.maxHeight)) {
        OMX_LOGE("[%d, %d] is out of range [%d, %d], failed to support this format.",
            mFrameWidth, mFrameHeight, mCapability.maxWidth, mCapability.maxHeight);
        notify(OMX_EventError, OMX_ErrorBadParameter, 0, nullptr);
        mSignalledError = true;
        return false;
    }
    if (mInputBufferInfoVec.empty()) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(!mInputBufferInfoVec.empty() failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    InputBufferInfo *inputBufInfo = &(*mInputBufferInfoVec.begin());
    if (frame.output.dataLength > 0 || (frame.inHeader->nFlags & OMX_BUFFERFLAG_EOS)) {
        frame.outQueue->erase(frame.outQueue->begin());
        frame.outHeader->nTimeStamp = inputBufInfo->mTimeUs;
        frame.outHeader->nFlags |= (inputBufInfo->mFlags | OMX_BUFFERFLAG_ENDOFFRAME);
        frame.outHeader->nFilledLen = frame.output.dataLength;
        frame.outInfo->mOwnedByUs = false;
        notifyFillBufferDone(frame.outHeader);
    }
    mInputBufferInfoVec.erase(mInputBufferInfoVec.begin());
    return true;
}

bool SPRDHEVCEncoder::processOneInputFrame(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue)
{
    EncodeFrameContext frame = {
        &inQueue,
        &outQueue,
        *inQueue.begin(),
        *outQueue.begin(),
        (*inQueue.begin())->mHeader,
        (*outQueue.begin())->mHeader,
        { *outQueue.begin(), (*outQueue.begin())->mHeader,
            reinterpret_cast<uint8_t*>((*outQueue.begin())->mHeader->pBuffer), 0 }
    };
    resetOutputHeader(frame.outHeader);
    if (!emitCodecConfig(frame.output, outQueue)) {
        return false;
    }
    OMX_LOGV("%s, line:%d, inHeader->nFilledLen: %d, mStoreMetaData: %d, mVideoColorFormat: 0x%x",
        __FUNCTION__, __LINE__, frame.inHeader->nFilledLen, mStoreMetaData, mVideoColorFormat);
    InputBufferInfo info;
    info.mTimeUs = frame.inHeader->nTimeStamp;
    info.mFlags = frame.inHeader->nFlags;
    mInputBufferInfoVec.push_back(info);
    if (!encodeInputBuffer(frame.inHeader, frame.output)) {
        return false;
    }
    return FinalizeOutputBuffer(frame);
}

void SPRDHEVCEncoder::onQueueFilled(OMX_U32 portIndex)
{
    (void)portIndex;
    if (mSignalledError) {
        return;
    }
    if (!mStarted) {
        if (initEncoder()  != OMX_ErrorNone) {
            return;
        }
    }
    std::list<BufferInfo*>& inQueue = getPortQueue(PORT_INDEX_INPUT);
    std::list<BufferInfo*>& outQueue = getPortQueue(PORT_INDEX_OUTPUT);
    while (!inQueue.empty() && !outQueue.empty()) {
        if (!processOneInputFrame(inQueue, outQueue)) {
            return;
        }
    }
}

bool SPRDHEVCEncoder::OpenEncoder(const char *libName)
{
    if (mLibHandle) {
        DeferHevcEncoderDlclose(mLibHandle, "reopen");
        mLibHandle = nullptr;
    }
    ResetEncoderSymbols();
    OMX_LOGI("OpenEncoder, lib: %s", libName);
    mLibHandle = dlopen(libName, RTLD_NOW);
    if (mLibHandle == nullptr) {
        OMX_LOGE("OpenEncoder, can't open lib: %s", libName);
        return false;
    }
    if (!LoadHevcEncoderSymbol(mLibHandle, libName, "H265EncPreInit", &mH265EncPreInit) ||
        !LoadHevcEncoderSymbol(mLibHandle, libName, "H265EncGetFnum", &mH265EncGetFnum) ||
        !LoadHevcEncoderSymbol(mLibHandle, libName, "H265EncInit", &mH265EncInit) ||
        !LoadHevcEncoderSymbol(mLibHandle, libName, "H265EncSetConf", &mH265EncSetConf) ||
        !LoadHevcEncoderSymbol(mLibHandle, libName, "H265EncGetConf", &mH265EncGetConf) ||
        !LoadHevcEncoderSymbol(mLibHandle, libName, "H265EncStrmEncode", &mH265EncStrmEncode) ||
        !LoadHevcEncoderSymbol(mLibHandle, libName, "H265EncGenHeader", &mH265EncGenHeader) ||
        !LoadHevcEncoderSymbol(mLibHandle, libName, "H265EncRelease", &mH265EncRelease) ||
        !LoadHevcEncoderSymbol(mLibHandle, libName, "H265EncGetCodecCapability", &mH265EncGetCodecCapability) ||
        !LoadHevcEncoderSymbol(mLibHandle, libName, "H265Enc_get_iova", &mH265EncGetIOVA) ||
        !LoadHevcEncoderSymbol(mLibHandle, libName, "H265Enc_free_iova", &mH265EncFreeIOVA) ||
        !LoadHevcEncoderSymbol(mLibHandle, libName, "H265Enc_get_IOMMU_status", &mH265EncGetIOMMUStatus) ||
        !LoadHevcEncoderSymbol(mLibHandle, libName, "H265Enc_NeedAlign", &mH265EncNeedAlign)) {
        ResetEncoderSymbols();
        return CloseHevcEncoderLibraryOnFailure(&mLibHandle);
    }
    mEncoderSymbolsReady = true;
    return true;
}
int32_t SPRDHEVCEncoder::ExtMemAllocWrapper(
    void *aUserData, unsigned int sizeExtra, MMCodecBuffer *extraMemBfr)
{
    return static_cast<SPRDHEVCEncoder *>(aUserData)->VspMallocCb(sizeExtra, extraMemBfr);
}
int32 SPRDHEVCEncoder::VspMallocCb(uint32 sizeExtra, MMCodecBuffer *extraMemBfr)
{
    int fd;
    unsigned long phyAddr;
    size_t size;
    if (sizeExtra == 0 || extraMemBfr == nullptr || mHandle == nullptr) {
        OMX_LOGE("invalid VspMallocCb param, sizeExtra: %u, extraMemBfr: %p, mHandle: %p",
            sizeExtra, static_cast<void *>(extraMemBfr), static_cast<void *>(mHandle));
        return MEM_ALLOC_FAILURE;
    }
    if (mPmemExtra != nullptr) {
        if (mIOMMUEnabled && mEncoderSymbolsReady && mH265EncFreeIOVA != nullptr) {
            (*mH265EncFreeIOVA)((HEVCHandle*)mHandle, mPbufExtraP, mPbufExtraSize);
        } else if (mIOMMUEnabled) {
            OMX_LOGW("skip free old extra IOVA due to incomplete encoder state");
        }
        releaseMemIon(&mPmemExtra);
        mPmemExtra = nullptr;
        mPbufExtraP = 0;
        mPbufExtraSize = 0;
        OMX_LOGI("Free extra buffer\n");
    }
    mPmemExtra = new VideoMemAllocator(sizeExtra, VideoMemAllocator::NO_CACHE, mIOMMUEnabled);
    if ((fd = mPmemExtra->getHeapID()) < HEAP_ID_ERROR) {
        OMX_LOGE("Failed to alloc extra memory\n");
        return MEM_ALLOC_FAILURE;
    }
    if (mIOMMUEnabled) {
        if (!mEncoderSymbolsReady || mH265EncGetIOVA == nullptr) {
            OMX_LOGE("IOMMU enabled but encoder IOVA callback is unavailable");
            return MEM_ALLOC_FAILURE;
        }
        (*mH265EncGetIOVA)(reinterpret_cast<HEVCHandle*>(mHandle), fd, &phyAddr, &size);
    } else {
        mPmemExtra->getPhyAddrFromMemAlloc(&phyAddr, &size);
    }
    mPbufExtraV = reinterpret_cast<uint8*>(mPmemExtra->getBase());
    mPbufExtraP = phyAddr;
    mPbufExtraSize = size;
    extraMemBfr->commonBufferPtr = mPbufExtraV;
    extraMemBfr->commonBufferPtrPhy = mPbufExtraP;
    extraMemBfr->size = mPbufExtraSize;
    return SUCCESS_RETURN_CODE;
}
SprdOMXComponent *createSprdOMXComponent(
    const char *name, const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData, OMX_COMPONENTTYPE **component)
{
    return new SPRDHEVCEncoder(name, callbacks, appData, component);
}
}
}
