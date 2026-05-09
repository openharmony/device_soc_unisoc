/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HDI_DRM_COMPOSITION_H
#define HDI_DRM_COMPOSITION_H
#include <vector>
#include <memory>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "drm_device.h"
#include "hdi_composer.h"
#include "hdi_device_common.h"
#include "hdi_drm_layer.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
class AtomicReqPtr {
public:
    explicit AtomicReqPtr(drmModeAtomicReqPtr ptr) : mPtr(ptr) {}
    virtual ~AtomicReqPtr()
    {
        if (mPtr != nullptr) {
            drmModeAtomicFree(mPtr);
        }
    }
    drmModeAtomicReqPtr Get() const
    {
        return mPtr;
    }

private:
    drmModeAtomicReqPtr mPtr;
};

class HdiDrmComposition : public HdiComposition {
public:
    HdiDrmComposition(const std::shared_ptr<DrmConnector> &connector, const std::shared_ptr<DrmCrtc> &crtc,
        const std::shared_ptr<DrmDevice> &drmDevice);
    virtual ~HdiDrmComposition() {}
    int32_t Init();
    int32_t SetLayers(std::vector<HdiLayer *> &layers, HdiLayer &clientLayer);
    int32_t Apply(bool modeSet);
    int32_t UpdateMode(std::unique_ptr<DrmModeBlock> &modeBlock, const drmModeAtomicReq &pset);
    int32_t powerOff = 0;
    HdiLayer *mClientLayer;

private:
    int32_t ApplyPlane(HdiDrmLayer &layer, DrmPlane &drmPlane, drmModeAtomicReqPtr pset) const;
    void UpdateFps();
    void SetPlaneProperties(DrmPlane &drmPlane, drmModeAtomicReqPtr pset, HdiDrmLayer &layer) const;
    int32_t PrepareAtomicReq(AtomicReqPtr *atomicReqPtr, uint64_t *crtcOutFence) const;
    void UpdateReleaseFences(uint64_t crtcOutFence);
    std::shared_ptr<DrmDevice> mDrmDevice;
    std::shared_ptr<DrmConnector> mConnector;
    std::shared_ptr<DrmCrtc> mCrtc;
    std::vector<std::shared_ptr<DrmPlane>> mPrimPlanes;
    std::vector<std::shared_ptr<DrmPlane>> mOverlayPlanes;
    std::vector<std::shared_ptr<DrmPlane>> mPlanes;
    int frameNum = 0;
    uint64_t frameTimeStart = 0;
    uint64_t frameTimeEnd = 0;
};
} // OHOS
} // HDI
} // DISPLAY

#endif // HDI_DRM_COMPOSITION_H