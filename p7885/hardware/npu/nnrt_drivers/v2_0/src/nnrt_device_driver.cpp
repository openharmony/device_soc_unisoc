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

#include <hdf_base.h>
#include <hdf_device_desc.h>
#include <hdf_log.h>
#include <hdf_sbuf_ipc.h>
#include "v2_0/nnrt_device_stub.h"

#define HDF_LOG_TAG    nnrt_device_driver

using namespace OHOS::HDI::Nnrt::V2_0;

struct HdfNnrtDeviceHost {
    struct IDeviceIoService ioService;
    OHOS::sptr<OHOS::IRemoteObject> stub;
};

static int32_t NnrtDeviceDriverDispatch(struct HdfDeviceIoClient *client, int cmdId, struct HdfSBuf *data,
    struct HdfSBuf *reply)
{
    auto *hdfNnrtDeviceHost = CONTAINER_OF(client->device->service, struct HdfNnrtDeviceHost, ioService);

    OHOS::MessageParcel *dataParcel = nullptr;
    OHOS::MessageParcel *replyParcel = nullptr;
    OHOS::MessageOption option;

    if (SbufToParcel(data, &dataParcel) != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: invalid data sbuf object to dispatch", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (SbufToParcel(reply, &replyParcel) != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: invalid reply sbuf object to dispatch", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    return hdfNnrtDeviceHost->stub->SendRequest(cmdId, *dataParcel, *replyParcel, option);
}

static int HdfNnrtDeviceDriverInit(struct HdfDeviceObject *deviceObject)
{
    HDF_LOGI("%{public}s: driver init start", __func__);
    if (deviceObject == nullptr || deviceObject->service == nullptr) {
        HDF_LOGE("%{public}s: invalid deviceObject", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    auto *hdfNnrtDeviceHost = CONTAINER_OF(deviceObject->service, struct HdfNnrtDeviceHost, ioService);
    deviceObject->priv = hdfNnrtDeviceHost;
    return HDF_SUCCESS;
}

static int HdfNnrtDeviceDriverBind(struct HdfDeviceObject *deviceObject)
{
    HDF_LOGI("%{public}s: driver bind start", __func__);
    auto *hdfNnrtDeviceHost = new (std::nothrow) HdfNnrtDeviceHost;
    if (hdfNnrtDeviceHost == nullptr) {
        HDF_LOGE("%{public}s: failed to create create HdfNnrtDeviceHost object", __func__);
        return HDF_FAILURE;
    }

    hdfNnrtDeviceHost->ioService.Dispatch = NnrtDeviceDriverDispatch;
    hdfNnrtDeviceHost->ioService.Open = NULL;
    hdfNnrtDeviceHost->ioService.Release = NULL;

    auto serviceImpl = OHOS::HDI::Nnrt::V2_0::INnrtDevice::Get(true);
    if (serviceImpl == nullptr) {
        HDF_LOGE("%{public}s: failed to get of implement service", __func__);
        delete hdfNnrtDeviceHost;
        return HDF_FAILURE;
    }

    auto descriptor = OHOS::HDI::Nnrt::V2_0::INnrtDevice::GetDescriptor();
    HDF_LOGI("%{public}s: registering service with descriptor", __func__);

    hdfNnrtDeviceHost->stub = OHOS::HDI::ObjectCollector::GetInstance().GetOrNewObject(serviceImpl,
        descriptor);
    if (hdfNnrtDeviceHost->stub == nullptr) {
        HDF_LOGE("%{public}s: failed to get stub object", __func__);
        delete hdfNnrtDeviceHost;
        return HDF_FAILURE;
    }

    deviceObject->service = &hdfNnrtDeviceHost->ioService;
    return HDF_SUCCESS;
}

static void HdfNnrtDeviceDriverRelease(struct HdfDeviceObject *deviceObject)
{
    HDF_LOGI("%{public}s: driver release start", __func__);
    if (deviceObject->service == nullptr) {
        return;
    }

    auto *hdfNnrtDeviceHost = CONTAINER_OF(deviceObject->service, struct HdfNnrtDeviceHost, ioService);
    if (hdfNnrtDeviceHost != nullptr) {
        delete hdfNnrtDeviceHost;
    }
}

struct HdfDriverEntry g_nnrtdeviceDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "nnrt",
    .Bind = HdfNnrtDeviceDriverBind,
    .Init = HdfNnrtDeviceDriverInit,
    .Release = HdfNnrtDeviceDriverRelease,
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
HDF_INIT(g_nnrtdeviceDriverEntry);
#ifdef __cplusplus
}
#endif /* __cplusplus */
