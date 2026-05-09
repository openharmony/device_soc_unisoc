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

#include "nnrt_device_service.h"
#include <hdf_base.h>
#include <hdf_log.h>
#include <dlfcn.h>

#define HDF_LOG_TAG nnrt_device_service

namespace OHOS {
namespace HDI {
namespace Nnrt {
namespace V2_0 {
extern "C" INnrtDevice *NnrtDeviceImplGetInstance(void)
{
    return new (std::nothrow) NnrtDeviceService();
}

NnrtDeviceService::NnrtDeviceService()
    : libHandle_(nullptr),
    createVdiFunc_(nullptr),
    destroyVdiFunc_(nullptr),
    vdiImpl_(nullptr)
{
    int32_t ret = LoadVdi();
    if (ret == HDF_SUCCESS) {
        vdiImpl_ = createVdiFunc_();
        if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
            return;
        }
    } else {
        HDF_LOGE("Load nnrt device VDI failed, lib: %{public}s", NNRT_DEVICE_VDI_LIBRARY);
    }
}

NnrtDeviceService::~NnrtDeviceService()
{
    if ((destroyVdiFunc_ != nullptr) && (vdiImpl_ != nullptr)) {
        destroyVdiFunc_(vdiImpl_);
    }
    if (libHandle_ != nullptr) {
        dlclose(libHandle_);
    }
}

int32_t NnrtDeviceService::LoadVdi()
{
    const char* errStr = dlerror();
    if (errStr != nullptr) {
        HDF_LOGE("nnrt loadvdi, clear earlier dlerror: %{public}s", errStr);
    }
    libHandle_ = dlopen(NNRT_DEVICE_VDI_LIBRARY, RTLD_LAZY);
    if (libHandle_ == nullptr) {
        errStr = dlerror();
        HDF_LOGE("opendl failed, error:%{public}s\n", errStr);
    }
    if (NnrtCheckNullPointerOrReturnValue(libHandle_, __func__, __FILE__, __LINE__)) {
        return HDF_FAILURE;
    }

    createVdiFunc_ = reinterpret_cast<CreateNnrtDeviceVdiFunc>(dlsym(libHandle_, "CreateNnrtDeviceVdi"));
    if (createVdiFunc_ == nullptr) {
        errStr = dlerror();
        if (errStr != nullptr) {
            HDF_LOGE("nnrt CreateNnrtDeviceVdi dlsym error: %{public}s", errStr);
        }
        dlclose(libHandle_);
        return HDF_FAILURE;
    }

    destroyVdiFunc_ = reinterpret_cast<DestroyNnrtDeviceVdiFunc>(dlsym(libHandle_, "DestroyNnrtDeviceVdi"));
    if (destroyVdiFunc_ == nullptr) {
        errStr = dlerror();
        if (errStr != nullptr) {
            HDF_LOGE("composer DestroyNnrtDeviceVdi dlsym error: %{public}s", errStr);
        }
        dlclose(libHandle_);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t NnrtDeviceService::GetDeviceName(std::string& name)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->GetDeviceName(name);
}

int32_t NnrtDeviceService::GetVendorName(std::string& name)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->GetVendorName(name);
}

int32_t NnrtDeviceService::GetDeviceType(DeviceType& deviceType)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->GetDeviceType(deviceType);
}

int32_t NnrtDeviceService::GetDeviceStatus(DeviceStatus& status)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->GetDeviceStatus(status);
}

int32_t NnrtDeviceService::GetSupportedOperation(const Model& model, std::vector<bool>& ops)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->GetSupportedOperation(model, ops);
}

int32_t NnrtDeviceService::IsFloat16PrecisionSupported(bool& isSupported)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->IsFloat16PrecisionSupported(isSupported);
}

int32_t NnrtDeviceService::IsPerformanceModeSupported(bool& isSupported)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->IsPerformanceModeSupported(isSupported);
}

int32_t NnrtDeviceService::IsPrioritySupported(bool& isSupported)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->IsPrioritySupported(isSupported);
}

int32_t NnrtDeviceService::IsDynamicInputSupported(bool& isSupported)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->IsDynamicInputSupported(isSupported);
}

int32_t NnrtDeviceService::PrepareModel(const Model& model, const ModelConfig& config,
    sptr<OHOS::HDI::Nnrt::V2_0::IPreparedModel>& preparedModel)
{
    HDF_LOGI("%{public}s: called", __func__);
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->PrepareModel(model, config, preparedModel);
}

int32_t NnrtDeviceService::IsModelCacheSupported(bool& isSupported)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->IsModelCacheSupported(isSupported);
}

int32_t NnrtDeviceService::PrepareModelFromModelCache(const std::vector<SharedBuffer>& modelCache,
    const ModelConfig& config, sptr<OHOS::HDI::Nnrt::V2_0::IPreparedModel>& preparedModel)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->PrepareModelFromModelCache(modelCache, config, preparedModel);
}

int32_t NnrtDeviceService::PrepareOfflineModel(const std::vector<SharedBuffer>& offlineModels,
    const ModelConfig& config, sptr<OHOS::HDI::Nnrt::V2_0::IPreparedModel>& preparedModel)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->PrepareOfflineModel(offlineModels, config, preparedModel);
}

int32_t NnrtDeviceService::AllocateBuffer(uint32_t length, SharedBuffer& buffer)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->AllocateBuffer(length, buffer);
}

int32_t NnrtDeviceService::ReleaseBuffer(const SharedBuffer& buffer)
{
    if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR);
    }
    return vdiImpl_->ReleaseBuffer(buffer);
}

} // V2_0
} // Nnrt
} // HDI
} // OHOS
