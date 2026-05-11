/*
# Copyright 2016-2026 Unisoc (Shanghai) Technologies Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
 */

/************************************************************************/

//#ifdef WIN32

#include "sensor_raw.h"

#define NR_MAP_PARAM
#include "isp_nr.h"
#undef NR_MAP_PARAM
#define SPRD_TURING_DEFINE 1
/* Begin Include */
#include "sensor_0_raw_param_cap_0.c"
#include "sensor_0_raw_param_cap_1.c"
#include "sensor_0_raw_param_common.c"
#include "sensor_0_raw_param_prv_0.c"
#include "sensor_0_raw_param_prv_1.c"
#include "sensor_0_raw_param_video_0.c"
#include "sensor_0_raw_param_video_1.c"

/* End Include */

//#endif

/************************************************************************/

/* IspToolVersion=R1.17.0501 */

/* Capture Sizes:
        3264x2448,1632x1224
*/

/************************************************************************/

static struct SensorRawResolutionInfoTab s_0_trim_info = {
    0x00,
    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}};

/************************************************************************/

static struct SensorRawIoctrl s_0_ioctrl = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/********************************************************************************
 * static struct sensor_version_info s_****_version_info, **** is the sensor
 *name . Param[2]/ Param[3] are ASCII values of the sensor name string ****.
 * Please modify the sensor name by using rename sensor function of the ISP
 *TOOL, then the Param[2]/ Param[3] are changed accordingly. NO modifying
 *manually.
 ********************************************************************************/

static struct SensorVersionInfo s_0_version_info = {
    0x000D0010,
    {{0x00000030, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000}},
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000};

/************************************************************************/

static uint32_t s_0_libuse_info[] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000};

/************************************************************************/

static struct SensorRawInfo s_0_mipi_raw_info = {
    &s_0_version_info,
    {
        {s_0_tune_info_common, sizeof(s_0_tune_info_common)},
        {s_0_tune_info_prv_0, sizeof(s_0_tune_info_prv_0)},
        {s_0_tune_info_prv_1, sizeof(s_0_tune_info_prv_1)},
        {NULL, 0},
        {NULL, 0},
        {s_0_tune_info_cap_0, sizeof(s_0_tune_info_cap_0)},
        {s_0_tune_info_cap_1, sizeof(s_0_tune_info_cap_1)},
        {NULL, 0},
        {NULL, 0},
        {s_0_tune_info_video_0, sizeof(s_0_tune_info_video_0)},
        {s_0_tune_info_video_1, sizeof(s_0_tune_info_video_1)},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
    },
    &s_0_trim_info,
    &s_0_ioctrl,
    (struct SensorLibuseInfo *)s_0_libuse_info,
    {
        &s_0_fix_info_common,
        &s_0_fix_info_prv_0,
        &s_0_fix_info_prv_1,
        NULL,
        NULL,
        &s_0_fix_info_cap_0,
        &s_0_fix_info_cap_1,
        NULL,
        NULL,
        &s_0_fix_info_video_0,
        &s_0_fix_info_video_1,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
    },
    {
        {s_0_common_tool_ui_input, sizeof(s_0_common_tool_ui_input)},
        {s_0_prv_0_tool_ui_input, sizeof(s_0_prv_0_tool_ui_input)},
        {s_0_prv_1_tool_ui_input, sizeof(s_0_prv_1_tool_ui_input)},
        {NULL, 0},
        {NULL, 0},
        {s_0_cap_0_tool_ui_input, sizeof(s_0_cap_0_tool_ui_input)},
        {s_0_cap_1_tool_ui_input, sizeof(s_0_cap_1_tool_ui_input)},
        {NULL, 0},
        {NULL, 0},
        {s_0_video_0_tool_ui_input, sizeof(s_0_video_0_tool_ui_input)},
        {s_0_video_1_tool_ui_input, sizeof(s_0_video_1_tool_ui_input)},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
        {NULL, 0},
    },
    {
        &s_0_nr_scene_map_param,
        &s_0_nr_level_number_map_param,
        &s_0_default_nr_level_map_param,
    },
};
