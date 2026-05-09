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

#include <sys/mman.h>
#include <sys/time.h>

#include <cstdio>
#include <cstring>
#define TIME_BASE 1000
#define COUNT_FRAMES_NUM (300)
constexpr uint32_t DRM_MODE_SRC_POS_SHIFT = 16;

#include <cerrno>

#include "hdi_drm_composition.h"
#include "hdi_drm_layer.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
HdiDrmComposition::HdiDrmComposition(const std::shared_ptr<DrmConnector>& connector,
                                     const std::shared_ptr<DrmCrtc>& crtc,
                                     const std::shared_ptr<DrmDevice>& drmDevice)
    : mDrmDevice(drmDevice),
      mConnector(connector),
      mCrtc(crtc)
{
    DISPLAY_LOGD();
}

int32_t HdiDrmComposition::Init()
{
    DISPLAY_LOGD();
    mPrimPlanes.clear();
    mOverlayPlanes.clear();
    mPlanes.clear();
    DISPLAY_CHK_RETURN((mCrtc == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("crtc is null"));
    DISPLAY_CHK_RETURN((mConnector == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("connector is null"));
    DISPLAY_CHK_RETURN((mDrmDevice == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("drmDevice is null"));

    mPrimPlanes = mDrmDevice->GetDrmPlane(mCrtc->GetPipe(), DRM_PLANE_TYPE_PRIMARY);
    mOverlayPlanes = mDrmDevice->GetDrmPlane(mCrtc->GetPipe(), DRM_PLANE_TYPE_OVERLAY);
    DISPLAY_CHK_RETURN((mPrimPlanes.size() == 0), DISPLAY_FAILURE, DISPLAY_LOGE("has no primary plane"));

    mPlanes.insert(mPlanes.end(), mPrimPlanes.begin(), mPrimPlanes.end());
    mPlanes.insert(mPlanes.end(), mOverlayPlanes.begin(), mOverlayPlanes.end());

    DISPLAY_LOGI(
        "HdiDrmComposition::Init: pipe=%{public}d, primary=%{public}zu, overlay=%{public}zu, total=%{public}zu",
        mCrtc->GetPipe(),
        mPrimPlanes.size(),
        mOverlayPlanes.size(),
        mPlanes.size());

    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::SetLayers(std::vector<HdiLayer*>& layers, HdiLayer& clientLayer)
{
    // now we do not surpport present direct
    DISPLAY_LOGD();
    mCompLayers.clear();
    mClientLayer = &clientLayer;
    if (clientLayer.GetAcceleratorType() == ACCELERATOR_DPU) {
        mCompLayers.push_back(mClientLayer);
    }
    for (uint32_t i = 0; i < layers.size(); i++) {
        if (layers[i]->GetAcceleratorType() == ACCELERATOR_DPU) {
            mCompLayers.push_back(layers[i]);
        }
    }
    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::ApplyPlane(HdiDrmLayer& layer, DrmPlane& drmPlane, drmModeAtomicReqPtr pset) const
{
#ifdef HIHOPE_OS_DEBUG
    HITRACE_METER(HITRACE_TAG_GRAPHIC_AGP);
#endif
    // set fence in
    int ret;
    int fenceFd = layer.GetAcquireFenceFd();
    int propId = drmPlane.GetPropFenceInId();
    DISPLAY_LOGD();
    if (propId != 0) {
        DISPLAY_LOGD("set the fence in prop");
        if (fenceFd >= 0) {
            ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), propId, fenceFd);
            DISPLAY_LOGD("set the IfenceProp plane id %{public}d, propId %{public}d, fenceFd %{public}d",
                         drmPlane.GetId(),
                         propId,
                         fenceFd);
            DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set IN_FENCE_FD failed"));
        }
    }

    // set fb id
    DrmGemBuffer* gemBuffer = layer.GetGemBuffer();
    DISPLAY_CHK_RETURN((gemBuffer == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("current gemBuffer is nullptr"));
    DISPLAY_CHK_RETURN((!gemBuffer->IsValid()), DISPLAY_FAILURE, DISPLAY_LOGE("the DrmGemBuffer is invalid"));
    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropFbId(), gemBuffer->GetFbId());
    DISPLAY_LOGD("set the fb planeid %{public}d, propId %{public}d, fbId %{public}d",
                 drmPlane.GetId(),
                 drmPlane.GetPropFbId(),
                 gemBuffer->GetFbId());
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set fb id fialed errno : %{public}d", errno));

    // set crtc id
    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropCrtcId(), mCrtc->GetId());
    DISPLAY_LOGD("set the crtc planeId %{public}d, propId %{public}d, crtcId %{public}d",
                 drmPlane.GetId(),
                 drmPlane.GetPropCrtcId(),
                 mCrtc->GetId());
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set crtc id fialed errno : %{public}d", errno));

    DrmMode mode;
    mConnector->GetModeFromId(mCrtc->GetActiveModeId(), &mode);
    if (&layer == mClientLayer) {
        IRect tmpRect = {0, 0, mode.GetModeInfoPtr()->hdisplay, mode.GetModeInfoPtr()->vdisplay};
        layer.SetLayerCrop(&tmpRect);
        layer.SetLayerRegion(&tmpRect);
        static LayerAlpha hdiAlpha = {.enGlobalAlpha = 1,
                                      .enPixelAlpha = 0,
                                      .alpha0 = 255,
                                      .alpha1 = 255,
                                      .gAlpha = 255};
        layer.SetLayerAlpha(&hdiAlpha);
    }
    // fl0414++ set property_crtc_x ...
    SetPlaneProperties(drmPlane, pset, layer);

    return DISPLAY_SUCCESS;
}

void HdiDrmComposition::SetPlaneProperties(DrmPlane &drmPlane, drmModeAtomicReqPtr pset, HdiDrmLayer &layer) const
{
    IRect rect = layer.GetLayerDisplayRect();
    IRect crop = layer.GetLayerCrop();

    drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.propertyCrtcX, rect.x);
    drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.propertyCrtcY, rect.y);
    drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.propertyCrtcW, rect.w);
    drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.propertyCrtcH, rect.h);
    drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.propertySrcX, (uint64_t)crop.x << DRM_MODE_SRC_POS_SHIFT);
    drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.propertySrcY, (uint64_t)crop.y << DRM_MODE_SRC_POS_SHIFT);
    drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.propertySrcW, (uint64_t)crop.w << DRM_MODE_SRC_POS_SHIFT);
    drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.propertySrcH, (uint64_t)crop.h << DRM_MODE_SRC_POS_SHIFT);

    uint64_t rotation = DRM_MODE_ROTATE_0;
    switch (layer.GetTransFormType()) {
        case ROTATE_90:
            rotation = DRM_MODE_ROTATE_270;
            break;
        case ROTATE_180:
            rotation = DRM_MODE_ROTATE_180;
            break;
        case ROTATE_270:
            rotation = DRM_MODE_ROTATE_90;
            break;
        default:
            rotation = DRM_MODE_ROTATE_0;
            break;
    }
    drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.propertyRotation, rotation);
    drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.propertyBlendMode, 0);

    if (layer.GetAlpha().enGlobalAlpha) {
        drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.propertyAlpha, layer.GetAlpha().gAlpha);
    }

    if ((layer.GetCurrentBuffer()->GetFormat() >= PIXEL_FMT_YUV_422_I) &&
        (layer.GetCurrentBuffer()->GetFormat() <= PIXEL_FMT_VYUY_422_PKG)) {
        drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.propertyY2rCoef, 1);
    }
}

