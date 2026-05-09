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
#ifndef UNISOC_HANDLER_H_
#define UNISOC_HANDLER_H_
namespace OHOS {
namespace OMX {
struct Message;
struct Looper;
struct Handler {
public:
    virtual ~Handler() { }
    void setLooper(Looper *looper)
    {
        mLooper = looper;
    }
    Looper *getLooper()
    {
        return mLooper;
    }
    virtual void onMessageReceived(const Message *msg) = 0;
private:
    Looper *mLooper;
};
};    // namespace OMX
};    // namespace OHOS
#endif //UNISOC_HANDLER_H_
