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
#ifndef SPRD_SIMPLE_OMX_COMPONENT_H_
#define SPRD_SIMPLE_OMX_COMPONENT_H_
#include "SprdOMXComponent.h"
#include <atomic>
#include <list>
#include <vector>
#include "sprd_omx_typedef.h"
#include "Handler.h"
#include "Message.h"
#include "SprdOMXUtils.h"
namespace OHOS {
namespace OMX {
#ifdef CONFIG_POWER_HINT
struct VideoPowerHintInstance;
#endif
struct CodecProfileLevel {
    OMX_U32 mProfile;
    OMX_U32 mLevel;
};
struct OmxYcbcr {
    void *y;
    void *cb;
    void *cr;
    size_t ystride;
    size_t cstride;
    size_t chromaStep;
    /** reserved for future use, set to 0 by gralloc's (*lock_ycbcr)() */
    uint32_t reserved[8];
};
struct SprdSimpleOMXComponent : public SprdOMXComponent {
    SprdSimpleOMXComponent(
        const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component);
    struct BufferInfo {
        OMX_BUFFERHEADERTYPE *mHeader;
        bool mOwnedByUs;
        bool mFrameConfig;
        uint32_t deintlWidth;
        uint32_t deintlHeight;
        int32_t picId;
        int32_t mNodeId;
    };
    struct PortInfo {
        OMX_PARAM_PORTDEFINITIONTYPE mDef;
        std::vector<BufferInfo> mBuffers;
        std::list<BufferInfo *> mQueue;
        enum {
            NONE,
            DISABLING,
            ENABLING,
        } mTransition;
    };
    enum {
        OMX_INDEX_STORE_META_DATA_IN_BUFFERS = OMX_IndexVendorStartUnused + 1,
        OMX_INDEX_STORE_ANW_BUFFER_IN_METADATA,
        OMX_INDEX_PREPARE_FOR_ADAPTIVE_PLAYBACK,
        OMX_INDEX_CONFIGURE_VIDEO_TUNNEL_MODE,
        OMX_INDEX_USE_ANB,
        OMX_INDEX_USE_NATIVE_BUFFER,
        OMX_INDEX_ENABLE_ANB,
        OMX_INDEX_ALLOCATE_NATIVE_HANDLE,
        OMX_INDEX_GET_ANB_USAGE,
        OMX_INDEX_CONFIG_THUMBNAIL_MODE,
        OMX_INDEX_CONFIG_VIDEO_RECORD_MODE,
        OMX_INDEX_CONFIG_ENC_SCENE_MODE,
        OMX_INDEX_CONFIG_DEC_SCENE_MODE,
        OMX_INDEX_DESCRIBE_COLOR_ASPECTS,
        OMX_INDEX_DESCRIBE_COLOR_FORMAT,
        OMX_INDEX_PREPEND_SPSPPS_TO_IDR,
        OMX_INDEX_DESCRIBE_HDR_STATIC_INFO,
        OMX_INDEX_DESCRIBE_HDR10_PLUS_INFO,
    };
    std::mutex mThreadLock;    //Added for deintl thread
    Vector<BufferInfo*> mBufferNodes;
    std::list<BufferInfo *> mDecOutputBufQueue;
    std::list<BufferInfo *> mDeinterInputBufQueue;
    bool mDumpYUVEnabled;
    bool mDumpStrmEnabled;
    mutable FILE* mFileYUV;
    mutable FILE* mFileStream;
    bool mIsForWFD;
#ifdef CONFIG_POWER_HINT
    static bool mIsPowerHintRunning;
    VideoPowerHintInstance* mVideoPowerHint;
#endif
    PortInfo *editPortInfo(OMX_U32 pOrtIndex);
    std::list<BufferInfo*>& getPortQueue(OMX_U32 pOrtIndex);
    virtual void drainOneOutputBuffer(OMX_S32 picId, const void *pBufferHeader, OMX_U64 pTs);
    void PrepareForDestruction() override;
protected:
    struct FlexYuvPlanarParams {
        uint8_t *dst;
        size_t dstStride;
        size_t dstVStride;
        struct OmxYcbcr *ycbcr;
        int32_t width;
        int32_t height;
    };

