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
#define LOG_TAG "SprdVideoEncoderOMXComponent"
#include <utils/omx_log.h>
#include "SprdVideoEncoderOMXComponent.h"
#include "sprd_omx_typedef.h"

namespace OHOS {
namespace OMX {
static const char *MEDIA_MIMETYPE_VIDEO_RAW = "video/raw";

struct EncoderPortCommonConfig {
    OMX_U32 portIndex;
    OMX_DIRTYPE dir;
    OMX_U32 numBuffers;
    size_t bufferSize;
    OMX_U32 alignment;
    OMX_U32 width;
    OMX_U32 height;
    OMX_U32 bitrate;
    OMX_U32 frameRateQ16;
};

struct EncoderInputPortConfig {
    OMX_U32 numBuffers;
    size_t inputBufferSize;
    OMX_U32 width;
    OMX_U32 height;
    OMX_U32 bitrate;
    OMX_U32 frameRate;
    OMX_U32 colorFormat;
};

struct EncoderOutputPortConfig {
    OMX_U32 numBuffers;
    size_t outputBufferSize;
    const char *mimeType;
    OMX_VIDEO_CODINGTYPE compressionFormat;
    OMX_U32 width;
    OMX_U32 height;
    OMX_U32 bitrate;
};
/* Port configuration constants */
#define NUM_PORTS                        2
#define INPUT_PORT_INDEX                 0
#define OUTPUT_PORT_INDEX                1
#define INPUT_BUFFER_ALIGNMENT           1
#define OUTPUT_BUFFER_ALIGNMENT          2
#define DEFAULT_FRAME_RATE               30
#define DEFAULT_BIT_RATE                 192000
#define DEFAULT_VIDEO_WIDTH              176
#define DEFAULT_VIDEO_HEIGHT             144
#define DEFAULT_P_FRAMES                 29
#define MAX_COLOR_FORMAT_SUPPORTED       4
/* Frame alignment constants */
#define FRAME_ALIGNMENT_MASK             0xFFFFFFF0  /* Align to 16 bytes */
#define FRAME_ALIGNMENT_VALUE            16
#define FRAME_ALIGNMENT_OFFSET           15
/* Color format indices */
#define COLOR_FORMAT_IDX_SPRD_YVU420_SEMIPLANAR  0
#define COLOR_FORMAT_IDX_YUV420_SEMIPLANAR       1
#define COLOR_FORMAT_IDX_YUV420_PLANAR           2
#define COLOR_FORMAT_IDX_OPAQUE                  3
/* Encoder scene mode constants */
#define ENC_SCENE_MODE_NORMAL            0
#define ENC_SCENE_MODE_WFD               2
/* YUV buffer size calculation */
#define YUV420_MULTIPLIER_NUMERATOR      3
#define YUV420_MULTIPLIER_DENOMINATOR    2
/* Version information */
#define OMX_VERSION_MAJOR                1
/* Metadata configuration */
#define METADATA_DISABLED                0
#define METADATA_ENABLED                 1
/* Frame rate conversion */
#define FRAME_RATE_Q16_SHIFT             16
/* Memory alignment */
#define MEMORY_ALIGNMENT_BASE            1
template<class T>
static void InitOMXParams(T *params)
{
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = OMX_VERSION_MAJOR;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

static void InitEncoderPortCommon(
    OMX_PARAM_PORTDEFINITIONTYPE *def, const EncoderPortCommonConfig &config)
{
    def->nPortIndex = config.portIndex;
    def->eDir = config.dir;
    def->nBufferCountMin = config.numBuffers;
    def->nBufferCountActual = config.numBuffers;
    def->nBufferSize = config.bufferSize;
    def->bEnabled = OMX_TRUE;
    def->bPopulated = OMX_FALSE;
    def->eDomain = OMX_PortDomainVideo;
    def->bBuffersContiguous = OMX_FALSE;
    def->nBufferAlignment = config.alignment;
    def->format.video.nBitrate = config.bitrate;
    def->format.video.nFrameWidth = config.width;
    def->format.video.nFrameHeight = config.height;
    def->format.video.nStride = config.width;
    def->format.video.nSliceHeight = config.height;
    def->format.video.xFramerate = config.frameRateQ16;
}

static void ConfigureEncoderInputPort(
    OMX_PARAM_PORTDEFINITIONTYPE *def, const EncoderInputPortConfig &config)
{
    EncoderPortCommonConfig commonConfig = {
        INPUT_PORT_INDEX,
        OMX_DirInput,
        config.numBuffers,
        config.inputBufferSize,
        INPUT_BUFFER_ALIGNMENT,
        config.width,
        config.height,
        config.bitrate,
        config.frameRate << FRAME_RATE_Q16_SHIFT,
    };
    InitEncoderPortCommon(def, commonConfig);
    def->format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_RAW);
    def->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    def->format.video.eColorFormat = static_cast<OMX_COLOR_FORMATTYPE>(config.colorFormat);
}

static void ConfigureEncoderOutputPort(
    OMX_PARAM_PORTDEFINITIONTYPE *def, const EncoderOutputPortConfig &config)
{
    EncoderPortCommonConfig commonConfig = {
        OUTPUT_PORT_INDEX,
        OMX_DirOutput,
        config.numBuffers,
        config.outputBufferSize,
        OUTPUT_BUFFER_ALIGNMENT,
        config.width,
        config.height,
        config.bitrate,
        0,
    };
    InitEncoderPortCommon(def, commonConfig);
    def->format.video.cMIMEType = const_cast<char *>(config.mimeType);
    def->format.video.eCompressionFormat = config.compressionFormat;
    def->format.video.eColorFormat = OMX_COLOR_FormatUnused;
}

static int32_t GetAlignedSliceHeight(bool needAlign, OMX_U32 height)
{
    if (!needAlign) {
        return static_cast<int32_t>(height);
    }
    return (height + FRAME_ALIGNMENT_OFFSET) & FRAME_ALIGNMENT_MASK;
}

SprdVideoEncoderOMXComponent::SprdVideoEncoderOMXComponent(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
    mNumInputFrames(-1),
    mSetFreqCount(0),
    mBitrate(0),
    mIsChangeBitrate(false),
    mEncSceneMode(ENC_SCENE_MODE_NORMAL),
    mSetEncMode(false),
    mVideoWidth(DEFAULT_VIDEO_WIDTH),
    mVideoHeight(DEFAULT_VIDEO_HEIGHT),
    mVideoFrameRate(DEFAULT_FRAME_RATE),
    mVideoBitRate(DEFAULT_BIT_RATE),
    mVideoColorFormat(OMX_SPRD_COLOR_FORMAT_YV_U420_SEMI_PLANAR),
    mStoreMetaData(OMX_FALSE),
    mIOMMUEnabled(false),
    mIOMMUID(-1),
    mStarted(false),
    mSignalledError(false),
    mKeyFrameRequested(false),
    mPbuf_inter(nullptr),
    mPbuf_yuv_v(nullptr),
    mPbuf_yuv_p(0),
    mPbuf_yuv_size(0),
    mPbufStreamV(nullptr),
    mPbufStreamP(0),
    mPbufStreamSize(0),
    mPbufExtraV(nullptr),
    mPbufExtraP(0),
    mPbufExtraSize(0),
    mPFrames(DEFAULT_P_FRAMES),
    mNeedAlign(false),
    mLibHandle(nullptr),
    mFrameWidth(DEFAULT_VIDEO_WIDTH),
    mFrameHeight(DEFAULT_VIDEO_HEIGHT),
    mSupportRGBEnc(false)
{
    OMX_LOGI("Construct SprdVideoEncoderOMXComponent, this: 0x%p", static_cast<void *>(this));
#ifdef CONFIG_SPRD_RECORD_EIS
    mEISMode = false;
#endif
}
SprdVideoEncoderOMXComponent::~SprdVideoEncoderOMXComponent()
{
    OMX_LOGI("Destruct SprdVideoEncoderOMXComponent, this: 0x%p", static_cast<void *>(this));
    #ifdef CONFIG_POWER_HINT
    if (mIsForWFD) {
        mIsPowerHintRunning = false;
        OMX_LOGI("~Encoder for wfd exit");
    }
    #endif
}
OMX_ERRORTYPE SprdVideoEncoderOMXComponent::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_IndexParamVideoErrorCorrection: {
            return OMX_ErrorNotImplemented;
        }
        case OMX_IndexParamVideoBitrate: {
            OMX_VIDEO_PARAM_BITRATETYPE *bitRate =
                (OMX_VIDEO_PARAM_BITRATETYPE *) params;
            if (bitRate->nPortIndex != OUTPUT_PORT_INDEX) {
                return OMX_ErrorUndefined;
            }
            bitRate->eControlRate = OMX_Video_ControlRateVariable;
            bitRate->nTargetBitrate = mVideoBitRate;
            return OMX_ErrorNone;
        }
        case OMX_IndexParamVideoInit: {
            OMX_PORT_PARAM_TYPE* pParam = (OMX_PORT_PARAM_TYPE*)params;
            pParam->nPorts = NUM_PORTS;
            pParam->nStartPortNumber = INPUT_PORT_INDEX;
            return OMX_ErrorNone;
        }
        default:
            return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}
OMX_ERRORTYPE SprdVideoEncoderOMXComponent::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_IndexParamVideoErrorCorrection: {
            return OMX_ErrorNotImplemented;
        }
        case OMX_INDEX_CONFIG_VIDEO_RECORD_MODE: {
#ifdef CONFIG_SPRD_RECORD_EIS
            OMX_BOOL *pEnable = (OMX_BOOL *)params;
            if (*pEnable == METADATA_ENABLED) {
                mEISMode = OMX_TRUE;
            }
            OMX_LOGI("record eismode=%d", mEISMode);
#endif
            return OMX_ErrorNone;
        }
        case OMX_IndexParamPortDefinition: {
            OMX_PARAM_PORTDEFINITIONTYPE *defParams =
                (OMX_PARAM_PORTDEFINITIONTYPE *)params;
            PortInfo *port = editPortInfo(defParams->nPortIndex);
            OMX_ERRORTYPE err = SprdSimpleOMXComponent::internalSetParameter(index, params);
            if (OMX_ErrorNone != err) {
                return err;
            }
            errno_t ret = memmove_s(&port->mDef.format.video, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE),
                &defParams->format.video, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
            if (ret != 0) {
                OMX_LOGE("memmove_s failed in line %d, ret=%d", __LINE__, ret);
            }
            return OMX_ErrorNone;
        }
        case OMX_IndexParamVideoIntraRefresh: {
            return OMX_ErrorNone;   ///hw encoder may not support this mode
        }
        default:
            return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}
OMX_ERRORTYPE SprdVideoEncoderOMXComponent::setConfigIntraRefreshVop(const OMX_PTR params)
{
    OMX_CONFIG_INTRAREFRESHVOPTYPE *pConfigIntraRefreshVOP =
        (OMX_CONFIG_INTRAREFRESHVOPTYPE *)params;
    if (pConfigIntraRefreshVOP->nPortIndex != OUTPUT_PORT_INDEX) {
        return OMX_ErrorBadPortIndex;
    }
    mKeyFrameRequested = pConfigIntraRefreshVOP->IntraRefreshVOP;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SprdVideoEncoderOMXComponent::setConfigVideoBitrate(const OMX_PTR params)
{
    OMX_VIDEO_CONFIG_BITRATETYPE *pConfigParams =
        (OMX_VIDEO_CONFIG_BITRATETYPE *)params;
    if (pConfigParams->nPortIndex != OUTPUT_PORT_INDEX) {
        return OMX_ErrorBadPortIndex;
    }
    mBitrate = pConfigParams->nEncodeBitrate;
    mIsChangeBitrate = METADATA_ENABLED;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SprdVideoEncoderOMXComponent::setConfigEncSceneMode(const OMX_PTR params)
{
    int *pEncSceneMode = reinterpret_cast<int*>(params);
    mEncSceneMode = *pEncSceneMode;
    mSetEncMode = true;
#ifdef CONFIG_POWER_HINT
    if (mEncSceneMode == ENC_SCENE_MODE_WFD) {
        mIsForWFD = true;
        mIsPowerHintRunning = true;
    }
#endif
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SprdVideoEncoderOMXComponent::setConfigVideoFramerate(const OMX_PTR params)
{
    OMX_CONFIG_FRAMERATETYPE *framerateParams =
        reinterpret_cast<OMX_CONFIG_FRAMERATETYPE *>(params);
    if (framerateParams == nullptr || framerateParams->nPortIndex != INPUT_PORT_INDEX) {
        return OMX_ErrorBadPortIndex;
    }
    if (framerateParams->xEncodeFramerate > 0) {
        mVideoFrameRate = static_cast<int32_t>(framerateParams->xEncodeFramerate >> FRAME_RATE_Q16_SHIFT);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SprdVideoEncoderOMXComponent::setConfigOperatingRate(const OMX_PTR params)
{
    OMX_PARAM_U32TYPE *operatingRate = reinterpret_cast<OMX_PARAM_U32TYPE *>(params);
    if (operatingRate == nullptr || operatingRate->nPortIndex >= NUM_PORTS) {
        return OMX_ErrorBadPortIndex;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE SprdVideoEncoderOMXComponent::internalSetConfig(
    OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig)
{
    switch (static_cast<int>(index)) {
        case OMX_IndexConfigVideoIntraVOPRefresh:
            return setConfigIntraRefreshVop(params);
        case OMX_IndexConfigVideoBitrate:
            return setConfigVideoBitrate(params);
        case OMX_INDEX_CONFIG_ENC_SCENE_MODE:
            return setConfigEncSceneMode(params);
        case OMX_IndexConfigVideoFramerate:
            return setConfigVideoFramerate(params);
        case OMX_IndexParamOperatingRate:
            return setConfigOperatingRate(params);
        default:
            return SprdSimpleOMXComponent::internalSetConfig(index, params, frameConfig);
    }
}
OMX_ERRORTYPE SprdVideoEncoderOMXComponent::getConfig(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    return SprdSimpleOMXComponent::getConfig(index, params);
}
OMX_ERRORTYPE SprdVideoEncoderOMXComponent::getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index)
{
    if (strcmp(name, "OMX.index.storeMetaDataInBuffers") == 0) {
        *index = (OMX_INDEXTYPE) OMX_INDEX_STORE_META_DATA_IN_BUFFERS;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.index.describeColorFormat") == 0) {
        *index = (OMX_INDEXTYPE) OMX_INDEX_DESCRIBE_COLOR_FORMAT;
        return OMX_ErrorNone;
    }
    return SprdSimpleOMXComponent::getExtensionIndex(name, index);
}
void SprdVideoEncoderOMXComponent::InitPorts(
    OMX_U32 numBuffers,
    const size_t inputBufferSize,
    const size_t outputBufferSize,
    const char *MIMEType,
    OMX_VIDEO_CODINGTYPE compressionFormat) const
{
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);
    EncoderInputPortConfig inputConfig = {
        numBuffers,
        inputBufferSize,
        mVideoWidth,
        mVideoHeight,
        mVideoBitRate,
        mVideoFrameRate,
        OMX_SPRD_COLOR_FORMAT_YV_U420_SEMI_PLANAR,
    };
    ConfigureEncoderInputPort(&def, inputConfig);
    const_cast<SprdVideoEncoderOMXComponent*>(this)->addPort(def);
    EncoderOutputPortConfig outputConfig = {
        numBuffers,
        outputBufferSize,
        MIMEType,
        compressionFormat,
        mVideoWidth,
        mVideoHeight,
        mVideoBitRate,
    };
    ConfigureEncoderOutputPort(&def, outputConfig);
    const_cast<SprdVideoEncoderOMXComponent*>(this)->addPort(def);
}
OMX_ERRORTYPE SprdVideoEncoderOMXComponent::initEncParams()
{
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE SprdVideoEncoderOMXComponent::initEncoder()
{
    if (mStarted) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(!mStarted) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    OMX_ERRORTYPE errType = initEncParams();
    if (OMX_ErrorNone != errType) {
        OMX_LOGE("Failed to initialized encoder params");
        mSignalledError = true;
        notify(OMX_EventError, OMX_ErrorUndefined, 0, 0);
        return errType;
    }
    mStarted = true;
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SprdVideoEncoderOMXComponent::releaseResource()
{
    if (mPbufExtraV != nullptr) {
        releaseMemIon(&mPmemExtra);
        mPbufExtraV = nullptr;
        mPbufExtraP = 0;
        mPbufExtraSize = 0;
    }
    if (mPbufStreamV != nullptr) {
        releaseMemIon(&mPmemStream);
        mPbufStreamV = nullptr;
        mPbufStreamP = 0;
        mPbufStreamSize = 0;
    }
    if (mPbuf_yuv_v != nullptr) {
        releaseMemIon(&mYUVInPmemHeap);
        mPbuf_yuv_v = nullptr;
        mPbuf_yuv_p = 0;
        mPbuf_yuv_size = 0;
    }
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SprdVideoEncoderOMXComponent::getParamVideoPortFormat(
    OMX_VIDEO_CODINGTYPE compressionFormat, OMX_PTR params)
{
    OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
        (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;
    if (formatParams->nPortIndex > OUTPUT_PORT_INDEX) {
        return OMX_ErrorUndefined;
    }
    if (formatParams->nIndex >= MAX_COLOR_FORMAT_SUPPORTED) {
        return OMX_ErrorNoMore;
    }
    if (formatParams->nPortIndex == INPUT_PORT_INDEX) {
        formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
        if (formatParams->nIndex == COLOR_FORMAT_IDX_SPRD_YVU420_SEMIPLANAR) {
            formatParams->eColorFormat = OMX_SPRD_COLOR_FORMAT_YV_U420_SEMI_PLANAR;
        } else if (formatParams->nIndex == COLOR_FORMAT_IDX_YUV420_SEMIPLANAR) {
            formatParams->eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        } else if (formatParams->nIndex == COLOR_FORMAT_IDX_YUV420_PLANAR) {
            formatParams->eColorFormat = OMX_COLOR_FormatYUV420Planar;
        } else {
            formatParams->eColorFormat = OMX_COLOR_FORMAT_OPAQUE;
        }
    } else {
        formatParams->eCompressionFormat = (OMX_VIDEO_CODINGTYPE)compressionFormat;
        formatParams->eColorFormat = OMX_COLOR_FormatUnused;
    }
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SprdVideoEncoderOMXComponent::setParamPortDefinition(
    OMX_VIDEO_CODINGTYPE compressionFormat, OMX_INDEXTYPE index, OMX_PTR params)
{
    OMX_PARAM_PORTDEFINITIONTYPE *def =
        (OMX_PARAM_PORTDEFINITIONTYPE *)params;
    if (def->nPortIndex > OUTPUT_PORT_INDEX) {
        return OMX_ErrorUndefined;
    }
    OMX_LOGI("internalSetParameter, eCompressionFormat: %d, eColorFormat: 0x%x",
        def->format.video.eCompressionFormat, def->format.video.eColorFormat);
    setOutputPortformat(def, mFrameWidth, mFrameHeight);
    int32_t sliceHeight = GetAlignedSliceHeight(mNeedAlign, def->format.video.nFrameHeight);
    int32_t stride = (def->format.video.nFrameWidth + FRAME_ALIGNMENT_OFFSET) & FRAME_ALIGNMENT_MASK;
    // As encoder we should modify bufferSize on Input port
    // Make sure we have enough input date for input buffer
    if (def->nPortIndex <= OUTPUT_PORT_INDEX) {
        uint32_t bufferSize = stride * sliceHeight * YUV420_MULTIPLIER_NUMERATOR / YUV420_MULTIPLIER_DENOMINATOR;
        if (bufferSize > def->nBufferSize) {
            def->nBufferSize = bufferSize;
        }
    }
    //translate Flexible 8-bit YUV format to our default YUV format
    if (def->format.video.eColorFormat == OMX_COLOR_FORMAT_YU_V420_FLEXIBLE) {
        OMX_LOGI("internalSetParameter, translate Flexible 8-bit YUV format to SPRD YVU420SemiPlanar");
        def->format.video.eColorFormat = OMX_SPRD_COLOR_FORMAT_YV_U420_SEMI_PLANAR;
    }
    OMX_ERRORTYPE err = SprdVideoEncoderOMXComponent::internalSetParameter(index, params);
    if (OMX_ErrorNone != err) {
        return err;
    }
    if (def->nPortIndex == INPUT_PORT_INDEX) {
        updateInputPortDefinition(def, stride, sliceHeight);
    } else {
        mVideoBitRate = def->format.video.nBitrate;
    }
    return OMX_ErrorNone;
}
void SprdVideoEncoderOMXComponent::updateInputPortDefinition(
    OMX_PARAM_PORTDEFINITIONTYPE *def, int32_t stride, int32_t sliceHeight)
{
    mFrameWidth = def->format.video.nFrameWidth;
    mFrameHeight = def->format.video.nFrameHeight;
    mVideoWidth = stride;
    mVideoHeight = sliceHeight;
    mVideoFrameRate = def->format.video.xFramerate >> FRAME_RATE_Q16_SHIFT;
    mVideoColorFormat = def->format.video.eColorFormat;
}
OMX_ERRORTYPE SprdVideoEncoderOMXComponent::setParamStandardComponentRole(
    const char *role, OMX_PTR params)
{
    const OMX_PARAM_COMPONENTROLETYPE *roleParams =
        (const OMX_PARAM_COMPONENTROLETYPE *)params;
    if (strncmp((const char *)roleParams->cRole,
        role,
        OMX_MAX_STRINGNAME_SIZE - MEMORY_ALIGNMENT_BASE)) {
        return OMX_ErrorUndefined;
    }
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SprdVideoEncoderOMXComponent::setParamVideoPortFormat(
    OMX_VIDEO_CODINGTYPE compressionFormat, OMX_PTR params)
{
    const OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
        (const OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;
    if (formatParams->nPortIndex > OUTPUT_PORT_INDEX) {
        return OMX_ErrorUndefined;
    }
    if (formatParams->nPortIndex == INPUT_PORT_INDEX) {
        mVideoColorFormat = formatParams->eColorFormat;
    } else {
        if (formatParams->eCompressionFormat != compressionFormat ||
                formatParams->eColorFormat != OMX_COLOR_FormatUnused) {
            return OMX_ErrorUndefined;
        }
    }
    return OMX_ErrorNone;
}
}
}
