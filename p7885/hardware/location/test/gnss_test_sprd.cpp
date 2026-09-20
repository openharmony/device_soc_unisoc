/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <dlfcn.h>
#include <hdf_log.h>
#include <unistd.h>

#include "gnss_interface_impl.h"
#include "location_vendor_interface.h"
#include "location_vendor_lib.h"

static const std::string VENDOR_NAME = "vendorGnssAdapter.so";
using namespace OHOS::HDI::Location;

namespace OHOS {
namespace HDI {
namespace Location {
namespace {
const std::string VENDOR_NAME = "vendorGnssAdapter.so";
} // namespace
std::mutex LocationVendorInterface::mutex_;
LocationVendorInterface* LocationVendorInterface::instance_ = nullptr;

LocationVendorInterface::LocationVendorInterface()
{
    Init();
    HDF_LOGI("%{public}s constructed.", __func__);
}

LocationVendorInterface::~LocationVendorInterface()
{
    CleanUp();
    HDF_LOGI("%{public}s destructed.", __func__);
}

LocationVendorInterface* LocationVendorInterface::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = new LocationVendorInterface();
        }
    }
    return instance_;
}

void LocationVendorInterface::DestroyInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ != nullptr) {
        delete instance_;
        instance_ = nullptr;
    }
}

void LocationVendorInterface::Init()
{
    HDF_LOGI("%{public}s.", __func__);
    vendorHandle_ = dlopen(VENDOR_NAME.c_str(), RTLD_LAZY);
    if (!vendorHandle_) {
        HDF_LOGE("%{public}s:dlopen %{public}s failed: %{public}s", __func__, VENDOR_NAME.c_str(), dlerror());
        return;
    }

    GnssVendorDevice* gnssDevice = static_cast<GnssVendorDevice*>(dlsym(vendorHandle_, "syml_GnssVendorInterface"));
    if (gnssDevice == nullptr) {
        HDF_LOGE("%{public}s:dlsym GnssInterface failed.", __func__);
        return;
    }
    vendorInterface_ = gnssDevice->getGnssInterface();
    if (vendorInterface_ == nullptr) {
        HDF_LOGE("%{public}s:getGnssInterface failed.", __func__);
        return;
    }
}

const GnssVendorInterface* LocationVendorInterface::GetGnssVendorInterface() const
{
    if (vendorInterface_ == nullptr) {
        HDF_LOGE("%{public}s:GetGnssVendorInterface() failed.", __func__);
    }
    return vendorInterface_;
}

const void* LocationVendorInterface::GetModuleInterface(int moduleId) const
{
    auto vendorInterface = GetGnssVendorInterface();
    if (vendorInterface == nullptr) {
        HDF_LOGE("%{public}s:can not get vendorInterface.", __func__);
        return nullptr;
    }
    auto moduleInterface = vendorInterface->getGnssModuleIface(moduleId);
    if (moduleInterface == nullptr) {
        HDF_LOGE("%{public}s:can not get moduleInterface.", __func__);
    }
    return moduleInterface;
}

void LocationVendorInterface::CleanUp()
{
    if (vendorInterface_ == nullptr) {
        return;
    }
    vendorInterface_ = nullptr;
    dlclose(vendorHandle_);
    vendorHandle_ = nullptr;
}
} // namespace Location
} // namespace HDI
} // namespace OHOS

