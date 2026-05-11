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

#include <hdi_session.h>
#include <cerrno>
#include <mutex>
#include "display_adapter_interface.h"
#include "hdf_trace.h"
#include "hdi_netlink_monitor.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
HdiSession &HdiSession::GetInstance()
{
    static HdiSession instance;
    static std::once_flag once;
    std::call_once(once, [&]() { instance.Init(); });
    return instance;
}

void HdiSession::Init()
{
    DISPLAY_LOGD();
    mHdiDevices = HdiDeviceInterface::DiscoveryDevice();
    DISPLAY_LOGD("devices size %{public}zd", mHdiDevices.size());
    mHdiDisplays.clear();
    for (auto device : mHdiDevices) {
        auto displays = device->DiscoveryDisplay();
        mHdiDisplays.insert(displays.begin(), displays.end());
    }
}

int32_t HdiSession::RegHotPlugCallback(HotPlugCallback callback, void *data)
{
    DISPLAY_CHK_RETURN((callback == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("the callback is nullptr"));
    mHotPlugCallBacks[callback] = data;
    for (auto displayMap : mHdiDisplays) {
        auto display = displayMap.second;
        if (display->IsConnected()) {
            DoHotPlugCallback(display->GetId(), true);
        }
    }
    return DISPLAY_SUCCESS;
}

void HdiSession::DoHotPlugCallback(uint32_t devId, bool connect)
{
    DISPLAY_LOGD();
    for (const auto &callback : mHotPlugCallBacks) {
        callback.first(devId, connect, callback.second);
    }
}
} // OHOS
} // HDI
} // DISPLAY