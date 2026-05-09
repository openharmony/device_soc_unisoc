/*
 * Copyright 2023 Shenzhen Kaihong DID Co., Ltd.
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
#ifndef SPRD_AVC_DECODER_OHOS_H
#define SPRD_AVC_DECODER_OHOS_H
#include "SPRDAVCDecoder.h"
#include "avc_dec_api.h"
//ohos ext
#include "codec_omx_ext.h"
#include "display_type.h"
#include "buffer_handle.h"
#include "sprd_omx_typedef.h"
namespace OHOS {
namespace OMX {
struct SPRDAVCDecoderOhos : public SPRDAVCDecoder {
    SPRDAVCDecoderOhos(
    const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component);
protected:
//OHOS overwrite
    virtual OMX_ERRORTYPE internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) override;
    virtual OMX_ERRORTYPE internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) override;
    bool GetVuiParams(H264SwDecInfo *mDecoderInfo) override;
    virtual OMX_ERRORTYPE internalUseBuffer(
        OMX_BUFFERHEADERTYPE **buffer, const UseBufferParams &params) override;
    virtual OMX_ERRORTYPE internalSetConfig(
    OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig) override;
    virtual OMX_ERRORTYPE getConfig(OMX_INDEXTYPE index, OMX_PTR params) override;
    bool SupportsDescribeColorAspects() override;
    bool DrainAllOutputBuffers() override;
    void drainOneOutputBuffer(int32_t picId, const void *pBufferHeader, OMX_U64 pts) override;
    void onBufferConfig(OMX_BUFFERHEADERTYPE *header) override;
    void notifyFillBufferDone(OMX_BUFFERHEADERTYPE *header) override;
    void freePlatform(OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header) override;
private:
    struct OhosOutputBufferConfig {
        OMX_U32 size;
        OMX_U8 *buffer;
        VideoMemAllocator *memory;
        unsigned long phyAddr;
        size_t bufferSize;
        OMX_U8 *platformPrivate;
    };

    /*ext flag when using ohos native buffer handle*/
    #ifndef OMX_BUFFER_SWAP_FLAG
    #define OMX_BUFFER_SWAP_FLAG 0x00000100
    #endif
    OMX_ERRORTYPE useOhosBufferHandle(
    OMX_BUFFERHEADERTYPE **header,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size,
        OMX_U8 *ptr);
    OMX_U32 normalizeOhosOutputBufferSize(OMX_U32 size) const;
    OMX_ERRORTYPE buildOhosOutputBufferConfig(
        OMX_U32 size, OMX_U8 *ptr, OhosOutputBufferConfig &config, BufferHandle *&bufferHandle) const;
    OMX_ERRORTYPE setOhosNativeBufferEnabled(const CodecEnableNativeBufferParams *formatParams);
    OMX_ERRORTYPE initVideoPort(OMX_PORT_PARAM_TYPE *portParams) const;
    OMX_ERRORTYPE getSupportBufferType(UseBufferType *defParams);
    OMX_ERRORTYPE getBufferHandleUsage(GetBufferHandleUsageParams *defParams) const;
    OMX_ERRORTYPE getBufferSupplier(OMX_PARAM_BUFFERSUPPLIERTYPE *defParams);
    OMX_ERRORTYPE getCodecVideoPortFormat(CodecVideoPortFormatParam *formatParams);
    OMX_ERRORTYPE setUseBufferType(const UseBufferType *defParams);
    OMX_ERRORTYPE setCodecVideoPortFormat(const CodecVideoPortFormatParam *formatParams);
    OMX_ERRORTYPE setVideoPortFormat(const OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams);
    OMX_ERRORTYPE setBufferSupplier(const OMX_PARAM_BUFFERSUPPLIERTYPE *defParams);
    OMX_ERRORTYPE setProcessName(const ProcessNameParam *nameParams) const;
    OMX_ERRORTYPE mapPlatformBuffer(BufferHandle *bufferhandle, OMX_U8 *&pBufferPlatform) const;
    OMX_ERRORTYPE allocateOhosOutputMemory(VideoMemAllocator *&vm, unsigned long &phyAddr,
        size_t &bufferSize, OMX_U32 size) const;
    OMX_ERRORTYPE configureOhosOutputBuffer(
        OMX_BUFFERHEADERTYPE **header, const OhosOutputBufferConfig &config);
    void releasePlatformBuffer(OMX_BUFFERHEADERTYPE *header, BufferCtrlStruct *bufferCtrl) const;
    void dispatchFillBufferDone(OMX_BUFFERHEADERTYPE *header) const;
    bool prepareCopyParams(OMX_BUFFERHEADERTYPE *header, BufferHandle *&bufferhandle,
        uint32_t &copyWidth, uint32_t &copyHeight) const;
    void copyOhosLumaPlane(OMX_U8 *dstInfo, OMX_U8 *buffInfo, const BufferHandle *bufferhandle) const;
    void copyOhosChromaPlane(OMX_U8 *dstInfo, OMX_U8 *buffInfo, const BufferHandle *bufferhandle) const;
    void initOhosHeader(OMX_BUFFERHEADERTYPE *header, const UseBufferParams &params) const;
    void initDefaultOutputBufferCtrl(BufferCtrlStruct *bufferCtrl, BufferPrivateStruct *bufferPrivate) const;
    OMX_ERRORTYPE initOhosOutputPrivate(OMX_BUFFERHEADERTYPE **header, const UseBufferParams &params);
    OMX_ERRORTYPE initOhosInputPrivate(OMX_BUFFERHEADERTYPE *header, BufferPrivateStruct *bufferPrivate);
    void addOhosBufferToPort(const UseBufferParams &params, OMX_BUFFERHEADERTYPE *header);
    OMX_ERRORTYPE internalBufferSwap(OMX_BUFFERHEADERTYPE *header);
    OMX_ERRORTYPE internalBufferSwapReset(OMX_BUFFERHEADERTYPE *header);
    bool validateDrainQueues(std::list<BufferInfo*>& outQueue) const;
    void selectDrainQueue(std::list<BufferInfo*>& outQueue,
        std::list<BufferInfo *>::iterator &it,
        std::list<BufferInfo *>::iterator &itEnd);
    bool findDrainBuffer(std::list<BufferInfo *>::iterator &it,
        std::list<BufferInfo *>::iterator itEnd, void *bufferHeader) const;
    bool findOutputBufferByHeader(std::list<BufferInfo*>& outQueue, const void *bufferHeader,
        std::list<BufferInfo *>::iterator &it) const;
    void prepareDrainHeader(BufferInfo *&outInfo, OMX_BUFFERHEADERTYPE *&outHeader,
        std::list<BufferInfo *>::iterator it, OMX_U64 pts, bool hasFrame);
    void dispatchDrainHeader(std::list<BufferInfo*>& outQueue,
        std::list<BufferInfo *>::iterator it, BufferInfo *outInfo,
        OMX_BUFFERHEADERTYPE *outHeader);
    bool drainOneOutputFrame(std::list<BufferInfo*>& outQueue);
};
}  // namespace OMX
}  // namespace OHOS
#endif
