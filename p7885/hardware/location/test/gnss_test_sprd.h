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
#ifndef GNSS_TEST_SPRD_H_
#define GNSS_TEST_SPRD_H_

#include "location_log.h"

#define GPS_MAX_SVS 32
#define GNSS_MAX_SVS 64

enum DL_RET {
    SO_OK = 0,
    LOAD_NOK = -1,
    FUNC_NOK = -2,
};

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

#endif // GNSS_TEST_SPRD_H_