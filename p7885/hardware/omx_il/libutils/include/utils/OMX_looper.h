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
#ifndef UNISOC_LOOPER_H_
#define UNISOC_LOOPER_H_
#include <cstdint>
#include <list>
#include <mutex>
#include <condition_variable>
#include "Errors.h"
#include "XThread.h"
namespace OHOS {
namespace OMX {
struct Message;
struct Handler;
struct Looper : public XThread {
public:
    void start() override;
    void stop() override;
private:
    friend struct Message;
    struct Event {
        int64_t mWhenUs;
        Message *mMessage;
    };
    std::mutex mLock;
    std::condition_variable mQueueChangedCondition;
    std::list<Event> mEventQueue;
    void post(Message *msg, int64_t delayUs);
    bool loop() override;
    static int64_t GetNowUs();
};
};  // namespace OMX
};  // namespace OHOS
#endif //UNISOC_LOOPER_H_
