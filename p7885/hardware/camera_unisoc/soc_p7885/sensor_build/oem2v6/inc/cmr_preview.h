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

#ifndef _CMR_PREVIEW_H_
#define _CMR_PREVIEW_H_
#include "cmr_common.h"
#include "cmr_type.h"
#include "sensor_drv_u.h"

#ifdef __cplusplus
extern "C" {
#endif


// abilty, max support buf num
#define ZSL_FRM_CNT GRAB_BUF_MAX
#define ZSL_ROT_FRM_CNT GRAB_BUF_MAX
#define PREV_FRM_CNT GRAB_BUF_MAX
#define PREV_ROT_FRM_CNT GRAB_BUF_MAX

#define PREV_FRM_ALLOC_CNT 8
#define PREV_ROT_FRM_ALLOC_CNT 8
#define PREV_ULTRA_WIDE_ALLOC_CNT 8
#define VIDEO_ULTRA_WIDE_ALLOC_CNT 8
#define VIDEO_BUF_ALLOC_MAX (PREV_FRM_ALLOC_CNT + PREV_ROT_FRM_ALLOC_CNT + VIDEO_SLOWMOTION_ALLOC_CNT)
#define ZSL_ULTRA_WIDE_ALLOC_CNT 1 //5
#define ZSL_FRM_ALLOC_CNT 10	//8

#define VIDEO_SLOWMOTION_ALLOC_CNT 144
#define VIDEO_960FPS_STEP1 93
#define VIDEO_960FPS_STEP2 62
#define VIDEO_960FPS_FILTER_NUM 3
#define VIDEO_960FPS_FRAME_NUM (VIDEO_960FPS_STEP1 * 2 + VIDEO_960FPS_STEP2)

#define CHANNEL0_BUF_CNT 30
#define CHANNEL1_BUF_CNT 8
#define CHANNEL1_BUF_CNT_ROT 8
#define CHANNEL2_BUF_CNT 8
#define CHANNEL2_BUF_CNT_HIGHFPS 16
#define CHANNEL2_BUF_CNT_ROT 8
#define CHANNEL3_BUF_CNT 8
#define CHANNEL3_BUF_CNT_ROT 8
#define CHANNEL4_BUF_CNT 8
#define CHANNEL4_BUF_CNT_ROT 8
#define CHANNEL5_BUF_CNT 30

#define RESET_AE_PARAM_INTERVAL 2

enum preview_func_type {
    PREVIEW_FUNC_START_PREVIEW = 0,
    PREVIEW_FUNC_STOP_PREVIEW,
    PREVIEW_FUNC_START_CAPTURE,
    PREVIEW_FUNC_MAX
};

enum preview_cb_type {
    PREVIEW_RSP_CB_SUCCESS = 0,
    PREVIEW_EVT_CB_FRAME,
    PREVIEW_EXIT_CB_FAILED,
    PREVIEW_EVT_CB_FLUSH,
    PREVIEW_EVT_CB_RESUME,
    PREVIEW_EXIT_CB_PREPARE,
    PREVIEW_EVT_CB_RAW_FRAME,
    PREVIEW_EVT_MAX
};

enum preview_op_evt {
    PREVIEW_CHN_PAUSE = CMR_GRAB_MAX + 1,
    PREVIEW_CHN_RESUME,
    PREVIEW_OP_MAX
};

enum preview_frame_type {
    PREVIEW_FRAME = 0,
    PREVIEW_VIDEO_FRAME,
    PREVIEW_ZSL_FRAME,
    PREVIEW_ZSL_RAW_FRAME,
    PREVIEW_ZSL_RAW_PROC_FRAME,
    PREVIEW_YUV_CAP_FRAME,
    CHANNEL0_FRAME,
    CHANNEL1_FRAME,
    CHANNEL2_FRAME,
    CHANNEL3_FRAME,
    CHANNEL4_FRAME,
    CHANNEL5_FRAME,
    PREVIEW_THUMBNAIL_FRAME,
    NO_ZSL_FRAME,
    PREVIEW_CANCELED_FRAME,
    PREVIEW_VIDEO_CANCELED_FRAME,
    PREVIEW_ZSL_CANCELED_FRAME,
    CALLBACK_CANCELED_FRAME,
    PREVIEW_FRAME_TYPE_MAX
};

typedef cmr_int (*preview_cb_func)(cmr_handle oem_handle,
                                   enum preview_cb_type cb_type,
                                   enum preview_func_type func_type,
                                   void *parm);

struct preview_md_ops {
    cmr_int (*channel_cfg)(cmr_handle oem_handle, cmr_handle caller_handle,
                           cmr_u32 camera_id,
                           struct channel_start_param *param_ptr,
                           cmr_u32 *channel_id, struct img_data_end *endian);
    cmr_int (*channel_start)(cmr_handle oem_handle, cmr_u32 channel_bits,
                             cmr_uint skip_bumber);
    cmr_int (*channel_pause)(cmr_handle oem_handle, cmr_uint channel_id,
                             cmr_u32 reconfig_flag);
    cmr_int (*channel_resume)(cmr_handle oem_handle, cmr_uint channel_id,
                              cmr_u32 skip_number, cmr_u32 deci_factor,
                              cmr_u32 frm_num);
    cmr_int (*channel_free_frame)(cmr_handle oem_handle, cmr_u32 channel_id,
                                  cmr_u32 index);
    cmr_int (*channel_stop)(cmr_handle oem_handle, cmr_u32 channel_bits);
    cmr_int (*channel_buff_cfg)(cmr_handle oem_handle,
                                struct buffer_cfg *buf_cfg);
    cmr_int (*channel_cap_cfg)(cmr_handle oem_handle, cmr_handle caller_handle,
                               cmr_u32 camera_id, struct cap_cfg *cap_cfg,
                               cmr_u32 *channel_id,
                               struct img_data_end *endian);
#ifdef CONFIG_CAMERA_OFFLINE
    cmr_int (*channel_dcam_size)(cmr_handle oem_handle,
                                 struct sprd_dcam_path_size *dcam_cfg);
#endif
    cmr_int (*channel_scale_capability)(cmr_handle oem_handle, cmr_u32 *width,
                                        cmr_u32 *sc_factor,
                                        cmr_u32 *sc_threshold);
    cmr_int (*channel_path_capability)(cmr_handle oem_handle,
                                       struct cmr_path_capability *capability);
    cmr_int (*channel_get_cap_time)(cmr_handle oem_handle, cmr_u32 *sec,
                                    cmr_u32 *usec);
    cmr_int (*isp_start_video)(cmr_handle oem_handle,
                               struct video_start_param *param_ptr);
    cmr_int (*isp_stop_video)(cmr_handle oem_handle);
    cmr_int (*sensor_open)(cmr_handle oem_handle, cmr_u32 camera_id);
    cmr_int (*sensor_close)(cmr_handle oem_handle, cmr_u32 camera_id);
    cmr_int (*sensor_dump_trace)(cmr_handle oem_handle, cmr_u32 camera_id);
    cmr_int (*start_rot)(cmr_handle oem_handle, cmr_handle caller_handle,
                         struct img_frm *src, struct img_frm *dst,
                         struct cmr_op_mean *mean);
    cmr_int (*start_dma)(cmr_handle oem_handle, cmr_handle caller_handle,
                         struct img_frm *src, struct img_frm *dst);
    cmr_int (*preview_pre_proc)(cmr_handle oem_handle, cmr_u32 camera_id,
                                cmr_u32 preview_sn_mode);
    cmr_int (*preview_post_proc)(cmr_handle oem_handle, cmr_u32 camera_id);
    cmr_int (*capture_pre_proc)(cmr_handle oem_handle, cmr_u32 camera_id,
                                cmr_u32 preview_sn_mode,
                                cmr_u32 capture_sn_mode, cmr_u32 is_restart,
                                cmr_u32 is_sn_reopen);
    cmr_int (*capture_post_proc)(cmr_handle oem_handle, cmr_u32 camera_id);
    cmr_int (*get_sensor_info)(cmr_handle oem_handle, cmr_uint sensor_id,
                               struct sensor_exp_info *sensor_info);
    cmr_int (*get_sensor_autotest_mode)(cmr_handle oem_handle,
                                        cmr_uint sensor_id,
                                        cmr_uint *is_autotest);
    cmr_int (*get_sensor_fps_info)(cmr_handle oem_handle, cmr_uint sensor_id,
                                   cmr_u32 sn_mode,
                                   struct sensor_mode_fps_tag *fps_info);
    cmr_int (*get_sensor_otp)(cmr_handle oem_handle, cmr_u8 dual_flag,
                              struct sensor_otp_cust_info *dual_otp_data);
    cmr_int (*get_buff_handle)(cmr_handle oem_handle, int frame_type,
                               cam_graphic_buffer_info_t *buf_info);
    cmr_int (*release_buff_handle)(cmr_handle oem_handle, int frame_type,
                                   cam_graphic_buffer_info_t *buf_info);
    cmr_int (*isp_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type,
                         struct common_isp_cmd_param *parm);
    cmr_int (*get_raw_fmt)(cmr_handle oem_handle, cmr_u32 *parm);
    cmr_int (*slowmotion_ctrl)(cmr_handle oem_handle, cmr_uint vid_frm_cnt, cmr_uint frm_cnt);
    cmr_int (*slw_960fps_cfg)(cmr_handle oem_handle);
    cmr_int (*start_capture)(cmr_handle oem_handle, cmr_uint cap_cnt);
    cmr_int (*stop_capture)(cmr_handle oem_handle);
};

struct preview_init_param {
    cmr_handle oem_handle;
    cmr_uint sensor_bits;
    preview_cb_func oem_cb;
    struct preview_md_ops ops;
    void *private_data;
};

struct ae_param_group {
    cmr_uint count;
    cmr_u32 sensitivity[5];
    cmr_u32 exposure_time[5];
    cmr_u32 ae_mode;
};

struct preview_param {
    cmr_uint is_fd_on;
    cmr_uint is_support_fd;
    cmr_u32 preview_eb;
    cmr_uint preview_fmt;
    cmr_uint prev_rot;
    struct img_size preview_size;

    cmr_u32 video_eb;
    struct img_size video_size;
    cmr_u32 video_fmt;

    cmr_u32 channel0_eb;
    cmr_u32 channel0_fmt;
    struct img_size channel0_size;

    cmr_u32 channel1_eb;
    cmr_u32 channel1_fmt;
    cmr_u32 channel1_rot_angle;
    struct img_size channel1_size;
    cmr_u32 channel1_flip_on;

    cmr_u32 channel2_eb;
    cmr_u32 channel2_fmt;
    cmr_u32 channel2_rot_angle;
    struct img_size channel2_size;
    cmr_u32 channel2_flip_on;

    cmr_u32 channel3_eb;
    cmr_u32 channel3_fmt;
    cmr_u32 channel3_rot_angle;
    struct img_size channel3_size;
    cmr_u32 channel3_flip_on;

    cmr_u32 channel4_eb;
    cmr_u32 channel4_fmt;
    cmr_u32 channel4_rot_angle;
    struct img_size channel4_size;
    cmr_u32 channel4_flip_on;

    cmr_u32 channel5_eb;
    cmr_u32 channel5_fmt;
    struct img_size channel5_size;

    cmr_u32 snapshot_eb;
    struct img_size picture_size;
    struct img_size raw_capture_size;
    struct img_size thumb_size;
    cmr_uint cap_fmt;
    cmr_u32 cap_rot;
    cmr_u32 is_cfg_rot_cap;
    cmr_u32 encode_angle;

    cmr_u32 is_dv;
    cmr_u32 isp_width_limit;
    cmr_u32 frame_ctrl;  // 0:stop,1:continue
    cmr_u32 frame_count; // 0xffffffff for zsl
    cmr_u32 flip_on;
    cmr_u32 tool_eb;
    void *private_data;
    cmr_u32 is_lls_enable;
    cmr_u32 is_ultra_wide;
    cmr_u32 is_960fps;
    cmr_u32 sprd_zsl_enabled;
    cmr_u32 high_fps_enabled;
    cmr_u32 sprd_afbc_enabled;
    cmr_u32 video_slowmotion_eb;
    cmr_u32 sprd_eis_enabled;
    cmr_u32 isp_to_dram;
    cmr_u32 video_snapshot_type;
    cmr_u32 sprd_3dcalibration_enabled;
    cmr_u32 remosaic_type; /* 1:software, 2:hardware, 0:not */
    cmr_u32 limited_4in1_width;
    cmr_u32 limited_4in1_height;
    struct sensor_4in1_info info_4in1;
    struct cmr_zoom_param zoom_setting;
    struct memory_param memory_setting;

    uint8_t is_yuv_capture;
    cmr_u8 control_scene_mode;
    enum top_app_id app_id;
};

struct preview_out_param {
    cmr_u32 preview_chn_id;
    cmr_u32 preview_sn_mode;
    struct img_data_end preview_data_endian;
    cmr_u32 snapshot_chn_id;
    cmr_u32 snapshot_sn_mode;
    struct img_data_end snapshot_data_endian;
    cmr_u32 zsl_frame;
    struct img_size actual_snapshot_size;
    struct img_size cap_org_size;
    cmr_u32 video_chn_id;
    cmr_u32 video_sn_mode;
    struct img_data_end video_data_endian;
    struct img_size actual_video_size;

    cmr_u32 channel0_chn_id;
    cmr_u32 channel0_sn_mode;
    struct img_data_end channel0_endian;
    struct img_size channel0_size;

    cmr_u32 channel1_chn_id;
    cmr_u32 channel1_sn_mode;
    struct img_data_end channel1_endian;
    struct img_size channel1_size;

    cmr_u32 channel2_chn_id;
    cmr_u32 channel2_sn_mode;
    struct img_data_end channel2_endian;
    struct img_size channel2_size;

    cmr_u32 channel3_chn_id;
    cmr_u32 channel3_sn_mode;
    struct img_data_end channel3_endian;
    struct img_size channel3_size;

    cmr_u32 channel4_chn_id;
    cmr_u32 channel4_sn_mode;
    struct img_data_end channel4_endian;
    struct img_size channel4_size;

    cmr_u32 channel5_chn_id;
    cmr_u32 channel5_sn_mode;
    struct img_data_end channel5_endian;
    struct img_size channel5_size;

    struct snp_proc_param post_proc_setting;
    struct sprd_dcam_raw_fmt raw_fmt;
};

struct preview_zsl_info {
    cmr_u32 is_support;
    cmr_u32 max_width;
    cmr_u32 max_height;
};

enum isp_status {
    PREV_ISP_IDLE = 0,
    PREV_ISP_COWORK,
    PREV_ISP_POST_PROCESS,
    PREV_ISP_ERR,
    PREV_ISP_MAX,
};

enum cvt_status {
    PREV_CVT_IDLE = 0,
    PREV_CVT_ROTATING,
    PREV_CVT_ROT_DONE,
    PREV_CVT_MAX,
};

enum chn_status { PREV_CHN_IDLE = 0, PREV_CHN_BUSY };

enum recovery_status {
    PREV_RECOVERY_IDLE = 0,
    PREV_RECOVERING,
    PREV_RECOVERY_DONE
};

enum recovery_mode {
    RECOVERY_LIGHTLY = 0,
    RECOVERY_MIDDLE,
    RECOVERY_HEAVY
};

struct rot_param {
    cmr_uint angle;
    struct img_frm *src_img;
    struct img_frm *dst_img;
};

// for channel0
typedef struct channel0 {
    cmr_u32 enable;
    struct img_size size;
    cmr_u32 buf_size;
    cmr_u32 buf_cnt;
    // valid_buf_cnt means how many buffers set to kernel driver
    cmr_u32 valid_buf_cnt;
    cmr_u32 shrink;
    cmr_u32 chn_id;
    cmr_u32 chn_status;
    cmr_u32 format;
    cmr_u32 skip_num;
    cmr_u32 skip_mode;
    unsigned long frm_cnt;
    struct img_data_end endian;
    struct img_frm frm[CHANNEL0_BUF_CNT];
    struct img_frm frm_reserved;
    cmr_u32 frm_valid;

    cmr_s32 fd[CHANNEL0_BUF_CNT];
    unsigned long addr_phy[CHANNEL0_BUF_CNT];
    unsigned long addr_vir[CHANNEL0_BUF_CNT];
} channel0_t;

// for channel1
typedef struct channel1 {
    cmr_u32 enable;
    struct img_size size;
    cmr_u32 buf_size;
    cmr_u32 buf_cnt;
    // valid_buf_cnt means how many buffers set to kernel driver
    cmr_u32 valid_buf_cnt;
    cmr_u32 shrink;
    cmr_u32 chn_id;
    cmr_u32 chn_status;
    cmr_u32 format;
    cmr_u32 skip_num;
    cmr_u32 skip_mode;
    unsigned long frm_cnt;
    struct img_data_end endian;
    struct img_frm frm[CHANNEL1_BUF_CNT];
    struct img_frm frm_reserved;
    cmr_u32 frm_valid;

    struct img_frm rot_frm[CHANNEL1_BUF_CNT_ROT];
    cmr_u32 rot_frm_lock_flag[CHANNEL1_BUF_CNT_ROT];
    cmr_u32 rot_index;

    cmr_s32 fd[CHANNEL1_BUF_CNT + CHANNEL1_BUF_CNT_ROT];
    unsigned long addr_phy[CHANNEL1_BUF_CNT + CHANNEL1_BUF_CNT_ROT];
    unsigned long addr_vir[CHANNEL1_BUF_CNT + CHANNEL1_BUF_CNT_ROT];
} channel1_t;

// for channel2
typedef struct channel2 {
    cmr_u32 enable;
    struct img_size size;
    cmr_u32 buf_size;
    cmr_u32 buf_cnt;
    // valid_buf_cnt means how many buffers set to kernel driver
    cmr_u32 valid_buf_cnt;
    cmr_u32 shrink;
    cmr_u32 chn_id;
    cmr_u32 chn_status;
    cmr_u32 format;
    cmr_u32 skip_num;
    cmr_u32 skip_mode;
    unsigned long frm_cnt;
    struct img_data_end endian;
    struct img_frm frm[CHANNEL2_BUF_CNT*2];
    struct img_frm frm_reserved;
    cmr_u32 frm_valid;

    struct img_frm rot_frm[CHANNEL2_BUF_CNT_ROT];
    cmr_u32 rot_frm_lock_flag[CHANNEL2_BUF_CNT_ROT];
    cmr_u32 rot_index;

    cmr_s32 fd[CHANNEL2_BUF_CNT + CHANNEL2_BUF_CNT_ROT];
    unsigned long addr_phy[CHANNEL2_BUF_CNT + CHANNEL2_BUF_CNT_ROT];
    unsigned long addr_vir[CHANNEL2_BUF_CNT + CHANNEL2_BUF_CNT_ROT];
} channel2_t;

// for channel3
typedef struct channel3 {
    cmr_u32 enable;
    struct img_size size;
    cmr_u32 buf_size;
    cmr_u32 buf_cnt;
    // valid_buf_cnt means how many buffers set to kernel driver
    cmr_u32 valid_buf_cnt;
    cmr_u32 shrink;
    cmr_u32 chn_id;
    cmr_u32 chn_status;
    cmr_u32 format;
    cmr_u32 skip_num;
    cmr_u32 skip_mode;
    unsigned long frm_cnt;
    struct img_data_end endian;
    struct img_frm frm[CHANNEL3_BUF_CNT];
    struct img_frm frm_reserved;
    cmr_u32 frm_valid;
    struct ae_param_group ae_param_list; //need initialize

    struct img_frm rot_frm[CHANNEL3_BUF_CNT_ROT];
    cmr_u32 rot_frm_lock_flag[CHANNEL3_BUF_CNT_ROT];
    cmr_u32 rot_index;
    cmr_uint g_channel3_frame_dequeue_cnt;

    cmr_s32 fd[CHANNEL3_BUF_CNT + CHANNEL3_BUF_CNT_ROT];
    unsigned long addr_phy[CHANNEL3_BUF_CNT + CHANNEL3_BUF_CNT_ROT];
    unsigned long addr_vir[CHANNEL3_BUF_CNT + CHANNEL3_BUF_CNT_ROT];
} channel3_t;

// for channel4
typedef struct channel4 {
    cmr_u32 enable;
    struct img_size size;
    cmr_u32 buf_size;
    cmr_u32 buf_cnt;
    // valid_buf_cnt means how many buffers set to kernel driver
    cmr_u32 valid_buf_cnt;
    cmr_u32 shrink;
    cmr_u32 chn_id;
    cmr_u32 chn_status;
    cmr_u32 format;
    cmr_u32 skip_num;
    cmr_u32 skip_mode;
    unsigned long frm_cnt;
    struct img_data_end endian;
    struct img_frm frm[CHANNEL4_BUF_CNT];
    struct img_frm frm_reserved;
    cmr_u32 frm_valid;

    struct img_frm rot_frm[CHANNEL4_BUF_CNT_ROT];
    cmr_u32 rot_frm_lock_flag[CHANNEL4_BUF_CNT_ROT];
    cmr_u32 rot_index;

    cmr_s32 fd[CHANNEL4_BUF_CNT + CHANNEL4_BUF_CNT_ROT];
    unsigned long addr_phy[CHANNEL4_BUF_CNT + CHANNEL4_BUF_CNT_ROT];
    unsigned long addr_vir[CHANNEL4_BUF_CNT + CHANNEL4_BUF_CNT_ROT];
} channel4_t;

// for channel5
typedef struct channel5 {
    cmr_u32 enable;
    struct img_size size;
    cmr_u32 buf_size;
    cmr_u32 buf_cnt;
    // valid_buf_cnt means how many buffers set to kernel driver
    cmr_u32 valid_buf_cnt;
    cmr_u32 shrink;
    cmr_u32 chn_id;
    cmr_u32 chn_status;
    cmr_u32 format;
    cmr_u32 skip_num;
    cmr_u32 skip_mode;
    unsigned long frm_cnt;
    struct img_data_end endian;
    struct img_frm frm[CHANNEL5_BUF_CNT];
    struct img_frm frm_reserved;
    cmr_u32 frm_valid;

    cmr_s32 fd[CHANNEL5_BUF_CNT];
    unsigned long addr_phy[CHANNEL5_BUF_CNT];
    unsigned long addr_vir[CHANNEL5_BUF_CNT];
} channel5_t;

struct frc_param {
    cmr_uint video_req_cnt;
    cmr_uint video_slw_cnt;
    cmr_uint frame_cnt;
    cmr_uint frc_frame_id;
    cmr_s64 start_timestamp;
    cmr_uint frm_is_lock[VIDEO_SLOWMOTION_ALLOC_CNT];
    struct frm_info filter_frm[VIDEO_960FPS_FILTER_NUM];
};

typedef struct ion_mme {
    cmr_s32 fd;
    cmr_uint vir;
    cmr_uint phy;
}ION_ADDR;

struct prev_context {
    cmr_uint camera_id;
    struct preview_param prev_param;

    cmr_int out_ret_val; /*for external function get return value*/

    /*preview*/
    struct img_size actual_prev_size;
    cmr_uint prev_status;
    cmr_uint prev_mode;
    cmr_uint latest_prev_mode;
    struct img_rect prev_rect;
    cmr_uint skip_mode;
    cmr_uint prev_channel_deci;
    cmr_uint prev_preflash_skip_en;
    cmr_uint prev_skip_num;
    cmr_uint prev_channel_id;
    cmr_uint prev_channel_status;
    struct img_data_end prev_data_endian;
    cmr_uint prev_frm_cnt;
    struct rot_param rot_param;
    cmr_s64 restart_timestamp;
    cmr_uint restart_skip_cnt;
    cmr_uint restart_skip_en;
    struct img_size lv_size;    /*isp lv size*/
    struct img_size video_size; /*isp video and scl size*/

    cmr_uint prev_self_restart;
    cmr_uint prev_buf_id;
    struct img_frm prev_frm[PREV_FRM_CNT];
    struct img_frm prev_reserved_frm;
    cmr_uint prev_rot_index;
    cmr_uint prev_rot_frm_is_lock[PREV_ROT_FRM_CNT];
    cmr_uint prev_ultra_wide_index;
    cmr_uint prev_ultra_wide_frm_is_lock[PREV_ULTRA_WIDE_ALLOC_CNT];
    struct img_frm prev_rot_frm[PREV_ROT_FRM_CNT];
    struct img_frm prev_ultra_wide_frm[PREV_ULTRA_WIDE_ALLOC_CNT];
    cmr_uint prev_phys_addr_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_uint prev_virt_addr_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_s32 prev_fd_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_uint prev_reserved_phys_addr;
    cmr_uint prev_reserved_virt_addr;
    cmr_s32 prev_reserved_fd;
    cmr_uint prev_mem_size;
    cmr_uint prev_mem_num;
    cmr_int prev_mem_valid_num;

    cmr_uint prev_mem_yuv_size;
    cmr_uint prev_mem_yuv_num;
    cmr_uint prev_phys_yuv_addr;
    cmr_uint prev_virt_yuv_addr;
    cmr_s32 prev_mfd_yuv;
    /*video*/
    struct img_size actual_video_size;
    cmr_uint video_status;
    cmr_uint video_mode;
    struct img_rect video_rect;
    // cmr_uint                        video_skip_mode;
    // cmr_uint                        prev_skip_num;
    cmr_uint video_channel_id;
    cmr_uint video_channel_status;
    struct img_data_end video_data_endian;
    cmr_uint video_frm_cnt;
    struct rot_param video_rot_param;
    cmr_s64 video_restart_timestamp;
    cmr_uint video_restart_skip_cnt;
    cmr_uint video_restart_skip_en;
    cmr_uint video_set_param;

    cmr_uint video_self_restart;
    cmr_uint video_buf_id;
    struct img_frm video_frm[PREV_FRM_CNT];
    struct img_frm video_reserved_frm;
    cmr_uint video_rot_index;
    cmr_uint video_rot_frm_is_lock[PREV_ROT_FRM_CNT];
    cmr_uint video_ultra_wide_index;
     cmr_uint video_ultra_wide_frm_is_lock[VIDEO_ULTRA_WIDE_ALLOC_CNT];
    struct img_frm video_rot_frm[PREV_ROT_FRM_CNT];
    cmr_uint video_slw_index;
    cmr_uint video_slw_frm_is_lock[VIDEO_SLOWMOTION_ALLOC_CNT];
    struct img_frm video_ultra_wide_frm[VIDEO_ULTRA_WIDE_ALLOC_CNT];
    struct img_frm video_slw_frm[VIDEO_SLOWMOTION_ALLOC_CNT];
    cmr_uint video_phys_addr_array[VIDEO_BUF_ALLOC_MAX];
    cmr_uint video_virt_addr_array[VIDEO_BUF_ALLOC_MAX];
    cmr_s32 video_fd_array[VIDEO_BUF_ALLOC_MAX];
    void *video_ultra_wide_handle_array[VIDEO_ULTRA_WIDE_ALLOC_CNT];
    cmr_uint video_reserved_phys_addr;
    cmr_uint video_reserved_virt_addr;
    cmr_s32 video_reserved_fd;
    cmr_uint video_mem_size;
    cmr_uint video_mem_num;
    cmr_int video_mem_valid_num;
    cmr_int cache_buffer_cont;

    cmr_u32 eis_video_mem_size;
    cmr_u32 eis_video_ultra_wide_mem_num;
    cmr_uint eis_video_phys_addr;
    cmr_uint eis_video_virt_addr;
    cmr_s32 eis_video_fd;
    void *dst_eis_video_buffer_handle;
    sem_t ultra_video;

    // for channel0
    channel0_t channel0;
    cmr_uint channel0_work_mode;

    // for channel1
    channel1_t channel1;
    struct img_size channel1_actual_pic_size;
    cmr_uint channel1_work_mode;

    // for channel2
    channel2_t channel2;
    struct img_size channel2_actual_pic_size;
    cmr_uint channel2_work_mode;

    // for channel3
    channel3_t channel3;
    struct img_size channel3_actual_pic_size;
    int channel3_pending_request;
    cmr_uint channel3_work_mode;

    // for channel4
    channel4_t channel4;
    struct img_size channel4_actual_pic_size;
    cmr_uint channel4_work_mode;

    // for channel5
    channel5_t channel5;
    cmr_uint channel5_work_mode;

    /*capture*/
    cmr_uint cap_mode;
    struct img_size max_size;
    struct img_size aligned_pic_size;
    struct img_size actual_pic_size;
    struct img_size dealign_actual_pic_size;
    struct channel_start_param restart_chn_param;
    cmr_uint cap_channel_id;
    cmr_uint cap_channel_status;
    cmr_uint zsl_channel_status;
    struct img_data_end cap_data_endian;
    cmr_uint cap_frm_cnt;
    cmr_uint cap_skip_num;
    cmr_uint cap_org_fmt;
    struct img_size cap_org_size;
    cmr_uint cap_zoom_mode;
    struct img_rect cap_sn_trim_rect;
    struct img_size cap_sn_size;
    struct img_rect cap_scale_src_rect;

    cmr_uint cap_phys_addr_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint cap_virt_addr_array[CMR_CAPTURE_MEM_SUM];
    cmr_s32 cap_fd_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint cap_phys_addr_path_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint cap_virt_addr_path_array[CMR_CAPTURE_MEM_SUM];
    cmr_s32 cap_fd_path_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint super_phys_addr_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint super_virt_addr_array[CMR_CAPTURE_MEM_SUM];
    cmr_s32 super_fd_array[CMR_CAPTURE_MEM_SUM];

    ION_ADDR snp_target_jpg;
    ION_ADDR snp_target_yuv;
    ION_ADDR snp_cap_yuv;
    ION_ADDR snp_raw;
    ION_ADDR snp_raw2;
    ION_ADDR snp_thumb_jpg;
    ION_ADDR snp_thumb_yuv;

    struct cmr_cap_mem cap_mem[CMR_CAPTURE_MEM_SUM];
    struct img_frm cap_frm[CMR_CAPTURE_MEM_SUM];
    cmr_uint is_zsl_frm;

    cmr_s64 cap_zsl_restart_timestamp;
    cmr_uint cap_zsl_restart_skip_cnt;
    cmr_uint cap_zsl_restart_skip_en;
    cmr_uint cap_zsl_frm_cnt;
    struct img_frm cap_zsl_frm[ZSL_FRM_CNT];
    struct img_frm cap_zsl_reserved_frm;
    struct buffer_cfg prealloc_zsl_buffer;
    cmr_uint cap_zsl_rot_index;
    cmr_uint cap_zsl_rot_frm_is_lock[ZSL_ROT_FRM_CNT];
    struct img_frm cap_zsl_rot_frm[ZSL_ROT_FRM_CNT];
    cmr_uint cap_zsl_ultra_wide_frm_is_lock[ZSL_FRM_ALLOC_CNT + ZSL_ULTRA_WIDE_ALLOC_CNT];
    struct img_frm cap_zsl_ultra_wide_frm[ZSL_ULTRA_WIDE_ALLOC_CNT];    //r001_lwp

    cmr_uint cap_zsl_phys_addr_array[ZSL_FRM_CNT + ZSL_ROT_FRM_CNT];
    cmr_uint cap_zsl_virt_addr_array[ZSL_FRM_CNT + ZSL_ROT_FRM_CNT];
    cmr_s32 cap_zsl_fd_array[ZSL_FRM_CNT + ZSL_ROT_FRM_CNT];
    cmr_s32 cap_zsl_ultra_wide_fd_array[ZSL_ROT_FRM_CNT];      //r001_lwp
    void *cap_zsl_ultra_wide_handle_array[ZSL_ROT_FRM_CNT];   //r001_lwp
    cmr_uint cap_zsl_reserved_phys_addr;
    cmr_uint cap_zsl_reserved_virt_addr;
    cmr_s32 cap_zsl_reserved_fd;

    cmr_uint cap_zsl_mem_size;
    cmr_uint cap_zsl_mem_num;
    cmr_int cap_zsl_mem_valid_num;

    cmr_uint cap_4in1_phys_addr_array[CAP_4IN1_NUM];
    cmr_uint cap_4in1_virt_addr_array[CAP_4IN1_NUM];
    cmr_s32 cap_4in1_fd_array[CAP_4IN1_NUM];
    cmr_uint cap_4in1_mem_size;
    cmr_uint cap_4in1_mem_num;
    cmr_int cap_4in1_mem_valid_num;

    cmr_uint is_reprocessing;
    cmr_uint capture_scene_mode;

    struct touch_coordinate touch_info;

    /*common*/
    struct img_size dcam_output_size;
    cmr_u32 need_isp;
    cmr_u32 need_binning;
    cmr_handle fd_handle;
    cmr_handle md_handle;
    cmr_handle frc_handle;
    cmr_handle frc_filter_handle;
    cmr_handle zsl_ultra_wide_pro_handle;
    cmr_handle video_ultra_wide_handle;
    cmr_handle refocus_handle;
    cmr_handle ai_scence_handle;
    cmr_uint recovery_status;
    cmr_uint recovery_cnt;
    cmr_uint recovery_en;
    cmr_uint isp_status;
    struct sensor_exp_info sensor_info;
    cmr_uint ae_time;
    void *private_data;
    cmr_uint vcm_step;
    /* face detect */
    cmr_s32 auto_tracking_last_frame;
    cmr_u32 fd_ae_info[FD_AE_MAX_INDEX];
    cmr_u32 hist[CAMERA_ISP_HIST_ITEMS];
    cmr_u32 af_status;
    cmr_uint threednr_cap_smallwidth;
    cmr_uint threednr_cap_smallheight;
    bool prev_zoom;
    bool cap_zoom;
    bool video_zoom;

    //20191030
    cmr_u32 sensor_out_width;
    cmr_u32 sensor_out_height;
    /* super macro */
    cmr_uint is_super;
    cmr_s32 gender_age_race;
    cmr_u32 dev_support_raw[DCAM_RAW_MAX];
    struct sprd_dcam_raw_fmt cur_raw;
    struct frc_param frc_slw;
    cmr_u32 is_raw_capture;
    cmr_u32 frc_mode;
    cmr_u32 frc_filter_mode;
    struct isp_ae_expo_cap_param cbstream_ae_cap_param;
    cmr_u32 skip_index;
};

struct prev_thread_cxt {
    cmr_uint is_inited;
    cmr_handle thread_handle;
    sem_t prev_sync_sem;
    sem_t prev_recovery_sem;
    pthread_mutex_t prev_mutex;
    pthread_mutex_t prev_stop_mutex;
    pthread_mutex_t video_mutex;
    /*callback thread*/
    cmr_handle cb_thread_handle;
    cmr_handle assist_thread_handle;
    cmr_handle zsl_thread_handle;
    cmr_handle video_thread_handle;
    cmr_handle prealloc_thread_handle;
    cmr_handle swalg_thread_handle;
};

struct prev_handle {
    cmr_handle oem_handle;
    cmr_uint sensor_bits; // multi-sensors need multi mem ? channel_cfg
    preview_cb_func oem_cb;
    struct preview_md_ops ops;
    void *private_data;
    struct prev_thread_cxt thread_cxt;
    struct prev_context prev_cxt[CAMERA_ID_MAX];
    cmr_uint frame_active;
    pthread_mutex_t fd_mutex;
};

cmr_int cmr_preview_init(struct preview_init_param *init_param_ptr,
                         cmr_handle *preview_handle_ptr);

cmr_int cmr_preview_deinit(cmr_handle preview_handle);

cmr_int cmr_preview_set_param(cmr_handle preview_handle, cmr_u32 camera_id,
                              struct preview_param *param_ptr,
                              struct preview_out_param *out_param_ptr);

cmr_int cmr_preview_start(cmr_handle preview_handle, cmr_u32 camera_id);

cmr_int cmr_preview_stop(cmr_handle preview_handle, cmr_u32 camera_id);

cmr_int cmr_preview_cancel_snapshot(cmr_handle preview_handle,
                                    cmr_u32 camera_id);

cmr_int cmr_preview_get_status(cmr_handle preview_handle, cmr_u32 camera_id);

void cmr_preview_wait_recovery(cmr_handle preview_handle, cmr_u32 camera_id);

cmr_int cmr_preview_get_prev_rect(cmr_handle preview_handle, cmr_u32 camera_id,
                                  struct img_rect *rect);

cmr_int cmr_preview_receive_data(cmr_handle preview_handle, cmr_u32 camera_id,
                                 cmr_uint evt, void *data);

cmr_int cmr_preview_get_sn_size(cmr_handle preview_handle, cmr_u32 camera_id,
                                 struct img_size *pic_size);
cmr_int cmr_preview_update_zoom(cmr_handle preview_handle, cmr_u32 camera_id,
                                struct cmr_zoom_param *param);

cmr_int cmr_preview_release_frame(cmr_handle preview_handle, cmr_u32 camera_id,
                                  cmr_uint index);

cmr_int cmr_preview_facedetect_set_ae_stab(cmr_handle preview_handle,
                                           cmr_u32 camera_id, cmr_u32 *ae_stab);

cmr_int cmr_preview_facedetect_set_hist(cmr_handle preview_handle,
                                        cmr_u32 camera_id, const cmr_u32 *ae_stab);

cmr_int cmr_preview_ctrl_motiondetect(cmr_handle preview_handle,
                                    cmr_u32 camera_id, cmr_uint on_off);

cmr_int cmr_preview_is_support_zsl(cmr_handle preview_handle, cmr_u32 camera_id,
                                   cmr_uint *is_support);

cmr_int cmr_preview_get_max_cap_size(cmr_handle preview_handle,
                                     cmr_u32 camera_id, cmr_uint *max_w,
                                     cmr_uint *max_h);

cmr_int cmr_preview_set_cap_size(
    cmr_handle preview_handle, cmr_u32 is_reprocessing, cmr_u32 camera_id,
    cmr_u32 width,
    cmr_u32 height); /**add for 3d capture to reset reprocessing capture size*/

cmr_int cmr_preview_get_post_proc_param(cmr_handle preview_handle,
                                        cmr_u32 camera_id, cmr_u32 encode_angle,
                                        struct snp_proc_param *out_param_ptr);

cmr_int cmr_preview_before_set_param(cmr_handle preview_handle,
                                     cmr_u32 camera_id,
                                     enum preview_param_mode mode);

cmr_int cmr_preview_after_set_param(cmr_handle preview_handle,
                                    cmr_u32 camera_id,
                                    enum preview_param_mode mode,
                                    enum img_skip_mode skip_mode,
                                    cmr_u32 skip_number);

cmr_int cmr_preview_set_preview_buffer(cmr_handle preview_handle,
                                       cmr_u32 camera_id,
                                       cam_buffer_info_t buffer);

cmr_int cmr_preview_set_video_buffer(cmr_handle preview_handle,
                                     cmr_u32 camera_id,
                                     cam_buffer_info_t buffer);

cmr_int cmr_preview_set_zsl_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                                   cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                                   cmr_s32 fd);