int32_t HdiDrmComposition::UpdateMode(std::unique_ptr<DrmModeBlock>& modeBlock, const drmModeAtomicReq& pset)
{
    (void)pset;
    // set the mode
    int ret;
    DISPLAY_LOGD();
    if (mCrtc->NeedModeSet()) {
        modeBlock = mConnector->GetModeBlockFromId(mCrtc->GetActiveModeId());
        if ((modeBlock != nullptr) && (modeBlock->GetBlockId() != DRM_INVALID_ID)) {
            // set to active
            DISPLAY_LOGD("set crtc to active id %{public}d ", mCrtc->GetId());

            drmModeAtomicReq* req = drmModeAtomicAlloc();
            ret =
                drmModeAtomicAddProperty(req, mCrtc->GetId(), mCrtc->GetActivePropId(), 1); // (&pset, mCrtc->GetId(),
                                                                                            // mCrtc->GetActivePropId(),
                                                                                            // 1);
            DISPLAY_CHK_RETURN((ret < 0),
                               DISPLAY_FAILURE,
                               DISPLAY_LOGE("can not add the active prop errno %{public}d", errno));

            // set the mode id
            DISPLAY_LOGD("set the mode");
            ret = drmModeAtomicAddProperty(req, mCrtc->GetId(), mCrtc->GetModePropId(), modeBlock->GetBlockId());
            DISPLAY_LOGD("set the mode planeId %{public}d, propId %{public}d, GetBlockId: %{public}d",
                         mCrtc->GetId(),
                         mCrtc->GetModePropId(),
                         modeBlock->GetBlockId());
            DISPLAY_CHK_RETURN((ret < 0),
                               DISPLAY_FAILURE,
                               DISPLAY_LOGE("can not add the mode prop errno %{public}d", errno));

            ret = drmModeAtomicAddProperty(req, mCrtc->GetId(), mCrtc->GetFpsChangePropId(), true);
            DISPLAY_LOGD("set frame change true, planeId %{public}d, propId %{public}d",
                         mCrtc->GetId(),
                         mCrtc->GetFpsChangePropId());
            DISPLAY_CHK_RETURN((ret < 0),
                               DISPLAY_FAILURE,
                               DISPLAY_LOGE("can not add the fps change prop errno %{public}d", errno));

            ret = drmModeAtomicAddProperty(req, mConnector->GetId(), mConnector->GetPropCrtcId(), mCrtc->GetId());
            DISPLAY_LOGD("set the connector id: %{public}d, propId %{public}d, crtcId %{public}d",
                         mConnector->GetId(),
                         mConnector->GetPropCrtcId(),
                         mCrtc->GetId());
            DISPLAY_CHK_RETURN((ret < 0),
                               DISPLAY_FAILURE,
                               DISPLAY_LOGE("can not add the crtc id prop %{public}d", errno));

            int drmFd = mDrmDevice->GetDrmFd();
            drmModeAtomicCommit(drmFd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
            drmModeAtomicFree(req);

            mCrtc->CleanNeedModeSet();
        }
    }
    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::PrepareAtomicReq(AtomicReqPtr *atomicReqPtr, uint64_t *crtcOutFence) const
{
    int ret = drmModeAtomicAddProperty(atomicReqPtr->Get(), mCrtc->GetId(), mCrtc->GetOutFencePropId(),
                                       (uint64_t)crtcOutFence);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the outfence property of crtc failed "));

    for (uint32_t i = 0; i < mCompLayers.size(); i++) {
        HdiDrmLayer* layer = static_cast<HdiDrmLayer*>(mCompLayers[i]);
        auto& drmPlane = mPlanes[i];
        ret = ApplyPlane(*layer, *drmPlane, atomicReqPtr->Get());
        DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("apply plane failed"));
    }
    return DISPLAY_SUCCESS;
}

void HdiDrmComposition::UpdateReleaseFences(uint64_t crtcOutFence)
{
    for (uint32_t i = 0; i < mCompLayers.size(); i++) {
        HdiDrmLayer* layer = static_cast<HdiDrmLayer*>(mCompLayers[i]);
        if (i == 0) {
            layer->SetReleaseFence(crtcOutFence);
        } else {
            layer->SetReleaseFence(dup(crtcOutFence));
        }
    }

    if (mClientLayer->GetAcceleratorType() != ACCELERATOR_DPU) {
        mClientLayer->SetReleaseFence(dup(crtcOutFence));
    }
}

int32_t HdiDrmComposition::Apply(bool modeSet)
{
    (void)modeSet;
    uint64_t crtcOutFence = -1;
    std::unique_ptr<DrmModeBlock> modeBlock;
    int drmFd = mDrmDevice->GetDrmFd();

    DISPLAY_CHK_RETURN((mPlanes.size() < mCompLayers.size()), DISPLAY_FAILURE, DISPLAY_LOGE("plane not enough"));
    UpdateFps();

    drmModeAtomicReqPtr pset = drmModeAtomicAlloc();
    DISPLAY_CHK_RETURN((pset == nullptr), DISPLAY_NULL_PTR,
                       DISPLAY_LOGE("drm atomic alloc failed errno %{public}d", errno));
    AtomicReqPtr atomicReqPtr = AtomicReqPtr(pset);

    int32_t ret = PrepareAtomicReq(&atomicReqPtr, &crtcOutFence);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("prepare atomic req failed"));

    ret = UpdateMode(modeBlock, *(atomicReqPtr.Get()));
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("update mode failed"));

    if (powerOff == 0) {
        uint32_t flags = DRM_MODE_ATOMIC_ALLOW_MODESET | DRM_MODE_ATOMIC_NONBLOCK;
        ret = drmModeAtomicCommit(drmFd, atomicReqPtr.Get(), flags, nullptr);
        DISPLAY_CHK_RETURN((ret != 0), DISPLAY_FAILURE,
                           DISPLAY_LOGE("drmModeAtomicCommit failed %{public}d errno %{public}d", ret, errno));
    }

    UpdateReleaseFences(crtcOutFence);
    return DISPLAY_SUCCESS;
}

void HdiDrmComposition::UpdateFps()
{
    if (frameNum % COUNT_FRAMES_NUM == 0) {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        frameTimeEnd = tv.tv_sec * TIME_BASE + tv.tv_usec / TIME_BASE;
        if (frameTimeStart > 0) {
            DISPLAY_LOGE("Drm fps %{public}lu", COUNT_FRAMES_NUM * TIME_BASE / (frameTimeEnd - frameTimeStart));
        }
        frameTimeStart = frameTimeEnd;
    }
    frameNum++;
}

} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS
