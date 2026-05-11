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

#include "vendorGnssAdapter.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#define GNSSMGT "/vendor/lib64/libgnssmgt.z.so"
#define APN_NAME "CMNET"

template<typename T>
inline void UN_USED(T&&) {}

using namespace OHOS::Location;
using namespace OHOS::HDI::Location;

typedef OHOS::HDI::Location::GnssVendorInterface GnssVendorInterfaceType;

// private functions
PFnGnssmgtInit g_gpsInitPrvFn = nullptr;
PFnGnssmgtStart g_gpsStartPrvFn = nullptr;
PFnGnssmgtStop g_gpsStopPrvFn = nullptr;
PFnGnssmgtCleanup g_gpsCleanupPrvFn = nullptr;
PFnAGnssmgtInit g_agpsInitPrvFn = nullptr;
PFnAGnssmgtSetPosMode g_agpsSetPosModePrvFn = nullptr;
PFnAGnssmgtSetServer g_agpsSetServerPrvFn = nullptr;
PFnAGnssmgtOpenConn g_agpsOpenConnPrvFn = nullptr;

PFnAGnssmgtUpdateNwState g_agpsUpdateNwStatePrvFn = nullptr;
PFnAGnssmgtUpdateNwAvailability g_agpsUpdateNwAvailabilityPrvFn = nullptr;
PFnAGnssmgtSetSetId g_agpsSetIdPrvFn = nullptr;
PFnAGnssmgtSetRefLoc g_agpsSetRefLocPrvFn = nullptr;

PFnAGnssmgtAgpsCloseConn g_agpsCloseConnPrvFn = nullptr;
PFnAGnssmgtAgpsOpenFailed g_agpsOpenFailedPrvFn = nullptr;
PFnAGnssmgtAgpsOpenWithApnIpType g_agpsOpenWithApnPrvFn = nullptr;
PFnAGnssmgtRilInit g_agpsRilInitPrvFn = nullptr;
PFnAGnssmgtRilRefLocation g_agpsRilRefLocationPrvFn = nullptr;
PFnAGnssmgtRilSetId g_agpsRilSetIdPrvFn = nullptr;
PFnAGnssmgtRilUpdateNetworkAvailability g_agpsRilNetAbilityPrvFn = nullptr;
PFnAGnssmgtRilUpdateNetworkState g_agpsRilNetStatePrvFn = nullptr;

// read data;
GpsLocation g_gPSloc;
GpsSvStatus g_svState;

/*
ohos function
  const void* get_module_iface(int iface);
*/
const GnssVendorInterfaceType* GetGnssInterface();
const AgnssModuleInterface* GetAgnssInterface();

// Export so vendorinface entry
extern "C" {
__attribute__((visibility("default")))
GnssVendorDevice GnssVendorInterface = {sizeof(GnssVendorDevice), GetGnssInterface};
}

/* sprd so callback */
static void LocationCallback(GpsLocation* location);
static void StatusCallback(GpsStatus* status);
static void SvStatusCallback(GpsSvStatus* svStatus);
static void NmeaCallback(GpsUtcTime timestamp, const char* nmea, int length);
static void SetCapabilitiesCallback(uint32_t capabilities);
static void AcquireWakelockCallback();
static void ReleaseWakelockCallback();
static void RequestUtcTimeCallback();
static void GnssSetSystemInfoTest(const GnssSystemInfo* info);
static void GnssSvStatusCallbackTest(GnssSvStatus* svInfo);
pthread_t CreateThreadCallback(const char* name, void (*start)(void*), void* arg);

/* sprd so callback */
static void AgnssStatusCallback(AGpsStatus* status);
static void AgnssRilRequestSetIdCallback(uint32_t flags);
static void AgnssRilRequestRefLocCallback(uint32_t flags);
pthread_t AgnssCreateThreadCb(const char* name, void (*start)(void*), void* arg);

AGpsCallbacks g_sAGpsCallbacks = {AgnssStatusCallback, AgnssCreateThreadCb};

