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
#include "utils/omx_log.h"
#include "include/SprdVideoDecoderOMXComponent.h"
#include "utils/omx_log.h"
#include "OMXGraphicBufferMapper.h"
/* AFBC decoding scene mode value */
#define DEC_SCENE_MODE_AFBC 4
/* Buffer size calculation factor for YUV420 format: (width * height * 3) / 2 */
#define YUV420_BUFFER_SIZE_FACTOR 3
#define YUV420_BUFFER_SIZE_DIVISOR 2
/* Default buffer alignment values */
#define DEFAULT_INPUT_BUFFER_ALIGNMENT 1
#define DEFAULT_OUTPUT_BUFFER_ALIGNMENT 2
#undef LOG_TAG
#define LOG_TAG "SprdVideoDecoderOMXComponent"
namespace OHOS {
namespace OMX {
template<class T>
static void InitOMXParams(T *params)
{
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    params->nVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    params->nVersion.s.nRevision = REVISION_NUMBER;
    params->nVersion.s.nStep = STEP_NUMBER;
}

struct DecoderPortCommonConfig {
    OMX_U32 portIndex;
    OMX_DIRTYPE dir;
    OMX_U32 minBuffers;
    OMX_U32 actualBuffers;
    OMX_U32 width;
    OMX_U32 height;
};

struct DecoderInputPortConfig {
    OMX_U32 minBuffers;
    OMX_U32 actualBuffers;
    OMX_U32 inputBufferSize;
    const char *mimeType;
    OMX_VIDEO_CODINGTYPE codingType;
    OMX_U32 width;
    OMX_U32 height;
};

static OMX_ERRORTYPE FillVideoPortFormatParam(OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams,
    OMX_VIDEO_CODINGTYPE codingType, OMX_COLOR_FORMATTYPE outputColorFormat)
{
    if (formatParams->nPortIndex == K_INPUT_PORT_INDEX) {
        formatParams->eCompressionFormat = codingType;
        formatParams->eColorFormat = OMX_COLOR_FormatUnused;
        formatParams->xFramerate = 0;
        return OMX_ErrorNone;
    }
    formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
    formatParams->eColorFormat = outputColorFormat;
    formatParams->xFramerate = 0;
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE FillProfileLevelQueryParam(OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel,
    const CodecProfileLevel *profileLevels, size_t numProfileLevels)
{
    if (profileLevel->nProfileIndex >= numProfileLevels) {
        return OMX_ErrorNoMore;
    }
    profileLevel->eProfile = profileLevels[profileLevel->nProfileIndex].mProfile;
    profileLevel->eLevel = profileLevels[profileLevel->nProfileIndex].mLevel;
    return OMX_ErrorNone;
}

static void InitDecoderPortCommon(
    OMX_PARAM_PORTDEFINITIONTYPE *def, const DecoderPortCommonConfig &config)
{
    def->nPortIndex = config.portIndex;
    def->eDir = config.dir;
    def->nBufferCountMin = config.minBuffers;
    def->nBufferCountActual = config.actualBuffers;
    def->bEnabled = OMX_TRUE;
    def->bPopulated = OMX_FALSE;
    def->eDomain = OMX_PortDomainVideo;
    def->bBuffersContiguous = OMX_FALSE;
    def->format.video.pNativeRender = nullptr;
    def->format.video.nFrameWidth = config.width;
    def->format.video.nFrameHeight = config.height;
    def->format.video.nStride = config.width;
    def->format.video.nSliceHeight = config.height;
    def->format.video.nBitrate = 0;
    def->format.video.xFramerate = 0;
    def->format.video.bFlagErrorConcealment = OMX_FALSE;
    def->format.video.pNativeWindow = nullptr;
}

static void ConfigureDecoderInputPort(
    OMX_PARAM_PORTDEFINITIONTYPE *def, const DecoderInputPortConfig &config)
{
    DecoderPortCommonConfig commonConfig = {
        K_INPUT_PORT_INDEX,
        OMX_DirInput,
        config.minBuffers,
        config.actualBuffers,
        config.width,
        config.height,
    };
    InitDecoderPortCommon(def, commonConfig);
    def->nBufferSize = config.inputBufferSize;
    def->nBufferAlignment = DEFAULT_INPUT_BUFFER_ALIGNMENT;
    def->format.video.cMIMEType = const_cast<char *>(config.mimeType);
    def->format.video.eCompressionFormat = config.codingType;
    def->format.video.eColorFormat = OMX_COLOR_FormatUnused;
}

static void ConfigureDecoderOutputPort(
    OMX_PARAM_PORTDEFINITIONTYPE *def,
    OMX_U32 minBuffers,
    OMX_U32 actualBuffers,
    OMX_U32 width,
    OMX_U32 height)
{
    DecoderPortCommonConfig commonConfig = {
        K_OUTPUT_PORT_INDEX,
        OMX_DirOutput,
        minBuffers,
        actualBuffers,
        width,
        height,
    };
    InitDecoderPortCommon(def, commonConfig);
    def->nBufferAlignment = DEFAULT_OUTPUT_BUFFER_ALIGNMENT;
    def->format.video.cMIMEType = const_cast<char *>("video/raw");
    def->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    def->format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    def->nBufferSize = (width * height * YUV420_BUFFER_SIZE_FACTOR) /
        YUV420_BUFFER_SIZE_DIVISOR;
}

SprdVideoDecoderOMXComponent::SprdVideoDecoderOMXComponent(const DecoderComponentInitParams &params)
    : SprdSimpleOMXComponent(params.name, params.callbacks, params.appData, params.component),
    mOutputPortSettingsChange(NONE),
    mInputBufferCount(0),
    mSetFreqCount(0),
    mFbcMode(FBC_NONE),
    mUsage(0),
    mEOSStatus(INPUT_DATA_AVAILABLE),
    mHeadersDecoded(false),
    mSignalledError(false),
    mAllocateBuffers(false),
    mIOMMUEnabled(false),
    mIOMMUID(-1),
    mPmemStream(nullptr),
    mPbufStreamV(nullptr),
    mPbufStreamP(0),
    mPbufStreamSize(0),
    mPmemExtra(nullptr),
    mPbufExtraV(nullptr),
    mPbufExtraP(0),
    mPbufExtraSize(0),
    mLibHandle(nullptr),
    mDecSceneMode(0),
    mDecoderSwFlag(false),
    mChangeToSwDec(false),
    mNeedIVOP(true),
    mStopDecode(false),
    mThumbnailMode(OMX_FALSE),
    mSecureFlag(false),
    mMinCompressionRatio(1),
    mComponentRole(params.componentRole),
    mCodingType(params.codingType),
    mProfileLevels(params.profileLevels),
    mNumProfileLevels(params.numProfileLevels),
    mUpdateColorAspects(false)
{
    iUseOMXNativeBuffer[OMX_DirInput] = OMX_FALSE;
    iUseOMXNativeBuffer[OMX_DirOutput] = OMX_FALSE;
}
void SprdVideoDecoderOMXComponent::InitPorts(const PortConfig &config)
{
    mMinCompressionRatio = config.minCompressionRatio;
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);
    DecoderInputPortConfig inputConfig = {
        config.numMinInputBuffers,
        config.numInputBuffers,
        config.inputBufferSize,
        config.mimeType,
        mCodingType,
        config.width,
        config.height,
    };
    ConfigureDecoderInputPort(&def, inputConfig);
    addPort(def);
    ConfigureDecoderOutputPort(
        &def, config.numMinOutputBuffers, config.numOutputBuffers, config.width, config.height);
    addPort(def);
}
OMX_ERRORTYPE SprdVideoDecoderOMXComponent::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_IndexParamVideoPortFormat: {
            OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
                static_cast<OMX_VIDEO_PARAM_PORTFORMATTYPE *>(params);
            if (!IsValidOmxParam(formatParams)) {
                return OMX_ErrorBadParameter;
            }
            if (formatParams->nPortIndex > K_OUTPUT_PORT_INDEX) {
                return OMX_ErrorBadPortIndex;
            }
            if (formatParams->nIndex != 0) {
                return OMX_ErrorNoMore;
            }
            OMX_COLOR_FORMATTYPE outputColorFormat = OMX_COLOR_FormatUnused;
            if (formatParams->nPortIndex == K_OUTPUT_PORT_INDEX) {
                outputColorFormat = editPortInfo(K_OUTPUT_PORT_INDEX)->mDef.format.video.eColorFormat;
                OMX_LOGI("internalGetParameter, OMX_IndexParamVideoPortFormat, eColorFormat: 0x%x",
                    outputColorFormat);
            }
            return FillVideoPortFormatParam(formatParams, mCodingType, outputColorFormat);
        }
        case OMX_IndexParamVideoProfileLevelQuerySupported: {
            OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel =
                static_cast<OMX_VIDEO_PARAM_PROFILELEVELTYPE *>(params);
            if (!IsValidOmxParam(profileLevel)) {
                return OMX_ErrorBadParameter;
            }
            if (profileLevel->nPortIndex != K_INPUT_PORT_INDEX) {
                OMX_LOGE("Invalid port index: %d", profileLevel->nPortIndex);
                return OMX_ErrorUnsupportedIndex;
            }
            return FillProfileLevelQueryParam(profileLevel, mProfileLevels, mNumProfileLevels);
        }
        default:
            return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}
OMX_ERRORTYPE SprdVideoDecoderOMXComponent::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_IndexParamStandardComponentRole: {
            const OMX_PARAM_COMPONENTROLETYPE *roleParams =
                (const OMX_PARAM_COMPONENTROLETYPE *)params;
            if (!IsValidOmxParam(roleParams)) {
                return OMX_ErrorBadParameter;
            }
            if (strncmp((const char *)roleParams->cRole,
                        mComponentRole,
                        OMX_MAX_STRINGNAME_SIZE - 1)) {
                return OMX_ErrorUndefined;
            }
            return OMX_ErrorNone;
        }
        case OMX_IndexParamVideoPortFormat: {
            OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;
            if (formatParams->nPortIndex > K_OUTPUT_PORT_INDEX) {
                return OMX_ErrorUndefined;
            }
            return OMX_ErrorNone;
        }
        case OMX_INDEX_PREPARE_FOR_ADAPTIVE_PLAYBACK: {
            return OMX_ErrorNone;
        }
        default:
            return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}
