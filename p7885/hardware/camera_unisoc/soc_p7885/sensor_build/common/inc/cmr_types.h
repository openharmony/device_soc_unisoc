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

#ifndef _CMR_TYPES_H_
#define _CMR_TYPES_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#include <sys/types.h>
#include "stdint.h"

typedef enum {
    PREVIEW_TYPE = 0,
    SNAPSHOT_TYPE,
    CALLBACK_TYPE,
    ASYNC_TYPE,
    REPROCESS_TYPE,
    VIDEO_TYPE,
    PREVIEW_VIDEO_TYPE,
    NOZSL_TYPE,
    SNAPSHOT_RAW_TYPE,
    SNAPSHOT_YUV_TYPE,
    MAX_TYPE,
} PipeLineType;


enum camera_mem_cb_type {
    CAMERA_PREVIEW = 0,
    CAMERA_SNAPSHOT,
    CAMERA_SNAPSHOT_ZSL,
    CAMERA_VIDEO,
    CAMERA_PREVIEW_RESERVED,
    CAMERA_SNAPSHOT_ZSL_RESERVED_MEM,
    CAMERA_SNAPSHOT_ZSL_RESERVED,
    CAMERA_VIDEO_RESERVED,
    CAMERA_ISP_LSC,
    CAMERA_ISP_BINGING4AWB,
    CAMERA_SNAPSHOT_PATH,
    CAMERA_ISP_FIRMWARE,
    CAMERA_SNAPSHOT_HIGHISO,
    CAMERA_ISP_RAW_DATA,
    CAMERA_ISP_ANTI_FLICKER,
    CAMERA_ISP_RAWAE,
    CAMERA_ISP_PREVIEW_Y,
    CAMERA_ISP_PREVIEW_YUV,
    CAMERA_DEPTH_MAP,
    CAMERA_DEPTH_MAP_RESERVED,
    CAMERA_PDAF_RAW,
    CAMERA_PDAF_RAW_RESERVED,
    CAMERA_ISP_STATIS,
    CAMERA_SNAPSHOT_3DNR,
    CAMERA_PREVIEW_3DNR,
    CAMERA_PREVIEW_SCALE_3DNR,
    CAMERA_PREVIEW_SCALE_AI_SCENE,
    CAMERA_PREVIEW_SCALE_AUTO_TRACKING,
    CAMERA_PREVIEW_DEPTH,
    CAMERA_PREVIEW_SW_OUT,
    CAMERA_PREVIEW_ULTRA_WIDE,
    CAMERA_VIDEO_ULTRA_WIDE,
    CAMERA_SNAPSHOT_ULTRA_WIDE,
    CAMERA_4IN1_PROC,
    CAMERA_SNAPSHOT_SLAVE_RESERVED,
    CAMERA_ISPSTATS_AEM,
    CAMERA_ISPSTATS_AFM,
    CAMERA_ISPSTATS_AFL,
    CAMERA_ISPSTATS_PDAF,
    CAMERA_ISPSTATS_BAYERHIST,
    CAMERA_ISPSTATS_YUVHIST,
    CAMERA_ISPSTATS_LSCM,
    CAMERA_ISPSTATS_3DNR,
    CAMERA_ISPSTATS_EBD,
    CAMERA_ISPSTATS_GTM,
    CAMERA_ISPSTATS_DEBUG,
    CAMERA_CHANNEL_0_RESERVED,
    CAMERA_CHANNEL_1,
    CAMERA_CHANNEL_1_RESERVED,
    CAMERA_CHANNEL_2,
    CAMERA_CHANNEL_2_RESERVED,
    CAMERA_CHANNEL_3,
    CAMERA_CHANNEL_3_RESERVED,
    CAMERA_CHANNEL_4,
    CAMERA_CHANNEL_4_RESERVED,
    CAMERA_CHANNEL_5_RESERVED,
    CAMERA_FD_SMALL,
    CAMERA_SNAPSHOT_ZSL_RAW,
    CAMERA_SNAPSHOT_ZSL_RGB,
    CAMERA_CAPTURE_FD_SMALL,
    CAMERA_AI_SFNR_FRAME,
    CAMERA_SNAPSHOT_FAST_THUMB,
    CAMERA_VIDEO_SLW_960FPS,
    CAMERA_SNAPSHOT_JPEG,
    CAMERA_SNAPSHOT_RAW,
    CAMERA_SNAPSHOT_RAW2,
    CAMERA_SNAPSHOT_YUV,
    CAMERA_SNAPSHOT_CAP_YUV,
    CAMERA_SNAPSHOT_THUMB_YUV,
    CAMERA_SNAPSHOT_THUMB_JPEG,
    CAMERA_DST_4IN1_PROC,
    CAMERA_MEM_CB_TYPE_MAX
};