AGpsRilCallbacks g_sAGpsRilCallbacks = {AgnssRilRequestSetIdCallback,
                                        AgnssRilRequestRefLocCallback,
                                        AgnssCreateThreadCb};

GpsCallbacks g_sGpsCallbacks = {sizeof(GpsCallbacks),
                                LocationCallback,
                                StatusCallback,
                                SvStatusCallback,
                                NmeaCallback,
                                SetCapabilitiesCallback,
                                AcquireWakelockCallback,
                                ReleaseWakelockCallback,
                                CreateThreadCallback,
                                RequestUtcTimeCallback,
                                GnssSetSystemInfoTest,
                                GnssSvStatusCallbackTest};

int GnssEnable(GnssCallbackStruct* callbacks);
int GnssDisable(void);
int GnssStart(uint32_t type);
int GnssStop(uint32_t type);
int GnssInjectsReferenceInformation(int type, GnssReferenceInfo* info);
int GnssSetGnssConfigPara(GnssConfigParameter* para);
void GnssRemoveAuxiliaryData(uint16_t flags);
int GnssInjectExtendedEphemeris(char* data, int length);
int GnssGetCachedLocationsSize();
void GnssFlushCachedGnssLocations();
const void* GetGnssModuleIface(int iface);

void AGnssStatusChange(const AgnssDataConnectionRequest* status);
void AGnssGetSetId(uint16_t type);
void AGnssGetRefLoc(uint32_t type);

bool AGnssSetCallback(AgnssCallbackIfaces* callbacks);
bool AGnssSetRefLocation(const AgnssReferenceInfo* refLoc);
bool AGnssSetSetId(uint16_t type, const char* setid, size_t len);
bool AGnssSetAgnssServer(uint16_t type, const char* server, size_t len, int32_t port);

GnssVendorInterfaceType g_gnssVendorIfc = {
    sizeof(GnssVendorInterfaceType),
    GnssEnable,
    GnssDisable,
    GnssStart,
    GnssStop,
    GnssInjectsReferenceInformation,
    GnssSetGnssConfigPara,
    GnssRemoveAuxiliaryData,
    GnssInjectExtendedEphemeris,
    GnssGetCachedLocationsSize,
    GnssFlushCachedGnssLocations,
    GetGnssModuleIface // get_module_iface
};

AgnssModuleInterface g_AGMI_ = {
    sizeof(AgnssModuleInterface), AGnssSetCallback, AGnssSetRefLocation, AGnssSetSetId, AGnssSetAgnssServer};

void* g_vendorLibHandle = nullptr;
GnssCallbackStruct g_GCS_ = {0};

void LoadVendorSymbs()
{
    g_gpsInitPrvFn = (PFnGnssmgtInit)dlsym(g_vendorLibHandle, "gnssmgt_init");
    LBSLOGE(GNSS, "%{public}s g_gpsInitPrvFn addr is %{public}x", GNSSMGT, g_gpsInitPrvFn);
    g_gpsStartPrvFn = (PFnGnssmgtStart)dlsym(g_vendorLibHandle, "gnssmgt_start");
    LBSLOGE(GNSS, "%{public}s g_gpsStartPrvFn addr is %{public}x", GNSSMGT, g_gpsStartPrvFn);
    g_gpsStopPrvFn = (PFnGnssmgtStop)dlsym(g_vendorLibHandle, "gnssmgt_stop");
    LBSLOGE(GNSS, "%{public}s g_gpsStopPrvFn addr is %{public}x", GNSSMGT, g_gpsStopPrvFn);
    g_gpsCleanupPrvFn = (PFnGnssmgtCleanup)dlsym(g_vendorLibHandle, "gnssmgt_cleanup");
    LBSLOGE(GNSS, "%{public}s g_gpsCleanupPrvFn addr is %{public}x", GNSSMGT, g_gpsCleanupPrvFn);
    g_agpsInitPrvFn = (PFnAGnssmgtInit)dlsym(g_vendorLibHandle, "gnssmgt_agps_init");
    LBSLOGE(GNSS, "%{public}s g_agpsInitPrvFn addr is %{public}x", GNSSMGT, g_agpsInitPrvFn);
    g_agpsSetPosModePrvFn = (PFnAGnssmgtSetPosMode)dlsym(g_vendorLibHandle, "gnssmgt_setPosMode");
    LBSLOGE(GNSS, "%{public}s g_agpsSetPosModePrvFn addr is %{public}x", GNSSMGT, g_agpsSetPosModePrvFn);
    g_agpsSetServerPrvFn = (PFnAGnssmgtSetServer)dlsym(g_vendorLibHandle, "gnssmgt_agps_setServer");
    LBSLOGE(GNSS, "%{public}s g_agpsSetServerPrvFn addr is %{public}x", GNSSMGT, g_agpsSetServerPrvFn);
    g_agpsOpenConnPrvFn = (PFnAGnssmgtOpenConn)dlsym(g_vendorLibHandle, "gnssmgt_agps_openConn");
    LBSLOGE(GNSS, "%{public}s g_agpsOpenConnPrvFn addr is %{public}x", GNSSMGT, g_agpsOpenConnPrvFn);
    g_agpsCloseConnPrvFn = (PFnAGnssmgtAgpsCloseConn)dlsym(g_vendorLibHandle, "gnssmgt_agps_closeConn");
    LBSLOGE(GNSS_TEST, "%{public}s g_agpsCloseConnPrvFn addr is %{public}x", GNSSMGT, g_agpsCloseConnPrvFn);
    g_agpsOpenFailedPrvFn = (PFnAGnssmgtAgpsOpenFailed)dlsym(g_vendorLibHandle, "gnssmgt_agps_openFailed");
    LBSLOGE(GNSS_TEST, "%{public}s g_agpsOpenFailedPrvFn addr is %{public}x", GNSSMGT, g_agpsOpenFailedPrvFn);
    g_agpsOpenWithApnPrvFn = (PFnAGnssmgtAgpsOpenWithApnIpType)dlsym(g_vendorLibHandle,
                                                                     "gnssmgt_agps_openWithApnIpType");
    LBSLOGE(GNSS_TEST, "%{public}s g_agpsOpenWithApnPrvFn addr is %{public}x", GNSSMGT, g_agpsOpenWithApnPrvFn);
    g_agpsRilInitPrvFn = (PFnAGnssmgtRilInit)dlsym(g_vendorLibHandle, "gnssmgt_agps_ril_init");
    LBSLOGE(GNSS_TEST, "%{public}s g_agpsRilInitPrvFn addr is %{public}x", GNSSMGT, g_agpsRilInitPrvFn);
    g_agpsRilRefLocationPrvFn = (PFnAGnssmgtRilRefLocation)dlsym(g_vendorLibHandle, "gnssmgt_agps_ril_setRefLoc");
    LBSLOGE(GNSS_TEST, "%{public}s g_agpsRilRefLocationPrvFn addr is %{public}x", GNSSMGT, g_agpsRilRefLocationPrvFn);
    g_agpsRilSetIdPrvFn = (PFnAGnssmgtRilSetId)dlsym(g_vendorLibHandle, "gnssmgt_agps_ril_setSetID");
    LBSLOGE(GNSS_TEST, "%{public}s g_agpsRilSetIdPrvFn addr is %{public}x", GNSSMGT, g_agpsRilSetIdPrvFn);
    g_agpsRilNetAbilityPrvFn = (PFnAGnssmgtRilUpdateNetworkAvailability)dlsym(g_vendorLibHandle,
                                                                              "gnssmgt_agps_ril_updateNwAvailability");
    LBSLOGE(GNSS_TEST, "%{public}s g_agpsRilNetAbilityPrvFn addr is %{public}x", GNSSMGT, g_agpsRilNetAbilityPrvFn);
    g_agpsRilNetStatePrvFn = (PFnAGnssmgtRilUpdateNetworkState)dlsym(g_vendorLibHandle,
                                                                     "gnssmgt_agps_ril_updateNwState");
    LBSLOGE(GNSS_TEST, "%{public}s g_agpsRilNetStatePrvFn addr is %{public}x", GNSSMGT, g_agpsRilNetStatePrvFn);
}

int GnssEnable(GnssCallbackStruct* callbacks)
{
    LBSLOGE(GNSS, "%{public}s entered", GNSSMGT);
    if (callbacks == nullptr) {
        LBSLOGE(GNSS, "%{public}s callbacks == nullptr return", GNSSMGT);
    }
    LBSLOGE(GNSS, "%{public}s mayulong callbacks");
    DL_RET ret = SO_OK;
    g_GCS_ = *callbacks;
    g_vendorLibHandle = dlopen(GNSSMGT, RTLD_LAZY);
    LBSLOGE(GNSS, "%{public}s GNSSMGT handle addr is %x", GNSSMGT, g_vendorLibHandle);

    if (g_vendorLibHandle == nullptr) {
        LBSLOGE(GNSS, "%{public}s load failed", GNSSMGT);
        return LOAD_NOK;
    }

    LoadVendorSymbs();

    if (g_gpsInitPrvFn == nullptr || g_gpsStartPrvFn == nullptr || g_gpsStopPrvFn == nullptr ||
        g_gpsCleanupPrvFn == nullptr) {
        LBSLOGE(GNSS, "functions load failed", GNSSMGT);
        return FUNC_NOK;
    }

    g_gpsInitPrvFn(&g_sGpsCallbacks);
    return ret;
}

int GnssDisable(void)
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    if (g_gpsCleanupPrvFn) {
        g_gpsCleanupPrvFn();
    } else {
        LBSLOGE(GNSS, "g_gpsCleanupPrvFn is null, %s maybe not load properyly\n", __func__, GNSSMGT);
    }

    dlclose(g_vendorLibHandle);
    g_vendorLibHandle = nullptr;
    g_gpsInitPrvFn = nullptr;
    g_gpsStartPrvFn = nullptr;
    g_gpsStopPrvFn = nullptr;
    g_gpsCleanupPrvFn = nullptr;
    return 0;
}

int GnssStart(uint32_t type)
{
    UN_USED(type);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    if (g_gpsStartPrvFn) {
        g_gpsStartPrvFn();
    } else {
        LBSLOGE(GNSS, "g_gpsStartPrvFn is null, %s maybe not load properyly\n", __func__, GNSSMGT);
    }

    return 0;
}

int GnssStop(uint32_t type)
{
    UN_USED(type);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);

    if (g_gpsStopPrvFn) {
        g_gpsStopPrvFn();
    } else {
        LBSLOGE(GNSS, "g_gpsStopPrvFn is null, %s maybe not load properyly\n", __func__, GNSSMGT);
    }

    return 0;
}

int GnssInjectsReferenceInformation(int type, GnssReferenceInfo* info)
{
    UN_USED(type);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    return 0;
}

int GnssSetGnssConfigPara(GnssConfigParameter* para)
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    return 0;
}

void GnssRemoveAuxiliaryData(uint16_t flags)
{
    UN_USED(flags);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
}

int GnssInjectExtendedEphemeris(char* data, int length)
{
    UN_USED(data);
    UN_USED(length);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    return 0;
}

int GnssGetCachedLocationsSize()
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    return 0;
}

void GnssFlushCachedGnssLocations()
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
}

const void* GetGnssModuleIface(int iface)
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    /*
      AGNSS_MODULE_INTERFACE = 1,
      GNSS_GEOFENCING_MODULE_INTERFACE = 2,
      GNSS_NET_INITIATED_MODULE_INTERFACE = 3,
      GNSS_MEASUREMENT_MODULE_INTERFACE = 4,
    */
    if (iface == AGMI) {
        return GetAgnssInterface();
    }

    return nullptr;
}

/*ohos callback function*/
const GnssVendorInterfaceType* GetGnssInterface()
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    return &g_gnssVendorIfc;
}

const AgnssModuleInterface* GetAgnssInterface()
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    return &g_AGMI_;
}

