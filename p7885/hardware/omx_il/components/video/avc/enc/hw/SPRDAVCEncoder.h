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
#ifndef SPRD_AVC_ENCODER_H_
#define SPRD_AVC_ENCODER_H_
#include "SprdVideoEncoderBase.h"
#include "avc_enc_api.h"
#include "SprdOMXComponent.h"
#define H264ENC_INTERNAL_BUFFER_SIZE  (0x200000)
namespace OHOS {
namespace OMX {
struct SPRDAVCEncoder : public SprdVideoEncoderBase {
    SPRDAVCEncoder(
    const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component);
protected:
    struct GraphicBufferMapping {
        uint8_t *py;
        uint8_t *pyPhy;
        size_t bufferSize;
        unsigned long iova;
        size_t iovaLen;
        bool needUnmap;
    };

    struct EncodeOutputState {
        BufferInfo *outInfo;
        OMX_BUFFERHEADERTYPE *outHeader;
        uint8_t *outPtr;
        uint32_t dataLength;
        bool shouldReturn;
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
    OMX_BOOL mPrependSPSPPS;
    virtual ~SPRDAVCEncoder();
    OMX_ERRORTYPE internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) override;
    OMX_ERRORTYPE internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) override;
    OMX_ERRORTYPE initCheck() const override;
    void onQueueFilled(OMX_U32 portIndex) override;
    OMX_ERRORTYPE getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index) override;
    void CheckUpdateColorAspects(const MMEncConfig *encConfig) const
    {
        (void)encConfig;
    }
protected:
    enum {
        K_NUM_BUFFERS = 4,
    };
    TagAvcHandle          *mHandle = nullptr;
    MMEncConfig *mEncConfig = nullptr;
    MMEncVideoInfo mEncInfo;
    MMEncCapability mCapability;
    int32_t  mrateCtrlEnable = 1;
    bool     mSpsPpsHeaderReceived = false;
    bool     mUVExchange = false;
    int mMFEnum = 1;
    PMemIon mPmemStream[MAX_SRC_FRAME_BUFFER] = { nullptr };
    MMCodecBSBuffer mStreamBfr;
    AVCProfile mAVCEncProfile = AVC_BASELINE;
    AVCLevel   mAVCEncLevel = AVC_LEVEL2;
    FT_H264EncPreInit        mH264EncPreInit = nullptr;
    FT_H264EncGetFnum mH264EncGetFnum = nullptr;
    FT_H264EncInit        mH264EncInit = nullptr;
    FT_H264EncSetConf        mH264EncSetConf = nullptr;
    FT_H264EncGetConf        mH264EncGetConf = nullptr;
    FT_H264EncStrmEncode        mH264EncStrmEncode = nullptr;
    FT_H264EncGenHeader        mH264EncGenHeader = nullptr;
    FT_H264EncRelease        mH264EncRelease = nullptr;
    FT_H264EncGetCodecCapability  mH264EncGetCodecCapability = nullptr;
    FT_H264EncGetIova  mH264EncGetIOVA = nullptr;
    FT_H264EncFreeIova  mH264EncFreeIOVA = nullptr;
    FT_H264EncGetIommuStatus  mH264EncGetIOMMUStatus = nullptr;
    FT_H264EncNeedAlign  mH264EncNeedAlign = nullptr;
    bool InitializeHandle();
    bool InitializeEncoderLibrary(const char *name);
    bool InitializeEncoderSession();
    void InitializeEncoderRuntimeFlags(const char *name);
    void ResetEncoderSymbols();
    OMX_ERRORTYPE prepareEncoderBuffers();
    OMX_ERRORTYPE startEncoderSession();
    void InitPorts() const;
    OMX_ERRORTYPE initEncParams() override;
    void InitEncInfo();
    OMX_ERRORTYPE allocateStreamBuffers(size_t sizeStream);
    OMX_ERRORTYPE configureEncConfig();
    OMX_ERRORTYPE releaseEncoder();
    bool OpenEncoder(const char *libName);
    OMX_ERRORTYPE releaseResource();
    void FlushCacheforBsBuf() const;
    void BeginCpuAccessforBsBuf() const;
    void EndCpuAccessforBsBuf() const;
    void resetOutputHeader(OMX_BUFFERHEADERTYPE *outHeader);
    bool appendEncodedData(OMX_BUFFERHEADERTYPE *outHeader, uint8_t *&outPtr,
        void *srcBuffer, int32_t srcSize, uint32_t &dataLength);
    bool SyncStreamBuffer(int fd, unsigned int flags);
    void patchStartCodeAndLog(uint8_t *buffer, const char *tag, bool log16Bytes);
    bool CopyHeaderToBuffer(MMEncOut &header, const char *tag, bool log16Bytes);
    bool generateCodecHeader(int fd, MMEncOut &header, int32_t headerType,
        const char *tag, bool log16Bytes);
    bool AppendCodecConfigHeaders(EncodeOutputState *output, int fd);
    bool finalizeCodecConfigOutput(EncodeOutputState &output, std::list<BufferInfo*>& outQueue);
    bool emitCodecConfig(EncodeOutputState &output, std::list<BufferInfo*>& outQueue);
    OMX_ERRORTYPE getSupportBufferType(UseBufferType *defParams);
    OMX_ERRORTYPE getVideoAvc(OMX_VIDEO_PARAM_AVCTYPE *avcParams);
    OMX_ERRORTYPE getCodecVideoPortFormat(CodecVideoPortFormatParam *formatParams);
    OMX_ERRORTYPE getProfileLevelQuerySupported(OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel);
    OMX_ERRORTYPE setUseBufferType(const UseBufferType *defParams);
    OMX_ERRORTYPE setProcessName(const ProcessNameParam *nameParams) const;
    OMX_ERRORTYPE setVideoBitrate(const OMX_VIDEO_PARAM_BITRATETYPE *bitRate);
    OMX_ERRORTYPE setPortDefinition(const OMX_PTR params);
    OMX_ERRORTYPE setCodecVideoPortFormat(const CodecVideoPortFormatParam *formatParams);
    OMX_ERRORTYPE setVideoAvc(const OMX_VIDEO_PARAM_AVCTYPE *avcType);
    void queueInputBufferInfo(OMX_BUFFERHEADERTYPE *inHeader);
    bool mapGraphicBuffer(OMX_BUFFERHEADERTYPE *inHeader, GraphicBufferMapping &mapping);
    void UnmapGraphicBuffer(const GraphicBufferMapping &mapping);
    void configureEncodeInput(MMEncIn &vidIn, OMX_BUFFERHEADERTYPE *inHeader, uint8_t *py, uint8_t *pyPhy);
    void SyncAllStreamBuffers();
    bool PatchEncodedFrame(MMEncOut &vidOut);
    bool runEncode(const GraphicBufferMapping &mapping, OMX_BUFFERHEADERTYPE *inHeader, MMEncOut &vidOut, int &ret);
    bool AppendEncodedFrame(MMEncOut &vidOut, EncodeOutputState *output);
    bool encodeInputBuffer(OMX_BUFFERHEADERTYPE *inHeader, EncodeOutputState &output);
    bool ValidateOutputDimensions();
    bool submitEncodedOutput(EncodeFrameContext &frame, const InputBufferInfo &inputBufInfo);
    EncodeFrameContext BuildFrameContext(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue);
    void FailCurrentFrame(EncodeFrameContext &frame, bool inputInfoQueued);
    bool FinalizeOutputBuffer(EncodeFrameContext &frame);
    bool processOneInputFrame(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue);
    bool LoadEncoderSymbols(const char *libName);
    static void FlushCacheWrapper(const void *aUserData);
    static void BeginCPUAccessWrapper(const void *aUserData);
    static void EndCPUAccessWrapper(const void *aUserData);
    static int32_t ExtMemAllocWrapper(void *aUserData, unsigned int sizeExtra, MMCodecBuffer *extraMemBfr);
    void ReleaseExtraBuffer();
    int VspMallocCb(unsigned int sizeExtra, MMCodecBuffer *extraMemBfr);
    SPRDAVCEncoder(const SPRDAVCEncoder &);
    SPRDAVCEncoder &operator=(const SPRDAVCEncoder &);
    OMX_BOOL iUseOhosNativeBuffer[2] = { OMX_FALSE, OMX_FALSE };
    uint32_t mStride = 0;
    uint32_t mSliceHeight = 0;
    bool dumpFlag = true;
    bool mEncoderSymbolsReady = false;
    bool mEncoderPreInitDone = false;
    OMX_ERRORTYPE mInitCheck = OMX_ErrorInsufficientResources;
};
}
}
#endif  // SPRD_AVC_ENCODER_H_
