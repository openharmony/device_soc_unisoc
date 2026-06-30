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
#include <cstring>
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
namespace {
constexpr OMX_U32 K_PRIMARY_ROLE_INDEX = 0;
constexpr size_t K_STRING_TERMINATOR_SIZE = 1;

struct ComponentRoleInfo {
    const char *name;
    const char *role;
};

const ComponentRoleInfo COMPONENT_ROLES[] = {
    { "OMX.sprd.h264.decoder", "video_decoder.avc" },
    { "OMX.sprd.h264.encoder", "video_encoder.avc" },
    { "OMX.sprd.hevc.decoder", "video_decoder.hevc" },
    { "OMX.sprd.h265.encoder", "video_encoder.hevc" },
};

const char *FindRoleByComponentName(const char *componentName)
{
    if (componentName == nullptr) {
        return nullptr;
    }
    for (const ComponentRoleInfo &roleInfo : COMPONENT_ROLES) {
        if (std::strcmp(componentName, roleInfo.name) == 0) {
            return roleInfo.role;
        }
    }
    return nullptr;
}
}  // namespace

SprdOMXComponent::SprdOMXComponent(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : mName(name), mCallbacks(callbacks), mComponent(new OMX_COMPONENTTYPE), mHwLibHandle(nullptr)
{
    BuildComponentHandle(appData);
    *component = mComponent;
    OMX_LOGI("Construct SprdOMXComponent (%p)", static_cast<void *>(this));
}
void SprdOMXComponent::BuildComponentHandle(OMX_PTR appData)
{
    FillComponentVersion();
    mComponent->pComponentPrivate = this;
    mComponent->pApplicationPrivate = appData;
    BindComponentEntryPoints();
}
void SprdOMXComponent::FillComponentVersion()
{
    OMX_VERSIONTYPE &version = mComponent->nVersion;
    mComponent->nSize = sizeof(*mComponent);
    version.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    version.s.nVersionMinor = VERSIONMINOR_NUMBER;
    version.s.nRevision = REVISION_NUMBER;
    version.s.nStep = STEP_NUMBER;
}
void SprdOMXComponent::BindComponentEntryPoints()
{
    OMX_COMPONENTTYPE *component = mComponent;
    component->GetComponentVersion = GetComponentVersionWrapper;
    component->SendCommand = SendCommandWrapper;
    component->GetParameter = GetParameterWrapper;
    component->SetParameter = SetParameterWrapper;
    component->GetConfig = GetConfigWrapper;
    component->SetConfig = SetConfigWrapper;
    component->GetExtensionIndex = GetExtensionIndexWrapper;
    component->GetState = GetStateWrapper;
    component->ComponentTunnelRequest = ComponentTunnelRequestWrapper;
    component->UseBuffer = UseBufferWrapper;
    component->AllocateBuffer = AllocateBufferWrapper;
    component->FreeBuffer = FreeBufferWrapper;
    component->EmptyThisBuffer = EmptyThisBufferWrapper;
    component->FillThisBuffer = FillThisBufferWrapper;
    component->SetCallbacks = SetCallbacksWrapper;
    component->ComponentDeInit = ComponentDeInitWrapper;
    component->UseEGLImage = UseEGLImageWrapper;
    component->ComponentRoleEnum = ComponentRoleEnumWrapper;
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
    (*mCallbacks->EventHandler)(mComponent, AppData(), event, data1, data2, data);
}
void SprdOMXComponent::notifyEmptyBufferDone(OMX_BUFFERHEADERTYPE *header)
{
    (*mCallbacks->EmptyBufferDone)(mComponent, AppData(), header);
}
void SprdOMXComponent::notifyFillBufferDone(OMX_BUFFERHEADERTYPE *header)
{
    (*mCallbacks->FillBufferDone)(mComponent, AppData(), header);
}
OMX_PTR SprdOMXComponent::AppData() const
{
    return mComponent->pApplicationPrivate;
}
SprdOMXComponent *SprdOMXComponent::FromHandle(OMX_HANDLETYPE component)
{
    if (component == nullptr) {
        OMX_LOGE("%{public}s received null component", __FUNCTION__);
        return nullptr;
    }
    OMX_COMPONENTTYPE *omxComponent = static_cast<OMX_COMPONENTTYPE *>(component);
    SprdOMXComponent *owner = static_cast<SprdOMXComponent *>(omxComponent->pComponentPrivate);
    if (owner == nullptr) {
        OMX_LOGE("%{public}s received null component private", __FUNCTION__);
    }
    return owner;
}
// static //zsx add
OMX_ERRORTYPE SprdOMXComponent::GetComponentVersionWrapper(
    OMX_HANDLETYPE Component,
    OMX_STRING ComponentName,
    OMX_VERSIONTYPE* ComponentVersion,
    OMX_VERSIONTYPE* SpecVersion,
    OMX_UUIDTYPE* ComponentUUID)
{
    SprdOMXComponent *me = FromHandle(Component);
    return me == nullptr ? OMX_ErrorInvalidComponent :
        me->getComponentVersion(ComponentName, ComponentVersion, SpecVersion, ComponentUUID);
}
// static
OMX_ERRORTYPE SprdOMXComponent::SendCommandWrapper(
    OMX_HANDLETYPE component,
    OMX_COMMANDTYPE cmd,
    OMX_U32 param,
    OMX_PTR data)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->sendCommand(cmd, param, data);
}
// static
OMX_ERRORTYPE SprdOMXComponent::GetParameterWrapper(
    OMX_HANDLETYPE component,
    OMX_INDEXTYPE index,
    OMX_PTR params)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->getParameter(index, params);
}
// static
OMX_ERRORTYPE SprdOMXComponent::SetParameterWrapper(
    OMX_HANDLETYPE component,
    OMX_INDEXTYPE index,
    OMX_PTR params)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->setParameter(index, params);
}
// static
OMX_ERRORTYPE SprdOMXComponent::GetConfigWrapper(
    OMX_HANDLETYPE component,
    OMX_INDEXTYPE index,
    OMX_PTR params)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->getConfig(index, params);
}
// static
OMX_ERRORTYPE SprdOMXComponent::SetConfigWrapper(
    OMX_HANDLETYPE component,
    OMX_INDEXTYPE index,
    OMX_PTR params)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->setConfig(index, params);
}
// static
OMX_ERRORTYPE SprdOMXComponent::GetExtensionIndexWrapper(
    OMX_HANDLETYPE component,
    OMX_STRING name,
    OMX_INDEXTYPE *index)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->getExtensionIndex(name, index);
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
    SprdOMXComponent *me = FromHandle(component);
    if (me == nullptr) {
        return OMX_ErrorInvalidComponent;
    }
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
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->allocateBuffer(buffer, portIndex, appPrivate, size);
}
// static
OMX_ERRORTYPE SprdOMXComponent::FreeBufferWrapper(
    OMX_HANDLETYPE component,
    OMX_U32 portIndex,
    OMX_BUFFERHEADERTYPE *buffer)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->freeBuffer(portIndex, buffer);
}
// static
OMX_ERRORTYPE SprdOMXComponent::EmptyThisBufferWrapper(
    OMX_HANDLETYPE component,
    OMX_BUFFERHEADERTYPE *buffer)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->emptyThisBuffer(buffer);
}
// static
OMX_ERRORTYPE SprdOMXComponent::FillThisBufferWrapper(
    OMX_HANDLETYPE component,
    OMX_BUFFERHEADERTYPE *buffer)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->fillThisBuffer(buffer);
}
// static
OMX_ERRORTYPE SprdOMXComponent::GetStateWrapper(
    OMX_HANDLETYPE component,
    OMX_STATETYPE *state)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->getState(state);
}