/* sprd callback function*/
static void LocationCallback(GpsLocation* location)
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    GnssLocation ohos_location = {sizeof(GnssLocation),
                                  (uint32_t)location->flags,
                                  location->latitude,
                                  location->longitude,
                                  location->altitude,
                                  location->speed,
                                  location->bearing,
                                  location->accuracy,
                                  location->veraccuracy,
                                  location->speedaccuracy,
                                  location->bearaccuracy,
                                  (int64_t)location->timestamp,
                                  (int64_t)location->timestamp};

    g_GCS_.gnssCallback.locationUpdate(&ohos_location); // GnssLocation* location
}

static void StatusCallback(GpsStatus* status)
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    /* GNSS status unknown. */
    //  GNSS_STATUS_NONE = 0,
    /* GNSS has begun navigating. */
    //  GNSS_STATUS_SESSION_BEGIN = 1,
    /* GNSS has stopped navigating. */
    //  GNSS_STATUS_SESSION_END = 2,
    /* GNSS has powered on but is not navigating. */
    //  GNSS_STATUS_ENGINE_ON = 3,
    /* GNSS is powered off. */
    //  GNSS_STATUS_ENGINE_OFF = 4
    uint16_t ohosStatus = status->status;
    g_GCS_.gnssCallback.gnssWorkingStatusUpdate(&ohosStatus);
}
static void SvStatusCallback(GpsSvStatus* svStatus)
{
    UN_USED(svStatus);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    GnssSatelliteStatus* status;
    status->size = sizeof(GnssSatelliteStatus);
}

static void NmeaCallback(GpsUtcTime timestamp, const char* nmea, int length)
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    if (nmea == nullptr) {
        LBSLOGE(GNSS, "nmea pointer is null \n", __func__);
        return;
    }
    g_GCS_.gnssCallback.nmeaUpdate((int64_t)timestamp, nmea, length);
}

static void SetCapabilitiesCallback(uint32_t capabilities)
{
    UN_USED(capabilities);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
}

static void AcquireWakelockCallback()
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
}

static void ReleaseWakelockCallback()
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
}

static void RequestUtcTimeCallback()
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
}

static void GnssSetSystemInfoTest(const GnssSystemInfo* info)
{
    UN_USED(info);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
}

static void GnssSvStatusCallbackTest(GnssSvStatus* svInfo)
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    GnssSatelliteStatus status;
    status.size = svInfo->size;
    status.satellitesNum = svInfo->numbSvs;
    for (int i = 0; i < svInfo->numbSvs; i++) {
        status.satellitesList[i] = {
            svInfo->gnss_sv_list[i].size,
            svInfo->gnss_sv_list[i].svid,
            svInfo->gnss_sv_list[i].constellation,
            svInfo->gnss_sv_list[i].cN0Dbhz,
            svInfo->gnss_sv_list[i].elevation,
            svInfo->gnss_sv_list[i].zaimuth,
            svInfo->gnss_sv_list[i].carrierFreq,
            (uint32_t)svInfo->gnss_sv_list[i].flags,
        };
    }

    g_GCS_.gnssCallback.satelliteStatusUpdate(&status);
}

pthread_t CreateThreadCallback(const char* name, void (*start)(void*), void* arg)
{
    UN_USED(name);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    pthread_t pid = 0;
    int ret = 0;
    if (start == nullptr) {
        LBSLOGE(GNSS, "start pointer is null \n", __func__);
        return pid;
    }
    ret = pthread_create(&pid, nullptr, reinterpret_cast<void* (*)(void*)>(start), arg);
    if (ret != 0) {
        LBSLOGE(GNSS, "crate pthread failure \n", __func__);
        return pid;
    }
    return pid;
}

void AGnssStatusChange(const AgnssDataConnectionRequest* status)
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
}

void AGnssGetSetId(uint16_t type)
{
    UN_USED(type);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
}

void AGnssGetRefLoc(uint32_t type)
{
    UN_USED(type);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
}

