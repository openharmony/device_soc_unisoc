/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "display_buffer_vdi_impl.h"
#include <cstdlib>
#include "cinttypes"
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "securec.h"
#include "display_log.h"
#include "allocator.h"
#include "allocator_manager.h"
#include "hdf_base.h"
#include "v1_0/display_composer_type.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
using namespace OHOS::HDI::Display::Composer::V1_0;
using namespace OHOS::HDI::Display::Buffer::V1_2;

namespace {
std::mutex g_metadataMutex;
std::unordered_map<int32_t, std::unordered_map<uint32_t, std::vector<uint8_t>>> g_metadataMap;
} // namespace

void DumpSetMetadata(const BufferHandle& handle, uint32_t key, const std::vector<uint8_t>& value)
{
    std::string valueStr;
    for (size_t i = 0; i < value.size(); i++) {
        char tmp[4] = { 0 };
        if (sprintf_s(tmp, sizeof(tmp), "%02x ", value[i]) >= 0) {
            valueStr += tmp;
        }
    }
    DISPLAY_LOGI("SetMetadata fd=%{public}d key=%{public}u valueSize=%{public}zu value=[%{public}s]",
        handle.fd, key, value.size(), valueStr.c_str());
}

DisplayBufferVdiImpl::DisplayBufferVdiImpl()
{
    int ret = AllocatorManager::GetInstance().Init();
    if (ret != HDF_SUCCESS) {
        DISPLAY_LOGE("gbm construct failed");
    }
}

DisplayBufferVdiImpl::~DisplayBufferVdiImpl()
{
    AllocatorManager::GetInstance().DeInit();
}

int32_t DisplayBufferVdiImpl::AllocMem(const AllocInfo& info, BufferHandle*& handle) const
{
    return AllocatorManager::GetInstance().AllocMem(info, &handle);
}

void DisplayBufferVdiImpl::FreeMem(const BufferHandle& handle) const
{
    size_t copySize = sizeof(BufferHandle) + sizeof(int32_t) * (handle.reserveFds + handle.reserveInts);
    BufferHandle *handleCopy = static_cast<BufferHandle *>(malloc(copySize));
    if (handleCopy == nullptr) {
        DISPLAY_LOGE("malloc handle copy failed");
        return;
    }
    errno_t ret = memcpy_s(handleCopy, copySize, &handle, copySize);
    if (ret != EOK) {
        DISPLAY_LOGE("copy handle failed");
        free(handleCopy);
        return;
    }
    AllocatorManager::GetInstance().FreeMem(handleCopy);
}

void* DisplayBufferVdiImpl::Mmap(const BufferHandle& handle) const
{
    return AllocatorManager::GetInstance().Mmap(const_cast<BufferHandle *>(&handle));
}

int32_t DisplayBufferVdiImpl::Unmap(const BufferHandle& handle) const
{
    return AllocatorManager::GetInstance().Unmap(const_cast<BufferHandle *>(&handle));
}

int32_t DisplayBufferVdiImpl::FlushCache(const BufferHandle& handle) const
{
    return AllocatorManager::GetInstance().FlushCache(const_cast<BufferHandle *>(&handle));
}

int32_t DisplayBufferVdiImpl::InvalidateCache(const BufferHandle& handle) const
{
    return AllocatorManager::GetInstance().InvalidateCache(const_cast<BufferHandle *>(&handle));
}

int32_t DisplayBufferVdiImpl::IsSupportedAlloc(const std::vector<VerifyAllocInfo>& infos,
    std::vector<bool>& supporteds) const
{
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayBufferVdiImpl::RegisterBuffer(const BufferHandle& handle)
{
    DISPLAY_LOGE("%s is not supported", __func__);
    return DISPLAY_NOT_SUPPORT;
}
int32_t DisplayBufferVdiImpl::SetMetadata(const BufferHandle& handle, uint32_t key, const std::vector<uint8_t>& value)
{
    DumpSetMetadata(handle, key, value);

    std::lock_guard<std::mutex> lock(g_metadataMutex);
    g_metadataMap[handle.fd][key] = value;
    return HDF_SUCCESS;
}
int32_t DisplayBufferVdiImpl::GetMetadata(const BufferHandle& handle, uint32_t key, std::vector<uint8_t>& value)
{
    std::lock_guard<std::mutex> lock(g_metadataMutex);
    auto iterFd = g_metadataMap.find(handle.fd);
    if (iterFd == g_metadataMap.end()) {
        return DISPLAY_PARAM_ERR;
    }
    auto iterKey = iterFd->second.find(key);
    if (iterKey == iterFd->second.end()) {
        return DISPLAY_PARAM_ERR;
    }
    value = iterKey->second;
    return HDF_SUCCESS;
}
int32_t DisplayBufferVdiImpl::ListMetadataKeys(const BufferHandle& handle, std::vector<uint32_t>& keys)
{
    std::lock_guard<std::mutex> lock(g_metadataMutex);
    auto iterFd = g_metadataMap.find(handle.fd);
    if (iterFd == g_metadataMap.end()) {
        return DISPLAY_PARAM_ERR;
    }
    keys.clear();
    for (auto& item : iterFd->second) {
        keys.push_back(item.first);
    }
    return HDF_SUCCESS;
}

int32_t DisplayBufferVdiImpl::EraseMetadataKey(const BufferHandle& handle, uint32_t key)
{
    std::lock_guard<std::mutex> lock(g_metadataMutex);
    auto iterFd = g_metadataMap.find(handle.fd);
    if (iterFd == g_metadataMap.end()) {
        return DISPLAY_PARAM_ERR;
    }
    iterFd->second.erase(key);
    return HDF_SUCCESS;
}

int32_t DisplayBufferVdiImpl::GetImageLayout(const BufferHandle& handle,
    Display::Buffer::V1_2::ImageLayout& layouts) const
{
    DISPLAY_LOGE("%s is not supported", __func__);
    return DISPLAY_NOT_SUPPORT;
}

extern "C" IDisplayBufferVdi* CreateDisplayBufferVdi()
{
    return new DisplayBufferVdiImpl();
}

extern "C" void DestroyDisplayBufferVdi(IDisplayBufferVdi* vdi)
{
    delete vdi;
}

extern "C" int32_t AllocMemVdi(const AllocInfo& info, BufferHandle*& handle)
{
    std::shared_ptr<IDisplayBufferVdi> hdiImpl = std::make_shared<DisplayBufferVdiImpl>();

    return hdiImpl->AllocMem(info, handle);
}
} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS
