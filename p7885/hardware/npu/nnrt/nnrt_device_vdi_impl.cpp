/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "innrt_device_vdi.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <unistd.h>

#include "ashmem.h"
#include "IUniAI.h"
#include "nnrt_common.h"
#include "prepared_model_impl_2_0.h"

namespace OHOS {
namespace HDI {
namespace Nnrt {
namespace V2_0 {
namespace {
constexpr const char* ASHMEM_NAME = "nnrt_unisoc_buffer";

std::string ReadExtensionString(const ModelConfig& config, const std::string& key)
{
    auto it = config.extensions.find(key);
    if (it == config.extensions.end()) {
        return {};
    }
    const auto& raw = it->second;
    if (raw.empty()) {
        return {};
    }
    size_t end = raw.size();
    while (end > 0 && raw[end - 1] == 0) {
        --end;
    }
    return std::string(reinterpret_cast<const char*>(raw.data()), end);
}

bool ReadExtensionBool(const ModelConfig& config, const std::string& key, bool defaultVal)
{
    auto it = config.extensions.find(key);
    if (it == config.extensions.end() || it->second.empty()) {
        return defaultVal;
    }
    return it->second[0] != 0;
}

class NnrtDeviceVdiImpl final : public INnrtDeviceVdi {
public:
    NnrtDeviceVdiImpl() = default;
    ~NnrtDeviceVdiImpl() override
    {
        for (auto& item : ashmemMap_) {
            if (item.second != nullptr) {
                item.second->UnmapAshmem();
                item.second->CloseAshmem();
            }
        }
        ashmemMap_.clear();
    }

    int32_t GetDeviceName(std::string& name) override
    {
        name = "P7885-NPU";
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }

    int32_t GetVendorName(std::string& name) override
    {
        name = "UNISOC";
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }

    int32_t GetDeviceType(DeviceType& deviceType) override
    {
        deviceType = DeviceType::ACCELERATOR;
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }

    int32_t GetDeviceStatus(DeviceStatus& status) override
    {
        auto uniAI = unisoc::IUniAI::Create();
        if (!uniAI) {
            status = DeviceStatus::UNKNOWN;
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
        }

        auto backendState = uniAI->GetBackendState(unisoc::UniAIBackends::NPU);
        switch (backendState) {
            case unisoc::BackendState::DEVICE_AVAILABLE:
                status = DeviceStatus::AVAILABLE;
                return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
            case unisoc::BackendState::DEVICE_BUSY:
                status = DeviceStatus::BUSY;
                return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
            case unisoc::BackendState::DEVICE_UNAVAILABLE:
            case unisoc::BackendState::DEVICE_INVALID:
            default:
                status = DeviceStatus::OFFLINE;
                return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
        }
    }

    int32_t GetSupportedOperation(const Model& model, std::vector<bool>& ops) override
    {
        ops.assign(model.nodes.size(), false);
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }

    int32_t IsFloat16PrecisionSupported(bool& isSupported) const override
    {
        isSupported = true;
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }

    int32_t IsPerformanceModeSupported(bool& isSupported) const override
    {
        isSupported = true;
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }

    int32_t IsPrioritySupported(bool& isSupported) const override
    {
        isSupported = true;
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }

    int32_t IsDynamicInputSupported(bool& isSupported) const override
    {
        isSupported = true;
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }

    int32_t PrepareModel(const Model& model, const ModelConfig& config, sptr<IPreparedModel>& preparedModel) override
    {
        (void)model;
        (void)config;
        (void)preparedModel;
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_NOT_SUPPORT);
    }

    int32_t IsModelCacheSupported(bool& isSupported) const override
    {
        isSupported = true;
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }

    int32_t PrepareModelFromModelCache(const std::vector<SharedBuffer>& modelCache, const ModelConfig& config,
        sptr<IPreparedModel>& preparedModel) override
    {
        if (modelCache.empty()) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_MODEL);
        }
        return PrepareOfflineModel(modelCache, config, preparedModel);
    }

    int32_t PrepareOfflineModel(const std::vector<SharedBuffer>& offlineModels, const ModelConfig& config,
        sptr<IPreparedModel>& preparedModel) override
    {
        if (offlineModels.empty()) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_MODEL);
        }

