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
#include "sprd_omx_core.h"
#include "SprdOMXComponent.h"
#include <pthread.h>
#include <dlfcn.h>
#include <cstdio>
#include "utils/omx_log.h"
#include "sprd_omx_core_impl.h"
#include <thread>
#include <chrono>
#include <unistd.h>
#include <cstring>
#include <vector>
#undef LOG_TAG
#define LOG_TAG "SPRD_OMX_CORE"
#define MAX_COMPONENT_NAME_LEN 128
using namespace OHOS::OMX;
static OMXCore* g_omxCore = nullptr;
static bool g_bInitialized = false;
static std::vector<void *> g_pendingLibHandles;
// mfx OMX IL Core thread safety
static pthread_mutex_t g_omxCoreMutex = PTHREAD_MUTEX_INITIALIZER;

static void QueueLibHandleForDeferredClose(void *libHandle)
{
    if (libHandle == nullptr) {
        return;
    }
    for (void *pendingHandle : g_pendingLibHandles) {
        if (pendingHandle == libHandle) {
            return;
        }
    }
    g_pendingLibHandles.push_back(libHandle);
}

static void CloseDeferredLibHandles()
{
    for (void *libHandle : g_pendingLibHandles) {
        if (libHandle == nullptr) {
            continue;
        }
        OMX_LOGI("deferred dlclose in OMX_Deinit: %p", libHandle);
        dlclose(libHandle);
    }
    g_pendingLibHandles.clear();
}

