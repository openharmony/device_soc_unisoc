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
#ifndef SPRD_HEVC_ENCODER_H_
#define SPRD_HEVC_ENCODER_H_
#include "SprdVideoEncoderBase.h"
#include "hevc_enc_api.h"
#include "OMX_VideoExt.h"
#include "SprdOMXComponent.h"
#define H265ENC_INTERNAL_BUFFER_SIZE  (0x200000)
namespace OHOS {
namespace OMX {
struct SPRDHEVCEncoder : public SprdVideoEncoderBase {
    SPRDHEVCEncoder(
    const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component);
protected:
    struct EncodeOutputState {
        BufferInfo *outInfo;
        OMX_BUFFERHEADERTYPE *outHeader;
        uint8_t *outPtr;
        uint32_t dataLength;
    };
    struct EncodeFrameContext {
        std::list<BufferInfo*> *inQueue;
        std::list<BufferInfo*> *outQueue;
        BufferInfo *inInfo;
        BufferInfo *outInfo;
        OMX_BUFFERHEADERTYPE *inHeader;
        OMX_BUFFERHEADERTYPE *outHeader;
        EncodeOutputState output;
    };
    struct MappedInputBuffer {
        void *bufferHandle;
        uint8_t *virtualAddr;
        uint8_t *physicalAddr;
        unsigned long iova;
        size_t iovaLen;
        bool needUnmap;
        uint32_t width;
        uint32_t height;
        uint32_t cropX;
        uint32_t cropY;
    };
    OMX_BOOL mPrependSPSPPS;
    virtual ~SPRDHEVCEncoder();
    OMX_ERRORTYPE internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) override;
    OMX_ERRORTYPE internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) override;
    OMX_ERRORTYPE initCheck() const override;
    void onQueueFilled(OMX_U32 portIndex) override;
    OMX_ERRORTYPE getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index) override;
    virtual void setPFrames(OMX_U32* pPFrames, OMX_VIDEO_PARAM_HEVCTYPE* hevcType)
    {
        (void)pPFrames;
        (void)hevcType;
    }
    virtual void CheckUpdateColorAspects(const MMEncConfig *encConfig) const
    {
        (void)encConfig;
    }
