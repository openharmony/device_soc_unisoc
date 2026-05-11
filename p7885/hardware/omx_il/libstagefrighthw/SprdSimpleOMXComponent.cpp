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
#include "include/SprdSimpleOMXComponent.h"
#include <utils/OMX_looper.h>
#include "utils/omx_log.h"
#include <utils/time_util.h>
#include <cstddef>
#include <cstdint>
#undef LOG_TAG
#define LOG_TAG "SprdSimpleOMXComponent"
/* Buffer alignment size for output port */
#define BUFFER_ALIGNMENT_SIZE 1024
/* Buffer alignment mask */
#define BUFFER_ALIGNMENT_MASK 1023
/* Component name length for MP3 codec */
#define COMPONENT_NAME_MP3_LENGTH 12
/* Default invalid port index */
#define INVALID_PORT_INDEX UINT32_MAX
/* Memory alignment boundary */
#define MEMORY_ALIGNMENT_BOUNDARY 8
/* Standard OMX major version number */
#define OMX_VERSION_MAJOR 1
/* Version step number */
#define VERSION_STEP_NUMBER 0
/* Version revision number */
#define VERSION_REVISION_NUMBER 0
/* Port index offset in parameter structure */
#define PORT_INDEX_OFFSET 8
/* Maximum port index */
#define MAX_PORT_INDEX 1
/* File path buffer length */
#define FILE_PATH_BUFFER_LENGTH 100
/* Reference count initial value */
#define REFERENCE_COUNT_INITIAL 1
/* Reference count zero value */
#define REFERENCE_COUNT_ZERO 0
/* Chroma step value for planar format */
#define CHROMA_STEP_PLANAR 1
/* Width and height division factor for chroma */
#define CHROMA_DIVISION_FACTOR 2
/* Default acquire hint parameters */
#define ACQUIRE_HINT_PARAM1 0
#define ACQUIRE_HINT_PARAM2 0
/* File write unit size */
#define FILE_WRITE_UNIT_SIZE 1
/* File open mode for binary write */
#define FILE_OPEN_MODE_WRITE_BINARY "wb"

namespace {
std::list<OHOS::OMX::SprdSimpleOMXComponent::BufferInfo *> &InvalidPortQueue()
{
    static std::list<OHOS::OMX::SprdSimpleOMXComponent::BufferInfo *> invalidQueue;
    return invalidQueue;
}

OHOS::OMX::SprdSimpleOMXComponent::PortInfo &InvalidPortInfo()
{
    static OHOS::OMX::SprdSimpleOMXComponent::PortInfo invalidPort = {};
    return invalidPort;
}

struct ChromaCopyContext {
    uint8_t *dstU;
    uint8_t *dstV;
    const uint8_t *srcU;
    const uint8_t *srcV;
    size_t chromaHeight;
    size_t chromaWidth;
    size_t dstChromaStride;
    size_t srcChromaStride;
    size_t chromaStep;
};

struct ChromaRowPointers {
    uint8_t *dstU;
    uint8_t *dstV;
    const uint8_t *srcU;
    const uint8_t *srcV;
};

bool CopyPlaneRow(uint8_t *dst, size_t dstPlaneCapacity, const uint8_t *src, size_t rowBytes,
    const char *planeName)
{
    errno_t ret = memmove_s(dst, dstPlaneCapacity, src, rowBytes);
    if (ret != 0) {
        OMX_LOGE("memmove_s failed in ConvertFlexYUVToPlanar (%s plane), error: %d", planeName, ret);
        return false;
    }
    return true;
}

void CopyArbitraryChromaRow(ChromaRowPointers &row, size_t chromaWidth, size_t chromaStep)
{
    for (size_t x = chromaWidth; x > 0; --x) {
        *row.dstU++ = *row.srcU;
        *row.dstV++ = *row.srcV;
        row.srcU += chromaStep;
        row.srcV += chromaStep;
    }
}

bool CopyPlanarChroma(const ChromaCopyContext &ctx)
{
    uint8_t *dstU = ctx.dstU;
    uint8_t *dstV = ctx.dstV;
    const uint8_t *srcU = ctx.srcU;
    const uint8_t *srcV = ctx.srcV;
    const size_t dstPlaneCapacity = ctx.dstChromaStride * ctx.chromaHeight;
    for (size_t y = ctx.chromaHeight; y > 0; --y) {
        if (!CopyPlaneRow(dstU, dstPlaneCapacity, srcU, ctx.chromaWidth, "U")) {
            return false;
        }
        if (!CopyPlaneRow(dstV, dstPlaneCapacity, srcV, ctx.chromaWidth, "V")) {
            return false;
        }
        dstU += ctx.dstChromaStride;
        srcU += ctx.srcChromaStride;
        dstV += ctx.dstChromaStride;
        srcV += ctx.srcChromaStride;
    }
    return true;
}

void CopyArbitraryChroma(const ChromaCopyContext &ctx)
{
    ChromaRowPointers row = {ctx.dstU, ctx.dstV, ctx.srcU, ctx.srcV};
    for (size_t y = ctx.chromaHeight; y > 0; --y) {
        CopyArbitraryChromaRow(row, ctx.chromaWidth, ctx.chromaStep);
        row.dstU += ctx.dstChromaStride - ctx.chromaWidth;
        row.dstV += ctx.dstChromaStride - ctx.chromaWidth;
        row.srcU += ctx.srcChromaStride - ctx.chromaWidth * ctx.chromaStep;
        row.srcV += ctx.srcChromaStride - ctx.chromaWidth * ctx.chromaStep;
    }
}

FILE *SafeCloseDumpFile(FILE *file, const char *tag)
{
    if (file == nullptr) {
        return nullptr;
    }

    // Corrupted file pointers have caused SIGBUS in fclose on some platforms.
    if ((reinterpret_cast<uintptr_t>(file) & (alignof(void *) - 1)) != 0) {
        OMX_LOGE("skip fclose for corrupted %s pointer: %p", tag, file);
        return nullptr;
    }

    int ret = fclose(file);
    if (ret != 0) {
        OMX_LOGW("fclose failed for %s, ret=%d", tag, ret);
    }
    return nullptr;
}
}  // namespace