cmr_int cmr_preview_set_zsl_prealloc_buffers(cmr_handle preview_handle, cmr_u32 camera_id);


cmr_int cmr_preview_pop_zsl_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                                    struct frm_info *data);

cmr_int cmr_preview_set_ultra_wide_pro_frame(cmr_handle preview_handle, cmr_u32 camera_id,
                                    struct img_frm *data);
int cmr_channel0_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer);

int cmr_channel1_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer);

int cmr_channel2_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer);

int cmr_channel3_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer);

int cmr_channel4_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer);

int cmr_channel5_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer);

cmr_int prev_set_ae_time(cmr_handle preview_handle, cmr_u32 camera_id,
                         void *data);

cmr_int cmr_preview_channel3_pending_request(cmr_handle preview_handle, cmr_u32 camera_id,
                         int *pending_request);

cmr_int cmr_preview_get_zoom_factor(cmr_handle preview_handle,
                                    cmr_u32 camera_id, struct cmr_zoom *zoom_factor);

cmr_int cmr_preview_set_fd_touch_param(cmr_handle preview_handle,
                                       cmr_u32 camera_id,
                                       struct fd_touch_info *input_param);

cmr_int cmr_preview_get_prev_aspect_ratio(cmr_handle preview_handle,
                                          cmr_u32 camera_id,
                                          float *ratio);