static OMX_ERRORTYPE sprdOmxCoreLockFunc()
{
    int resLock = pthread_mutex_lock(&g_omxCoreMutex);
    if (resLock) return OMX_ErrorUndefined;
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE sprdOmxCoreUnlockFunc()
{
    int resUnlock = pthread_mutex_unlock(&g_omxCoreMutex);
    if (resUnlock) return OMX_ErrorUndefined;
    return OMX_ErrorNone;
}

static void SprdOmxLogFunc(const char *msg)
{
    OMX_LOGI("%s: %s", __FUNCTION__, msg);
}

static void SprdOmxLogDFunc(const char *msg, int value)
{
    OMX_LOGD("%s: %s = %d", __FUNCTION__, msg, value);
}

typedef SprdOMXComponent *(*CreateSprdOMXComponentFunc)(
    const char *, const OMX_CALLBACKTYPE *, OMX_PTR, OMX_COMPONENTTYPE **);

struct CreateComponentParams {
    OMX_HANDLETYPE *handle;
    OMX_STRING componentName;
    OMX_PTR appData;
    OMX_CALLBACKTYPE *callbacks;
    const char *libName;
    void *libHandle;
};

static const size_t K_NUM_COMPONENTS =
    sizeof(K_COMPONENTS) / sizeof(K_COMPONENTS[0]);

static ssize_t FindComponentIndexByName(const char *componentName)
{
    for (size_t i = 0; i < K_NUM_COMPONENTS; ++i) {
        if (!strcmp(componentName, K_COMPONENTS[i].mName)) {
            return static_cast<ssize_t>(i);
        }
    }
    return -1;
}

static ssize_t FindComponentIndexByRole(const char *role)
{
    for (size_t i = 0; i < K_NUM_COMPONENTS; ++i) {
        if (!strcmp(role, K_COMPONENTS[i].mRole)) {
            return static_cast<ssize_t>(i);
        }
    }
    return -1;
}

static void CopyOmxString(char *dest, const char *src, size_t maxLength)
{
    if (dest == nullptr || src == nullptr || maxLength == 0) {
        return;
    }
    // strncpy_s returns non-zero on error; check and handle to ensure safety.
    int rc = strncpy_s(dest, maxLength, src, maxLength - 1);
    if (rc != 0) {
        // On error, ensure the destination is an empty, NUL-terminated string.
        dest[0] = '\0';
    } else {
        // Explicitly ensure NUL-termination as an extra safety measure.
        dest[maxLength - 1] = '\0';
    }
}

static CreateSprdOMXComponentFunc LoadCreateComponentFunc(void *libHandle)
{
    return reinterpret_cast<CreateSprdOMXComponentFunc>(dlsym(
        libHandle,
        "_ZN4OHOS3OMX22createSprdOMXComponentEPKcPK16OMX_CALLBACKTYPE"
        "PvPP17OMX_COMPONENTTYPE"));
}

static OMX_ERRORTYPE CreateOmxComponentInstance(
    const CreateComponentParams &params)
{
    (void)params.libName;
    CreateSprdOMXComponentFunc createFunc = LoadCreateComponentFunc(params.libHandle);
    if (createFunc == nullptr) {
        OMX_LOGD("createSprdOMXComponent == nullptr");
        return OMX_ErrorComponentNotFound;
    }
    PSprdOMXComponent codec = createFunc(
        params.componentName,
        params.callbacks,
        params.appData,
        reinterpret_cast<OMX_COMPONENTTYPE **>(params.handle));
    if (codec == nullptr) {
        OMX_LOGD("codec == nullptr");
        return OMX_ErrorInsufficientResources;
    }
    OMX_ERRORTYPE err = codec->initCheck();
    if (err != OMX_ErrorNone) {
        OMX_LOGD("codec->initCheck() returned an error");
        codec->PrepareForDestruction();
        codec.clear();
        return err;
    }
    SprdOmxLogFunc("realloc");
    IncOmxCore(codec, g_omxCore);
    codec->SetLibHandle(params.libHandle);
    OMX_LOGI("Created OMXPlugin : %s", params.componentName);
    SprdOmxLogFunc("-");
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE LoadMatchingComponent(
    OMX_HANDLETYPE *pHandle,
    OMX_STRING componentName,
    OMX_PTR appData,
    OMX_CALLBACKTYPE *callbacks,
    size_t index)
{
    std::string libName = "libstagefright_";
    libName.append(K_COMPONENTS[index].mLibNameSuffix);
    libName.append(".z.so");
    OMX_LOGD("libName: %s", libName.c_str());
    void *libHandle = dlopen(libName.c_str(), RTLD_NOW);
    if (libHandle == nullptr) {
        OMX_LOGE("unable to dlopen %s: %s", libName.c_str(), dlerror());
        return OMX_ErrorComponentNotFound;
    }
    CreateComponentParams params = {pHandle, componentName, appData, callbacks, libName.c_str(), libHandle};
    OMX_ERRORTYPE err = CreateOmxComponentInstance(params);
    if (err != OMX_ErrorNone) {
        // Some vendor codec stacks spawn helper threads during init and only stop
        // them asynchronously on failure. Unloading the component library here can
        // race with those late-starting/late-exiting threads and crash in ld-musl.
        // Defer unloading until OMX_Deinit() to avoid executing unmapped code.
        QueueLibHandleForDeferredClose(libHandle);
        OMX_LOGW("defer dlclose on failed component init: %s, err=0x%x",
            componentName, static_cast<unsigned int>(err));
    }
    return err;
}

/*------------------------------------------------------------------------------*/
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Init(void)
{
    SprdOmxLogFunc("+");
    if (sprdOmxCoreLockFunc() != OMX_ErrorNone) return OMX_ErrorUndefined;
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    SprdOmxLogDFunc("g_bInitialized", g_bInitialized);
    if (!g_bInitialized) {
        g_omxCore = new OMXCore();
        g_bInitialized = true;
    }
    g_omxCore->mOmxCoreRefCount++;
    SprdOmxLogDFunc("g_OMXCoreRefCount", g_omxCore->mOmxCoreRefCount);
    if (sprdOmxCoreUnlockFunc() != OMX_ErrorNone) return OMX_ErrorUndefined;
    SprdOmxLogFunc("-");
    return OMX_ErrorNone;
}
/*------------------------------------------------------------------------------*/
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Deinit(void)
{
    if (sprdOmxCoreLockFunc() != OMX_ErrorNone) return OMX_ErrorUndefined;
    SprdOmxLogDFunc("g_bInitialized", g_bInitialized);
    if (g_bInitialized) {
        --g_omxCore->mOmxCoreRefCount;
        SprdOmxLogDFunc("g_OMXCoreRefCount", g_omxCore->mOmxCoreRefCount);
        if (g_omxCore->mOmxCoreRefCount == 0) {
            CloseDeferredLibHandles();
            delete g_omxCore;
            g_omxCore = nullptr;
            g_bInitialized = false;
        }
    }
    SprdOmxLogDFunc("g_bInitialized", g_bInitialized);
    if (sprdOmxCoreUnlockFunc() != OMX_ErrorNone) return OMX_ErrorUndefined;
    return OMX_ErrorNone;
}
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_GetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks)
{
    OMX_LOGD("+");
    if (sprdOmxCoreLockFunc() != OMX_ErrorNone) {
        return OMX_ErrorUndefined;
    }
    OMX_LOGD("makeComponentInstance: %s", cComponentName);
    ssize_t componentIndex = FindComponentIndexByName(cComponentName);
    if (componentIndex < 0) {
        sprdOmxCoreUnlockFunc();
        return OMX_ErrorInvalidComponentName;
    }
    OMX_ERRORTYPE err = LoadMatchingComponent(
        pHandle, cComponentName, pAppData, pCallBacks, static_cast<size_t>(componentIndex));
    sprdOmxCoreUnlockFunc();
    return err;
}
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_FreeHandle(
    OMX_IN  OMX_HANDLETYPE hComponent)
{
    OMX_LOGD("+");
    if (sprdOmxCoreLockFunc() != OMX_ErrorNone) {
        return OMX_ErrorUndefined;
    }
    /*asynchronous thread in lib have no interface to check */
    // Keep the original wait to avoid upper-layer timeout regressions.
    usleep(50000);
    SprdOMXComponent* omx_component =
       (SprdOMXComponent*)(((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    omx_component->PrepareForDestruction();
    void *libHandle = omx_component->LibHandle();
    if ((getOMXCoreCount(omx_component)) != (1)) {
        OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK((getOMXCoreCount(omx_component)) == (1)) failed.",
            __FUNCTION__, FILENAME_ONLY, __LINE__);
        sprdOmxCoreUnlockFunc();
        return OMX_ErrorUndefined;
    }
    decOMXCore(omx_component, g_omxCore);
    if (libHandle != nullptr) {
        // Do not dlclose in OMX_FreeHandle; defer to OMX_Deinit for safety.
        QueueLibHandleForDeferredClose(libHandle);
        OMX_LOGW("defer dlclose in OMX_FreeHandle for stability: %p", libHandle);
    }
    omx_component = nullptr;
    SprdOmxLogFunc("-");
    sprdOmxCoreUnlockFunc();
    return OMX_ErrorNone;
}
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_SetupTunnel(
    OMX_HANDLETYPE output,
    OMX_U32 outport,
    OMX_HANDLETYPE input,
    OMX_U32 inport)
{
    SprdOmxLogFunc("OMX_SetupTunnel");
    return OMX_ErrorNone;
}
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex)
{
    if (nIndex >= K_NUM_COMPONENTS) {
        return OMX_ErrorNoMore;
    }
    strncpy_s(cComponentName, nNameLength, K_COMPONENTS[nIndex].mName, nNameLength - 1);
    return OMX_ErrorNone;
}
OMX_API OMX_ERRORTYPE OMX_GetRolesOfComponent(
    OMX_IN      OMX_STRING compName,
    OMX_INOUT   OMX_U32 *pNumRoles,
    OMX_OUT     OMX_U8 **roles)
{
    SprdOmxLogFunc("+");
    if (!pNumRoles) {
        return OMX_ErrorBadParameter;
    }
    ssize_t componentIndex = FindComponentIndexByName(compName);
    if (componentIndex < 0) {
        OMX_LOGD("-");
        return OMX_ErrorInvalidComponentName;
    }
    if (roles) {
        CopyOmxString(reinterpret_cast<char*>(roles[0]),
            K_COMPONENTS[componentIndex].mRole, OMX_MAX_STRINGNAME_SIZE);
    }
    *pNumRoles = 1;
    OMX_LOGD("-");
    return OMX_ErrorNone;
}
OMX_API OMX_ERRORTYPE OMX_GetComponentsOfRole(
    OMX_IN      OMX_STRING role,
    OMX_INOUT   OMX_U32 *pNumComps,
    OMX_INOUT   OMX_U8  **compNames)
{
    OMX_LOGD("+");
    if (!role || !pNumComps) {
        return OMX_ErrorBadParameter;
    }
    if (!g_bInitialized) {
        SprdOmxLogFunc("-");
        return OMX_ErrorNone;
    }
    OMX_U32 maxComponents = *pNumComps;
    *pNumComps = 0;
    ssize_t componentIndex = FindComponentIndexByRole(role);
    if (componentIndex < 0) {
        SprdOmxLogFunc("-");
        return OMX_ErrorNone;
    }
    if (compNames && maxComponents <= *pNumComps) {
        SprdOmxLogFunc("-");
        return OMX_ErrorBadParameter;
    }
    if (compNames) {
        CopyOmxString(reinterpret_cast<char*>(compNames[*pNumComps]),
            K_COMPONENTS[componentIndex].mName, OMX_MAX_STRINGNAME_SIZE);
    }
    ++(*pNumComps);
    SprdOmxLogFunc("-");
    return OMX_ErrorNone;
}
