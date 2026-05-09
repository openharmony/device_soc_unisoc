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

#include "drm_encoder.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
DrmEncoder::DrmEncoder(drmModeEncoder e)
{
    mEncoderId = e.encoder_id;
    mCrtcId = e.crtc_id;
    mPossibleCrtcs = e.possible_crtcs;
    DISPLAY_LOGI("DrmEncoder: id=%{public}d, crtc_id=%{public}d, possible_crtcs=0x%{public}x",
        mEncoderId, mCrtcId, mPossibleCrtcs);
}

int32_t DrmEncoder::PickIdleCrtcId(const IdMapPtr<DrmCrtc> &crtcs, uint32_t *crtcId)
{
    DISPLAY_CHK_RETURN((crtcId == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("crtcId is null"));
    DISPLAY_LOGI("PickIdleCrtcId: encoder %{public}d, current crtc_id=%{public}d, "
        "possible_crtcs=0x%{public}x, available crtcs=%{public}zu",
        mEncoderId, mCrtcId, mPossibleCrtcs, crtcs.size());

    std::shared_ptr<DrmCrtc> crtc;

    // 首先尝试使用 encoder 当前绑定的 CRTC（如果有的话）
    if (mCrtcId != 0) {
        auto crtcIter = crtcs.find(mCrtcId);
        if (crtcIter != crtcs.end() && crtcIter->second->CanBind()) {
            crtc = crtcIter->second;
            DISPLAY_LOGI("Using encoder's current CRTC %{public}d (pipe %{public}d)",
                crtc->GetId(), crtc->GetPipe());
        }
    }

    // 如果当前 CRTC 不可用，根据 possible_crtcs 位掩码选择合适的 CRTC
    if (crtc == nullptr) {
        DISPLAY_LOGI("Searching for idle CRTC matching possible_crtcs=0x%{public}x", mPossibleCrtcs);
        for (const auto &posCrtcPair : crtcs) {
            auto &posCrtc = posCrtcPair.second;
            uint32_t pipe = posCrtc->GetPipe();
            bool canBind = posCrtc->CanBind();
            bool matches = ((1 << pipe) & mPossibleCrtcs) != 0;

            DISPLAY_LOGI("  CRTC %{public}d (pipe %{public}d): canBind=%{public}d, "
                "matches=0x%{public}x & 0x%{public}x = %{public}d",
                posCrtc->GetId(), pipe, canBind, (1 << pipe), mPossibleCrtcs, matches);

            if (canBind && matches) {
                crtc = posCrtc;
                DISPLAY_LOGI("  -> Selected CRTC %{public}d (pipe %{public}d)", crtc->GetId(), pipe);
                break;
            }
        }
    }

    DISPLAY_CHK_RETURN((crtc == nullptr), DISPLAY_FAILURE,
        DISPLAY_LOGE("encoder %{public}d can not bind to any idle crtc (possible_crtcs=0x%{public}x)",
            mEncoderId, mPossibleCrtcs));

    *crtcId = crtc->GetId();
    DISPLAY_LOGI("PickIdleCrtcId: encoder %{public}d -> CRTC %{public}d (pipe %{public}d)",
        mEncoderId, *crtcId, crtc->GetPipe());
    return DISPLAY_SUCCESS;
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