private:
    enum {
        K_NUM_BUFFERS = 4,
    };
    TagHevcHandle          *mHandle = nullptr;
    TagHevcEncParam        *mEncParams = nullptr;
    MMEncConfig *mEncConfig = nullptr;
    MMEncVideoInfo mEncInfo;
    MMEncCapability mCapability;
    bool     mSpsPpsHeaderReceived = false;
    int mMFEnum = 1;
    PMemIon mPmemStream[MAX_SRC_FRAME_BUFFER] = { nullptr };
    MMCodecBSBuffer mStreamBfr;
    HEVCProfile mHEVCEncProfile = AVC_BASELINE;
    HEVCLevel   mHEVCEncLevel = AVC_LEVEL2;
    FT_H265EncPreInit        mH265EncPreInit = nullptr;
    FT_H265EncGetFnum     mH265EncGetFnum = nullptr;
    FT_H265EncInit        mH265EncInit = nullptr;
    FT_H265EncSetConf        mH265EncSetConf = nullptr;
    FT_H265EncGetConf        mH265EncGetConf = nullptr;
    FT_H265EncStrmEncode        mH265EncStrmEncode = nullptr;
    FT_H265EncGenHeader        mH265EncGenHeader = nullptr;
    FT_H265EncRelease        mH265EncRelease = nullptr;
    FT_H265EncGetCodecCapability  mH265EncGetCodecCapability = nullptr;
    FtH265EncGetIova  mH265EncGetIOVA = nullptr;
    FtH265EncFreeIova  mH265EncFreeIOVA = nullptr;
    FtH265EncGetIommuStatus  mH265EncGetIOMMUStatus = nullptr;
    FtH265EncNeedAlign  mH265EncNeedAlign = nullptr;
    bool InitializeEncoderHandle(const char *name);
    bool InitializeEncoderRuntime(const char *name);
    void ResetEncoderSymbols();
    void InitPorts() const;
    OMX_ERRORTYPE initEncParams() override;
    OMX_ERRORTYPE validateEncParamPointers();
    void InitEncParamDefaults(MMEncCodecPriority &encPriority);
    void ConfigureDdrFrequency() const;
    size_t CalculateStreamBufferSize() const;
    OMX_ERRORTYPE allocateStreamBuffers(size_t sizeStream);
    void InitVideoInfo();
    OMX_ERRORTYPE applyEncoderConfig(MMEncCodecPriority &encPriority);
    void FinalizeEncParams();
    OMX_ERRORTYPE releaseEncoder();
    OMX_ERRORTYPE releaseResource();
    bool OpenEncoder(const char *libName);
    void resetOutputHeader(OMX_BUFFERHEADERTYPE *outHeader);
    bool appendEncodedData(OMX_BUFFERHEADERTYPE *outHeader, uint8_t *&outPtr,
        uint8_t *srcBuffer, int32_t srcSize, uint32_t &dataLength);
    void patchStartCodeAndLog(uint8_t *buffer, const char *tag, bool log16Bytes = false);
    bool generateAndAppendCodecHeader(int32_t headerType, const char *tag, bool log16Bytes,
        EncodeOutputState *output);
    bool emitCodecConfig(EncodeOutputState &output, std::list<BufferInfo*>& outQueue);
    OMX_ERRORTYPE getSupportBufferType(UseBufferType *defParams);
    OMX_ERRORTYPE getVideoHevc(OMX_VIDEO_PARAM_HEVCTYPE *hevcParams);
    OMX_ERRORTYPE getProfileLevelQuerySupported(OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel);
    OMX_ERRORTYPE getCodecVideoPortFormat(CodecVideoPortFormatParam *formatParams);
    OMX_ERRORTYPE setVideoBitrate(const OMX_VIDEO_PARAM_BITRATETYPE *bitRate);
    OMX_ERRORTYPE setVideoHevc(const OMX_VIDEO_PARAM_HEVCTYPE *hevcType);
    OMX_ERRORTYPE setCodecVideoPortFormat(const CodecVideoPortFormatParam *formatParams);
    bool mapInputGraphicBuffer(OMX_BUFFERHEADERTYPE *inHeader, MappedInputBuffer &mappedBuffer);
    void UnmapInputGraphicBuffer(const MappedInputBuffer &mappedBuffer) const;
    void configureEncodeInput(MMEncIn &vidIn, OMX_BUFFERHEADERTYPE *inHeader, const MappedInputBuffer &mappedBuffer);
    void SyncInputStreamBuffers() const;
    bool FinalizeEncodedFrame(MMEncOut &vidOut, EncodeOutputState &output);
    bool encodeInputBuffer(OMX_BUFFERHEADERTYPE *inHeader, EncodeOutputState &output);
    bool FinalizeOutputBuffer(EncodeFrameContext &frame);
    bool processOneInputFrame(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue);
    void FlushCacheforBsBuf() const;
    void BeginCpuAccessforBsBuf() const;
    void EndCpuAccessforBsBuf() const;
    static void FlushCacheWrapper(const void *aUserData);
    static void BeginCPUAccessWrapper(const void *aUserData);
    static void EndCPUAccessWrapper(const void *aUserData);
    static int32_t ExtMemAllocWrapper(void *aUserData, unsigned int sizeExtra, MMCodecBuffer *extraMemBfr);
    int VspMallocCb(unsigned int sizeExtra, MMCodecBuffer *extraMemBfr);
    SPRDHEVCEncoder(const SPRDHEVCEncoder &);
    SPRDHEVCEncoder &operator=(const SPRDHEVCEncoder &);
    bool mEncoderSymbolsReady = false;
    bool mEncoderPreInitDone = false;
    OMX_ERRORTYPE mInitCheck = OMX_ErrorInsufficientResources;
};
}
}
#endif  // SPRD_HEVC_ENCODER_H_
