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
#include <unistd.h>
#include <sys/types.h>
#include "include/SprdOMXComponent.h"
#include "utils/omx_log.h"
#undef LOG_TAG
#define LOG_TAG "SprdOMXComponent"
/* Number of 32-bit words used for UUID */
#define UUID_NUM_COMPONENTS 3
/* UUID component indices */
#define UUID_COMPONENT_SIZE_INDEX 0
#define UUID_COMPONENT_PID_INDEX 1
#define UUID_COMPONENT_UID_INDEX 2
namespace OHOS {
namespace OMX {
SprdOMXComponent::SprdOMXComponent(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : mName(name), mCallbacks(callbacks), mComponent(new OMX_COMPONENTTYPE)
{
    mComponent->nSize = sizeof(*mComponent);
    mComponent->nVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    mComponent->nVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    mComponent->nVersion.s.nRevision = REVISION_NUMBER;
    mComponent->nVersion.s.nStep = STEP_NUMBER;
    mComponent->pComponentPrivate = this;
    mComponent->pApplicationPrivate = appData;
    mComponent->GetComponentVersion = GetComponentVersionWrapper;
    mComponent->SendCommand = SendCommandWrapper;
    mComponent->GetParameter = GetParameterWrapper;
    mComponent->SetParameter = SetParameterWrapper;
    mComponent->GetConfig = GetConfigWrapper;
    mComponent->SetConfig = SetConfigWrapper;
    mComponent->GetExtensionIndex = GetExtensionIndexWrapper;
    mComponent->GetState = GetStateWrapper;
    mComponent->ComponentTunnelRequest = nullptr;
    mComponent->UseBuffer = UseBufferWrapper;
    mComponent->AllocateBuffer = AllocateBufferWrapper;
    mComponent->FreeBuffer = FreeBufferWrapper;
    mComponent->EmptyThisBuffer = EmptyThisBufferWrapper;
    mComponent->FillThisBuffer = FillThisBufferWrapper;
    mComponent->SetCallbacks = nullptr;
    mComponent->ComponentDeInit = nullptr;
    mComponent->UseEGLImage = nullptr;
    mComponent->ComponentRoleEnum = nullptr;
    *component = mComponent;
    OMX_LOGI("Construct SprdOMXComponent (%p)", static_cast<void *>(this));
}
SprdOMXComponent::~SprdOMXComponent()
{
    if (mComponent != nullptr) {
        delete mComponent;
        mComponent = nullptr;
    }
}
void SprdOMXComponent::SetLibHandle(void *libHandle)
{
    if (libHandle == nullptr) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] CHECK(libHandle != nullptr) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        return;
    }
    mHwLibHandle = libHandle;
}
void *SprdOMXComponent::LibHandle() const
{
    return mHwLibHandle;
}
OMX_ERRORTYPE SprdOMXComponent::initCheck() const
{
    return OMX_ErrorNone;
}
const char *SprdOMXComponent::Name() const
{
    return mName.c_str();
}
void SprdOMXComponent::notify(
    OMX_EVENTTYPE event,
    OMX_U32 data1, OMX_U32 data2, OMX_PTR data)
{
    (*mCallbacks->EventHandler)(mComponent, mComponent->pApplicationPrivate, event, data1, data2, data);
}
void SprdOMXComponent::notifyEmptyBufferDone(OMX_BUFFERHEADERTYPE *header)
{
    (*mCallbacks->EmptyBufferDone)(
        mComponent, mComponent->pApplicationPrivate, header);
}
void SprdOMXComponent::notifyFillBufferDone(OMX_BUFFERHEADERTYPE *header)
{
    (*mCallbacks->FillBufferDone)(
        mComponent, mComponent->pApplicationPrivate, header);
}
// static //zsx add
OMX_ERRORTYPE SprdOMXComponent::GetComponentVersionWrapper(
    OMX_HANDLETYPE Component,
    OMX_STRING ComponentName,
    OMX_VERSIONTYPE* ComponentVersion,
    OMX_VERSIONTYPE* SpecVersion,
    OMX_UUIDTYPE* ComponentUUID)
{
    SprdOMXComponent *me = (SprdOMXComponent *) ((OMX_COMPONENTTYPE *)Component)->pComponentPrivate;
    return me->getComponentVersion(ComponentName, ComponentVersion, SpecVersion, ComponentUUID);
}
// static
OMX_ERRORTYPE SprdOMXComponent::SendCommandWrapper(
    OMX_HANDLETYPE component,
    OMX_COMMANDTYPE cmd,
    OMX_U32 param,
    OMX_PTR data)
{
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    return me->sendCommand(cmd, param, data);
}
// static
OMX_ERRORTYPE SprdOMXComponent::GetParameterWrapper(
    OMX_HANDLETYPE component,
    OMX_INDEXTYPE index,
    OMX_PTR params)
{
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    return me->getParameter(index, params);
}
// static
OMX_ERRORTYPE SprdOMXComponent::SetParameterWrapper(
    OMX_HANDLETYPE component,
    OMX_INDEXTYPE index,
    OMX_PTR params)
{
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    return me->setParameter(index, params);
}
// static
OMX_ERRORTYPE SprdOMXComponent::GetConfigWrapper(
    OMX_HANDLETYPE component,
    OMX_INDEXTYPE index,
    OMX_PTR params)
{
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    return me->getConfig(index, params);
}
// static
OMX_ERRORTYPE SprdOMXComponent::SetConfigWrapper(
    OMX_HANDLETYPE component,
    OMX_INDEXTYPE index,
    OMX_PTR params)
{
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    return me->setConfig(index, params);
}
// static
OMX_ERRORTYPE SprdOMXComponent::GetExtensionIndexWrapper(
    OMX_HANDLETYPE component,
    OMX_STRING name,
    OMX_INDEXTYPE *index)
{
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    return me->getExtensionIndex(name, index);
}
// static
OMX_ERRORTYPE SprdOMXComponent::UseBufferWrapper(
    OMX_HANDLETYPE component,
    OMX_BUFFERHEADERTYPE **buffer,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    OMX_U32 size,
    OMX_U8 *ptr)
{
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    UseBufferParams params = {portIndex, appPrivate, size, ptr, nullptr};
    return me->useBuffer(buffer, params);
}
// static
OMX_ERRORTYPE SprdOMXComponent::AllocateBufferWrapper(
    OMX_HANDLETYPE component,
    OMX_BUFFERHEADERTYPE **buffer,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    OMX_U32 size)
{
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    return me->allocateBuffer(buffer, portIndex, appPrivate, size);
}
// static
OMX_ERRORTYPE SprdOMXComponent::FreeBufferWrapper(
    OMX_HANDLETYPE component,
    OMX_U32 portIndex,
    OMX_BUFFERHEADERTYPE *buffer)
{
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    return me->freeBuffer(portIndex, buffer);
}
// static
OMX_ERRORTYPE SprdOMXComponent::EmptyThisBufferWrapper(
    OMX_HANDLETYPE component,
    OMX_BUFFERHEADERTYPE *buffer)
{
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    return me->emptyThisBuffer(buffer);
}
// static
OMX_ERRORTYPE SprdOMXComponent::FillThisBufferWrapper(
    OMX_HANDLETYPE component,
    OMX_BUFFERHEADERTYPE *buffer)
{
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    return me->fillThisBuffer(buffer);
}
// static
OMX_ERRORTYPE SprdOMXComponent::GetStateWrapper(
    OMX_HANDLETYPE component,
    OMX_STATETYPE *state)
{
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    return me->getState(state);
}

OMX_ERRORTYPE SprdOMXComponent::getComponentVersion(
    OMX_STRING ComponentName, OMX_VERSIONTYPE* ComponentVersion,
    OMX_VERSIONTYPE* SpecVersion, OMX_UUIDTYPE* ComponentUUID)
{
    OMX_U32     compUUID[UUID_NUM_COMPONENTS];
    /*ComponentName*/
    errno_t ret = memmove_s(ComponentName, OMX_MAX_STRINGNAME_SIZE, mName.c_str(), mName.size());
    if (ret != 0) {
        OMX_LOGE("memmove_s failed in line %d, ret=%d", __LINE__, ret);
    }
    /**/
    ret = memmove_s(SpecVersion, sizeof(OMX_VERSIONTYPE), &mComponent->nVersion, sizeof(OMX_VERSIONTYPE));
    if (ret != 0) {
        OMX_LOGE("memmove_s failed in line %d, ret=%d", __LINE__, ret);
    }
    /**/
    ret = memmove_s(ComponentVersion, sizeof(OMX_VERSIONTYPE), &mComponent->nVersion, sizeof(OMX_VERSIONTYPE));
    if (ret != 0) {
        OMX_LOGE("memmove_s failed in line %d, ret=%d", __LINE__, ret);
    }
    /* Fill UUID with handle address, PID and UID.
     * This should guarantee uiniqness */
    compUUID[UUID_COMPONENT_SIZE_INDEX] = mComponent->nSize;
    compUUID[UUID_COMPONENT_PID_INDEX] = getpid();
    compUUID[UUID_COMPONENT_UID_INDEX] = getuid();
    ret = memmove_s(ComponentUUID, sizeof(OMX_UUIDTYPE), compUUID, UUID_NUM_COMPONENTS * sizeof(*compUUID));
    if (ret != 0) {
        OMX_LOGE("memmove_s failed in line %d, ret=%d", __LINE__, ret);
    }
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SprdOMXComponent::sendCommand(
    OMX_COMMANDTYPE cmd, OMX_U32 param, OMX_PTR data)
{
    (void)cmd;
    (void)param;
    (void)data;
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE SprdOMXComponent::getParameter(
    OMX_INDEXTYPE /* index */, OMX_PTR /* params */)
{
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE SprdOMXComponent::setParameter(
    OMX_INDEXTYPE /* index */, const OMX_PTR /* params */)
{
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE SprdOMXComponent::getConfig(
    OMX_INDEXTYPE /* index */, OMX_PTR /* params */)
{
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE SprdOMXComponent::setConfig(
    OMX_INDEXTYPE /* index */, const OMX_PTR /* params */)
{
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE SprdOMXComponent::getExtensionIndex(
    const char * /* name */, OMX_INDEXTYPE * /* index */)
{
    return OMX_ErrorUnsupportedIndex;
}
OMX_ERRORTYPE SprdOMXComponent::useBuffer(
    OMX_BUFFERHEADERTYPE ** /* buffer */, const UseBufferParams &params)
{
    (void)params;
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE SprdOMXComponent::allocateBuffer(
    OMX_BUFFERHEADERTYPE ** /* buffer */,
    OMX_U32 /* portIndex */,
    OMX_PTR /* appPrivate */,
    OMX_U32 /* size */)
{
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE SprdOMXComponent::freeBuffer(
    OMX_U32 /* portIndex */,
    OMX_BUFFERHEADERTYPE * /* buffer */)
{
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE SprdOMXComponent::emptyThisBuffer(
    OMX_BUFFERHEADERTYPE * /* buffer */)
{
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE SprdOMXComponent::fillThisBuffer(
    OMX_BUFFERHEADERTYPE * /* buffer */)
{
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE SprdOMXComponent::getState(OMX_STATETYPE * /* state */)
{
    return OMX_ErrorUndefined;
}
};    // namespace OMX
};    // namespace OHOS