static void LocationUpdate(GnssLocation* location)
{
    if (location == nullptr) {
        HDF_LOGE("%{public}s:location is nullptr.", __func__);
        return;
    }
    HDF_LOGE("%{public}s:LocationUpdate.", __func__);

    HDF_LOGE("%{public}s:location.latitude is %{public}f.", __func__, location->latitude);
    printf("%s:location.latitude is %f.\n", __func__, location->latitude);
    HDF_LOGE("%{public}s:location.longitude is %{public}f.\n", __func__, location->longitude);
    printf("%s:location.longitude is %f.\n", __func__, location->longitude);
    HDF_LOGE("%{public}s:location.altitude is %{public}f.", __func__, location->altitude);
    printf("%s:location.altitude is %f.\n", __func__, location->altitude);
    HDF_LOGE("%{public}s:location.horizontalAccuracy is %{public}f.", __func__, location->horizontalAccuracy);
    printf("%s:location.horizontalAccuracy is %f.\n", __func__, location->horizontalAccuracy);
    HDF_LOGE("%{public}s:location.speed is %{public}f.", __func__, location->speed);
    printf("%s:location.speed is %f.\n", __func__, location->speed);
    HDF_LOGE("%{public}s:location.bearing is %{public}f.", __func__, location->bearing);
    printf("%s:location.bearing is %f.\n", __func__, location->bearing);
    HDF_LOGE("%{public}s:location.timestamp is %{public}ld.", __func__, location->timeForFix);
    printf("%s:location.timestamp is %ld.\n", __func__, location->timeForFix);
    HDF_LOGE("%{public}s:location.timestampSinceBoot is %{public}ld.", __func__, location->timeSinceBoot);
    printf("%s:location.timestampSinceBoot is %ld.\n", __func__, location->timeSinceBoot);
}

static void StatusCallback(uint16_t* status)
{
    HDF_LOGE("%{public}s: entered.", __func__);
    printf("%s: entered.", __func__);
    if (status == nullptr) {
        HDF_LOGE("%{public}s:param is nullptr.", __func__);
        return;
    }
    GnssWorkingStatus gnssStatus = static_cast<GnssWorkingStatus>(*status);
    HDF_LOGE("%{public}s: the status is %{public}d.", __func__, gnssStatus);
    printf("%s: the status is %d.\n", __func__, gnssStatus);
    HDF_LOGE("%{public}s: the status is %{public}d.",
             __func__,
             static_cast<std::underlying_type<GnssWorkingStatus>::type>(gnssStatus));
    printf("%s: the status is %d.\n", __func__, static_cast<std::underlying_type<GnssWorkingStatus>::type>(gnssStatus));
}

static void SvStatusCallback(GnssSatelliteStatus* svInfo)
{
    HDF_LOGE("%{public}s: entered.", __func__);

    if (svInfo == nullptr) {
        HDF_LOGE("%{public}s:sv_info is null.", __func__);
        return;
    }
    if (svInfo->satellitesNum == 0) {
        HDF_LOGE("%{public}s:satellites_num == 0.", __func__);
        return;
    }

    HDF_LOGE("%{public}s: the satellites_num is %{public}d.", __func__, svInfo->satellitesNum);

    for (int i = 0; i < svInfo->satellitesNum; i++) {
        HDF_LOGE("%{public}s: the %{public}dth satellite Id is %{public}d.",
                 __func__,
                 i,
                 svInfo->satellitesList[i].satelliteId);
    }
    for (int i = 0; i < svInfo->satellitesNum; i++) {
        HDF_LOGE("%{public}s: the %{public}dth satellite type is %{public}u.",
                 __func__,
                 i,
                 svInfo->satellitesList[i].constellationCategory);
        HDF_LOGE("%{public}s: the %{public}dth satellite elevation is %{public}f.",
                 __func__,
                 i,
                 svInfo->satellitesList[i].elevation);
        HDF_LOGE("%{public}s: the %{public}dth satellite azimuth is %{public}f.",
                 __func__,
                 i,
                 svInfo->satellitesList[i].azimuth);
        HDF_LOGE("%{public}s: the %{public}dth satellite carrierFrequency is %{public}f.",
                 __func__,
                 i,
                 svInfo->satellitesList[i].carrierFrequency);
        HDF_LOGE("%{public}s: the %{public}dth satellite cn0 is %{public}f.",
                 __func__,
                 i,
                 svInfo->satellitesList[i].cn0);
    }
}

static void NmeaCallback(int64_t timestamp, const char* nmea, int length)
{
    HDF_LOGE("%{public}s: entered.", __func__);
    printf("%s: entered.\n", __func__);
    if (nmea == nullptr) {
        HDF_LOGE("%{public}s:nmea is nullptr.", __func__);
        printf("%s:nmea is nullptr.\n", __func__);
        return;
    }
    HDF_LOGE("nmea length:%{public}d, str:%{public}s", length, nmea);
    printf("nmea length:%d, str:%s\n", length, nmea);
}

