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

#ifndef VENDOR_GNSS_ADAPTER_H_
#define VENDOR_GNSS_ADAPTER_H_

#include "location_log.h"
#include "location_vendor_lib.h"

#define GPS_MAX_SVS 32
#define GNSS_MAX_SVS 64

enum DL_RET {
    SO_OK = 0,
    LOAD_NOK = -1,
    FUNC_NOK = -2,
};

const int AGMI = 1;
const int GMI = 2;

typedef int64_t GpsUtcTime;
typedef struct {
    size_t size;
    uint16_t flags;
    double latitude;
    double longitude;
    double altitude;
    float speed;
    float bearing;
    float accuracy;
    GpsUtcTime timestamp;
    float veraccuracy;
    float speedaccuracy;
    float bearaccuracy;
} GpsLocation;

typedef struct {
    size_t size;
    int prn;
    float snr;
    float elevation;
    float azimuth;
} GpsSvInfo;

typedef struct {
    size_t size;
    GpsSvInfo sv_list[GPS_MAX_SVS];
    uint32_t ephemeris_mask;
    uint32_t almanac_mask;
    uint32_t used_in_fix_mask;
} GpsSvStatus;

typedef uint8_t GnssConstellationType;
typedef uint8_t GnssSvFlags;

typedef struct {
    size_t size;
    int16_t svid;
    GnssConstellationType constellation;
    float cN0Dbhz;
    float elevation;
    float zaimuth;
    GnssSvFlags flags;
    float carrierFreq;
} GnssSvInfo;

typedef struct {
    size_t size;
    int numbSvs;
    GnssSvInfo gnss_sv_list[GNSS_MAX_SVS];
} GnssSvStatus;

typedef uint16_t GpsStatusValue;

typedef struct {
    size_t size;
    GpsStatusValue status;
} GpsStatus;

typedef struct {
    size_t size;
    uint16_t year_of_hw;
} GnssSystemInfo;

typedef unsigned long int pthread_t;

typedef void (*PFnGpsLocationCallback)(GpsLocation* location);
typedef void (*PFnGpsStatusCallback)(GpsStatus* status);
typedef void (*PFnGpsSvStatusCallback)(GpsSvStatus* svInfo);
typedef void (*PFnGnssSvStatusCallback)(GnssSvStatus* svInfo);
typedef void (*PFnGpsNmeaCallback)(GpsUtcTime timestamp, const char* nmea, int length);
typedef void (*PFnGpsSetCapabilities)(uint32_t capbilities);
typedef void (*PFnGpsAcquireWakelock)();
typedef void (*PFnGpsReleaseWakelock)();
typedef void (*PFnGpsRequestUtcTime)();
typedef pthread_t (*PFnGpsCreateThread)(const char* name, void (*start)(void*), void* arg);
typedef void (*PFnGnssSetSystemInfo)(const GnssSystemInfo* info);

typedef struct {
    size_t size;
    PFnGpsLocationCallback location_cb;
    PFnGpsStatusCallback status_cb;
    PFnGpsSvStatusCallback sv_status_cb;
    PFnGpsNmeaCallback nmea_cb;
    PFnGpsSetCapabilities set_capabilities_cb;
    PFnGpsAcquireWakelock acquire_wakelock_cb;
    PFnGpsReleaseWakelock release_wakelock_cb;
    PFnGpsCreateThread create_thread_cb;
    PFnGpsRequestUtcTime request_utc_time_cb;
    PFnGnssSetSystemInfo set_system_info_cb;
    PFnGnssSvStatusCallback gnss_sv_status_cb;
} GpsCallbacks; // sprd callback;

typedef int (*PFnGnssmgtInit)(GpsCallbacks* callbacks);
typedef int (*PFnGnssmgtStart)(void);
typedef int (*PFnGnssmgtStop)(void);
typedef int (*PFnGnssmgtCleanup)(void);

/* AGPS PART */
typedef uint16_t ApnIpType;
#define APN_IP_INVALID 0
#define APN_IP_IPV4 1
#define APN_IP_IPV6 2
#define APN_IP_IPV4V6 3

typedef uint16_t AGpsSetIDType;
#define AGPS_SETID_TYPE_NONE 0
#define AGPS_SETID_TYPE_IMSI 1
#define AGPS_SETID_TYPE_MSISDN 2