// static
OMX_ERRORTYPE SprdOMXComponent::ComponentTunnelRequestWrapper(
    OMX_HANDLETYPE component,
    OMX_U32 port,
    OMX_HANDLETYPE tunneledComp,
    OMX_U32 tunneledPort,
    OMX_TUNNELSETUPTYPE *tunnelSetup)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent :
        me->componentTunnelRequest(port, tunneledComp, tunneledPort, tunnelSetup);
}

// static
OMX_ERRORTYPE SprdOMXComponent::SetCallbacksWrapper(
    OMX_HANDLETYPE component,
    OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->setCallbacks(callbacks, appData);
}

// static
OMX_ERRORTYPE SprdOMXComponent::UseEGLImageWrapper(
    OMX_HANDLETYPE component,
    OMX_BUFFERHEADERTYPE **buffer,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    void *eglImage)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->useEGLImage(buffer, portIndex, appPrivate, eglImage);
}

// static
OMX_ERRORTYPE SprdOMXComponent::ComponentRoleEnumWrapper(
    OMX_HANDLETYPE component,
    OMX_U8 *role,
    OMX_U32 index)
{
    SprdOMXComponent *me = FromHandle(component);
    return me == nullptr ? OMX_ErrorInvalidComponent : me->componentRoleEnum(role, index);
}