        const SharedBuffer& modelBuffer = offlineModels[0];
        if (modelBuffer.fd < 0 || modelBuffer.bufferSize == 0 || modelBuffer.dataSize == 0 ||
            modelBuffer.offset + modelBuffer.dataSize > modelBuffer.bufferSize) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER);
        }

        int dupFd = dup(modelBuffer.fd);
        if (dupFd < 0) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER);
        }

        sptr<Ashmem> modelAshmem = new (std::nothrow) Ashmem(dupFd, static_cast<int32_t>(modelBuffer.bufferSize));
        if (modelAshmem == nullptr) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_OUT_OF_MEMORY);
        }
        if (!modelAshmem->MapReadOnlyAshmem()) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_MEMORY_ERROR);
        }
        const void* modelPtr = modelAshmem->ReadFromAshmem(static_cast<int32_t>(modelBuffer.dataSize),
            static_cast<int32_t>(modelBuffer.offset));
        if (modelPtr == nullptr) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER);
        }

        const std::string backendPath = ReadExtensionString(config, "uniai.backend_path");
        auto uniAI = backendPath.empty() ? unisoc::IUniAI::Create() : unisoc::IUniAI::Create(backendPath.c_str());
        if (!uniAI) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
        }

        unisoc::NetworkId networkId {};
        unisoc::ModelType modelType {};
        auto loadRet = uniAI->LoadNetwork(modelPtr, static_cast<size_t>(modelBuffer.dataSize), networkId, modelType);
        if (loadRet != unisoc::Status::AI_SUCCESS) {
            return internal::ToNnrtResult(loadRet);
        }

        const bool enableCache = ReadExtensionBool(config, "uniai.enable_cache", false);
        const std::string cachePath = ReadExtensionString(config, "uniai.cache_path");
        const char* cachePathPtr = cachePath.empty() ? nullptr : cachePath.c_str();
        std::vector<unisoc::BackendId> preferentBackendList { unisoc::UniAIBackends::NPU, unisoc::UniAIBackends::CPU };
        auto compileRet = uniAI->NetworkCompile(networkId, modelType, preferentBackendList, enableCache, cachePathPtr);
        if (compileRet != unisoc::Status::AI_SUCCESS) {
            return internal::ToNnrtResult(compileRet);
        }

        preparedModel = new (std::nothrow) PreparedModelImpl(std::move(uniAI), networkId, modelAshmem);
        if (preparedModel == nullptr) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_OUT_OF_MEMORY);
        }

        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }

    int32_t AllocateBuffer(uint32_t length, SharedBuffer& buffer) override
    {
        if (length == 0) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER_SIZE);
        }

        sptr<Ashmem> ashptr = Ashmem::CreateAshmem(ASHMEM_NAME, static_cast<int32_t>(length));
        if (ashptr == nullptr) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_OUT_OF_MEMORY);
        }

        if (!ashptr->MapReadAndWriteAshmem()) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_MEMORY_ERROR);
        }

        buffer.fd = ashptr->GetAshmemFd();
        buffer.bufferSize = static_cast<uint32_t>(ashptr->GetAshmemSize());
        buffer.offset = 0;
        buffer.dataSize = length;

        ashmemMap_[buffer.fd] = ashptr;
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }

    int32_t ReleaseBuffer(const SharedBuffer& buffer) override
    {
        auto it = ashmemMap_.find(buffer.fd);
        if (it != ashmemMap_.end() && it->second != nullptr) {
            it->second->UnmapAshmem();
            it->second->CloseAshmem();
            ashmemMap_.erase(it);
        }
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }

private:
    std::map<int, sptr<Ashmem>> ashmemMap_;
};
} // namespace

extern "C" INnrtDeviceVdi* CreateNnrtDeviceVdi()
{
    return new (std::nothrow) NnrtDeviceVdiImpl();
}

extern "C" void DestroyNnrtDeviceVdi(INnrtDeviceVdi* vdi)
{
    delete vdi;
}

} // namespace V2_0
} // namespace Nnrt
} // namespace HDI
} // namespace OHOS
