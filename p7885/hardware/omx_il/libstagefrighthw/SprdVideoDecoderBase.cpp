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
#include "SprdVideoDecoderBase.h"
#include "OMXHardwareAPI.h"
#include <securec.h>
#include <cstring>
#include <new>
#undef LOG_TAG
#define LOG_TAG "SprdVideoDecoderBase"
/* NV12 format UV plane vertical subsampling ratio */
#define NV12_UV_VERTICAL_SUBSAMPLING_RATIO 2
namespace OHOS {
namespace OMX {
SprdVideoDecoderBase::SprdVideoDecoderBase(const DecoderComponentInitParams &params)
    : SprdVideoDecoderOMXComponent(params)
{
    // init all the color aspects to be Unspecified.
    (void)memset_s(&mDefaultColorAspects, sizeof(SprdColorAspects), 0, sizeof(SprdColorAspects));
    (void)memset_s(&mBitstreamColorAspects, sizeof(SprdColorAspects), 0, sizeof(SprdColorAspects));
    (void)memset_s(&mFinalColorAspects, sizeof(SprdColorAspects), 0, sizeof(SprdColorAspects));
}
OMX_ERRORTYPE SprdVideoDecoderBase::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_INDEX_ENABLE_ANB: {
            EnableOMXNativeBuffersParams *peanbp = (EnableOMXNativeBuffersParams *)params;
            peanbp->enable = iUseOMXNativeBuffer[OMX_DirOutput];
            OMX_LOGI("internalGetParameter, OMX_INDEX_ENABLE_ANB %d", peanbp->enable);
            return OMX_ErrorNone;
        }
        default:
            return SprdVideoDecoderOMXComponent::internalGetParameter(index, params);
    }
}
OMX_ERRORTYPE SprdVideoDecoderBase::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_INDEX_ENABLE_ANB: {
            EnableOMXNativeBuffersParams *peanbp = (EnableOMXNativeBuffersParams *)params;
            PortInfo *pOutPort = editPortInfo(K_OUTPUT_PORT_INDEX);
            if (peanbp->enable == OMX_FALSE) {
                OMX_LOGI("internalSetParameter, disable NativeBuffer");
                iUseOMXNativeBuffer[OMX_DirOutput] = OMX_FALSE;
                pOutPort->mDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            } else {
                OMX_LOGI("---PLEASE USE OHOS_BUFFER_HANDLE, DO NOT USE NativeBuffer");
                return OMX_ErrorUnsupportedIndex;
            }
            return OMX_ErrorNone;
        }
        default:
            return SprdVideoDecoderOMXComponent::internalSetParameter(index, params);
    }
}
OMX_ERRORTYPE SprdVideoDecoderBase::getConfig(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    switch (static_cast<int>(index)) {
        case OMX_INDEX_DESCRIBE_COLOR_ASPECTS: {
            if (!SupportsDescribeColorAspects()) {
                return OMX_ErrorUnsupportedIndex;
            }
            DescribeColorAspectsParams* colorAspectsParams =
                    (DescribeColorAspectsParams *)params;
            if (!IsValidOmxParam(colorAspectsParams)) {
                return OMX_ErrorBadParameter;
            }
            if (colorAspectsParams->nPortIndex != K_OUTPUT_PORT_INDEX) {
                return OMX_ErrorBadParameter;
            }
            colorAspectsParams->sAspects = mFinalColorAspects;
            if (colorAspectsParams->bRequestingDataSpace || colorAspectsParams->bDataSpaceChanged) {
                return OMX_ErrorUnsupportedSetting;
            }
            return OMX_ErrorNone;
        }
        default:
            return SprdVideoDecoderOMXComponent::getConfig(index, params);
    }
}
OMX_ERRORTYPE SprdVideoDecoderBase::internalSetConfig(
    OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig)
{
    switch (static_cast<int>(index)) {
        case OMX_INDEX_DESCRIBE_COLOR_ASPECTS: {
            if (!SupportsDescribeColorAspects()) {
                return OMX_ErrorUnsupportedIndex;
            }
            const DescribeColorAspectsParams* colorAspectsParams =
                (const DescribeColorAspectsParams *)params;
            if (!IsValidOmxParam(colorAspectsParams)) {
                return OMX_ErrorBadParameter;
            }
            if (colorAspectsParams->nPortIndex != K_OUTPUT_PORT_INDEX) {
                return OMX_ErrorBadParameter;
            }
            // Update color aspects if necessary.
            if (colorAspectsDiffer(colorAspectsParams->sAspects, mDefaultColorAspects)) {
                mDefaultColorAspects = colorAspectsParams->sAspects;
                status_t err = handleColorAspectsChange();
                if (err != STATUS_OK) {
                    OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(err == STATUS_OK) failed.",
                        __FUNCTION__, FILENAME_ONLY, __LINE__);
                    return OMX_ErrorUndefined;
                }
            }
            return OMX_ErrorNone;
        }
        default:
            return SprdVideoDecoderOMXComponent::internalSetConfig(index, params, frameConfig);
    }
}
bool SprdVideoDecoderBase::SupportsDescribeColorAspects()
{
    return GetColorAspectPreference() != K_NOT_SUPPORTED;
}
bool SprdVideoDecoderBase::colorAspectsDiffer(
    const SprdColorAspects &a, const SprdColorAspects &b)
{
    return false;
}
void SprdVideoDecoderBase::updateFinalColorAspects(
    const SprdColorAspects &otherAspects, const SprdColorAspects &preferredAspects)
{
    std::lock_guard<std::mutex> autoLock(mColorAspectsLock);
    SprdColorAspects newAspects;
    newAspects.rangeMode = preferredAspects.rangeMode != SprdColorAspects::kRangeUnspecified ?
        preferredAspects.rangeMode : otherAspects.rangeMode;
    newAspects.primaryId = preferredAspects.primaryId != SprdColorAspects::kPrimariesUnspecified ?
        preferredAspects.primaryId : otherAspects.primaryId;
    newAspects.transferId = preferredAspects.transferId != SprdColorAspects::kTransferUnspecified ?
        preferredAspects.transferId : otherAspects.transferId;
    newAspects.matrixCoeffId = preferredAspects.matrixCoeffId != SprdColorAspects::kMatrixUnspecified ?
        preferredAspects.matrixCoeffId : otherAspects.matrixCoeffId;
    // Check to see if need update mFinalColorAspects.
    if (colorAspectsDiffer(mFinalColorAspects, newAspects)) {
        mFinalColorAspects = newAspects;
        mUpdateColorAspects = true;
    }
}
status_t SprdVideoDecoderBase::handleColorAspectsChange()
{
    int perference = GetColorAspectPreference();
    OMX_LOGV("Color Aspects preference: %d ", perference);
    if (perference == K_PREFER_BITSTREAM) {
        updateFinalColorAspects(mDefaultColorAspects, mBitstreamColorAspects);
    } else if (perference == K_PREFER_CONTAINER) {
        updateFinalColorAspects(mBitstreamColorAspects, mDefaultColorAspects);
    } else {
        return OMX_ErrorUnsupportedSetting;
    }
    return STATUS_OK;
}
OMX_ERRORTYPE SprdVideoDecoderBase::getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index)
{
    OMX_LOGI("getExtensionIndex, name: %s", name);
    if (strcmp(name, "OMX.index.enableNativeBuffers") == 0) {
        *reinterpret_cast<int32_t*>(index) = OMX_INDEX_ENABLE_ANB;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.index.getNativeBufferUsage") == 0) {
        *reinterpret_cast<int32_t*>(index) = OMX_INDEX_GET_ANB_USAGE;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.index.useNativeBuffer2") == 0) {
        *reinterpret_cast<int32_t*>(index) = OMX_INDEX_USE_ANB;
        return OMX_ErrorNone;
    } else if (!strcmp(name, "OMX.sprd.index.DecSceneMode")) {
        *reinterpret_cast<int32_t*>(index) = OMX_INDEX_CONFIG_DEC_SCENE_MODE;
        return OMX_ErrorNone;
    } else if (!strcmp(name, "OMX.index.prepareForAdaptivePlayback")) {
        *reinterpret_cast<int32_t*>(index) = OMX_INDEX_PREPARE_FOR_ADAPTIVE_PLAYBACK;
        return OMX_ErrorNone;
    } else if (!strcmp(name, "OMX.index.describeColorAspects")
                && SupportsDescribeColorAspects()) {
        *reinterpret_cast<int32_t*>(index) = OMX_INDEX_DESCRIBE_COLOR_ASPECTS;
        return OMX_ErrorNone;
    }
    return OMX_ErrorNotImplemented;
}
void SprdVideoDecoderBase::NV12Crop(OMX_BUFFERHEADERTYPE *header,
                                    uint32_t srcWidth, uint32_t srcHeight, uint32_t dstWidth, uint32_t dstHeight)
{
    if (header == nullptr || header->pBuffer == nullptr || header->pPlatformPrivate == nullptr) {
        return;
    }
    OMX_LOGD("NV12Crop start, srcWidth = %u, srcHeight = %u",
        srcWidth, srcHeight);
    OMX_LOGD("dstWidth = %u, dstHeight = %u",
        dstWidth, dstHeight);
    OMX_U8* buffInfo = reinterpret_cast<OMX_U8*>(header->pBuffer);
    OMX_U8* dstInfo = reinterpret_cast<OMX_U8*>(header->pPlatformPrivate);
    for (uint32_t i = 0; i < dstHeight; i++) {
        errno_t ret = memmove_s(
            static_cast<void*>(&dstInfo[i * dstWidth]),
            static_cast<size_t>(dstWidth),
            static_cast<const void*>(&buffInfo[i * srcWidth]),
            static_cast<size_t>(dstWidth));
        if (ret != 0) {
            OMX_LOGE("memmove_s failed in line %d, ret=%d", __LINE__, ret);
        }
    }
    uint32_t dstSize = dstHeight * dstWidth;
    uint32_t srcSize = srcHeight * srcWidth;
    for (uint32_t i = 0; i < dstHeight / NV12_UV_VERTICAL_SUBSAMPLING_RATIO; i++) {
        errno_t ret = memmove_s(
            static_cast<void*>(&dstInfo[dstSize + i * dstWidth]),
            static_cast<size_t>(dstWidth),
            static_cast<const void*>(&buffInfo[srcSize + i * srcWidth]),
            static_cast<size_t>(dstWidth));
        if (ret != 0) {
            OMX_LOGE("memmove_s failed in line %d, ret=%d", __LINE__, ret);
        }
    }
}
};    // namespace OMX
};    // namespace OHOS
