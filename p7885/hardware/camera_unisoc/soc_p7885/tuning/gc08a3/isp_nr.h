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

#ifndef SPRD_DEVICE_H
#define SPRD_DEVICE_H

#ifdef NR_MAP_PARAM
static struct SensorNrSceneMapParam s_0_nr_scene_map_param = {
    {
        0x00000001, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000
    }
};
#endif

#ifdef _NR_BAYER_NR_PARAM_
#include "NR/common/normal/bayer_nr_param.h"
#endif

#ifdef _NR_VST_PARAM_
#include "NR/common/normal/vst_param.h"
#endif

#ifdef _NR_IVST_PARAM_
#include "NR/common/normal/ivst_param.h"
#endif

#ifdef _NR_RGB_DITHER_PARAM_
#include "NR/common/normal/rgb_dither_param.h"
#endif

#ifdef _NR_BPC_PARAM_
#include "NR/common/normal/bpc_param.h"
#endif

#ifdef _NR_CFAI_PARAM_
#include "NR/common/normal/cfai_param.h"
#endif

#ifdef _NR_RGB_AFM_PARAM_
#include "NR/common/normal/rgb_afm_param.h"
#endif

#ifdef _NR_CCE_UVDIV_PARAM_
#include "NR/common/normal/cce_uvdiv_param.h"
#endif

#ifdef _NR_3DNR_PARAM_
#include "NR/common/normal/3dnr_param.h"
#endif

#ifdef _NR_PPE_PARAM_
#include "NR/common/normal/ppe_param.h"
#endif

#ifdef _NR_UV_CDN_PARAM_
#include "NR/common/normal/uv_cdn_param.h"
#endif

#ifdef _NR_YNR_PARAM_
#include "NR/common/normal/ynr_param.h"
#endif

#ifdef _NR_EE_PARAM_
#include "NR/common/normal/ee_param.h"
#endif

#ifdef _NR_YUV_NOISEFILTER_PARAM_
#include "NR/common/normal/yuv_noisefilter_param.h"
#endif

#ifdef _NR_IMBALANCE_PARAM_
#include "NR/common/normal/imbalance_param.h"
#endif

#ifdef _NR_SW3DNR_PARAM_
#include "NR/common/normal/sw3dnr_param.h"
#endif

#ifdef _NR_BWU_BWD_PARAM_
#include "NR/common/normal/bwud_param.h"
#endif

#ifdef _NR_PYRAMID_ONLINE_PARAM_
#include "NR/common/normal/pyramid_onl_param.h"
#endif

#ifdef _NR_PYRAMID_OFFLINE_PARAM_
#include "NR/common/normal/pyramid_offl_param.h"
#endif

#ifdef _NR_DCT_PARAM_
#include "NR/common/normal/dct_param.h"
#endif

#ifdef _NR_CNR_H_PARAM_
#include "NR/common/normal/cnr_h_param.h"
#endif

#ifdef _NR_POST_CNR_H_PARAM_
#include "NR/common/normal/post_cnr_h_param.h"
#endif

#endif