typedef uint16_t AGpsRefLocationType;
#define AGPS_REF_LOCATION_TYPE_GSM_CELLID 1
#define AGPS_REF_LOCATION_TYPE_UMTS_CELLID 2
#define AGPS_REF_LOCATION_TYPE_MAC 3
#define AGPS_REF_LOCATION_TYPE_LTE_CELLID 4
#define AGPS_REF_LOCATION_TYPE_LTE_CELLID_EX 5
#define AGPS_REF_LOCATION_TYPE_NR_CELLID 8

#define CELLID_TYPE_GSM 1
#define CELLID_TYPE_UMTS 2
#define CELLID_TYPE_LTE 3
#define CELLID_TYPE_NR 4

/* Deprecated, to be removed in the next release. */
#define AGPS_REG_LOCATION_TYPE_MAC 3

/** Network types for update_network_state "type" parameter */
#define AGPS_RIL_NETWORK_TYPE_MOBILE 0
#define AGPS_RIL_NETWORK_TYPE_WIFI 1
#define AGPS_RIL_NETWORK_TYPE_MOBILE_MMS 2
#define AGPS_RIL_NETWORK_TYPE_MOBILE_SUPL 3
#define AGPS_RIL_NETWORK_TTYPE_MOBILE_DUN 4
#define AGPS_RIL_NETWORK_TTYPE_MOBILE_HIPRI 5
#define AGPS_RIL_NETWORK_TTYPE_WIMAX 6

/** Requested operational mode for GPS operation. */
typedef uint32_t GpsPositionMode;
/* IMPORTANT: Note that the following values must match
 * constants in GpsLocationProvider.java. */
/** Mode for running GPS standalone (no assistance). */
#define GPS_POSITION_MODE_STANDALONE 0
/** AGPS MS-Based mode. */
#define GPS_POSITION_MODE_MS_BASED 1
/**
 * AGPS MS-Assisted mode. This mode is not maintained by the platform anymore.
 * It is strongly recommended to use GPS_POSITION_MODE_MS_BASED instead.
 */
#define GPS_POSITION_MODE_MS_ASSISTED 2

/** Requested recurrence mode for GPS operation. */
typedef uint32_t GpsPositionRecurrence;
/* IMPORTANT: Note that the following values must match
 * constants in GpsLocationProvider.java. */
/** Receive GPS fixes on a recurring basis at a specified period. */
#define GPS_POSITION_RECURRENCE_PERIODIC 0
/** Request a single shot GPS fix. */
#define GPS_POSITION_RECURRENCE_SINGLE 1

#define MIN_INTERVAL 1000

/** AGPS type */
typedef uint16_t AGpsType;
#define AGPS_TYPE_SUPL 1
#define AGPS_TYPE_C2K 2

/** AGPS status event values. */
typedef uint16_t AGpsStatusValue;
/** GPS requests data connection for AGPS. */
#define GPS_REQUEST_AGPS_DATA_CONN 1
/** GPS releases the AGPS data connection. */
#define GPS_RELEASE_AGPS_DATA_CONN 2
/** AGPS data connection initiated */
#define GPS_AGPS_DATA_CONNECTED 3
/** AGPS data connection completed */
#define GPS_AGPS_DATA_CONN_DONE 4
/** AGPS data connection failed */
#define GPS_AGPS_DATA_CONN_FAILED 5

typedef struct {
    /** set to sizeof(AGpsStatus) */
    size_t size;

    AGpsType type;
    AGpsStatusValue status;

    /**
     * Must be set to a valid IPv4 address if the field 'addr' contains an IPv4
     * address, or set to INADDR_NONE otherwise.
     */
    uint32_t ipaddr;

    /**
     * Must contain the IPv4 (AF_INET) or IPv6 (AF_INET6) address to report.
     * Any other value of addr.ss_family will be rejected.
     */
    // struct sockaddr_storage addr;
} AGpsStatus;

/**
 * Callback with AGPS status information. Can only be called from a thread
 * created by create_thread_cb.
 */
typedef void (*PFnAgpsStatusCallback)(AGpsStatus* status);
typedef void (*PFnAgpsRilRequestSetId)(uint32_t flags);
typedef void (*PFnAgpsRilRequestRefLoc)(uint32_t flags);
typedef pthread_t (*PFnGpsCreateThread)(const char* name, void (*start)(void*), void* arg);

