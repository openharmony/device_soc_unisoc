/*
 * Copyright 2023 Shenzhen Kaihong DID Co., Ltd.
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
// Prevent duplicate inclusion (compatible with all compilers)
#ifndef OHOS_UNISOC_OMX_SPRDAVCENCODER_OHOS_H_
#define OHOS_UNISOC_OMX_SPRDAVCENCODER_OHOS_H_
#pragma once
#include "SPRDAVCEncoder.h"
struct SPRDAVCEncoderOhos:public OHOS::OMX::SPRDAVCEncoder {
    SPRDAVCEncoderOhos(
    const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component);
protected:
    virtual OMX_ERRORTYPE internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params);
    virtual OMX_ERRORTYPE internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params);
    virtual void onQueueFilled(OMX_U32 portIndex);
    virtual OMX_ERRORTYPE initEncParams();
private:
    using EncodeOutputState = OHOS::OMX::SPRDAVCEncoder::EncodeOutputState;
    using EncodeFrameContext = OHOS::OMX::SPRDAVCEncoder::EncodeFrameContext;
    using GraphicBufferMapping = OHOS::OMX::SPRDAVCEncoder::GraphicBufferMapping;
    void InitOhosEncInfo();
    OMX_ERRORTYPE initOhosBufferConfig();
    OMX_ERRORTYPE initOhosCodecSession();
    OMX_ERRORTYPE allocateOhosStreamBuffers(size_t sizeStream);
    OMX_ERRORTYPE configureOhosEncConfig();
    void resetOhosOutputHeader(OMX_BUFFERHEADERTYPE *outHeader) const;
    bool SyncOhosStreamBuffer(int fd, unsigned int flags) const;
    bool CopyOhosHeaderToBuffer(const MMEncOut &header, const char *tag, bool log16Bytes) const;
    bool generateOhosCodecHeader(int fd, MMEncOut &header, int32_t headerType,
        const char *tag, bool log16Bytes);
    bool appendOhosEncodedData(OMX_BUFFERHEADERTYPE *outHeader, uint8_t *&outPtr,
        void *source, uint32_t sourceSize, uint32_t &dataLength) const;
    bool AppendOhosCodecConfigHeaders(EncodeOutputState *output, int fd);
    bool finalizeOhosCodecConfigOutput(EncodeOutputState &output, std::list<BufferInfo*>& outQueue);
    bool emitOhosCodecConfig(EncodeOutputState &output, std::list<BufferInfo*>& outQueue);
    OMX_ERRORTYPE getOhosSupportBufferType(UseBufferType *defParams);
    OMX_ERRORTYPE getOhosCodecVideoPortFormat(CodecVideoPortFormatParam *formatParams);
    OMX_ERRORTYPE setOhosPortDefinition(const OMX_PTR params);
    OMX_ERRORTYPE setOhosCodecVideoPortFormat(const OMX_PTR params);
    void queueOhosInputBufferInfo(OMX_BUFFERHEADERTYPE *inHeader);
    bool mapOhosGraphicBuffer(OMX_BUFFERHEADERTYPE *inHeader, GraphicBufferMapping &mapping) const;
    void UnmapOhosGraphicBuffer(const GraphicBufferMapping &mapping) const;
    void configureOhosEncodeInput(MMEncIn &vidIn, OMX_BUFFERHEADERTYPE *inHeader,
        uint8_t *py, uint8_t *pyPhy);
    void SyncOhosStreamBuffers();
    bool PatchOhosEncodedFrame(MMEncOut &vidOut) const;
    bool encodeOhosInputBuffer(OMX_BUFFERHEADERTYPE *inHeader, EncodeOutputState *output);
    bool FinalizeOhosOutputBuffer(EncodeFrameContext &frame);
    bool processOneOhosInputFrame(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue);
    uint32_t mStride;
};
#endif // OHOS_UNISOC_OMX_SPRDAVCENCODER_OHOS_H_