OMX_ERRORTYPE SprdVideoDecoderOMXComponent::getConfig (
    OMX_INDEXTYPE index, OMX_PTR params)
{
    (void)index;
    (void)params;
    return OMX_ErrorUnsupportedIndex;
}
OMX_ERRORTYPE SprdVideoDecoderOMXComponent::internalSetConfig(
    OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig)
{
    (void)frameConfig;
    switch (static_cast<int>(index)) {
        case OMX_INDEX_CONFIG_DEC_SCENE_MODE: {
            int *pDecSceneMode = reinterpret_cast<int*>(params);
#ifdef CONFIG_AFBC_SUPPORT
            if (*pDecSceneMode == DEC_SCENE_MODE_AFBC) {
                mFbcMode =  AFBC;
            }
#endif
            OMX_LOGI("%s, %d, setConfig, mDecSceneMode = %d", __FUNCTION__, __LINE__, *pDecSceneMode);
            return OMX_ErrorNone;
        }
        default:
            return OMX_ErrorUnsupportedIndex;
    }
}
int SprdVideoDecoderOMXComponent::GetColorAspectPreference()
{
    return K_NOT_SUPPORTED;
}
void SprdVideoDecoderOMXComponent::onPortEnableCompleted(OMX_U32 portIndex, bool enabled)
{
    switch (mOutputPortSettingsChange) {
        case NONE:
            break;
        case AWAITING_DISABLED: {
            if (enabled) {
                OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(!enabled) failed.",
                    __FUNCTION__, FILENAME_ONLY, __LINE__);
                return;
            }
            mOutputPortSettingsChange = AWAITING_ENABLED;
            break;
        }
        default: {
            if ((static_cast<int>(mOutputPortSettingsChange)) != (static_cast<int>(AWAITING_ENABLED))) {
                OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((static_cast<int>(mOutputPortSettingsChange)) "
                    "== (static_cast<int>(AWAITING_ENABLED))) failed.",
                    __FUNCTION__, FILENAME_ONLY, __LINE__);
                return;
            }
            if (!enabled) {
                OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(enabled) failed.",
                    __FUNCTION__, FILENAME_ONLY, __LINE__);
                return;
            }
            mOutputPortSettingsChange = NONE;
            break;
        }
    }
}
void SprdVideoDecoderOMXComponent::OnReset()
{
    mSignalledError = false;
    //avoid process error after stop codec and restart codec when port settings changing.
    mOutputPortSettingsChange = NONE;
}
SprdVideoDecoderOMXComponent::~SprdVideoDecoderOMXComponent()
{
}
};    // namespace OMX
};    // namespace OHOS