AgnssCallbackIfaces g_GACS_;
bool AGnssSetCallback(AgnssCallbackIfaces* callbacks)
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    g_GACS_ = *callbacks;
    g_agpsInitPrvFn(&g_sAGpsCallbacks);
    g_agpsRilInitPrvFn(&g_sAGpsRilCallbacks);
    g_agpsSetPosModePrvFn(GPS_POSITION_MODE_MS_BASED, GPS_POSITION_RECURRENCE_PERIODIC, MIN_INTERVAL, 1, 1);
    return true;
}

static AGpsRefLocationType AGnssRefLocTypeConvert(uint32_t inType)
{
    switch (inType) {
        case CELLID_TYPE_GSM:
            return AGPS_REF_LOCATION_TYPE_GSM_CELLID;
        case CELLID_TYPE_UMTS:
            return AGPS_REF_LOCATION_TYPE_UMTS_CELLID;
        case CELLID_TYPE_LTE:
            return AGPS_REF_LOCATION_TYPE_LTE_CELLID;
        case CELLID_TYPE_NR:
            return AGPS_REF_LOCATION_TYPE_NR_CELLID;
        /* comment:
         case :
         return AGPS_REF_LOCATION_TYPE_LTE_CELLID_EX;
         case :
         return AGPS_REF_LOCATION_TYPE_MAC
        */
        default:
            LBSLOGE(GNSS_TEST, "%{public}s wrong cellType.", __func__);
    }

    return 0;
}

bool AGnssSetRefLocation(const AgnssReferenceInfo* refLoc)
{
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    AGpsRefLocation sRefLoc;
    sRefLoc.type = AGnssRefLocTypeConvert(refLoc->category);
    switch (sRefLoc.type) {
        case AGPS_REF_LOCATION_TYPE_GSM_CELLID:
        case AGPS_REF_LOCATION_TYPE_UMTS_CELLID:
        case AGPS_REF_LOCATION_TYPE_LTE_CELLID:
        case AGPS_REF_LOCATION_TYPE_LTE_CELLID_EX:
        case AGPS_REF_LOCATION_TYPE_NR_CELLID:
            sRefLoc.u.cellID.type = sRefLoc.type;
            sRefLoc.u.cellID.mcc = refLoc->u.cellId.mcc;
            sRefLoc.u.cellID.mnc = refLoc->u.cellId.mnc;
            sRefLoc.u.cellID.lac = refLoc->u.cellId.lac;
            sRefLoc.u.cellID.cid = refLoc->u.cellId.cid;
            sRefLoc.u.cellID.tac = refLoc->u.cellId.tac;
            sRefLoc.u.cellID.pcid = refLoc->u.cellId.pcid;
            break;
        case AGPS_REF_LOCATION_TYPE_MAC:
            break;
        default:
            LBSLOGE(GNSS_TEST, "%{public}s wrong sRefLoc.type.", __func__);
    }
    LBSLOGI(GNSS, "%{public}s: cellID.mcc is %u \n", __func__, sRefLoc.u.cellID.mcc);
    LBSLOGI(GNSS, "%{public}s: cellID.mnc is %u \n", __func__, sRefLoc.u.cellID.mnc);
    LBSLOGI(GNSS, "%{public}s: cellID.lac is %u \n", __func__, sRefLoc.u.cellID.lac);
    LBSLOGI(GNSS, "%{public}s: cellID.cid is %u \n", __func__, sRefLoc.u.cellID.cid);
    LBSLOGI(GNSS, "%{public}s: cellID.tac is %u \n", __func__, sRefLoc.u.cellID.tac);
    LBSLOGI(GNSS, "%{public}s: cellID.pcid is %u \n", __func__, sRefLoc.u.cellID.pcid);

    size_t size = sizeof(AGpsRefLocation);
    LBSLOGI(GNSS, "%{public}s: sRefLoc.u.cellID.size is %d \n", __func__, sRefLoc.u.cellID.type);
    g_agpsRilRefLocationPrvFn(&sRefLoc, size);
    return true;
}
bool AGnssSetSetId(uint16_t type, const char* setid, size_t len)
{
    UN_USED(len);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    g_agpsRilSetIdPrvFn(type, setid);
    return true;
}

