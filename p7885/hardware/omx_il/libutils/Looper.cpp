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
#include "OMX_looper.h"
#include <utils/time_util.h>
#include <utils/Message.h>
#include "utils/omx_log.h"
#undef LOG_TAG
#define LOG_TAG "Looper"
/* Time conversion constants */
#define NANOSECONDS_PER_MICROSECOND 1000LL          /* Nanoseconds per microsecond */
#define MICROSECONDS_PER_MILLISECOND 1000LL         /* Microseconds per millisecond */
#define TIME_UNIT_MAX_DIVISOR 1000                  /* Divisor for preventing overflow in time calculations */
#define NO_DELAY 0                                  /* No delay value */
#define INFINITE_DELAY_THRESHOLD INT64_MAX          /* Threshold for infinite delay */
#define ZERO_DELAY 0                                /* Zero delay for immediate execution */
namespace OHOS {
namespace OMX {
void Looper::post(Message *msg, int64_t delayUs)
{
    std::unique_lock<std::mutex> autoLock(mLock);
    int64_t whenUs;
    if (delayUs > NO_DELAY) {
        int64_t nowUs = GetNowUs();
        whenUs = (delayUs > INFINITE_DELAY_THRESHOLD - nowUs ? INFINITE_DELAY_THRESHOLD : nowUs + delayUs);
    } else {
        whenUs = GetNowUs();
    }
    std::list<Event>::iterator it = mEventQueue.begin();
    while (it != mEventQueue.end() && (*it).mWhenUs <= whenUs) {
        ++it;
    }
    Event event;
    event.mWhenUs = whenUs;
    event.mMessage = msg;
    if (it == mEventQueue.begin()) {
        mQueueChangedCondition.notify_all();
    }
    mEventQueue.insert(it, event);
}
void Looper::start()
{
    std::unique_lock<std::mutex> autoLock(mLock);
    XThread::start();
}
void Looper::stop()
{
    {
        std::unique_lock<std::mutex> autoLock(mLock);
        requestExit();
        mQueueChangedCondition.notify_all();
    }
    XThread::stop();
}
int64_t Looper::GetNowUs()
{
    return systemTime() / NANOSECONDS_PER_MICROSECOND;
}
//run in thread
bool Looper::loop()
{
    Event event;
    {
        std::unique_lock<std::mutex> autoLock(mLock);
        if (isExit()) {
            OMX_LOGI("Looper::loop had been exit!");
            return false;
        }
        if (mEventQueue.empty()) {
            mQueueChangedCondition.wait(autoLock);
            return true;
        }
        int64_t whenUs = (*mEventQueue.begin()).mWhenUs;
        int64_t nowUs = GetNowUs();
        if (whenUs > nowUs) {
            int64_t delayUs = whenUs - nowUs;
            if (delayUs > INFINITE_DELAY_THRESHOLD / TIME_UNIT_MAX_DIVISOR) {
                delayUs = INFINITE_DELAY_THRESHOLD / TIME_UNIT_MAX_DIVISOR;
            }
            OMX_LOGV("Looper::loop mQueueChangedCondition.wait_for %lld +", delayUs * MICROSECONDS_PER_MILLISECOND);
            std::cv_status status = mQueueChangedCondition.wait_for(
                autoLock, std::chrono::milliseconds(delayUs * MICROSECONDS_PER_MILLISECOND));
            OMX_LOGV("Looper::loop mQueueChangedCondition.wait_for - %s",
                status == std::cv_status::timeout ? "timeout" : "! timeout");
            return true;
        }
        event = *mEventQueue.begin();
        mEventQueue.erase(mEventQueue.begin());
    }
    event.mMessage->deliver();
    return true;
}
};    // namespace OMX
};    // namespace OHOS