    enum {
        K_WHAT_SEND_COMMAND,
        K_WHAT_EMPTY_THIS_BUFFER,
        K_WHAT_FILL_THIS_BUFFER,
    };
    typedef enum OMX_DUMP_TYPE {
        DUMP_YUV,
        DUMP_STREAM,
    } OmxDumptype;
    std::mutex mLock;
    void addPort(const OMX_PARAM_PORTDEFINITIONTYPE &def);
    void ConvertFlexYUVToPlanar(const FlexYuvPlanarParams &params);
    ~SprdSimpleOMXComponent() override;
    virtual OMX_ERRORTYPE internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR pArams);
    virtual OMX_ERRORTYPE internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR pArams);
    virtual OMX_ERRORTYPE internalSetConfig(
        OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig);
    virtual OMX_ERRORTYPE useBuffer(
        OMX_BUFFERHEADERTYPE **buffer, const UseBufferParams &params) override;
    virtual OMX_ERRORTYPE internalUseBuffer(
        OMX_BUFFERHEADERTYPE **buffer, const UseBufferParams &params);
    virtual OMX_ERRORTYPE allocateBuffer(
        OMX_BUFFERHEADERTYPE **buffer,
            OMX_U32 portIndex,
            OMX_PTR appPrivate,
            OMX_U32 size) override;
    virtual OMX_ERRORTYPE freeBuffer(
        OMX_U32 portIndex,
            OMX_BUFFERHEADERTYPE *buffer) override;
    void CheckTransitions();
    void InitDumpFiles(const char *name, const char *strmExtension);
    void DumpFiles(uint8_t *pBuffer, int32_t aInBufSize, OMX_DUMP_TYPE type);
    void CloseDumpFiles() const;
    virtual OMX_ERRORTYPE CheckParam(OMX_PTR header, OMX_U32 size);
#ifdef CONFIG_POWER_HINT
    void acquirePowerHint();
    void releasePowerHint();
#endif
    virtual void onQueueFilled(OMX_U32 pOrtIndex);
    virtual void onPortFlushCompleted(OMX_U32 pOrtIndex);
    virtual void onPortEnableCompleted(OMX_U32 portIndex, bool enabled);
    virtual void onPortFlushPrepare(OMX_U32 pOrtIndex);
    virtual void OnReset();
    virtual void onDecodePrepare(OMX_BUFFERHEADERTYPE *header);
    virtual void onBufferConfig(OMX_BUFFERHEADERTYPE *header);
    void onSendCommand(OMX_COMMANDTYPE cmd, OMX_U32 pAram);
    void onEmptyThisBuffer(OMX_BUFFERHEADERTYPE *header);
    void onFillThisBuffer(OMX_BUFFERHEADERTYPE *header);
    bool GetDumpValue(const char *propertyStr) const;
    void releaseMemIon(PMemIon* pPMemIon);
    Vector<PortInfo> mPorts;
private:
    class MessageHandler : public Handler {
    public:
        explicit MessageHandler(SprdSimpleOMXComponent* component);
        void onMessageReceived(const Message *msg);
    private:
        SprdSimpleOMXComponent* mComponent;
    };
    OMX_STATETYPE mState;
    OMX_STATETYPE mTargetState;
    std::atomic_bool mFrameConfig;
    mutable MessageHandler mMsgHandler;
    bool isSetParameterAllowed(
        OMX_INDEXTYPE index, const OMX_PTR params) const;
    bool IsLoadedToIdleTransitionComplete(bool *transitionComplete);
    bool IsIdleToLoadedTransitionComplete(bool *transitionComplete);
    void CompleteTransitionIfNeeded(bool transitionComplete);
    void HandlePortTransitions() const;
    bool ReturnBufferIfOwned(PortInfo *port, BufferInfo *buffer);
    void normalizeCanceledLoadedToIdle(OMX_STATETYPE *state);
    bool validateStateTransitionRequest(OMX_STATETYPE state);
    bool handleLoadedStateChange(OMX_STATETYPE state);
    bool handleIdleStateChange(OMX_STATETYPE state);
    bool handleExecutingStateChange(OMX_STATETYPE state);
    bool disablePortBuffers(PortInfo *port, OMX_U32 portIndex);
    bool validatePortEnableRequest(OMX_U32 portIndex, bool enable, PortInfo **port);
    bool releasePlatformBuffer(OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header);
    void releaseHeaderPrivates(OMX_BUFFERHEADERTYPE *header);
    bool findMessageHeader(const Message *msg, OMX_BUFFERHEADERTYPE **header, const char *key);
    bool handleSendCommandMessage(const Message *msg);
    virtual OMX_ERRORTYPE sendCommand(
        OMX_COMMANDTYPE cmd, OMX_U32 param, OMX_PTR data) override;
    virtual OMX_ERRORTYPE getParameter(
        OMX_INDEXTYPE index, OMX_PTR params) override;
    virtual OMX_ERRORTYPE setParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) override;
    virtual OMX_ERRORTYPE setConfig(
        OMX_INDEXTYPE index, const OMX_PTR params) override;
    virtual OMX_ERRORTYPE emptyThisBuffer(
        OMX_BUFFERHEADERTYPE *buffer) override;
    virtual OMX_ERRORTYPE fillThisBuffer(
        OMX_BUFFERHEADERTYPE *buffer) override;
    virtual OMX_ERRORTYPE getState(OMX_STATETYPE *state) override;
    void InitLooper(const char *name) const;
    void DeinitLooper();
    void onMessageReceived(const Message *msg);
    void onChangeState(OMX_STATETYPE state);
    void onPortEnable(OMX_U32 portIndex, bool enable);
    void onPortFlush(OMX_U32 portIndex, bool sendFlushComplete);
    SprdSimpleOMXComponent(const SprdSimpleOMXComponent &);
    SprdSimpleOMXComponent &operator=(const SprdSimpleOMXComponent &);
};
}  // namespace OMX
}  // namespace OHOS
#endif  // SPRD_SIMPLE_OMX_COMPONENT_H_
