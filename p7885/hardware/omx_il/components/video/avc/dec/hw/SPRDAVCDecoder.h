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
#ifndef SPRD_AVC_DECODER_H_
#define SPRD_AVC_DECODER_H_
#include <dlfcn.h>
#include "SprdVideoDecoderBase.h"
#include "avc_dec_api.h"
#include "SPRDDeinterlace.h"
#include "sprd_omx_typedef.h"
#define H264_DECODER_INTERNAL_BUFFER_SIZE (0x100000)
#define H264_DECODER_STREAM_BUFFER_SIZE (1024*1024*2)
#define H264_HEADER_SIZE (1024)
/* Maximum decoder instances */
#define MAX_INSTANCES 10
#define MAX_SECURE_INSTANCES 1
/* Alignment macros for memory and buffers */
#define MEMORY_ALIGNMENT_64 64
#define MEMORY_ALIGNMENT_16 16
#define MEMORY_ALIGNMENT_128 128
#define MEMORY_ALIGNMENT_32 32
#define KBYTE_ALIGNMENT_1024 1024
#define KBYTE_ALIGNMENT_4 4096
/* Buffer size and count macros */
#define DEFAULT_INPUT_BUFFER_SIZE (1920 * 1088 * 3 / 2 / 2)
#define DEFAULT_NUM_INPUT_BUFFERS 2
#define DEFAULT_NUM_OUTPUT_BUFFERS 2
#define DEFAULT_FRAME_WIDTH 320
#define DEFAULT_FRAME_HEIGHT 240
#define YUV420_FRAME_SIZE_MULTIPLIER 3
#define YUV420_FRAME_SIZE_DIVISOR 2
#define MAX_MBINFO_BUFFERS 24
#define ADDITIONAL_OUTPUT_BUFFERS 4
#define MIN_OUTPUT_BUFFER_COUNT 1
#define ADDITIONAL_NATIVE_BUFFER 1
#define ADDITIONAL_OUTPUT_BUFFER_FOR_TIMEOUT 1
#define ADDITIONAL_DEC_BUFFERS 3
#define MIN_DEINTERLACE_BUFFERS 1
/* H.264 specific macros */
#define H264_START_CODE_SIZE 4
#define H264_START_CODE_BYTE_0 0x00
#define H264_START_CODE_BYTE_1 0x00
#define H264_START_CODE_BYTE_2 0x00
#define H264_START_CODE_BYTE_3 0x01
#define H264_START_CODE_MIN_INDEX 2
#define H264_NAL_TYPE_SPS 0x07
#define H264_NAL_TYPE_PPS 0x08
#define H264_PROFILE_BASELINE 0
#define H264_PROFILE_MAIN 1
#define H264_PROFILE_HIGH 2
/* Time and delay macros */
#define DECODER_INIT_RETRY_DELAY_US 100000
#define DECODER_INIT_MAX_RETRIES 3
#define USEC_PER_MSEC 1000
/* Error and status codes */
#define ERROR_CODE_INVALID (-1)
#define ERROR_CODE_SUCCESS 0
#define STATUS_CODE_OK 0
#define STATUS_CODE_ERROR (-1)
/* Flag and mask macros */
#define BUFFER_FLAG_EOS_MASK 0x01
#define START_CODE_PREFIX 0x00000001
#define DEFAULT_BLOCK_SIZE_MASK 31
/* Memory type indices */
#define MEM_TYPE_SW_CACHABLE 0
#define MEM_TYPE_HW_NO_CACHE 1
/* Video resolution thresholds */
#define UHD_WIDTH_THRESHOLD 3840
#define UHD_HEIGHT_THRESHOLD 2160
/* Decoder scene mode values */
#define DEC_SCENE_MODE_VOLTE 3
/* Interlaced sequence detection return values */
#define INTERLACED_SEQUENCE_FLAG 2
/* Color aspect preference */
#define COLOR_ASPECT_PREFER_BITSTREAM 1
/* Deinterlace thread control */
#define DEINTL_THREAD_NOT_DONE 0
/* Array size for extra memory */
#define MAX_MEM_TYPES 2
#define ARRAY_INDEX_0 0
#define ARRAY_INDEX_1 1
#define ARRAY_INDEX_2 2
#define ARRAY_INDEX_3 3
#define ARRAY_INDEX_4 4
namespace OHOS {
namespace OMX {
struct TagAvcHandle;
struct SPRDAVCDecoder : public SprdVideoDecoderBase {
    SPRDAVCDecoder(const char *name,
                   const OMX_CALLBACKTYPE *callbacks,
                   OMX_PTR appData,
                   OMX_COMPONENTTYPE **component);
    OMX_ERRORTYPE initCheck() const;
protected:
    struct FinalizeInputContext {
        MMDecRet decRet;
        MMDecRet infoRet;
        MMDecInput *decIn;
        uint32_t addStartcodeLen;
        std::list<BufferInfo*> *inQueue;
        BufferInfo *inInfo;
        OMX_BUFFERHEADERTYPE *inHeader;
    };
    FT_H264DecInit mH264DecInit = nullptr;
    FT_H264DecGetInfo mH264DecGetInfo = nullptr;
    FT_H264DecDecode mH264DecDecode = nullptr;
    FT_H264DecRelease mH264DecRelease = nullptr;
    FtH264DecSetCurRecPic  mH264DecSetCurRecPic = nullptr;
    FtH264DecGetLastDspFrm  mH264DecGetLastDspFrm = nullptr;
    FtH264DecReleaseRefBuffers  mH264DecReleaseRefBuffers = nullptr;
    FT_H264DecMemInit mH264DecMemInit = nullptr;
    FT_H264GetCodecCapability mH264GetCodecCapability = nullptr;
    FT_H264DecGetNALType mH264DecGetNALType = nullptr;
    FT_H264DecSetparam mH264DecSetparam = nullptr;
    FtH264DecGetIova mH264DecGetIOVA = nullptr;
    FtH264DecFreeIova mH264DecFreeIOVA = nullptr;
    FtH264DecGetIommuStatus mH264DecGetIOMMUStatus = nullptr;
    FtH264DecInitStructForNewSeq mH264DecInitStructForNewSeq = nullptr;
    FT_H264DecNeedToSwitchToSwDecoder mH264DecNeedToSwitchToSwDecoder = nullptr;
    virtual ~SPRDAVCDecoder();
    virtual void freePlatform(OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header)
    {
        (void)portIndex;
        (void)header;
    }
    virtual OMX_ERRORTYPE internalGetParameter(OMX_INDEXTYPE index, OMX_PTR params);
    virtual OMX_ERRORTYPE internalSetParameter(OMX_INDEXTYPE index, const OMX_PTR params);
    virtual OMX_ERRORTYPE internalUseBuffer(
        OMX_BUFFERHEADERTYPE **buffer, const UseBufferParams &params);
    virtual OMX_ERRORTYPE allocateBuffer(
        OMX_BUFFERHEADERTYPE **header,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size);
    virtual OMX_ERRORTYPE freeBuffer(
        OMX_U32 portIndex,
        OMX_BUFFERHEADERTYPE *header);
    virtual OMX_ERRORTYPE getConfig(OMX_INDEXTYPE index, OMX_PTR params);
    virtual OMX_ERRORTYPE internalSetConfig(
    OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig);
    virtual void onQueueFilled(OMX_U32 portIndex);
    virtual void onPortFlushCompleted(OMX_U32 portIndex);
    virtual void onPortFlushPrepare(OMX_U32 portIndex);
    virtual OMX_ERRORTYPE getExtensionIndex(const char *name, OMX_INDEXTYPE *index);
    virtual void OnReset();
    virtual void onDecodePrepare(OMX_BUFFERHEADERTYPE *header);
    virtual int GetColorAspectPreference();
    virtual bool GetVuiParams(H264SwDecInfo *mDecoderInfo) = 0;
    virtual bool DrainAllOutputBuffers();
    virtual void drainOneOutputBuffer(int32_t picId, const void *pBufferHeader, OMX_U64 pts);
    enum {
        K_NUM_INPUT_BUFFERS  = 8,
        K_NUM_OUTPUT_BUFFERS = 8,
    };
    TagAvcHandle *mHandle = nullptr;
    int32_t picId = 0;
    uint32_t mFrameWidth = DEFAULT_FRAME_WIDTH;
    uint32_t mFrameHeight = DEFAULT_FRAME_HEIGHT;
    uint32_t mStride = DEFAULT_FRAME_WIDTH;
    uint32_t mSliceHeight = DEFAULT_FRAME_HEIGHT;
    uint32_t mPictureSize = DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT *
        YUV420_FRAME_SIZE_MULTIPLIER / YUV420_FRAME_SIZE_DIVISOR;
    uint32_t mCropWidth = DEFAULT_FRAME_WIDTH;
    uint32_t mCropHeight = DEFAULT_FRAME_HEIGHT;
    uint8_t mhigh10En = 0;
    std::mutex mLock;
    uint8_t *mCodecInterBuffer = nullptr;
    uint8_t *mCodecExtraBuffer = nullptr;
    PMemIon mPmemMbInfo[24] = { nullptr};
    uint8_t *mPmemMbInfoV[24] = { nullptr};
    unsigned long  mPmemMbInfoP[24] = { 0 };
    size_t  mPbufMbInfoSize[24] = { 0 };
    int mPbufMbInfoIdx = 0;
    bool mDecoderSawSPS = false;
    bool mDecoderSawPPS = false;
    uint8_t *mSPSData = nullptr;
    uint32_t mSPSDataSize = 0;
    uint8_t *mPPSData = nullptr;
    uint32_t mPPSDataSize = 0;
    bool mIsResume = false;
    SPRDDeinterlace* mDeintl = nullptr;
    bool mDeintInitialized = false;
    bool mNeedDeinterlace = false;
    H264SwDecInfo mDecoderInfo;
    bool mFrameDecoded = false;
    bool mIsInterlacedSequence = false;
    MMDecCapability mCapability;
    OMX_ERRORTYPE mInitCheck;
    status_t initDecoder();
    void InitDecoderHandle();
    void InitDecoderInitParams(MMCodecBuffer &camelBack, MMDecVideoFormat &videoFormat,
        MMDecCodecPriority &codecPriority) const;
    int TryInitDecoderInstance(MMCodecBuffer &camelBack, MMDecVideoFormat &videoFormat,
        MMDecCodecPriority &codecPriority) const;
    OMX_ERRORTYPE initDecoderInstance();
    void UpdateIommuStatus() const;
    OMX_ERRORTYPE adjustProfileLevelQuery(OMX_PTR params);
    void ReleaseDecoder(bool isNeedRelease = true);
    OMX_ERRORTYPE updatePortDefinition(OMX_PARAM_PORTDEFINITIONTYPE *defParams, PortInfo *port);
    void applyPortGeometryChange(OMX_U32 portIndex, uint32_t newWidth, uint32_t newHeight);
    void EnsureStreamBufferCapacity();
    void initOmxHeader(OMX_BUFFERHEADERTYPE *header, OMX_U32 portIndex, OMX_PTR appPrivate,
        OMX_U32 size, OMX_U8 *ptr);
    OMX_ERRORTYPE initOutputPrivate(OMX_BUFFERHEADERTYPE *header, BufferPrivateStruct *bufferPrivate);
    OMX_ERRORTYPE initSecureInputPrivate(OMX_BUFFERHEADERTYPE *header, BufferPrivateStruct *bufferPrivate);
    void addBufferToPort(OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header, OMX_U8 *ptr, OMX_U32 size);
    OMX_ERRORTYPE allocateSecureInputBuffer(OMX_BUFFERHEADERTYPE **header, OMX_PTR appPrivate, OMX_U32 size);
    OMX_ERRORTYPE allocateOutputMemory(size_t alignedSize, MemIon *&memory, int &fd,
        unsigned long &phyAddr, size_t &bufferSize);
    void initBufferPrivate(BufferPrivateStruct *bufferPrivate, MemIon *memory,
        unsigned long phyAddr, size_t bufferSize, int fd) const;
    OMX_ERRORTYPE allocateOutputBufferInternal(OMX_BUFFERHEADERTYPE **header, OMX_PTR appPrivate, OMX_U32 size);
    OMX_ERRORTYPE freeSecureInputBufferInternal(OMX_BUFFERHEADERTYPE *header);
    OMX_ERRORTYPE freeOutputBufferInternal(OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header);
    void releaseOutputBufferResources(BufferCtrlStruct *bufferCtrl, OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header);
    bool ShouldDisableAfbc(const H264SwDecInfo *info);
    void UpdateSwDecoderSuggestion();
    bool NeedsPortReconfigure(const H264SwDecInfo *info, bool cropChanged, bool bitdepChanged, bool needFlushBuffer);
    bool handlePortReconfiguration(const H264SwDecInfo *info, OMX_PARAM_PORTDEFINITIONTYPE *def,
        OMX_BOOL useNativeBuffer, bool needFlushBuffer);
    void DrainPendingDecoderFrames();
    void UpdateDecoderPictureState(const H264SwDecInfo *info);
    void UpdateOutputBufferCount(OMX_PARAM_PORTDEFINITIONTYPE *def, const H264SwDecInfo *info,
                                 OMX_BOOL useNativeBuffer);
    bool HandleSwDecoderSwitch();
    bool ShouldSkipQueueFill(OMX_U32 portIndex);
    bool shouldContinueDecodeLoop(const std::list<BufferInfo*>& inQueue, const std::list<BufferInfo*>& outQueue);
    bool processQueuedFrames(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue);
    void RestartDeinterlaceIfNeeded();
    bool acquireAvailableOutputHeader(std::list<BufferInfo*>& outQueue, OMX_BUFFERHEADERTYPE *&outHeader,
        BufferCtrlStruct *&bufferCtrl);
    bool prepareInputHeader(std::list<BufferInfo*>& inQueue, BufferInfo *&inInfo, OMX_BUFFERHEADERTYPE *&inHeader);
    void releaseCurrentInputBuffer(std::list<BufferInfo*>& inQueue, BufferInfo *&inInfo,
                                   OMX_BUFFERHEADERTYPE *&inHeader);
    void selectInputBitstreamSource(OMX_BUFFERHEADERTYPE *inHeader, uint8_t *&bitstream, uint32_t &bufferSize);
    void initDecoderInputBuffer(MMDecInput &decIn, OMX_BUFFERHEADERTYPE *inHeader);
    void copyInputDataToDecoder(MMDecInput &decIn, OMX_BUFFERHEADERTYPE *inHeader, uint8_t *bitstream,
        uint32_t bufferSize, uint32_t &addStartcodeLen);
    void prepareDecodeInput(OMX_BUFFERHEADERTYPE *inHeader, MMDecInput &decIn, MMDecOutput &decOut,
        uint32_t &addStartcodeLen);
    bool ConfigureDecoderInstance(const char *name);
    void InitializeDecoderPorts() const;
    bool TryInitSoftwareDecoder();
    void InitializeCodecConfigBuffers();
    void ResetMbInfoBuffers();
    bool prepareOutputFrameBuffer(OMX_BUFFERHEADERTYPE *outHeader, BufferCtrlStruct *bufferCtrl,
        OMX_BUFFERHEADERTYPE *inHeader);
    MMDecRet decodeFrame(OMX_BUFFERHEADERTYPE *inHeader, OMX_BUFFERHEADERTYPE *outHeader,
        BufferCtrlStruct *bufferCtrl, MMDecInput &decIn, MMDecOutput &decOut);
    int HandleDecodeResult(MMDecRet decRet, std::list<BufferInfo*>& inQueue, BufferInfo *&inInfo,
        OMX_BUFFERHEADERTYPE *&inHeader);
    bool ProcessDecoderInfo(std::list<BufferInfo*>& outQueue);
    bool ValidateDecoderDimensions();
    void initDeinterlaceIfNeeded(std::list<BufferInfo*>& outQueue);
    bool processOneQueuedFrame(std::list<BufferInfo*>& inQueue, std::list<BufferInfo*>& outQueue, bool &shouldBreak);
    void FinalizeInputFrame(FinalizeInputContext &context);
    void drainDecodedFrames(std::list<BufferInfo*>& outQueue, MMDecOutput &decOut);
    std::list<SprdSimpleOMXComponent::BufferInfo*> *selectDrainQueue(
        std::list<SprdSimpleOMXComponent::BufferInfo*>& outQueue);
    bool prepareDrainOutputHeader(std::list<SprdSimpleOMXComponent::BufferInfo*>& drainQueue,
        SprdSimpleOMXComponent::BufferInfo *&outInfo, OMX_BUFFERHEADERTYPE *&outHeader,
        std::list<SprdSimpleOMXComponent::BufferInfo*>::iterator &it);
    void submitDrainOutput(std::list<SprdSimpleOMXComponent::BufferInfo*>& outQueue,
        std::list<SprdSimpleOMXComponent::BufferInfo*>::iterator it,
        SprdSimpleOMXComponent::BufferInfo *outInfo, OMX_BUFFERHEADERTYPE *outHeader);
    void UpdatePortDefinitions(bool updateCrop = true, bool updateInputSize = false);
    bool HandleCropRectEvent(const CropParams *crop);
    bool HandleBitdepthChangeEvent(const unsigned char *high10En);
    bool HandlePortSettingChangeEvent(const H264SwDecInfo *info);
    static int32_t MbinfoMemAllocWrapper(void *aUserData, unsigned int sizeMbinfo, unsigned long *pPhyAddr);
    static int32_t ExtMemAllocWrapper(void *aUserData, unsigned int sizeExtra);
    static int32_t BindFrameWrapper(void *aUserData, void *pHeader);
    static int32_t UnbindFrameWrapper(void *aUserData, void *pHeader);
    static void  DrainOneOutputBuffercallback(void *decoder,
                                              int32_t picId, void *pBufferHeader, unsigned long long pts);
    int VspMallocMbinfoCb(unsigned int sizeMbinfo, unsigned long *pPhyAddr);
    int VspMallocCb(unsigned int sizeExtra);
    int VspBindCb(void *pHeader);
    int VspUnbindCb(void *pHeader);
    bool LoadRequiredDecoderSymbols(const char *libName) const;
    bool LoadSwSwitchSymbolIfNeeded(const char *libName);
    bool OpenDecoder(const char *libName);
    void cacheSpsData(const uint8 *data, uint32_t size);
    void cachePpsData(const uint8 *data, uint32_t size);
    void UpdateInterlaceSequenceState(int parseRet);
    void HandleNewSequence();
    void findCodecConfigData(OMX_BUFFERHEADERTYPE *header);
    void FreeOutputBufferIova();
    bool outputBuffersNotEnough(const H264SwDecInfo *, OMX_U32, OMX_U32, OMX_BOOL);
    void initDecOutputHeader(OMX_BUFFERHEADERTYPE *header);
    OMX_ERRORTYPE allocateOneDecOutputBuffer(H264SwDecInfo *decoderInfo, OMX_U32 nodeId);
    OMX_ERRORTYPE allocateDecOutputBuffer(H264SwDecInfo* pDecoderInfo);
    OMX_ERRORTYPE releaseDecOutputNode(BufferInfo *bufferNode);
    OMX_ERRORTYPE freeDecOutputBuffer();
    bool DrainAllDecOutputBufferforDeintl();
    void DrainOneDeintlBuffer(void *pBufferHeader, uint64 pts);
    int AllocateSoftwareExtraBuffer(unsigned int sizeExtra, MMCodecBuffer extraMem[]);
    int AllocateHardwareExtraBuffer(unsigned int sizeExtra, MMCodecBuffer extraMem[]);
    void SignalDeintlThread();
    void FlushDeintlBuffer();
    OMX_ERRORTYPE allocateStreamBuffer(OMX_U32 bufferSize);
    void ReleaseStreamBuffer();
    bool firstTime = true;
    bool mDeintlNeedRestart = false;
    SPRDAVCDecoder(const SPRDAVCDecoder &);
    SPRDAVCDecoder &operator=(const SPRDAVCDecoder &);
    OMX_BOOL iUseOhosNativeBuffer[2];
    int decTime = 0;
};

inline void SPRDAVCDecoder::UpdateIommuStatus() const
{
    int ret = (*mH264DecGetIOMMUStatus)(mHandle);
    OMX_LOGI("Get IOMMU Status: %d", ret);
    mIOMMUEnabled = (ret >= STATUS_CODE_OK) && !mSecureFlag;
}

inline void SPRDAVCDecoder::RestartDeinterlaceIfNeeded()
{
    if (!mDeintl || !mDeintlNeedRestart) {
        return;
    }
    mDeintl->StopDeinterlaceThread();
    mDeintl->mDone = DEINTL_THREAD_NOT_DONE;
    mDeintl->StartDeinterlaceThread();
    mDeintlNeedRestart = false;
}

inline bool SPRDAVCDecoder::ShouldSkipQueueFill(OMX_U32 portIndex)
{
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return true;
    }
    if (mDeintl && portIndex == K_OUTPUT_PORT_INDEX) {
        SignalDeintlThread();
    }
    return mEOSStatus == OUTPUT_FRAMES_FLUSHED || !HandleSwDecoderSwitch();
}

inline void SPRDAVCDecoder::cacheSpsData(const uint8 *data, uint32_t size)
{
    mSPSDataSize = size;
    errno_t ret = memmove_s(mSPSData, H264_HEADER_SIZE, data, mSPSDataSize);
    if (ret != 0) {
        OMX_LOGE("memmove_s failed in line %d, ret=%d", __LINE__, ret);
    }
}

inline void SPRDAVCDecoder::cachePpsData(const uint8 *data, uint32_t size)
{
    mPPSDataSize = size;
    errno_t ret = memmove_s(mPPSData, H264_HEADER_SIZE, data, mPPSDataSize);
    if (ret != 0) {
        OMX_LOGE("memmove_s failed in line %d, ret=%d", __LINE__, ret);
    }
}

inline void SPRDAVCDecoder::UpdateInterlaceSequenceState(int parseRet)
{
    if (mCapability.support1080i || parseRet == 0 || mDecoderSwFlag) {
        return;
    }
    mChangeToSwDec = true;
    mIsInterlacedSequence = true;
    mNeedDeinterlace = parseRet == INTERLACED_SEQUENCE_FLAG;
}

inline void SPRDAVCDecoder::HandleNewSequence()
{
    if ((*mH264DecInitStructForNewSeq)(mHandle) != MMDEC_OK) {
        OMX_LOGE("Failed to InitStructForNewSeq");
    }
}
}  // namespace OMX
}  // namespace OHOS
#endif  // SPRD_AVC_DECODER_H_