// static
OMX_ERRORTYPE SprdOMXComponent::ComponentDeInitWrapper(
    OMX_HANDLETYPE component)
{
    SprdOMXComponent *me = FromHandle(component);
    if (me == nullptr) {
        return OMX_ErrorInvalidComponent;
    }
    me->PrepareForDestruction();
    return OMX_ErrorNone;
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
OMX_ERRORTYPE SprdOMXComponent::componentTunnelRequest(
    OMX_U32 port,
    OMX_HANDLETYPE tunneledComp,
    OMX_U32 tunneledPort,
    OMX_TUNNELSETUPTYPE *tunnelSetup)
{
    OMX_LOGW("%{public}s: tunneling unsupported, component=%{public}s, port=%{public}u, "
        "tunneledComp=%{public}p, tunneledPort=%{public}u, tunnelSetup=%{public}p",
        __FUNCTION__, Name(), port, tunneledComp, tunneledPort, tunnelSetup);
    return OMX_ErrorTunnelingUnsupported;
}
OMX_ERRORTYPE SprdOMXComponent::setCallbacks(
    OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData)
{
    if (callbacks == nullptr) {
        OMX_LOGE("%{public}s: callbacks is null, component=%{public}s", __FUNCTION__, Name());
        return OMX_ErrorBadParameter;
    }
    mCallbacks = callbacks;
    mComponent->pApplicationPrivate = appData;
    OMX_LOGI("%{public}s: callbacks updated, component=%{public}s, callbacks=%{public}p, "
        "appData=%{public}p", __FUNCTION__, Name(), callbacks, appData);
    return OMX_ErrorNone;
}
OMX_ERRORTYPE SprdOMXComponent::useEGLImage(
    OMX_BUFFERHEADERTYPE **buffer,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    void *eglImage)
{
    OMX_LOGI("%{public}s: component=%{public}s, buffer=%{public}p, portIndex=%{public}u, "
        "appPrivate=%{public}p, eglImage=%{public}p", __FUNCTION__, Name(), buffer, portIndex,
        appPrivate, eglImage);
    return OMX_ErrorNotImplemented;
}
OMX_ERRORTYPE SprdOMXComponent::componentRoleEnum(
    OMX_U8 *role,
    OMX_U32 index)
{
    if (role == nullptr) {
        OMX_LOGE("%{public}s: role is null, component=%{public}s", __FUNCTION__, Name());
        return OMX_ErrorBadParameter;
    }
    if (index != K_PRIMARY_ROLE_INDEX) {
        OMX_LOGW("%{public}s: no more roles, component=%{public}s, index=%{public}u",
            __FUNCTION__, Name(), index);
        return OMX_ErrorNoMore;
    }
    const char *componentRole = FindRoleByComponentName(Name());
    if (componentRole == nullptr) {
        OMX_LOGE("%{public}s: role not found, component=%{public}s", __FUNCTION__, Name());
        return OMX_ErrorInvalidComponentName;
    }
    errno_t ret = strncpy_s(reinterpret_cast<char *>(role), OMX_MAX_STRINGNAME_SIZE,
        componentRole, OMX_MAX_STRINGNAME_SIZE - K_STRING_TERMINATOR_SIZE);
    if (ret != 0) {
        OMX_LOGE("%{public}s: failed to copy role, component=%{public}s, ret=%{public}d",
            __FUNCTION__, Name(), ret);
        return OMX_ErrorUndefined;
    }
    OMX_LOGI("%{public}s: component=%{public}s, role=%{public}s", __FUNCTION__, Name(), componentRole);
    return OMX_ErrorNone;
}
};    // namespace OMX
};    // namespace OHOS