static void GetGnssBasicCallbackMethods(GnssBasicCallbackIfaces* device)
{
    HDF_LOGE("%{public}s: entered.", __func__);
    printf("%s: entered.\n", __func__);
    if (device == nullptr) {
        HDF_LOGE("%{public}s: device == nullptr exit", __func__);
        printf("%s: device == nullptr exit\n", __func__);
        return;
    }
    device->size = sizeof(GnssCallbackStruct);
    device->locationUpdate = LocationUpdate;
    device->gnssWorkingStatusUpdate = StatusCallback;
    device->satelliteStatusUpdate = SvStatusCallback;
    device->nmeaUpdate = NmeaCallback;
    device->capabilitiesUpdate = nullptr;
    device->requestRefInfo = nullptr;
    device->requestExtendedEphemeris = nullptr;
    HDF_LOGE("%{public}s: finish.", __func__);
    printf("%s: finish.\n", __func__);
}

static void GetGnssCacheCallbackMethods(GnssCacheCallbackIfaces* device)
{
    HDF_LOGE("%{public}s: entered.", __func__);
    printf("%s: entered.\n", __func__);
    if (device == nullptr) {
        HDF_LOGE("%{public}s: device == nullptr exit", __func__);
        printf("%s: device == nullptr exit\n", __func__);
        return;
    }
    device->size = 0;
    device->cachedLocationUpdate = nullptr;
    HDF_LOGE("%{public}s: finish.", __func__);
    printf("%s: finish.\n", __func__);
}

static void GetGnssCallbackMethods(GnssCallbackStruct* device)
{
    HDF_LOGE("%{public}s: entered.", __func__);
    printf("%s: entered.\n", __func__);
    if (device == nullptr) {
        HDF_LOGE("%{public}s: device == nullptr exit", __func__);
        printf("%s: device == nullptr exit\n", __func__);
        return;
    }
    device->size = sizeof(GnssCallbackStruct);
    static GnssBasicCallbackIfaces basicCallback;
    GetGnssBasicCallbackMethods(&basicCallback);
    device->gnssCallback = basicCallback;
    static GnssCacheCallbackIfaces cacheCallback;
    GetGnssCacheCallbackMethods(&cacheCallback);
    device->gnssCacheCallback = cacheCallback;
    HDF_LOGE("%{public}s: finish.\n", __func__);
}

int main()
{
    HDF_LOGE("%{public}s:test entered.", __func__);
    printf("%s:test entered.\n", __func__);

    int ret;
    int gnsstype = 0;
    GnssCallbackStruct callbacks;
    GetGnssCallbackMethods(&callbacks);

    const GnssVendorInterface* gnssIface =
        OHOS::HDI::Location::LocationVendorInterface::LocationVendorInterface::GetInstance()->GetGnssVendorInterface();

    HDF_LOGE("%{public}s: gnssVI got.", __func__);
    printf("%s: gnssVI got.\n", __func__);

    // initiallization
    ret = gnssIface->enableGnss(&callbacks);
    if (ret != 0) {
        HDF_LOGE("%{public}s:vendor gnss enable failed.", __func__);
        printf("%s:vendor gnss enable failed.\n", __func__);
    }

    HDF_LOGE("%{public}s:vendor gnss enable successful.", __func__);
    printf("%s:vendor gnss enable successful.\n", __func__);

    ret = gnssIface->startGnss(gnsstype);
    if (ret != 0) {
        HDF_LOGE("%{public}s:vendor gnss start failed.", __func__);
        printf("%s:vendor gnss start failed.\n", __func__);
    }

    HDF_LOGE("%{public}s:vendor gnss start successful.", __func__);
    printf("%s:vendor gnss start successful.\n", __func__);

    int maxWait = 30;
    while (maxWait > 0) {
        sleep(1);
        maxWait--;
    }

    ret = gnssIface->stopGnss(gnsstype);

    ret = gnssIface->disableGnss();

    return ret;
}
