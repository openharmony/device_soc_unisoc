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

#include "drm_vsync_worker.h"
#include <chrono>
#include "display_common.h"
#include "drm_device.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
DrmVsyncWorker::DrmVsyncWorker() {}

int32_t DrmVsyncWorker::Init(int fd)
{
    DISPLAY_CHK_RETURN((fd < 0), DISPLAY_FAILURE, DISPLAY_LOGE("the fd is invalid"));
    mDrmFd = fd;
    DISPLAY_LOGD("the drm fd is %{public}d", fd);
    mThread = std::make_unique<std::thread>([this]() { WorkThread(); });
    DISPLAY_CHK_RETURN((mThread == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("can not create thread"));
    mRunning = true;
    mEnable = true;
    return DISPLAY_SUCCESS;
}

DrmVsyncWorker &DrmVsyncWorker::GetInstance()
{
    static DrmVsyncWorker instance;
    static std::once_flag once;
    std::call_once(once, [&]() {
        int ret = instance.Init(DrmDevice::GetDrmFd());
        if (ret != DISPLAY_SUCCESS) {
            DISPLAY_LOGE("Vsync Worker Init failed");
        }
    });
    return instance;
}

DrmVsyncWorker::~DrmVsyncWorker()
{
    DISPLAY_LOGD();
    {
        std::lock_guard<std::mutex> lg(mMutex);
        mRunning = false;
    }
    DISPLAY_LOGD();
    mCondition.notify_one();
    if (mThread != nullptr) {
        mThread->join();
    }
    DISPLAY_LOGD();
}

bool DrmVsyncWorker::WaitSignalAndCheckRuning()
{
    std::unique_lock<std::mutex> ul(mMutex);
    mCondition.wait(ul, [this]() { return (mEnable || !mRunning); });
    return mRunning;
}


uint64_t DrmVsyncWorker::WaitNextVBlank(unsigned int &sq)
{
    constexpr uint64_t secToNsec = 1000 * 1000 * 1000;
#if !defined(SPRD_7863) || !SPRD_7863
#ifdef HIHOPE_OS_DEBUG
    HITRACE_METER_NAME(HITRACE_TAG_GRAPHIC_AGP, "DrmVsyncWorker::WaitNextVBlank");
#endif
    constexpr uint64_t usecToNsec = 1000;
    drmVBlank vblank = {
        .request =
            drmVBlankReq {
                .type = DRM_VBLANK_RELATIVE,
                .sequence = 1,
                .signal = 0,
            }
    };
    /* The drmWaitVBlank need set the crtc pipe when there are multi crtcs in the system. */
    int ret = drmWaitVBlank(mDrmFd, &vblank);
    DISPLAY_CHK_RETURN((ret < 0), 0,
        DISPLAY_LOGE("wait vblank failed ret : %{public}d errno %{public}d mEnable %{public}d", ret, errno, mEnable));
    sq = vblank.reply.sequence;
    return static_cast<uint64_t>(vblank.reply.tval_sec * secToNsec + vblank.reply.tval_usec * usecToNsec);
#endif
}


void DrmVsyncWorker::EnableVsync(bool enable)
{
    DISPLAY_LOGD();
    {
        std::lock_guard<std::mutex> lg(mMutex);
        mEnable = enable;
#ifdef HIHOPE_OS_DEBUG
        CountTrace(HITRACE_TAG_GRAPHIC_AGP, "EnableVsync", mEnable);
#endif
    }
    mCondition.notify_one();
}

void DrmVsyncWorker::WorkThread()
{
    DISPLAY_LOGD();
    unsigned int seq = 0;
    uint64_t time = 0;
#ifdef HIHOPE_OS_DEBUG
    const std::string thread_name = "DrmVsyncWorker";
    constexpr uint32_t MAX_THREAD_NAME_LEN = 16;
    pthread_setname_np(pthread_self(), thread_name.substr(0, MAX_THREAD_NAME_LEN - 1).c_str());
#endif
    while (WaitSignalAndCheckRuning()) {
        // wait the vblank
        time = WaitNextVBlank(seq);
        if (mCallBack != nullptr) {
            if (mEnable) {
#ifdef HIHOPE_OS_DEBUG
                // from DEVICE to HDI(HdiScreen::OnVsync)
                HITRACE_METER_NAME(HITRACE_TAG_GRAPHIC_AGP, "VsyncCallBack->OnVsync");
#endif
                mCallBack->Vsync(seq, time);
            }
        } else {
            DISPLAY_LOGD("the callbac is nullptr");
        }
    }
}

void DrmVsyncWorker::ReqesterVBlankCb(std::shared_ptr<VsyncCallBack> &cb)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN_NOT_VALUE((cb == nullptr), DISPLAY_LOGE("the VBlankCallback is nullptr "));
    mCallBack = cb;
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
