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
#ifndef SPRD_VIDEO_ENCODER_OMX_COMPONENT_H_
#define SPRD_VIDEO_ENCODER_OMX_COMPONENT_H_
#include "sprd_omx_typedef.h"
#include "SprdSimpleOMXComponent.h"
#include "OMXVideoAPI.h"
namespace OHOS {
namespace OMX {
#define SPRD_ION_DEV "/dev/ion"
struct SprdVideoEncoderOMXComponent : public SprdSimpleOMXComponent {
    SprdVideoEncoderOMXComponent(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component);
protected:
    // OMX input buffer's timestamp and flags
    typedef struct {
        int64_t mTimeUs;
        int32_t mFlags;
    } InputBufferInfo;
    Vector<InputBufferInfo> mInputBufferInfoVec;
    int64_t  mNumInputFrames;
    int mSetFreqCount;
    int32_t mBitrate;
    bool     mIsChangeBitrate;
    int mEncSceneMode;
    bool mSetEncMode;
    int32_t  mVideoWidth;
    int32_t  mVideoHeight;
    int32_t  mVideoFrameRate;
    int32_t  mVideoBitRate;
    int32_t  mVideoColorFormat;
    OMX_U32 mPFrames;
    bool mNeedAlign;
    OMX_BOOL mStoreMetaData;
    mutable bool     mIOMMUEnabled;
    int mIOMMUID;
    bool     mStarted;
    bool     mSignalledError;
    bool     mKeyFrameRequested;
    void *mLibHandle;
    uint8_t *mPbuf_inter;
    PMemIon mYUVInPmemHeap;
    uint8_t *mPbuf_yuv_v;
    unsigned long mPbuf_yuv_p;
    size_t mPbuf_yuv_size;
    PMemIon mPmemStream;
    uint8_t *mPbufStreamV;
    unsigned long mPbufStreamP;
    size_t mPbufStreamSize;
    PMemIon mPmemExtra;
    uint8_t *mPbufExtraV;
    unsigned long  mPbufExtraP;
    size_t  mPbufExtraSize;
    int32_t  mFrameWidth;
    int32_t  mFrameHeight;
    bool mSupportRGBEnc;
#ifdef CONFIG_SPRD_RECORD_EIS
    int32_t meisMode;
#endif
    OMX_ERRORTYPE internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) override;
    OMX_ERRORTYPE internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) override;
    OMX_ERRORTYPE internalSetConfig(
        OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig) override;
    OMX_ERRORTYPE getConfig(
    OMX_INDEXTYPE index, OMX_PTR params) override;
    OMX_ERRORTYPE getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index) override;
    ~SprdVideoEncoderOMXComponent() override;
    void InitPorts(
    OMX_U32 numBuffers, size_t intputBufferSize, size_t outputBufferSize,
        const char *MIMEType, OMX_VIDEO_CODINGTYPE compressionFormat) const;
    OMX_ERRORTYPE initEncoder();
    virtual OMX_ERRORTYPE initEncParams();
    OMX_ERRORTYPE releaseResource();
    OMX_ERRORTYPE getParamVideoPortFormat(
    OMX_VIDEO_CODINGTYPE compressionFormat, OMX_PTR params);
    OMX_ERRORTYPE setParamPortDefinition(
    OMX_VIDEO_CODINGTYPE compressionFormat, OMX_INDEXTYPE index, OMX_PTR params);
    OMX_ERRORTYPE setParamStandardComponentRole(
    const char *role, OMX_PTR params);
    OMX_ERRORTYPE setParamVideoPortFormat(
    OMX_VIDEO_CODINGTYPE compressionFormat, OMX_PTR params);
    OMX_ERRORTYPE setConfigIntraRefreshVop(const OMX_PTR params);
    OMX_ERRORTYPE setConfigVideoBitrate(const OMX_PTR params);
    OMX_ERRORTYPE setConfigEncSceneMode(const OMX_PTR params);
    OMX_ERRORTYPE setConfigVideoFramerate(const OMX_PTR params);
    OMX_ERRORTYPE setConfigOperatingRate(const OMX_PTR params);
    void updateInputPortDefinition(
        OMX_PARAM_PORTDEFINITIONTYPE *def, int32_t stride, int32_t sliceHeight);
    virtual void setOutputPortformat(
    OMX_PARAM_PORTDEFINITIONTYPE *def, int32_t mFrameWidth, int32_t  mFrameHeight)
    {
        (void)def;
        (void)mFrameWidth;
        (void)mFrameHeight;
    }
private:
    SprdVideoEncoderOMXComponent(const SprdVideoEncoderOMXComponent &);
    SprdVideoEncoderOMXComponent &operator=(const SprdVideoEncoderOMXComponent &);
};
}
}
#endif  // SPRD_VIDEO_ENCODER_OMX_COMPONENT_H_
