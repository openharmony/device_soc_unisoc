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
#ifndef SENSOR_ZOOM_INFO_H
#define SENSOR_ZOOM_INFO_H
#include <stdio.h>
#include <string.h>
#include "cmr_type.h"

typedef enum {
    /* Main camera of the related cam subsystem which controls*/
    CAM_TYPE_MAIN = 0,
    /* Aux camera of the related cam subsystem */
    CAM_TYPE_AUX,
    CAM_TYPE_AUX1 = CAM_TYPE_AUX,
    CAM_TYPE_AUX2,
    CAM_TYPE_AUX3,
    CAM_TYPE_OPTIMUTI,
} CameraType;

struct zoom_region{
    float Min;
    float Max;
    uint8_t Enable;
};

struct zoom_info_config {
    int PhyCameras;
    float MaxDigitalZoom;
    float BinningRatio;
    struct zoom_region PhyCamZoomRegion[CAM_TYPE_OPTIMUTI];
};

typedef enum {
    DEFAULT_ZOOM,
    DUALVIDEO_ZOOM,
    DUALVIDEO_FRONT_ZOOM,
    COMBINATION_MAX
} CombinationType;

struct sensor_zoom_info {
    bool init;
    struct zoom_info_config zoom_info_cfg[COMBINATION_MAX];
};

cmr_int sensor_drv_json_get_zoom_config_info(const char *filename,
                                             struct sensor_zoom_info *pZoomInfo);
#endif