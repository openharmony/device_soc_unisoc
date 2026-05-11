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
#ifndef SPRD_HEVC_DECODER_H_
#define SPRD_HEVC_DECODER_H_
#include "SprdVideoDecoderBase.h"
#include "hevc_dec_api.h"
#include "codec_omx_ext.h"
#include "sprd_omx_typedef.h"
#include "OMXGraphicBufferMapper.h"
#include "buffer_handle.h"
#include "SprdOMXComponent.h"
namespace OHOS {
namespace OMX {
struct TagHevcHandle;
struct SPRDHEVCDecoder : public SprdVideoDecoderBase {
    SPRDHEVCDecoder(const char *name,
                    const OMX_CALLBACKTYPE *callbacks,
                    OMX_PTR appData,
                    OMX_COMPONENTTYPE **component);
    OMX_ERRORTYPE initCheck() const override;
protected:
    int32_t picId = 0;
    TagHevcHandle *mHandle = nullptr;
    FtH265DecSetCurRecPic  mH265DecSetCurRecPic = nullptr;
    virtual ~SPRDHEVCDecoder();
    OMX_ERRORTYPE internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) override;
    OMX_ERRORTYPE internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) override;
    OMX_ERRORTYPE internalUseBuffer(
        OMX_BUFFERHEADERTYPE **buffer, const UseBufferParams &params) override;
    OMX_ERRORTYPE allocateBuffer(
    OMX_BUFFERHEADERTYPE **header,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size)  override;
    OMX_ERRORTYPE freeBuffer(
    OMX_U32 portIndex,
        OMX_BUFFERHEADERTYPE *header) override;
    OMX_ERRORTYPE getConfig(OMX_INDEXTYPE index, OMX_PTR params) override;
    OMX_ERRORTYPE internalSetConfig(
    OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig) override;
    void onQueueFilled(OMX_U32 portIndex) override;
    void onPortFlushCompleted(OMX_U32 portIndex) override;
    void onPortFlushPrepare(OMX_U32 portIndex) override;
    OMX_ERRORTYPE getExtensionIndex(const char *name, OMX_INDEXTYPE *index) override;
    int GetColorAspectPreference() override;
    void UpdatePortDefinitions(bool updateCrop = true, bool updateInputSize = false);
    virtual bool GetVuiParams(H265SwDecInfo *mDecoderInfo);
    void drainOneOutputBuffer(int32_t picId, const void *pBufferHeader, OMX_U64 pts) override
    {
        (void) picId;
        (void) pBufferHeader;
        (void) pts;
    }
    void notifyFillBufferDone(OMX_BUFFERHEADERTYPE *header) override;
private:
    enum {
        K_NUM_INPUT_BUFFERS  = 5,
        K_NUM_OUTPUT_BUFFERS = 13,
    };
    uint32_t mFrameWidth = 320;
    uint32_t mFrameHeight = 240;
    uint32_t mStride = 320;
    uint32_t mSliceHeight = 240;
    uint32_t mStride16 = 0;
    uint32_t mPictureSize = 320 * 240 * 3 / 2;
    uint32_t mCropWidth = 320;
    uint32_t mCropHeight = 240;
    uint8_t mhigh10En = 0;
    Mutex mLock;
    uint8_t *mCodecInterBuffer = nullptr;
    uint8_t *mCodecExtraBuffer = nullptr;
    PMemIon mPmemMbInfo[17] = { nullptr };
    uint8_t *mPmemMbInfoV[17] = { nullptr };
    unsigned long  mPmemMbInfoP[17] = { 0 };
    size_t  mPbufMbInfoSize[17] = { 0 };
    int mPbufMbInfoIdx = 0;
    FT_H265DecInit mH265DecInit = nullptr;
    FT_H265DecGetInfo mH265DecGetInfo = nullptr;
    FT_H265DecDecode mH265DecDecode = nullptr;
    FT_H265DecRelease mH265DecRelease = nullptr;
    FtH265DecGetLastDspFrm  mH265DecGetLastDspFrm = nullptr;
    FtH265DecReleaseRefBuffers  mH265DecReleaseRefBuffers = nullptr;
    FT_H265DecMemInit mH265DecMemInit = nullptr;
    FT_H265GetCodecCapability mH265GetCodecCapability = nullptr;
    FT_H265DecGetNALType mH265DecGetNALType = nullptr;
    FT_H265DecSetparam mH265DecSetparam = nullptr;
    FtH265DecGetIova mH265DecGetIOVA = nullptr;
    FtH265DecFreeIova mH265DecFreeIOVA = nullptr;
    FtH265DecGetIommuStatus mH265DecGetIOMMUStatus = nullptr;
    MMDecCapability mCapability;
    OMX_ERRORTYPE mInitCheck = OMX_ErrorNone;
    META_DATA_T* st2094 = nullptr;
    OMX_BOOL iUseOhosNativeBuffer[2] = { OMX_FALSE, OMX_FALSE };
    status_t initDecoder();
    void InitSecureMode(const char *name);
    void InitPortsAndStatus() const;
    bool OpenAndInitDecoder();
    void InitMbInfoState();
    void InitDecoderHandle();
    OMX_ERRORTYPE initDecoderInstance();
    void UpdateIommuStatus() const;
    OMX_ERRORTYPE updatePortDefinition(OMX_PARAM_PORTDEFINITIONTYPE *defParams, PortInfo *port);
    void applyPortGeometryChange(OMX_U32 portIndex, uint32_t newWidth, uint32_t newHeight);
    void EnsureStreamBufferCapacity();
    void ReleaseDecoder();
    void ReleaseExtraBuffer();
    void ReleaseMbInfoBuffers();
    void ReleaseCodecResources();
    OMX_ERRORTYPE getSupportBufferType(UseBufferType *defParams);
    OMX_ERRORTYPE getUseBufferType(UseBufferType *defParams);
    OMX_ERRORTYPE getBufferHandleUsage(GetBufferHandleUsageParams *defParams);
    OMX_ERRORTYPE getPortDefinitionParam(OMX_PARAM_PORTDEFINITIONTYPE *defParams);
    OMX_ERRORTYPE getCodecVideoPortFormat(CodecVideoPortFormatParam *formatParams);
    OMX_ERRORTYPE setUseBufferType(const UseBufferType *defParams);
    OMX_ERRORTYPE setProcessName(const ProcessNameParam *nameParams);
    OMX_ERRORTYPE setPortDefinitionParam(const OMX_PARAM_PORTDEFINITIONTYPE *defParams);
    OMX_ERRORTYPE setCodecVideoPortFormat(const CodecVideoPortFormatParam *formatParams);
    void initOmxHeader(OMX_BUFFERHEADERTYPE *header, OMX_U32 portIndex, OMX_PTR appPrivate,
        OMX_U32 size, OMX_U8 *ptr);
    OMX_ERRORTYPE initOutputPrivate(OMX_BUFFERHEADERTYPE *header, OMX_U8 *ptr, BufferPrivateStruct *bufferPrivate);
    void initAllocatedOutputPrivate(BufferCtrlStruct *bufferCtrl, BufferPrivateStruct *bufferPrivate);
    OMX_ERRORTYPE initNativeOutputPrivate(OMX_BUFFERHEADERTYPE *header, OMX_U8 *ptr, BufferCtrlStruct *bufferCtrl);
    OMX_ERRORTYPE initSecureInputPrivate(OMX_BUFFERHEADERTYPE *header, BufferPrivateStruct *bufferPrivate);
    void addBufferToPort(OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header, OMX_U8 *ptr, OMX_U32 size);
    OMX_ERRORTYPE allocateSecureInputBuffer(OMX_BUFFERHEADERTYPE **header, OMX_PTR appPrivate, OMX_U32 size);
    OMX_ERRORTYPE allocateOutputBufferInternal(OMX_BUFFERHEADERTYPE **header, OMX_PTR appPrivate, OMX_U32 size);
    void drainOneOutputBuffer(int32_t picId, const void *pBufferHeader);
    bool DrainAllOutputBuffers();
    bool prepareDrainOutput(BufferInfo *&outInfo, OMX_BUFFERHEADERTYPE *&outHeader);
    void submitDrainOutput(std::list<BufferInfo*>& outQueue, BufferInfo *outInfo, OMX_BUFFERHEADERTYPE *outHeader);
    bool HandleCropRectEvent(const CropParams* crop);
    bool HandleBitdepthChangeEvent(const unsigned char *high10En);
    bool ShouldDisableAfbc(const H265SwDecInfo *info);
    void DrainPendingDecoderFrames();
    void UpdateDecoderPictureState(const H265SwDecInfo *info);
    void UpdateOutputBufferCount(OMX_PARAM_PORTDEFINITIONTYPE *def, const H265SwDecInfo *info,
                                 OMX_BOOL useNativeBuffer);
    bool ShouldReconfigurePort(const H265SwDecInfo *info, bool cropChanged, bool bitdepChanged,
        bool needFlushBuffer) const;
    bool DisableAfbcIfNeeded(const H265SwDecInfo *info);
    bool ApplyPortReconfiguration(const H265SwDecInfo *info, OMX_PARAM_PORTDEFINITIONTYPE *def,
        OMX_BOOL useNativeBuffer, bool needFlushBuffer);
    bool ShouldSkipQueueFill();
    bool shouldContinueDecodeLoop(const std::list<BufferInfo*>& inQueue, const std::list<BufferInfo*>& outQueue);
    bool processQueuedFrames(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue);
    bool acquireAvailableOutputHeader(std::list<BufferInfo*>& outQueue, OMX_BUFFERHEADERTYPE *&outHeader,
        BufferCtrlStruct *&bufferCtrl);
    bool prepareInputHeader(std::list<BufferInfo*>& inQueue, BufferInfo *&inInfo, OMX_BUFFERHEADERTYPE *&inHeader);
    void releaseCurrentInputBuffer(std::list<BufferInfo*>& inQueue, BufferInfo *&inInfo,
                                   OMX_BUFFERHEADERTYPE *&inHeader);
    void initDecoderInputBuffer(MMDecInput &decIn, OMX_BUFFERHEADERTYPE *inHeader);
    void copyInputDataToDecoder(MMDecInput &decIn, OMX_BUFFERHEADERTYPE *inHeader, uint32_t &addStartcodeLen);
    void prepareDecodeInput(OMX_BUFFERHEADERTYPE *inHeader, MMDecInput &decIn, MMDecOutput &decOut,
        uint32_t &addStartcodeLen);
    bool prepareOutputFrameBuffer(OMX_BUFFERHEADERTYPE *outHeader, BufferCtrlStruct *bufferCtrl,
        OMX_BUFFERHEADERTYPE *inHeader, int &fd);
    void SyncOutputBuffer(int fd, unsigned long flags);
    MMDecRet decodeFrame(OMX_BUFFERHEADERTYPE *inHeader, OMX_BUFFERHEADERTYPE *outHeader,
        BufferCtrlStruct *bufferCtrl, MMDecInput &decIn, MMDecOutput &decOut);
    bool HandleDecodeResult(MMDecRet decRet);
    bool ProcessDecoderInfo();
    bool processOneQueuedFrame(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue, bool &shouldBreak);
    bool FinalizeInputFrame(MMDecInput &decIn, std::list<BufferInfo*>& inQueue, BufferInfo *&inInfo,
        OMX_BUFFERHEADERTYPE *&inHeader);
    void drainDecodedFrames(std::list<BufferInfo*>& outQueue, MMDecOutput &decOut);
    bool HandlePortSettingChangeEvent(const H265SwDecInfo *info);
    static int32_t CtuInfoMemAllocWrapper(void *aUserData, unsigned int sizeMbinfo, unsigned long *pPhyAddr);
    static int32_t ExtMemAllocWrapper(void *aUserData, unsigned int sizeExtra);
    static int32_t BindFrameWrapper(void *aUserData, void *pHeader);
    static int32_t UnbindFrameWrapper(void *aUserData, void *pHeader);
    int VspMallocCtuinfoCb(unsigned int sizeMbinfo, unsigned long *pPhyAddr);
    int AllocateSoftwareExtraBuffer(unsigned int sizeExtra, MMCodecBuffer extraMem[]);
    int AllocateHardwareExtraBuffer(unsigned int sizeExtra, MMCodecBuffer extraMem[]);
    int VspMallocCb(unsigned int sizeExtra);
    int VspBindCb(void *pHeader);
    int VspUnbindCb(void *pHeader);
    bool OpenDecoder(const char* libName);
    bool outputBuffersNotEnough(const H265SwDecInfo *, OMX_U32, OMX_U32, OMX_BOOL);
    OMX_ERRORTYPE allocateStreamBuffer(OMX_U32 bufferSize);
    void ReleaseStreamBuffer();
    void releasePlatformBuffer(OMX_BUFFERHEADERTYPE *header, BufferCtrlStruct *bufferCtrl) const;
    void freePlatform(OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header);
    SPRDHEVCDecoder(const SPRDHEVCDecoder &);
    SPRDHEVCDecoder &operator=(const SPRDHEVCDecoder &);
};
}
}
#endif  // SPRD_HEVC_DECODER_H_
