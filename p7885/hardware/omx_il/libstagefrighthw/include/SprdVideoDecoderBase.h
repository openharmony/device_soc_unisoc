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
#ifndef SPRD_VIDEO_DECODER_BASE_H
#define SPRD_VIDEO_DECODER_BASE_H
#include "SprdVideoDecoderOMXComponent.h"
namespace OHOS {
namespace OMX {
struct SprdVideoDecoderBase : public SprdVideoDecoderOMXComponent {
    explicit SprdVideoDecoderBase(const DecoderComponentInitParams &params);
protected:
    virtual OMX_ERRORTYPE internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params) override;
    virtual OMX_ERRORTYPE internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) override;
    virtual OMX_ERRORTYPE getConfig(
        OMX_INDEXTYPE index, OMX_PTR params) override;
    virtual OMX_ERRORTYPE internalSetConfig(
        OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig) override;
    std::mutex mColorAspectsLock;
    // color aspects passed from the framework.
    SprdColorAspects mDefaultColorAspects;
    // color aspects parsed from the bitstream.
    SprdColorAspects mBitstreamColorAspects;
    // final color aspects after combining the above two aspects.
    SprdColorAspects mFinalColorAspects;
    virtual bool SupportsDescribeColorAspects();
    bool colorAspectsDiffer(const SprdColorAspects &a, const SprdColorAspects &b);
    // This functions takes two color aspects and updates the mFinalColorAspects
    // based on |preferredAspects|.
    void updateFinalColorAspects(
        const SprdColorAspects &otherAspects, const SprdColorAspects &preferredAspects);
    // This function will update the mFinalColorAspects based on codec preference.
    status_t handleColorAspectsChange();
    virtual OMX_ERRORTYPE getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index) override;
    void NV12Crop(OMX_BUFFERHEADERTYPE *header, uint32_t srcWidth,
        uint32_t srcHeight, uint32_t dstWidth, uint32_t dstHeight);
};
} // namespace OMX
}  // namespace OHOS
#endif