namespace OHOS {
namespace OMX {
#ifdef CONFIG_POWER_HINT
#include "VideoPowerHint.h"
bool SprdSimpleOMXComponent::mIsPowerHintRunning = false;
#endif
SprdSimpleOMXComponent::MessageHandler::MessageHandler(SprdSimpleOMXComponent * component)
{
    mComponent = component;
}
void SprdSimpleOMXComponent::MessageHandler::onMessageReceived(const Message * msg)
{
    mComponent->onMessageReceived(msg);
}
SprdSimpleOMXComponent::SprdSimpleOMXComponent(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdOMXComponent(name, callbacks, appData, component),
    mState(OMX_StateLoaded),
    mTargetState(OMX_StateLoaded),
#ifdef CONFIG_POWER_HINT
    mVideoPowerHint(nullptr),
#endif
    mFrameConfig(false),
    mDumpYUVEnabled(false),
    mDumpStrmEnabled(false),
    mFileYUV(nullptr),
    mFileStream(nullptr),
    mIsForWFD(false),
    mMsgHandler(this)
{
#ifdef CONFIG_POWER_HINT
    OMX_LOGD(" VideoPowerHintInstance mIsPowerHintRunning %d, name %s", mIsPowerHintRunning, name);
    if (mIsPowerHintRunning && (name != nullptr && strncmp(name, "OMX.sprd.mp3", COMPONENT_NAME_MP3_LENGTH) != 0)) {
        acquirePowerHint();
    }
#endif
    InitLooper(name);
}
void SprdSimpleOMXComponent::PrepareForDestruction()
{
    OMX_LOGD("PrepareForDestruction, %s", Name());
    DeinitLooper();
}
OMX_ERRORTYPE SprdSimpleOMXComponent::sendCommand(OMX_COMMANDTYPE cmd, OMX_U32 param, OMX_PTR data)
{
    OMX_LOGD("cmd: 0x%08x", cmd);
    Message *msg = new Message(K_WHAT_SEND_COMMAND, &mMsgHandler);
    msg->setInt32("cmd", cmd);
    msg->setInt32("param", param);
    msg->post();
    return OMX_ErrorNone;
}
bool SprdSimpleOMXComponent::isSetParameterAllowed(
    OMX_INDEXTYPE index, const OMX_PTR params) const
{
    if (mState == OMX_StateLoaded || mState == OMX_StateExecuting || mState == OMX_StatePause) {
        return true;
    }
    OMX_U32 portIndex;
    switch (index) {
        case OMX_IndexParamPortDefinition: {
            const OMX_PARAM_PORTDEFINITIONTYPE *portDefs =
                    (const OMX_PARAM_PORTDEFINITIONTYPE *) params;
            if (!IsValidOmxParam(portDefs)) {
                return false;
            }
            portIndex = portDefs->nPortIndex;
            break;
        }
        case OMX_IndexParamAudioPcm: {
            const OMX_AUDIO_PARAM_PCMMODETYPE *pcmMode =
                (const OMX_AUDIO_PARAM_PCMMODETYPE *) params;
            if (!IsValidOmxParam(pcmMode)) {
                return false;
            }
            portIndex = pcmMode->nPortIndex;
            break;
        }
        case OMX_IndexParamAudioAac: {
            const OMX_AUDIO_PARAM_AACPROFILETYPE *aacMode =
                (const OMX_AUDIO_PARAM_AACPROFILETYPE *) params;
            if (!IsValidOmxParam(aacMode)) {
                return false;
            }
            portIndex = aacMode->nPortIndex;
            break;
        }
        default:
            return false;
    }
    if (portIndex >= mPorts.size()) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] CHECK(portIndex < mPorts.size()) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    return mPorts.at(portIndex).mDef.bEnabled;
}
OMX_ERRORTYPE SprdSimpleOMXComponent::getParameter(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    std::lock_guard<std::mutex> autoLock(mLock);
    return internalGetParameter(index, params);
}
OMX_ERRORTYPE SprdSimpleOMXComponent::setParameter(
    OMX_INDEXTYPE index, const OMX_PTR params)
{
    std::lock_guard<std::mutex> autoLock(mLock);
    if (!isSetParameterAllowed(index, params)) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(isSetParameterAllowed(index, params)) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    return internalSetParameter(index, params);
}
OMX_ERRORTYPE SprdSimpleOMXComponent::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params)
{
    switch (index) {
        case OMX_IndexParamPortDefinition: {
            OMX_PARAM_PORTDEFINITIONTYPE *defParams =
                (OMX_PARAM_PORTDEFINITIONTYPE *)params;
            if (!IsValidOmxParam(defParams)) {
                return OMX_ErrorBadParameter;
            }
            if (defParams->nPortIndex >= mPorts.size() || defParams->nSize != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
                return OMX_ErrorUndefined;
            }
            const PortInfo *port = &mPorts.at(defParams->nPortIndex);
            errno_t ret = memmove_s(defParams, sizeof(port->mDef), &port->mDef, sizeof(port->mDef));
            if (ret != 0) {
                OMX_LOGE("memmove_s failed in internalGetParameter, error: %d", ret);
                return OMX_ErrorUndefined;
            }
            return OMX_ErrorNone;
        }
        default: {
            OMX_LOGE("this index : 0x%x is not completed!", index);
            return OMX_ErrorUnsupportedIndex;
        }
    }
}
OMX_ERRORTYPE SprdSimpleOMXComponent::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params)
{
    switch (index) {
        case OMX_IndexParamPortDefinition: {
            OMX_PARAM_PORTDEFINITIONTYPE *defParams =
                (OMX_PARAM_PORTDEFINITIONTYPE *)params;
            if (!IsValidOmxParam(defParams)) {
                return OMX_ErrorBadParameter;
            }
            if (defParams->nPortIndex >= mPorts.size()) {
                return OMX_ErrorBadPortIndex;
            }
            if (defParams->nSize != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
                return OMX_ErrorUnsupportedSetting;
            }
            PortInfo *port = &mPorts.at(defParams->nPortIndex);
            // default behavior is that we only allow buffer size to increase
            if (defParams->nBufferSize > port->mDef.nBufferSize) {
                port->mDef.nBufferSize = defParams->nBufferSize;
            }
            if (defParams->nBufferCountActual < port->mDef.nBufferCountMin) {
                OMX_LOGW("component requires at least %u buffers (%u requested)",
                    port->mDef.nBufferCountMin, defParams->nBufferCountActual);
                return OMX_ErrorUnsupportedSetting;
            }
            port->mDef.nBufferCountActual = defParams->nBufferCountActual;
            return OMX_ErrorNone;
        }
        default: {
            OMX_LOGE("this index : 0x%x is not completed!", index);
            return OMX_ErrorUnsupportedIndex;
        }
    }
}
OMX_ERRORTYPE SprdSimpleOMXComponent::internalSetConfig(
    OMX_INDEXTYPE index, const OMX_PTR params, bool *frameConfig)
{
    (void)frameConfig;
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE SprdSimpleOMXComponent::setConfig(
    OMX_INDEXTYPE index, const OMX_PTR params)
{
    bool frameConfig = mFrameConfig;
    OMX_ERRORTYPE err = internalSetConfig(index, params, &frameConfig);
    if (err == OMX_ErrorNone) {
        mFrameConfig = frameConfig;
    }
    return err;
}
OMX_ERRORTYPE SprdSimpleOMXComponent::useBuffer(
    OMX_BUFFERHEADERTYPE **header, const UseBufferParams &params)
{
    std::lock_guard<std::mutex> autoLock(mLock);
    if ((params.portIndex) >= (mPorts.size())) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((portIndex) < (mPorts.size())) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    PortInfo *port = &mPorts.at(params.portIndex);
        if (mState != OMX_StateLoaded && port->mDef.bEnabled != OMX_FALSE) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(mState == "
            "OMX_StateLoaded || port->mDef.bEnabled == OMX_FALSE) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    if ((port->mBuffers.size()) >= (port->mDef.nBufferCountActual)) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((port->mBuffers.size()) < "
            "(port->mDef.nBufferCountActual)) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    return internalUseBuffer(header, params);
}
OMX_ERRORTYPE SprdSimpleOMXComponent::internalUseBuffer(
    OMX_BUFFERHEADERTYPE **header, const UseBufferParams &params)
{
    (void)params.bufferPrivate;
    *header = new OMX_BUFFERHEADERTYPE;
    (*header)->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    (*header)->nVersion.s.nVersionMajor = OMX_VERSION_MAJOR;
    (*header)->nVersion.s.nVersionMinor = OMX_VERSION_MINOR;
    (*header)->nVersion.s.nRevision = VERSION_REVISION_NUMBER;
    (*header)->nVersion.s.nStep = VERSION_STEP_NUMBER;
    (*header)->pBuffer = params.ptr;
    (*header)->nAllocLen = params.size;
    (*header)->nFilledLen = 0;
    (*header)->nOffset = 0;
    (*header)->pAppPrivate = params.appPrivate;
    (*header)->pPlatformPrivate = nullptr;
    (*header)->pInputPortPrivate = nullptr;
    (*header)->pOutputPortPrivate = nullptr;
    (*header)->hMarkTargetComponent = nullptr;
    (*header)->pMarkData = nullptr;
    (*header)->nTickCount = 0;
    (*header)->nTimeStamp = 0;
    (*header)->nFlags = 0;
    (*header)->nOutputPortIndex = params.portIndex;
    (*header)->nInputPortIndex = params.portIndex;
    PortInfo *port = &mPorts.at(params.portIndex);
    port->mBuffers.push_back(BufferInfo());
    BufferInfo *buffer =
        &port->mBuffers.at(port->mBuffers.size() - 1);
    OMX_LOGI("internalUseBuffer, header=0x%p, pBuffer=0x%p, size=%d", *header, params.ptr, params.size);
    buffer->mHeader = *header;
    buffer->mOwnedByUs = false;
    if (port->mBuffers.size() == port->mDef.nBufferCountActual) {
        port->mDef.bPopulated = OMX_TRUE;
        CheckTransitions();
    }
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SprdSimpleOMXComponent::allocateBuffer(
    OMX_BUFFERHEADERTYPE **header,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    OMX_U32 size)
{
    OMX_U8* ptr;
    OMX_ERRORTYPE err;
    if (size == 0 || size > UINT32_MAX) {
        OMX_LOGE("allocateBuffer: invalid buffer size %u (max %u) for port %u", size, size, portIndex);
        return OMX_ErrorBadParameter;
    }
    if (portIndex == OMX_DirOutput) {
        ptr = new OMX_U8[size+BUFFER_ALIGNMENT_SIZE];
        OMX_U8* ptrAlign = reinterpret_cast<OMX_U8*>(
            (reinterpret_cast<unsigned long>(ptr) + BUFFER_ALIGNMENT_MASK) & (~BUFFER_ALIGNMENT_MASK));
        OMX_LOGI("Output allocateBuffer, ptr=0x%p, ptrAlign=0x%p", ptr, ptrAlign);
        UseBufferParams params = {portIndex, appPrivate, size, ptrAlign, nullptr};
        err = useBuffer(header, params);
    } else {
        ptr = new OMX_U8[size];
        OMX_LOGI("Input allocateBuffer, ptr=0x%p", ptr);
        UseBufferParams params = {portIndex, appPrivate, size, ptr, nullptr};
        err = useBuffer(header, params);
    }
    if (err != OMX_ErrorNone) {
        delete[] ptr;
        ptr = nullptr;
        return err;
    }
    if ((*header)->pPlatformPrivate != nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((*header)->pPlatformPrivate == nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    (*header)->pPlatformPrivate = ptr;
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SprdSimpleOMXComponent::freeBuffer(
    OMX_U32 portIndex,
    OMX_BUFFERHEADERTYPE *header)
{
    std::lock_guard<std::mutex> autoLock(mLock);
    if (portIndex >= mPorts.size()) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] Invalid port index %u",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    PortInfo *port = &mPorts.at(portIndex);
    bool found = false;
    for (size_t i = 0; i < port->mBuffers.size(); ++i) {
        BufferInfo *buffer = &port->mBuffers.at(i);
        if (buffer->mHeader == header) {
            if (buffer->mOwnedByUs) {
                OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(!buffer->mOwnedByUs) failed.",
                    __FUNCTION__, FILENAME_ONLY, __LINE__);
                return OMX_ErrorUndefined;
            }
            if (!releasePlatformBuffer(portIndex, header)) {
                return OMX_ErrorUndefined;
            }
            releaseHeaderPrivates(header);
            delete header;
            header = nullptr;
            port->mBuffers.erase(port->mBuffers.begin() + i);
            port->mDef.bPopulated = OMX_FALSE;
            CheckTransitions();
            found = true;
            break;
        }
    }
    if (!found) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(found) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return OMX_ErrorUndefined;
    }
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SprdSimpleOMXComponent::emptyThisBuffer(OMX_BUFFERHEADERTYPE *buffer)
{
    OMX_LOGD("emptyThisBuffer");
    Message *msg = new Message(K_WHAT_EMPTY_THIS_BUFFER, &mMsgHandler);
    msg->setPointer("header", buffer);
    msg->post();
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SprdSimpleOMXComponent::fillThisBuffer(OMX_BUFFERHEADERTYPE *buffer)
{
    OMX_LOGD("fillThisBuffer");
    Message *msg = new Message(K_WHAT_FILL_THIS_BUFFER, &mMsgHandler);
    msg->setPointer("header", buffer);
    msg->post();
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SprdSimpleOMXComponent::getState(OMX_STATETYPE *state)
{
    std::lock_guard<std::mutex> autoLock(mLock);
    *state = mState;
    return OMX_ErrorNone;
}
void SprdSimpleOMXComponent::onMessageReceived(const Message *msg)
{
    std::lock_guard<std::mutex> autoLock(mLock);
    OMX_LOGD("msg->what() = %d", msg->what());
    switch (msg->what()) {
        case K_WHAT_SEND_COMMAND: {
            if (!handleSendCommandMessage(msg)) {
                return;
            }
            break;
        }
        case K_WHAT_EMPTY_THIS_BUFFER: {
            OMX_BUFFERHEADERTYPE *header = nullptr;
            if (!findMessageHeader(msg, &header, "header")) {
                return;
            }
            onEmptyThisBuffer(header);
            break;
        }
        case K_WHAT_FILL_THIS_BUFFER: {
            OMX_BUFFERHEADERTYPE *header = nullptr;
            if (!findMessageHeader(msg, &header, "header")) {
                return;
            }
            onFillThisBuffer(header);
            break;
        }
        default:
            break;
    }
    delete msg;
    msg = nullptr;
}
void SprdSimpleOMXComponent::onEmptyThisBuffer(OMX_BUFFERHEADERTYPE *header)
{
    if (mState != OMX_StateExecuting || mTargetState != mState) {
        OMX_LOGE("%s, %d, check mState == OMX_StateExecuting failed!", __FUNCTION__, __LINE__);
        OMX_LOGE("mState: %d, mTargetState: %d", mState, mTargetState);
        return;
    }
    OMX_LOGD("onEmptyThisBuffer is called");
    PortInfo *port = editPortInfo(K_INPUT_PORT_INDEX);
    bool found = false;
    for (size_t j = 0; j < port->mBuffers.size(); ++j) {
        BufferInfo *buffer = &port->mBuffers.at(j);
        if (buffer->mHeader == header) {
            if (buffer->mOwnedByUs) {
                OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(!buffer->mOwnedByUs) failed.",
                    __FUNCTION__, FILENAME_ONLY, __LINE__);
                return;
            }
            buffer->mOwnedByUs = true;
            port->mQueue.push_back(buffer);
            onDecodePrepare(buffer->mHeader);
            onQueueFilled(OMX_DirInput);
            found = true;
            break;
        }
    }
    if (!found) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(found) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return;
    }
}
void SprdSimpleOMXComponent::onFillThisBuffer(OMX_BUFFERHEADERTYPE *header)
{
    OMX_LOGD("onFillThisBuffer is called");
    if (header == nullptr) {
        OMX_LOGE("onFillThisBuffer got null header");
        return;
    }
    if (!(mState == OMX_StateExecuting && mTargetState == mState)) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(mState == "
            "OMX_StateExecuting && mTargetState == mState) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return;
    }
    PortInfo *port = editPortInfo(K_OUTPUT_PORT_INDEX);
    bool found = false;
    for (size_t j = 0; j < port->mBuffers.size(); ++j) {
        BufferInfo *buffer = &port->mBuffers.at(j);
        if (buffer->mHeader == header) {
            if (buffer->mOwnedByUs) {
                OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(!buffer->mOwnedByUs) failed.",
                    __FUNCTION__, FILENAME_ONLY, __LINE__);
                return;
            }
            buffer->mOwnedByUs = true;
            BufferCtrlStruct *pBufCtrl = reinterpret_cast<BufferCtrlStruct*>(header->pOutputPortPrivate);
            if (pBufCtrl != nullptr && pBufCtrl->iRefCount > REFERENCE_COUNT_ZERO) {
                pBufCtrl->iRefCount--;
            }
            if (pBufCtrl != nullptr) {
                OMX_LOGD("fillThisBuffer, buffer: 0x%p, header: 0x%p", buffer, header);
                OMX_LOGD("iRefCount: %d", pBufCtrl->iRefCount);
            }
            onBufferConfig(buffer->mHeader);
            mThreadLock.lock();
            port->mQueue.push_back(buffer);
            mThreadLock.unlock();
            onQueueFilled(OMX_DirOutput);
            found = true;
            break;
        }
    }
    if (!found) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(found) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return;
    }
}
void SprdSimpleOMXComponent::onSendCommand(
    OMX_COMMANDTYPE cmd, OMX_U32 param)
{
    switch (cmd) {
        case OMX_CommandStateSet: {
            onChangeState((OMX_STATETYPE)param);
            break;
        }
        case OMX_CommandPortEnable:
        case OMX_CommandPortDisable: {
            onPortEnable(param, cmd == OMX_CommandPortEnable);
            break;
        }
        case OMX_CommandFlush: {
            onPortFlush(param, true /* sendFlushComplete */);
            break;
        }
        default: {
            OMX_LOGE("Unexpected case in switch");
            break;
        }
    }
}

void SprdSimpleOMXComponent::normalizeCanceledLoadedToIdle(OMX_STATETYPE *state)
{
    if (mState == OMX_StateLoaded && mTargetState == OMX_StateIdle && *state == OMX_StateLoaded) {
        OMX_LOGV("load->idle canceled");
        mState = mTargetState = OMX_StateIdle;
        *state = OMX_StateLoaded;
    }
}

bool SprdSimpleOMXComponent::validateStateTransitionRequest(OMX_STATETYPE state)
{
    if (mState == mTargetState) {
        return true;
    }
    OMX_LOGE("State change to state %d requested while still transitioning from state %d to %d",
        state, mState, mTargetState);
    notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
    return false;
}

bool SprdSimpleOMXComponent::handleLoadedStateChange(OMX_STATETYPE state)
{
    if (state == OMX_StateIdle) {
        return true;
    }
    OMX_LOGD("OMX_StateLoaded state = %d", state);
    notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
    return false;
}

bool SprdSimpleOMXComponent::handleIdleStateChange(OMX_STATETYPE state)
{
    if (state == OMX_StateLoaded || state == OMX_StateExecuting) {
        return true;
    }
    OMX_LOGD("OMX_StateIdle state = %d", state);
    notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
    return false;
}

bool SprdSimpleOMXComponent::handleExecutingStateChange(OMX_STATETYPE state)
{
    if (state == OMX_StatePause) {
        return true;
    }
    if (state != OMX_StateIdle) {
        OMX_LOGD("OMX_StateExecuting state = %d", state);
        notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
        return false;
    }
    PortInfo *port = &mPorts.at(OMX_DirOutput);
    if (port->mTransition == PortInfo::ENABLING) {
        port->mTransition = PortInfo::NONE;
    }
    for (size_t i = 0; i < mPorts.size(); ++i) {
        onPortFlush(i, false /* sendFlushComplete */);
    }
    mState = OMX_StateIdle;
    notify(OMX_EventCmdComplete, OMX_CommandStateSet, state, nullptr);
    return true;
}

bool SprdSimpleOMXComponent::validatePortEnableRequest(OMX_U32 portIndex, bool enable, PortInfo **port)
{
    if (portIndex >= mPorts.size()) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((portIndex) < (mPorts.size())) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    *port = &mPorts.at(portIndex);
    if (static_cast<int>((*port)->mTransition) != static_cast<int>(PortInfo::NONE)) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((static_cast<int>(port->mTransition)) == "
            "(static_cast<int>(PortInfo::NONE))) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    if ((*port)->mDef.bEnabled == enable) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(port->mDef.bEnabled == !enable) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    if ((*port)->mDef.eDir == OMX_DirOutput) {
        return true;
    }
    OMX_LOGE("Port enable/disable allowed only on output ports.");
    notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
    return false;
}

bool SprdSimpleOMXComponent::disablePortBuffers(PortInfo *port, OMX_U32 portIndex)
{
    port->mDef.bEnabled = OMX_FALSE;
    port->mTransition = PortInfo::DISABLING;
    if (portIndex == K_OUTPUT_PORT_INDEX) {
        mThreadLock.lock();
    }
    for (size_t i = 0; i < port->mBuffers.size(); ++i) {
        if (!ReturnBufferIfOwned(port, &port->mBuffers.at(i))) {
            if (portIndex == K_OUTPUT_PORT_INDEX) {
                mThreadLock.unlock();
            }
            return false;
        }
    }
    port->mQueue.clear();
    if (portIndex == K_OUTPUT_PORT_INDEX) {
        mThreadLock.unlock();
    }
    return true;
}

bool SprdSimpleOMXComponent::releasePlatformBuffer(OMX_U32 portIndex, OMX_BUFFERHEADERTYPE *header)
{
    if (header->pPlatformPrivate == nullptr) {
        return true;
    }
    if (portIndex == OMX_DirOutput) {
        delete[] reinterpret_cast<OMX_U8*>(header->pPlatformPrivate);
        header->pPlatformPrivate = nullptr;
        header->pBuffer = nullptr;
        return true;
    }
    if (header->pPlatformPrivate != header->pBuffer) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(header->pPlatformPrivate == header->pBuffer) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    delete[] header->pBuffer;
    header->pBuffer = nullptr;
    return true;
}

void SprdSimpleOMXComponent::releaseHeaderPrivates(OMX_BUFFERHEADERTYPE *header)
{
    if (header->pInputPortPrivate != nullptr) {
        delete (BufferCtrlStruct*)(header->pInputPortPrivate);
        header->pInputPortPrivate = nullptr;
    }
    if (header->pOutputPortPrivate != nullptr) {
        delete (BufferCtrlStruct*)(header->pOutputPortPrivate);
        header->pOutputPortPrivate = nullptr;
    }
}

bool SprdSimpleOMXComponent::findMessageHeader(const Message *msg, OMX_BUFFERHEADERTYPE **header, const char *key)
{
    if (msg->findPointer(key, (void **)header)) {
        return true;
    }
    OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(msg->findPointer(\"%s\", (void **)header)) failed.",
        __FUNCTION__, FILENAME_ONLY, __LINE__, key);
    return false;
}

bool SprdSimpleOMXComponent::handleSendCommandMessage(const Message *msg)
{
    int32_t cmd = 0;
    int32_t param = 0;
    if (!msg->findInt32("cmd", &cmd)) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(msg->findInt32(\"cmd\", &cmd)) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    if (!msg->findInt32("param", &param)) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(msg->findInt32(\"param\", &param)) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    onSendCommand((OMX_COMMANDTYPE)cmd, (OMX_U32)param);
    return true;
}

void SprdSimpleOMXComponent::onChangeState(OMX_STATETYPE state)
{
    normalizeCanceledLoadedToIdle(&state);
    if (!validateStateTransitionRequest(state)) {
        return;
    }
    switch (mState) {
        case OMX_StateLoaded:
            if (!handleLoadedStateChange(state)) {
                return;
            }
            break;
        case OMX_StateIdle:
            if (!handleIdleStateChange(state)) {
                return;
            }
            break;
        case OMX_StateExecuting: {
            if (!handleExecutingStateChange(state)) {
                return;
            }
            break;
        }
        default: {
            OMX_LOGE("Unexpected case in switch");
            return;
        }
    }
    mTargetState = state;
    CheckTransitions();
}
void SprdSimpleOMXComponent::OnReset()
{
    // no-op
}
void SprdSimpleOMXComponent::onPortEnable(OMX_U32 portIndex, bool enable)
{
    PortInfo *port = nullptr;
    if (!validatePortEnableRequest(portIndex, enable, &port)) {
        return;
    }
    if (!enable) {
        if (!disablePortBuffers(port, portIndex)) {
            return;
        }
    } else {
        port->mTransition = PortInfo::ENABLING;
    }
    CheckTransitions();
}
void SprdSimpleOMXComponent::onPortFlush(
    OMX_U32 portIndex, bool sendFlushComplete)
{
    if (portIndex == OMX_ALL) {
        for (size_t i = 0; i < mPorts.size(); ++i) {
            onPortFlush(i, sendFlushComplete);
        }
        if (sendFlushComplete) {
            notify(OMX_EventCmdComplete, OMX_CommandFlush, OMX_ALL, nullptr);
        }
        return;
    }
    if ((portIndex) >= (mPorts.size())) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((portIndex) < (mPorts.size())) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return;
    }
    PortInfo *port = &mPorts.at(portIndex);
    if (port->mTransition != PortInfo::NONE) {
        notify(OMX_EventError, OMX_ErrorUndefined, 0, 0);
    }
    if (portIndex == K_OUTPUT_PORT_INDEX) {
        mThreadLock.lock();
    }
    onPortFlushPrepare(portIndex);
    for (size_t i = 0; i < port->mBuffers.size(); ++i) {
        if (!ReturnBufferIfOwned(port, &port->mBuffers.at(i))) {
            if (portIndex == K_OUTPUT_PORT_INDEX) {
                mThreadLock.unlock();
            }
            return;
        }
    }
    port->mQueue.clear();
    onPortFlushCompleted(portIndex);
    if (portIndex == K_OUTPUT_PORT_INDEX) {
        mThreadLock.unlock();
    }
    if (sendFlushComplete) {
        notify(OMX_EventCmdComplete, OMX_CommandFlush, portIndex, nullptr);
    }
}
bool SprdSimpleOMXComponent::ReturnBufferIfOwned(PortInfo *port, BufferInfo *buffer)
{
    if (!buffer->mOwnedByUs) {
        return true;
    }
    buffer->mHeader->nFilledLen = 0;
    buffer->mHeader->nOffset = 0;
    buffer->mHeader->nFlags = 0;
    buffer->mOwnedByUs = false;
    if (port->mDef.eDir == OMX_DirInput) {
        notifyEmptyBufferDone(buffer->mHeader);
        return true;
    }
    if (port->mDef.eDir != OMX_DirOutput) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((port->mDef.eDir) == (OMX_DirOutput)) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return false;
    }
    BufferCtrlStruct *pBufCtrl =
        reinterpret_cast<BufferCtrlStruct*>(buffer->mHeader->pOutputPortPrivate);
    if (pBufCtrl) {
        pBufCtrl->iRefCount = REFERENCE_COUNT_INITIAL;
    }
    notifyFillBufferDone(buffer->mHeader);
    return true;
}
void SprdSimpleOMXComponent::CheckTransitions()
{
    if (mState != mTargetState) {
        bool transitionComplete = true;
        if (mState == OMX_StateLoaded) {
            if (!IsLoadedToIdleTransitionComplete(&transitionComplete)) {
                return;
            }
        } else if (mTargetState == OMX_StateLoaded) {
            if (!IsIdleToLoadedTransitionComplete(&transitionComplete)) {
                return;
            }
        }
        CompleteTransitionIfNeeded(transitionComplete);
    }
    HandlePortTransitions();
}
bool SprdSimpleOMXComponent::IsLoadedToIdleTransitionComplete(bool *transitionComplete)
{
    if (mTargetState != OMX_StateIdle) {
        OMX_LOGD("mState = OMX_StateLoaded mTargetState = %d", mTargetState);
        notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
        return false;
    }
    for (size_t i = 0; i < mPorts.size(); ++i) {
        const PortInfo &port = mPorts.at(i);
        if (port.mDef.bEnabled == OMX_FALSE) {
            continue;
        }
        if (port.mDef.bPopulated == OMX_FALSE) {
            *transitionComplete = false;
            break;
        }
    }
    return true;
}
bool SprdSimpleOMXComponent::IsIdleToLoadedTransitionComplete(bool *transitionComplete)
{
    if (mState != OMX_StateIdle) {
        OMX_LOGD("mTargetState = OMX_StateLoaded mState = %d", mState);
        notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
        return false;
    }
    for (size_t i = 0; i < mPorts.size(); ++i) {
        const PortInfo &port = mPorts.at(i);
        if (port.mDef.bEnabled == OMX_FALSE) {
            continue;
        }
        size_t n = port.mBuffers.size();
        if (n == 0) {
            continue;
        }
        if (n > port.mDef.nBufferCountActual) {
            OMX_LOGD("n = %zu, port.mDef.nBufferCountActual = %d", n, port.mDef.nBufferCountActual);
            notify(OMX_EventError, OMX_ErrorUndefined, 0, nullptr);
            return false;
        }
        OMX_BOOL expected = (n == port.mDef.nBufferCountActual) ? OMX_TRUE : OMX_FALSE;
        if (static_cast<int>(port.mDef.bPopulated) != static_cast<int>(expected)) {
            OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(populated state) failed.",
                __FUNCTION__, FILENAME_ONLY, __LINE__);
            return false;
        }
        *transitionComplete = false;
        break;
    }
    return true;
}
void SprdSimpleOMXComponent::CompleteTransitionIfNeeded(bool transitionComplete)
{
    if (!transitionComplete) {
        return;
    }
    mState = mTargetState;
    if (mState == OMX_StateLoaded) {
        OnReset();
    }
    notify(OMX_EventCmdComplete, OMX_CommandStateSet, mState, nullptr);
}
void SprdSimpleOMXComponent::HandlePortTransitions() const
{
    SprdSimpleOMXComponent *self = const_cast<SprdSimpleOMXComponent *>(this);
    for (size_t i = 0; i < self->mPorts.size(); ++i) {
        PortInfo *port = &self->mPorts.at(i);
        if (port->mTransition == PortInfo::DISABLING && port->mBuffers.empty()) {
            OMX_LOGV("Port %zu now disabled.", i);
            port->mTransition = PortInfo::NONE;
            self->notify(OMX_EventCmdComplete, OMX_CommandPortDisable, i, nullptr);
            self->onPortEnableCompleted(i, false /* enabled */);
            continue;
        }
        if (port->mTransition == PortInfo::ENABLING && port->mDef.bPopulated == OMX_TRUE) {
            OMX_LOGV("Port %zu now enabled.", i);
            port->mTransition = PortInfo::NONE;
            port->mDef.bEnabled = OMX_TRUE;
            self->notify(OMX_EventCmdComplete, OMX_CommandPortEnable, i, nullptr);
            self->onPortEnableCompleted(i, true /* enabled */);
        }
    }
}
void SprdSimpleOMXComponent::InitDumpFiles(const char *name, const char *strmExtension)
{
    int64_t startDump = systemTime();
    if (mDumpYUVEnabled) {
        char s1[FILE_PATH_BUFFER_LENGTH];
        SprdOMXUtils::getYuvFilePath(s1, FILE_PATH_BUFFER_LENGTH, name, (void*)this, startDump);
        mFileYUV = fopen(s1, FILE_OPEN_MODE_WRITE_BINARY);
        OMX_LOGD("yuv file name %s %p", s1, mFileYUV);
    }
    if (mDumpStrmEnabled) {
        char s2[FILE_PATH_BUFFER_LENGTH];
        SprdOMXUtils::StreamFilePathInfo streamInfo = {
            name,
            static_cast<void *>(this),
            strmExtension,
            startDump,
        };
        SprdOMXUtils::getStrmFilePath(s2, FILE_PATH_BUFFER_LENGTH, streamInfo);
        mFileStream = fopen(s2, FILE_OPEN_MODE_WRITE_BINARY);
        OMX_LOGD("bs file name %s %p", s2, mFileStream);
    }
}
void SprdSimpleOMXComponent::DumpFiles(uint8_t *pBuffer, int32_t aInBufSize, OMX_DUMP_TYPE type)
{
    if (mDumpYUVEnabled
            && type == DUMP_YUV
            && mFileYUV != nullptr) {
        fwrite(pBuffer, FILE_WRITE_UNIT_SIZE, aInBufSize, mFileYUV);
    }
    if (mDumpStrmEnabled
            && type == DUMP_STREAM
            && mFileStream != nullptr) {
        fwrite(pBuffer, FILE_WRITE_UNIT_SIZE, aInBufSize, mFileStream);
    }
}
void SprdSimpleOMXComponent::CloseDumpFiles() const
{
    mFileYUV = SafeCloseDumpFile(mFileYUV, "dump-yuv");
    mFileStream = SafeCloseDumpFile(mFileStream, "dump-stream");
}
#ifdef CONFIG_POWER_HINT
void SprdSimpleOMXComponent::acquirePowerHint()
{
    if (mVideoPowerHint != nullptr) {
        delete mVideoPowerHint;
        mVideoPowerHint = nullptr;
    }
    mVideoPowerHint = new VideoPowerHintInstance(mIsPowerHintRunning);
    if (mVideoPowerHint != nullptr) {
        mVideoPowerHint->acquire(ACQUIRE_HINT_PARAM1, ACQUIRE_HINT_PARAM2, mIsPowerHintRunning);
        OMX_LOGD("acquirePowerHint");
    }
}
void SprdSimpleOMXComponent::releasePowerHint()
{
    if (mVideoPowerHint != nullptr) {
        mVideoPowerHint->release();
        delete mVideoPowerHint;
        mVideoPowerHint = nullptr;
        OMX_LOGD("releasePowerHint");
    }
}
#endif
OMX_ERRORTYPE SprdSimpleOMXComponent::CheckParam(OMX_PTR header, OMX_U32 size)
{
    OMX_VERSIONTYPE* version = nullptr;
    OMX_U32 nPortIndex = INVALID_PORT_INDEX;
    if (header == nullptr) {
        OMX_LOGE("Param is Nullptr");
        return OMX_ErrorBadParameter;
    }
    if (*((OMX_U32*)header) != size) {
        OMX_LOGE("Sizeof ParamType %d dismatch nSize %d, please Check", *((OMX_U32*)header), size);
        return OMX_ErrorBadParameter;
    }
    version = (OMX_VERSIONTYPE*)(reinterpret_cast<char*>(header) + sizeof(OMX_U32));
    if (version->s.nVersionMajor != VERSIONMAJOR_NUMBER ||
        version->s.nVersionMinor != VERSIONMINOR_NUMBER) {
        OMX_LOGE("nVersion %d.%d dismatch, decoder version : %d.%d, Please Check",
            version->s.nVersionMajor, version->s.nVersionMinor, VERSIONMAJOR_NUMBER, VERSIONMINOR_NUMBER);
        return OMX_ErrorVersionMismatch;
    }
    nPortIndex = *(OMX_U32*)(reinterpret_cast<char*>(header) + PORT_INDEX_OFFSET);
    OMX_LOGE("nPortIndex = %d", nPortIndex);
    if (nPortIndex > MAX_PORT_INDEX) {
        OMX_LOGE("Wrong portIndex, please Check");
        return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNone;
}
void SprdSimpleOMXComponent::addPort(const OMX_PARAM_PORTDEFINITIONTYPE &def)
{
    if (def.nPortIndex != mPorts.size()) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((def.nPortIndex) == (mPorts.size())) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return;
    }
    mPorts.push_back(PortInfo());
    PortInfo *info = &mPorts.at(mPorts.size() - 1);
    info->mDef = def;
    info->mTransition = PortInfo::NONE;
}
void SprdSimpleOMXComponent::ConvertFlexYUVToPlanar(const FlexYuvPlanarParams &params)
{
    const uint8_t *src = reinterpret_cast<const uint8_t *>(params.ycbcr->y);
    const uint8_t *srcU = reinterpret_cast<const uint8_t *>(params.ycbcr->cb);
    const uint8_t *srcV = reinterpret_cast<const uint8_t *>(params.ycbcr->cr);
    uint8_t *dst = params.dst;
    uint8_t *dstU = dst + params.dstVStride * params.dstStride;
    uint8_t *dstV = dstU + (params.dstVStride >> 1) * (params.dstStride >> 1);
    size_t chromaHeight = params.height >> 1;
    size_t chromaWidth = params.width >> 1;
    size_t dstChromaStride = params.dstStride >> 1;
    size_t srcChromaStride = params.ycbcr->cstride;
    size_t chromaStep = params.ycbcr->chromaStep;
    ChromaCopyContext chromaCtx = {dstU, dstV, srcU, srcV, chromaHeight, chromaWidth,
        dstChromaStride, srcChromaStride, chromaStep};
    for (size_t y = params.height; y > 0; --y) {
        errno_t ret = memmove_s(dst, params.dstStride * params.height, src, params.width);
        if (ret != 0) {
            OMX_LOGE("memmove_s failed in ConvertFlexYUVToPlanar (Y plane), error: %d", ret);
            return;
        }
        dst += params.dstStride;
        src += params.ycbcr->ystride;
    }
    if (((params.ycbcr->cstride) == ((params.ycbcr->ystride) >> 1)) &&
        ((params.ycbcr->chromaStep) == CHROMA_STEP_PLANAR)) {
        if (!CopyPlanarChroma(chromaCtx)) {
            return;
        }
        return;
    }
    CopyArbitraryChroma(chromaCtx);
}
void SprdSimpleOMXComponent::onQueueFilled(OMX_U32 portIndex)
{
    (void)portIndex;
}
void SprdSimpleOMXComponent::onPortFlushCompleted(OMX_U32 portIndex)
{
    (void)portIndex;
}
void SprdSimpleOMXComponent::onPortEnableCompleted(
    OMX_U32 portIndex, bool enabled)
{
    (void)portIndex;
    (void)enabled;
}
void SprdSimpleOMXComponent::onPortFlushPrepare(OMX_U32 portIndex)
{
    (void)portIndex;
}

void SprdSimpleOMXComponent::drainOneOutputBuffer(OMX_S32 picId, const void *pBufferHeader, OMX_U64 pts)
{
    (void)picId;
    (void)pBufferHeader;
    (void)pts;
}

std::list <SprdSimpleOMXComponent::BufferInfo*>& SprdSimpleOMXComponent::getPortQueue(OMX_U32 portIndex)
{
    if ((portIndex) >= (mPorts.size())) {
        OMX_LOGE("invalid port index %u, port count %zu", portIndex, mPorts.size());
        return InvalidPortQueue();
    }
    return mPorts.at(portIndex).mQueue;
}
SprdSimpleOMXComponent::PortInfo *SprdSimpleOMXComponent::editPortInfo(
    OMX_U32 portIndex)
{
    if ((portIndex) >= (mPorts.size())) {
        OMX_LOGE("invalid port index %u, port count %zu", portIndex, mPorts.size());
        return &InvalidPortInfo();
    }
    return &mPorts.at(portIndex);
}
void SprdSimpleOMXComponent::onDecodePrepare(OMX_BUFFERHEADERTYPE *header)
{
    (void)header;
}
void SprdSimpleOMXComponent::onBufferConfig(OMX_BUFFERHEADERTYPE *header)
{
    (void)header;
}
SprdSimpleOMXComponent::~SprdSimpleOMXComponent()
{
    OMX_LOGD("Destruct SprdSimpleOMXComponent, this: 0x%p", static_cast<void *>(this));
#ifdef CONFIG_POWER_HINT
    releasePowerHint();
#endif
}
void SprdSimpleOMXComponent::InitLooper(const char *name) const
{
    Looper *looper = new Looper();
    looper->setName(name);
    mMsgHandler.setLooper(looper);
    looper->start();
}
void SprdSimpleOMXComponent::DeinitLooper()
{
    Looper *looper = mMsgHandler.getLooper();
    mMsgHandler.setLooper(nullptr);
    looper->stop();
    delete looper;
    looper = nullptr;
}
bool SprdSimpleOMXComponent::GetDumpValue(const char *propertyStr) const
{
    return SprdOMXUtils::GetDumpValue(propertyStr);
}
void SprdSimpleOMXComponent::releaseMemIon(PMemIon* ppMemIon)
{
    SprdOMXUtils::releaseMemIon(ppMemIon);
}
};    // namespace OMX
};    // namespace OHOS
