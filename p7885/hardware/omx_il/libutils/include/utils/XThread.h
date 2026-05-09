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
#ifndef UNISOC_XTHREAD_H
#define UNISOC_XTHREAD_H
#include <pthread.h>
namespace OHOS {
namespace OMX {
struct XThread {
public:
    XThread();
    virtual ~XThread();
    virtual void start();
    void requestExit();
    virtual void stop();
    void setName(const char *name);
protected:
    bool isExit();
    virtual bool loop()
    {
        return false;
    }
private:
    static void *threadFun(void *val);
    bool exit;
    pthread_t pt;
    char *mName;
};
};  // namespace OMX
};  // namespace OHOS
#endif //UNISOC_XTHREAD_H
