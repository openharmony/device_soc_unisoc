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

#ifndef _SNS_TOF_DRV_H_
#define _SNS_TOF_DRV_H_

#include "jpeg_exif_header.h"
#include "cmr_type.h"
#include "cmr_common.h"
#include "cmr_sensor_info.h"
#include "sensor_raw.h"
#include "sensor_drv_u.h"

#define TOF_SUCCESS CMR_CAMERA_SUCCESS
#define TOF_FAIL CMR_CAMERA_FAIL

struct sns_tof_drv_ops {

    /*export to isp*/
    int (*identify)(void);
    int (*init)(void);
    int (*deinit)(void);
    int (*ioctl)(enum sns_cmd cmd, void* param);
};

struct sns_tof_drv_entry {

    struct sns_tof_drv_ops tof_ops;
};

struct tof_drv_lib {
    void* tof_lib_handle;
    struct sns_tof_drv_entry* tof_info_ptr;
};
#endif