enum camera_mem_is_cache {
    CACHE_FASLE = 0,
    CACHE_TRUE,
    CACHE_MAX
};

typedef unsigned long cmr_uint;
typedef long cmr_int;
typedef long long cmr_int64;
typedef uint64_t cmr_u64;
typedef int64_t cmr_s64;
typedef unsigned int cmr_u32;
typedef int cmr_s32;
typedef unsigned short cmr_u16;
typedef short cmr_s16;
typedef unsigned char cmr_u8;
typedef signed char cmr_s8;
typedef void *cmr_handle;
typedef int64_t nsecs_t;
struct CAMERA_MEM_CB_TYPE_STAT {
    cmr_uint    mem_type;
    cmr_uint    is_cache;
};

struct img_rect {
    cmr_u32 start_x;
    cmr_u32 start_y;
    cmr_u32 width;
    cmr_u32 height;
};

struct img_size {
    cmr_u32 width;
    cmr_u32 height;
};

struct thumb_info {
    bool buf_enough;
    bool buf_allocated;    //request buffer whether has allocated
    cmr_u32 buf_type;      //request buffer type:GRAPHICBUFFER, HALFRAWBUFFER etc
    cmr_u32 ori_height;    //thumb ori frame is preview or zsl
    cmr_u32 ori_width;     //thumb ori frame is preview or zsl
    cmr_u32 thumb_width;
    cmr_u32 thumb_height;
    cmr_int rot_angle;
    cmr_int flip_on;
    cmr_int fastThumb_en;
    cmr_int request_id;
    cmr_int need_filter;
};

struct mfnr_hdr_info {
    cmr_u32 mfnrv4_num;
    cmr_u32 is_hdr_on;
};

struct lwpnode_info {
    char nodename[32];
    int nodetime;
};

struct node_exif {
    char nodename[32];
    int nodetime;
    int nodenum;
    int64_t MultiFrameNodeDuration;
};

struct videoStab_info {
    float zoom_ratio;
    float total_zoom;
    cmr_s64 ae_time;
};

struct ae_status_info {
    float ev;
    cmr_u32 sof_id;
    cmr_u64 cur_effect_exp_time;
    cmr_u32 cur_effect_sensitivity;
    cmr_u8 cap_end_flag;
};

typedef enum{
    MINVALUE = 0,
    PREV2VID,
    VID2PREV,
}StreamCopyDirection_t;

#ifndef bzero
#define bzero(p, len) memset(p, 0, len);
#endif

#ifndef UNUSED
#define UNUSED(x) (void) (x)
#endif
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

typedef cmr_int (*cmr_malloc)(cmr_u32 mem_type, cmr_handle oem_handle,
                              cmr_u32 *size, cmr_u32 *sum, cmr_uint *phy_addr,
                              cmr_uint *vir_addr, cmr_s32 *fd);
typedef cmr_int (*cmr_free)(cmr_u32 mem_type, cmr_handle oem_handle,
                            cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *fd,
                            cmr_u32 sum);
typedef cmr_int (*cmr_gpu_malloc)(cmr_u32 mem_type, cmr_handle oem_handle,
                                  cmr_u32 *size, cmr_u32 *sum,
                                  cmr_uint *phy_addr, cmr_uint *vir_addr,
                                  cmr_s32 *fd, void **handle, cmr_uint *width,
                                  cmr_uint *height);

typedef cmr_int (*cmr_invalidate_buf)(cmr_handle oem_handle,
                              cmr_s32 buf_fd, cmr_u32 size,
                              cmr_uint phy_addr, cmr_uint vir_addr);
typedef cmr_int (*cmr_flush_buf)(cmr_handle oem_handle,
                              cmr_s32 buf_fd, cmr_u32 size,
                              cmr_uint phy_addr, cmr_uint vir_addr);
#endif