bool AGnssSetAgnssServer(uint16_t type, const char* server, size_t len, int32_t port)
{
    UN_USED(len);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    g_agpsSetServerPrvFn(type, server, port);
    return true;
}

static void AgnssStatusCallback(AGpsStatus* status)
{
    AGpsType type = 0;
    int ret;
    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    LBSLOGI(GNSS_TEST,
            "AGPS type:%{public}s status:%{public}d ip:%{public}d\n",
            ((status->type == AGPS_TYPE_SUPL) ? "supl" : "c2k"),
            status->status,
            status->ipaddr);
    AgnssDataConnectionRequest ohos_status;
    ohos_status.size = status->size;
    ohos_status.agnssCategory = status->type;
    ohos_status.requestCategory = status->status;
    ohos_status.ipaddr = status->ipaddr;

    switch (status->status) {
        case GPS_REQUEST_AGPS_DATA_CONN:
            LBSLOGI(GNSS, "%{public}s AGPS connection request.\n", __func__);
            ret = g_agpsRilNetStatePrvFn(1, 0, 0, nullptr);
            LBSLOGI(GNSS, "%{public}s g_agpsRilNetStatePrvFn ret is %{public}d .\n", __func__, ret);
            if (ret != 0) {
                LBSLOGI(GNSS, "%{public}s AGPS update state failed.\n", __func__);
                return;
            }
            ret = g_agpsOpenConnPrvFn(APN_NAME);
            LBSLOGI(GNSS, "%{public}s g_agpsOpenConnPrvFn ret is %{public}d .\n", __func__, ret);
            if (ret != 0) {
                LBSLOGI(GNSS, "%{public}s AGPS open conn failed.\n", __func__);
                return;
            }
            break;
        case GPS_RELEASE_AGPS_DATA_CONN:
            LBSLOGI(GNSS, "%{public}s AGPS release conn failed.\n", __func__);
            break;
        case GPS_AGPS_DATA_CONNECTED:
            LBSLOGI(GNSS, "%{public}s AGPS data connection initiated.\n", __func__);
            break;
        case GPS_AGPS_DATA_CONN_DONE:
            LBSLOGI(GNSS, "%{public}s AGPS data connection completed.\n", __func__);
            break;
        case GPS_AGPS_DATA_CONN_FAILED:
            LBSLOGI(GNSS, "%{public}s AGPS data connection failed.\n", __func__);
            break;
        default:
            LBSLOGI(GNSS, "%{public}s AGPS unknown status.\n", __func__);
    }

    LBSLOGI(GNSS, "%{public}s: entering function requestSetupDataLink.\n", __func__);

    g_GACS_.requestSetupDataLink(&ohos_status);

    return;
}

pthread_t AgnssCreateThreadCb(const char* name, void (*start)(void*), void* arg)
{
    UN_USED(name);

    LBSLOGI(GNSS, "enter function %{public}s \n", __func__);
    pthread_t pid = 0;
    int ret = 0;
    if (start == nullptr) {
        LBSLOGE(GNSS, "start pointer is null \n", __func__);
        return pid;
    }
    ret = pthread_create(&pid, nullptr, reinterpret_cast<void* (*)(void*)>(start), arg);
    if (ret != 0) {
        LBSLOGE(GNSS, "crate pthread failure \n", __func__);
        return pid;
    }
    return pid;
}

static void AgnssRilRequestSetIdCallback(uint32_t flags)
{
    LBSLOGI(GNSS_TEST, "enter function %{public}s flags:%{public}d\n", __func__, flags);
    uint16_t type = static_cast<uint16_t>(flags);
    g_GACS_.requestSetid(type);
}

static void AgnssRilRequestRefLocCallback(uint32_t flags)
{
    LBSLOGI(GNSS_TEST, "enter function %{public}s, flags is:%{public}d\n", __func__, flags);
    g_GACS_.requestRefInfo(flags);
}
