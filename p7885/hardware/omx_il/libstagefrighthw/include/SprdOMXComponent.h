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
#ifndef SPRD_OMX_COMPONENT_H_
#define SPRD_OMX_COMPONENT_H_
#include <OMX_Component.h>
#include "SprdOMXComponentBase.h"
#include "sprd_omx_typedef.h"
#include <codec_omx_ext.h>
namespace OHOS {
namespace OMX {
typedef struct BufferCtrlStruct {
    uint32_t iRefCount;
    PMemIon pMem;
    int bufferFd;
    unsigned long phyAddr;
    size_t bufferSize;
    int id;
} BufferCtrlStruct;
typedef struct BufferPrivateStruct {
    MemIon* pMem;
    int bufferFd;
    unsigned long phyAddr;
    size_t bufferSize;
} BufferPrivateStruct;
typedef struct UseBufferParams {
    OMX_U32 portIndex;
    OMX_PTR appPrivate;
    OMX_U32 size;
    OMX_U8 *ptr;
    BufferPrivateStruct *bufferPrivate;
} UseBufferParams;
enum {
    K_INPUT_PORT_INDEX  = 0,
    K_OUTPUT_PORT_INDEX = 1,
    K_MAX_PORT_INDEX = 1,
};
/** HEVC Profile enum type */
typedef enum OMX_VIDEO_HEVCPROFILETYPE {
    OMX_VIDEO_HEVC_PROFILE_UNKNOWN = 0x0,
    OMX_VIDEO_HEVC_PROFILE_MAIN    = 0x1,
    OMX_VIDEO_HEVC_PROFILE_MAIN10  = 0x2,
    OMX_VIDEO_HEVC_PROFILE_MAX     = 0x7FFFFFFF
} OMX_VIDEO_HEVCPROFILETYPE;
/** HEVC Level enum type */
typedef enum OMX_VIDEO_HEVCLEVELTYPE {
    OMX_VIDEO_HEVC_LEVEL_UNKNOWN    = 0x0,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL1  = 0x1,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL1  = 0x2,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL2  = 0x4,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL2  = 0x8,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL21 = 0x10,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL21 = 0x20,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL3  = 0x40,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL3  = 0x80,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL31 = 0x100,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL31 = 0x200,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL4  = 0x400,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL4  = 0x800,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL41 = 0x1000,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL41 = 0x2000,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL5  = 0x4000,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL5  = 0x8000,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL51 = 0x10000,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL51 = 0x20000,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL52 = 0x40000,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL52 = 0x80000,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL6  = 0x100000,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL6  = 0x200000,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL61 = 0x400000,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL61 = 0x800000,
    OMX_VIDEO_HEVC_MAIN_TIER_LEVEL62 = 0x1000000,
    OMX_VIDEO_HEVC_HIGH_TIER_LEVEL62 = 0x2000000,
    OMX_VIDEO_HEVC_HIGH_TIERMAX     = 0x7FFFFFFF
} OMX_VIDEO_HEVCLEVELTYPE;
/** Structure for controlling HEVC video encoding and decoding */
typedef struct OMX_VIDEO_PARAM_HEVCTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_VIDEO_HEVCPROFILETYPE eProfile;
    OMX_VIDEO_HEVCLEVELTYPE eLevel;
} OmxVideoParamHevctype;
/** Structure to define if dependent slice segments should be used */
typedef struct OMX_VIDEO_SLICESEGMENTSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bDepedentSegments;
    OMX_BOOL bEnableLoopFilterAcrossSlices;
} OmxVideoSlicesegmentstype;
struct SprdOMXComponent : public SprdOMXComponentBase {
    enum {
        K_FENCE_TIMEOUT_MS = 1000
    };
    SprdOMXComponent(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component);
    virtual OMX_ERRORTYPE initCheck() const;
    void SetLibHandle(void *libHandle);
    void *LibHandle() const;
    virtual void PrepareForDestruction() { }
protected:
    virtual ~SprdOMXComponent();
    const char *Name() const;
    void notify(
        OMX_EVENTTYPE event,
        OMX_U32 data1, OMX_U32 data2, OMX_PTR data);
    void notifyEmptyBufferDone(OMX_BUFFERHEADERTYPE *header);
    virtual void notifyFillBufferDone(OMX_BUFFERHEADERTYPE *header);
    //OHOS add
    virtual OMX_ERRORTYPE getComponentVersion(
        OMX_STRING ComponentName, OMX_VERSIONTYPE* ComponentVersion,
        OMX_VERSIONTYPE* SpecVersion, OMX_UUIDTYPE* ComponentUUID);
    virtual OMX_ERRORTYPE sendCommand(
        OMX_COMMANDTYPE cmd, OMX_U32 param, OMX_PTR data);
    virtual OMX_ERRORTYPE getParameter(
        OMX_INDEXTYPE index, OMX_PTR pArams);
    virtual OMX_ERRORTYPE setParameter(
        OMX_INDEXTYPE index, const OMX_PTR pArams);
    virtual OMX_ERRORTYPE getConfig(
        OMX_INDEXTYPE index, OMX_PTR pArams);
    virtual OMX_ERRORTYPE setConfig(
        OMX_INDEXTYPE index, const OMX_PTR pArams);
    virtual OMX_ERRORTYPE getExtensionIndex(
        const char *name, OMX_INDEXTYPE *index);
    virtual OMX_ERRORTYPE useBuffer(
        OMX_BUFFERHEADERTYPE **buffer, const UseBufferParams &params);
    virtual OMX_ERRORTYPE allocateBuffer(
        OMX_BUFFERHEADERTYPE **buffer,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size);
    virtual OMX_ERRORTYPE freeBuffer(
        OMX_U32 portIndex,
        OMX_BUFFERHEADERTYPE *buffer);
    virtual OMX_ERRORTYPE emptyThisBuffer(
        OMX_BUFFERHEADERTYPE *buffer);
    virtual OMX_ERRORTYPE fillThisBuffer(
        OMX_BUFFERHEADERTYPE *buffer);
    virtual OMX_ERRORTYPE getState(OMX_STATETYPE *state);
    const OMX_CALLBACKTYPE *mCallbacks;
    OMX_COMPONENTTYPE *mComponent;
private:
    std::string mName;
    void *mHwLibHandle;
    static OMX_ERRORTYPE GetComponentVersionWrapper(
        OMX_HANDLETYPE Component,
        OMX_STRING ComponentName,
        OMX_VERSIONTYPE* ComponentVersion,
        OMX_VERSIONTYPE* SpecVersion,
        OMX_UUIDTYPE* ComponentUUID);
    //OHOS add
    static OMX_ERRORTYPE SendCommandWrapper(
        OMX_HANDLETYPE component,
        OMX_COMMANDTYPE cmd,
        OMX_U32 param,
        OMX_PTR data);
    static OMX_ERRORTYPE GetParameterWrapper(
    OMX_HANDLETYPE component,
        OMX_INDEXTYPE index,
        OMX_PTR pArams);
    static OMX_ERRORTYPE SetParameterWrapper(
    OMX_HANDLETYPE component,
        OMX_INDEXTYPE index,
        OMX_PTR pArams);
    static OMX_ERRORTYPE GetConfigWrapper(
    OMX_HANDLETYPE component,
        OMX_INDEXTYPE index,
        OMX_PTR pArams);
    static OMX_ERRORTYPE SetConfigWrapper(
    OMX_HANDLETYPE component,
        OMX_INDEXTYPE index,
        OMX_PTR pArams);
    static OMX_ERRORTYPE GetExtensionIndexWrapper(
    OMX_HANDLETYPE component,
        OMX_STRING name,
        OMX_INDEXTYPE *index);
    static OMX_ERRORTYPE UseBufferWrapper(
    OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE **buffer,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size,
        OMX_U8 *pTr);
    static OMX_ERRORTYPE AllocateBufferWrapper(
    OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE **buffer,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size);
    static OMX_ERRORTYPE FreeBufferWrapper(
    OMX_HANDLETYPE component,
        OMX_U32 portIndex,
        OMX_BUFFERHEADERTYPE *buffer);
    static OMX_ERRORTYPE EmptyThisBufferWrapper(
    OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE *buffer);
    static OMX_ERRORTYPE FillThisBufferWrapper(
    OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE *buffer);
    static OMX_ERRORTYPE GetStateWrapper(
    OMX_HANDLETYPE component,
        OMX_STATETYPE *state);
    SprdOMXComponent(const SprdOMXComponent &);
    SprdOMXComponent &operator=(const SprdOMXComponent &);
};
}  // namespace OMX
}  // namespace OHOS
#endif  // SPRD_OMX_COMPONENT_H_