/* CellID for 2G, 3G and LTE, used in AGPS. */
typedef struct {
    AGpsRefLocationType type;
    /** Mobile Country Code. */
    uint16_t mcc;
    /** Mobile Network Code .*/
    uint16_t mnc;
    /** Location Area Code in 2G, 3G and LTE. In 3G lac is discarded. In LTE,
     * lac is populated with tac, to ensure that we don't break old clients that
     * might rely in the old (wrong) behavior.
     */
    uint16_t lac;
    /** Cell id in 2G. Utran Cell id in 3G. Cell Global Id EUTRA in LTE. */
    uint32_t cid;

    /** Tracking Area Code in LTE. */
    uint16_t tac;
    /** Physical Cell id in LTE (not used in 2G and 3G) */
    uint16_t pcid;
} AGpsRefLocationCellID;

typedef struct {
    uint8_t mac[6];
} AGpsRefLocationMac;

typedef struct {
    /** Mobile Country Code. */
    uint16_t mcc;
    /** Mobile Network Code .*/
    uint16_t mnc;
    /** Cell id in 2G. Utran Cell id in 3G. Cell Global Id EUTRA in LTE. */
    uint64_t cid;
    /** Tracking Area Code in LTE. */
    uint32_t tac;
    /** Physical Cell id in LTE (not used in 2G and 3G) */
    uint16_t pcid;
    /** ARFCN used for SSB or CSI-RS measurements, 0..3279165. */
    uint32_t arfcn_nr;
} AGpsRefLocationCellIdNRType;

/** Represents ref locations */
typedef struct {
    AGpsRefLocationType type;
    union {
        AGpsRefLocationCellID cellID;
        AGpsRefLocationMac mac;
        AGpsRefLocationCellIdNRType cellID_nr;
    } u;
} AGpsRefLocation;

/** Callback structure for the AGPS interface. */
typedef struct {
    PFnAgpsStatusCallback status_cb;
    PFnGpsCreateThread create_thread_cb;
} AGpsCallbacks;

typedef struct {
    PFnAgpsRilRequestSetId request_setid;
    PFnAgpsRilRequestRefLoc request_refloc;
    PFnGpsCreateThread create_thread_cb;
} AGpsRilCallbacks;

/* AGMI interface
void gnssmgt_agps_ril_setRefLoc(const AGpsRefLocation* refLocation, size_t
szStruct); void gnssmgt_agps_ril_setSetID(AGpsSetIDType type, const char*
setid);
*/

typedef int (*PFnAGnssmgtInit)(AGpsCallbacks* callbacks);
typedef int (*PFnAGnssmgtSetPosMode)(GpsPositionMode mode, GpsPositionRecurrence recurrence, uint32_t minInterval,
                                     uint32_t prefAccuracy, uint32_t prefTime);
typedef int (*PFnAGnssmgtAgpsCloseConn)(void);
typedef int (*PFnAGnssmgtAgpsOpenFailed)(void);
typedef int (*PFnAGnssmgtSetServer)(AGpsType type, const char* hostname, int port);
typedef int (*PFnAGnssmgtOpenConn)(const char* apn);
typedef int (*PFnAGnssmgtAgpsOpenWithApnIpType)(const char* apn, ApnIpType apnIpType);
typedef int (*PFnAGnssmgtRilInit)(AGpsRilCallbacks* callbacks);
typedef int (*PFnAGnssmgtRilRefLocation)(const AGpsRefLocation* refLocation, size_t szStruct);
typedef int (*PFnAGnssmgtSetSetId)(AGpsSetIDType type, const char* setid);
typedef int (*PFnAGnssmgtSetRefLoc)(const AGpsRefLocation* refLocation, size_t szStruct);
typedef int (*PFnAGnssmgtUpdateNwState)(int connected, int type, int roaming, const char* extraInfo);
typedef int (*PFnAGnssmgtUpdateNwAvailability)(int avaiable, const char* apn);
typedef int (*PFnAGnssmgtRilSetId)(AGpsSetIDType type, const char* setid);
typedef int (*PFnAGnssmgtRilUpdateNetworkAvailability)(int avaiable, const char* apn);
typedef int (*PFnAGnssmgtRilUpdateNetworkState)(int connected, int type, int roaming, const char* extraInfo);

#endif // VENDOR_GNSS_ADAPTER_H_
