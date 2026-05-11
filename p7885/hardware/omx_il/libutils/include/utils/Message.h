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
#ifndef UNISOC_MESSAGE_H_
#define UNISOC_MESSAGE_H_
#include <cstdint>
#include "OMX_looper.h"
namespace OHOS {
namespace OMX {
struct Handler;
struct Message {
public:
    Message(uint32_t what, Handler *handler);
    void setWhat(uint32_t what);
    int what() const;
    void setInt32(const char *name, int32_t value);
    void setInt64(const char *name, int64_t value);
    void setFloat(const char *name, float value);
    void setDouble(const char *name, double value);
    void setPointer(const char *name, void *value);
    bool findInt32(const char *name, int32_t *value) const;
    bool findInt64(const char *name, int64_t *value) const;
    bool findFloat(const char *name, float *value) const;
    bool findDouble(const char *name, double *value) const;
    bool findPointer(const char *name, void **value) const;
    status_t post(int64_t delayUs = 0);
public:
    ~Message()
    {
        clear();
    }
private:
    friend struct Looper;
    Handler *mHandler;
    enum Type {
        kTypeInt32,
        kTypeInt64,
        kTypeFloat,
        kTypeDouble,
        kTypePointer,
    };
    struct Item {
        Item();
        union {
            int32_t int32Value;
            int64_t int64Value;
            float floatValue;
            double doubleValue;
            void *ptrValue;
        } u;
        const char *mName;
        uint32_t mNameLength;
        Type mType;
        void setName(const char *name, uint32_t len);
    };
    uint32_t mWhat;
    enum {
        kMaxNumItems = 64
    };
    Item mItems[kMaxNumItems];
    uint32_t mNumItems;
    Item *allocateItem(const char *name);
    void freeItemValue(Item *item);
    uint32_t findItemIndex(const char *name, uint32_t len) const;
    const Item *findItem(const char *name, Type type) const;
    void clear();
    void deliver();
};
};  // namespace OMX
};  // namespace OHOS
#endif //__UNISOC_MESSAGE_H_