cmr_uint cmr_preview_get_sn_work_mode(cmr_handle preview_handle,
                                      cmr_u32 camera_id);
cmr_s32 cmr_preview_start_capture_for_hdr_or_night(cmr_handle preview_handle, cmr_u32 camera_id, hdr_ev_info_t* hdr_info);

cmr_s32 add_cbsream_pending_request(struct prev_handle *handle, cmr_u32 camera_id, hdr_ev_info_t* hdr_info);

cmr_s32 clear_ev_queue(struct prev_handle *handle, cmr_u32 camera_id);

cmr_s32 reset_ae_setting_param(struct prev_handle *handle, cmr_u32 camera_id);

cmr_uint reduce_pending_request(struct prev_handle *handle, cmr_u32 camera_id);

cmr_s32 do_stop_capture_for_hdr(struct prev_handle *handle);

cmr_int cmr_preview_get_ultra_wide_handle(cmr_handle preview_handle,
                                      cmr_u32 camera_id,
                                      cmr_s32 buf_fd, void **handle);
cmr_int cmr_preview_set_video_status(cmr_handle preview_handle,
                                     cmr_u32 camera_id, bool on_off);
cmr_int cmr_preview_set_preview_skip_index(cmr_handle preview_handle,
                                     cmr_u32 camera_id, cmr_u32 index);

#ifdef __cplusplus
}
#endif

#endif
