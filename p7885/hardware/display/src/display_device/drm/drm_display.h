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

#ifndef DRM_DISPLAY_H
#define DRM_DISPLAY_H
#include <unordered_map>
#include <memory>
#include <mutex>

#include "drm_connector.h"
#include "drm_crtc.h"
#include "drm_device.h"
#include "drm_plane.h"
#include "hdi_composer.h"
#include "hdi_drm_composition.h"
#include "display_adapter_interface.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
class DrmDisplay : public HdiDisplay {
public:
    DrmDisplay(const std::shared_ptr<DrmConnector> &connector, const std::shared_ptr<DrmCrtc> &crtc,
        const std::shared_ptr<DrmDevice> &drmDevice);

    ~DrmDisplay() override;

    int32_t Init() const override;
    int32_t GetDisplayCapability(DisplayCapability *info) const override;
    int32_t GetDisplaySupportedModes(uint32_t *num, DisplayModeInfo *modes) const override;
    int32_t GetDisplayMode(uint32_t *modeId) const override;
    int32_t SetDisplayMode(uint32_t modeId) const override;
    int32_t GetDisplayPowerStatus(DispPowerStatus *status) const override;
    int32_t SetDisplayPowerStatus(DispPowerStatus status) override;
    int32_t GetDisplayBacklight(uint32_t *value) const override;
    int32_t SetDisplayBacklight(uint32_t value) const override;
    int32_t ChosePreferenceMode() const;
    int32_t RegDisplayVBlankCallback(VBlankCallback cb, const void *data) override;
    int32_t WaitForVBlank(uint64_t *ns) override;
    bool IsConnected() const override;
    int32_t SetDisplayVsyncEnabled(bool enabled) override;
    HdiDrmComposition *GetDrmComposition();
    int32_t AllocMem(const AllocInfo& info, BufferHandle*& handle) const;

protected:
    std::unique_ptr<HdiLayer> CreateHdiLayer(LayerType type) const override;

private:
    int32_t mStatus = POWER_STATUS_ON;
    int32_t PushFirstFrame() const;
    int32_t ConvertToHdiPowerState(uint32_t drmPowerState, DispPowerStatus &hdiPowerState) const;
    int32_t ConvertToDrmPowerState(DispPowerStatus hdiPowerState, uint32_t &drmPowerState) const;
    std::shared_ptr<DrmDevice> mDrmDevice;
    mutable std::shared_ptr<DrmConnector> mConnector;
    std::shared_ptr<DrmCrtc> mCrtc;
};
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY

#endif // HDI_DISPLAY_H
