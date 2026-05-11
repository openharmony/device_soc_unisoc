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
#ifndef SPRD_VIDEO_DECODER_OMX_COMPONENT_H_
#define SPRD_VIDEO_DECODER_OMX_COMPONENT_H_
#include "SprdSimpleOMXComponent.h"
#include "OMX_Core.h"
#include "OMX_Video.h"
#include "OMX_VideoExt.h"
#include "VideoMemAllocator.h"
#include "Errors.h"
#include "sprd_omx_typedef.h"
#include "OMXVideoAPI.h"
namespace OHOS {
namespace OMX {
#define SPRD_ION_DEV "/dev/ion"

struct DecoderComponentInitParams {
    const char *name;
    const char *componentRole;
    OMX_VIDEO_CODINGTYPE codingType;
    const CodecProfileLevel *profileLevels;
    size_t numProfileLevels;
    const OMX_CALLBACKTYPE *callbacks;
    OMX_PTR appData;
    OMX_COMPONENTTYPE **component;
};

struct SprdVideoDecoderOMXComponent : public SprdSimpleOMXComponent {
    explicit SprdVideoDecoderOMXComponent(const DecoderComponentInitParams &params);
protected:
    struct PortConfig {
        OMX_U32 numMinInputBuffers;
        OMX_U32 numInputBuffers;
        OMX_U32 inputBufferSize;
        OMX_U32 numMinOutputBuffers;
        OMX_U32 numOutputBuffers;
        const char *mimeType;
        OMX_U32 width;
        OMX_U32 height;
        OMX_U32 minCompressionRatio;
    };

    enum {
        K_NOT_SUPPORTED,
        K_PREFER_BITSTREAM,
        K_PREFER_CONTAINER,
    };
    enum EOSStatus {
        INPUT_DATA_AVAILABLE,
        INPUT_EOS_SEEN,
        OUTPUT_FRAMES_FLUSHED,
    };

    enum FBC_MODE {
        FBC_NONE,
        IFBC = 1,
        AFBC = 2
    };

    enum {
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    } mOutputPortSettingsChange;
    size_t mInputBufferCount;
    int mSetFreqCount;
    uint8_t mFbcMode;
    uint32_t mUsage;
    EOSStatus mEOSStatus;
    bool mHeadersDecoded;
    bool mSignalledError;
    bool mAllocateBuffers;
    mutable bool mIOMMUEnabled;
    int mIOMMUID;
    PMemIon mPmemStream;
    uint8_t *mPbufStreamV;
    unsigned long mPbufStreamP;
    size_t mPbufStreamSize;
    PMemIon mPmemExtra;
    uint8_t *mPbufExtraV;
    unsigned long  mPbufExtraP;
    size_t  mPbufExtraSize;
    //handle for  libomx_avcdec_sw_sprd.z.so
    void *mLibHandle;
    OMX_BOOL iUseOMXNativeBuffer[2];
    int mDecSceneMode;
    bool mDecoderSwFlag;
    bool mChangeToSwDec;
    bool mNeedIVOP;
    bool mStopDecode;
    OMX_BOOL mThumbnailMode;
    bool mSecureFlag;
    uint32_t mMinCompressionRatio;
    bool mUpdateColorAspects;
    ~SprdVideoDecoderOMXComponent() override;
    void onPortEnableCompleted(OMX_U32 portIndex, bool enabled) override;
    void OnReset() override;
    OMX_ERRORTYPE internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params) override;
    OMX_ERRORTYPE internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) override;
    OMX_ERRORTYPE getConfig(
        OMX_INDEXTYPE index, OMX_PTR params) override;
    OMX_ERRORTYPE internalSetConfig(
        OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig) override;
    virtual int GetColorAspectPreference();
    // This function sets input/output port buffer counts and size using the provided
    // per-port configuration.
    void InitPorts(const PortConfig &config);
private:
    const char *mComponentRole;
    OMX_VIDEO_CODINGTYPE mCodingType;
    const CodecProfileLevel *mProfileLevels;
    size_t mNumProfileLevels;
    SprdVideoDecoderOMXComponent(const SprdVideoDecoderOMXComponent &);
    SprdVideoDecoderOMXComponent &operator=(const SprdVideoDecoderOMXComponent &);
};
}  // namespace OMX
}  // namespace OHOS
#endif  // SPRD_VIDEO_DECODER_OMX_COMPONENT_H_
