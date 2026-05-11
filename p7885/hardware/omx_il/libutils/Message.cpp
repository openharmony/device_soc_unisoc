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
#include <utils/omx_log.h>
#include "Message.h"
#include <utils/Handler.h>
namespace OHOS {
namespace OMX {
Message::Message(uint32_t what, Handler *handler)
    : mHandler(handler),
    mWhat(what),
    mNumItems(0) {}
void Message::setWhat(uint32_t what)
{
    mWhat = what;
}
int Message::what() const
{
    return mWhat;
}
void Message::setInt32(const char *name, int32_t value)
{
    Item *item = allocateItem(name);
    item->mType = kTypeInt32;
    item->u.int32Value = value;
}
bool Message::findInt32(const char *name, int32_t *value) const
{
    const Item *item = findItem(name, kTypeInt32);
    if (item) {
        *value = item->u.int32Value;
        return true;
    }
    return false;
}
void Message::setInt64(const char *name, int64_t value)
{
    Item *item = allocateItem(name);
    item->mType = kTypeInt64;
    item->u.int64Value = value;
}
bool Message::findInt64(const char *name, int64_t *value) const
{
    const Item *item = findItem(name, kTypeInt64);
    if (item) {
        *value = item->u.int64Value;
        return true;
    }
    return false;
}
void Message::setFloat(const char *name, float value)
{
    Item *item = allocateItem(name);
    item->mType = kTypeFloat;
    item->u.floatValue = value;
}
bool Message::findFloat(const char *name, float *value) const
{
    const Item *item = findItem(name, kTypeFloat);
    if (item) {
        *value = item->u.floatValue;
        return true;
    }
    return false;
}
void Message::setDouble(const char *name, double value)
{
    Item *item = allocateItem(name);
    item->mType = kTypeDouble;
    item->u.doubleValue = value;
}
bool Message::findDouble(const char *name, double *value) const
{
    const Item *item = findItem(name, kTypeDouble);
    if (item) {
        *value = item->u.doubleValue;
        return true;
    }
    return false;
}
void Message::setPointer(const char *name, void *value)
{
    Item *item = allocateItem(name);
    item->mType = kTypePointer;
    item->u.ptrValue = value;
}
bool Message::findPointer(const char *name, void **value) const
{
    const Item *item = findItem(name, kTypePointer);
    if (item) {
        *value = item->u.ptrValue;
        return true;
    }
    return false;
}
Message::Item *Message::allocateItem(const char *name)
{
    uint32_t len = strlen(name);
    uint32_t i = findItemIndex(name, len);
    Message::Item *item;
    if (i < mNumItems) {
        item = &mItems[i];
        freeItemValue(item);
    } else {
        if (mNumItems >= kMaxNumItems) {
            OMX_LOGE("[%{public}s@%{public}s:%{public}d] OMX_CHECK(mNumItems < kMaxNumItems) failed.",
                __FUNCTION__, FILENAME_ONLY, __LINE__);
            return nullptr;
        }
        i = mNumItems++;
        item = &mItems[i];
        item->mType = kTypeInt32;
        item->setName(name, len);
    }
    return item;
}
void Message::freeItemValue(Message::Item *item)
{
    item->mType = kTypeInt32;
}
Message::Item::Item() : mName(nullptr),
                        mNameLength(0),
                        mType(kTypeInt32)
{
}
void Message::Item::setName(const char *name, uint32_t len)
{
    if (name == nullptr) {
        OMX_LOGE("setName: name is null (len=%u)", len);
        mNameLength = 0;
        return;
    }
    mNameLength = len;
    mName = new (std::nothrow) char[len + 1];
    if (mName == nullptr) {
        OMX_LOGE("setName: alloc memory failed (len=%u)", len);
        mNameLength = 0;
        return;
    }
    errno_t ret = memmove_s(
        const_cast<void*>(static_cast<const void*>(mName)),                // dest: char* → void*
        static_cast<size_t >(len + 1),           // destsz: uint32_t → size_t
        static_cast<const void*>(name),          // src: const char* → const void*
        static_cast<size_t >(len + 1));          // count: uint32_t → size_t
    if (ret != 0) {
        OMX_LOGE("setName: memmove_s failed (ret=%d, len=%u)", ret, len);
        delete[] mName;
        mName = nullptr;
        mNameLength = 0;
    }
}
uint32_t Message::findItemIndex(const char *name, uint32_t len) const
{
    uint32_t i = 0;
    for (; i < mNumItems; i++) {
        if (len != mItems[i].mNameLength) {
            continue;
        }
        if (!memcmp(mItems[i].mName, name, len)) {
            break;
        }
    }
    return i;
}
void Message::clear()
{
    for (uint32_t i = 0; i < mNumItems; ++i) {
        Item *item = &mItems[i];
        delete[] item->mName;
        item->mName = nullptr;
        freeItemValue(item);
    }
    mNumItems = 0;
}
const Message::Item *Message::findItem(const char *name, Type type) const
{
    uint32_t i = findItemIndex(name, strlen(name));
    if (i < mNumItems) {
        const Item *item = &mItems[i];
        return item->mType == type ? item : nullptr;
    }
    return nullptr;
}
void Message::deliver()
{
    if (mHandler != nullptr) {
        mHandler->onMessageReceived(this);
    }
}
status_t Message::post(int64_t delayUs)
{
    if (mHandler == nullptr || mHandler->getLooper() == nullptr) {
        OMX_LOGW("failed to post message as target looper for handler is gone.");
        return STATUS_NAME_NOT_FOUND;
    }
    mHandler->getLooper()->post(this, delayUs);
    return STATUS_OK;
}
};  // namespace OMX
};  // namespace OHOS
