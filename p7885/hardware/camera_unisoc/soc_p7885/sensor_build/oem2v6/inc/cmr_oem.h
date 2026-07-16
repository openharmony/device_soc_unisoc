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

#ifndef _CMR_OEM_H_
#define _CMR_OEM_H_

#include <inttypes.h>  
#include "cmr_common.h"
#include "SprdOEMCamera.h"
#include "cmr_cvt.h"
#include "cmr_focus.h"
#include "cmr_grab.h"
#include "cmr_isptool.h"
#include "cmr_jpeg.h"
#include "cmr_mem.h"
#include "cmr_msg.h"
#include "cmr_preview.h"
#include "cmr_sensor.h"
#include "cmr_setting.h"
#include "cmr_snapshot.h"
#include "isp_app.h"
#include "jpeg_exif_header.h"
#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
#include "cmr_mm_dvfs.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
/* should only define just one of the following two */
#define MIRROR_FLIP_ROTATION_BY_JPEG 1
#define JPG_ENCODE_WAIT_TIMEOUT 2000

struct debug_context {
    cmr_handle debug_handle;
    cmr_u32 inited;
    cmr_u32 dump_bits;
    char tags[128];
    void *dbg_data;
    struct buffer_cfg buff_cfg;
};

struct grab_context {
    cmr_handle grab_handle;
    /*	struct process_status    proc_status;*/
    cmr_handle caller_handle[GRAB_CHANNEL_MAX];
    cmr_u32 skip_number[GRAB_CHANNEL_MAX];
    cmr_u32 inited;
};

struct sensor_context {
    cmr_handle sensor_handle;
    cmr_u32 cur_id;
    cmr_u32 inited;
    struct sensor_exp_info sensor_info;
    EXIF_SPEC_PIC_TAKING_COND_T exif_info;
    struct sensor_ex_info cur_sns_ex_info;
    struct sensor_4in1_info info_4in1;
    struct img_otp_data warp_otp_data;
    struct img_size cur_sn_size;
};

struct isp_context {
    cmr_handle isp_handle;
    cmr_handle caller_handle;
    cmr_u32 isp_state; // 0 for preview, 1 for post process;
    cmr_u32 inited;
    cmr_u32 width_limit;
    cmr_u32 is_work;
    cmr_u32 is_snapshot;
    cmr_u32 is_af_bypass;
};

struct jpeg_context {
    cmr_handle jpeg_handle;
    cmr_u32 jpeg_state;
    cmr_u32 inited;
    cmr_handle enc_caller_handle;
    cmr_handle dec_caller_handle;
    struct jpeg_param param;
};

struct scaler_context {
    cmr_handle scaler_handle;
    cmr_u32 scale_state;
    cmr_u32 inited;
    cmr_handle caller_handle;
};

struct rotation_context {
    cmr_handle rotation_handle;
    cmr_u32 rot_state;
    cmr_u32 inited;
    cmr_handle caller_handle;
};

struct dma_context {
    cmr_handle dma_handle;
    cmr_u32 dma_state;
    cmr_u32 inited;
    cmr_handle caller_handle;
};

struct preview_context {
    cmr_handle preview_handle;
    cmr_u32 inited;
    cmr_u32 skip_num;
    cmr_u32 preview_eb;
    cmr_u32 preview_channel_id;
    cmr_u32 preview_sn_mode;
    struct img_data_end data_endian;
    cmr_u32 video_eb;
    cmr_u32 video_channel_id;
    cmr_u32 video_sn_mode;
    struct img_data_end video_data_endian;
    cmr_u32 channel0_eb;
    cmr_u32 channel0_chn_id;
    cmr_u32 channel0_sn_mode;
    struct img_data_end channel0_endian;
    cmr_u32 channel1_eb;
    cmr_u32 channel1_chn_id;
    cmr_u32 channel1_sn_mode;
    struct img_data_end channel1_endian;
    cmr_u32 channel2_eb;
    cmr_u32 channel2_chn_id;
    cmr_u32 channel2_sn_mode;
    struct img_data_end channel2_endian;
    cmr_u32 channel3_eb;
    cmr_u32 channel3_chn_id;
    cmr_u32 channel3_sn_mode;
    struct img_data_end channel3_endian;
    cmr_u32 channel4_eb;
    cmr_u32 channel4_chn_id;
    cmr_u32 channel4_sn_mode;
    struct img_data_end channel4_endian;
    cmr_u32 channel5_eb;
    cmr_u32 channel5_chn_id;
    cmr_u32 channel5_sn_mode;
    struct img_data_end channel5_endian;
    cmr_u32 snapshot_eb;
    cmr_uint status;
    struct img_size size;
    struct img_size video_size;
    struct img_size actual_video_size;
    struct frm_info video_cur_chn_data;
    struct img_rect rect;
    void *prv_aux_param;
    int cb_thr_pri;
    cmr_u8 hdr_cap_num;
    struct sprd_dcam_raw_fmt raw_fmt;
};

struct snapshot_context {
    cmr_handle snapshot_handle;
    cmr_u32 inited;
    cmr_u32 snapshot_sn_mode;
    cmr_u32 skip_num;
    cmr_u32 channel_id;
    cmr_u32 is_ai_sfnr;
    cmr_u32 is_mfsr;
    cmr_u32 total_num;
    cmr_u32 snap_cnt;
    cmr_u32 snp_mode;
    cmr_u32 is_cfg_rot_cap;
    cmr_u32 cfg_cap_rot;
    cmr_u32 status;
    cmr_u32 zsl_frame;
    cmr_u32 filter_type;
    cmr_u32 fb_on;
    cmr_uint is_req_snp;
    cmr_u8 is_super;
    // fix burst called cmr_grab_start_capture repeatedly
    cmr_u32 start_capture_flag;
    cmr_s64 cap_time_stamp;
    cmr_s64 cap_need_time_stamp;
    struct img_size request_size;
    struct img_size capture_align_size;
    struct img_size capture_org_size;
    struct img_size actual_capture_size;
    struct frm_info cur_frm_info;
    struct snp_proc_param post_proc_setting;
    struct img_data_end data_endian;
    struct frm_info cur_chn_data;
    struct touch_coordinate touch_xy;
    struct cmr_zoom_param zoom_param;
    struct cmr_zoom_param zoom_param_1x;
    void *ae_common_info;
    void *snp_aux_param;
    void *ae_exp_gain_info;
    cmr_u32 raw_fmt;
    cmr_u32 set_ev_group_flag;
    cmr_u32 set_ev_flag;
    cmr_u32 set_3dnr_flag;
};

struct focus_context {
    cmr_handle focus_handle;
    cmr_u32 inited;
    cmr_u32 af_support;
    cmr_u32 padding;
    cmr_u32 af_bypass;
    cmr_u32 is_in_focus;
    cmr_u32 af_locked;
    cmr_s64 af_start_time;
    cmr_s64 af_stop_time;
};

struct setting_context {
    cmr_handle setting_handle;
    cmr_u32 inited;
    cmr_u32 is_active;
    cmr_u32 is_auto_iso;
    cmr_uint iso_value;
    cmr_u64 awb_cmd_value;
};
#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
struct mm_dvfs_context {
    cmr_handle mm_dvfs_handle;
    cmr_u32 inited;
};
#endif

struct camera_settings {
    cmr_u32 preview_width;
    cmr_u32 preview_height;
    cmr_u32 snapshot_width;
    cmr_u32 snapshot_height;
    cmr_u32 focal_len;
    cmr_u32 brightness;
    cmr_u32 contrast;
    cmr_u32 effect;
    cmr_u32 expo_compen;
    cmr_u32 wb_mode;
    cmr_u32 saturation;
    cmr_u32 sharpness;
    cmr_u32 scene_mode;
    cmr_u32 flash;
    cmr_u32 auto_flash_status;
    cmr_u32 night_mode;
    cmr_u32 flicker_mode;
    cmr_u32 focus_rect;
    cmr_u32 af_mode;
    cmr_u32 iso;
    cmr_u32 luma_adapt;
    cmr_u32 video_mode;
    cmr_u32 frame_rate;
    cmr_u32 sensor_mode;
    cmr_u32 auto_exposure_mode;
    cmr_u32 preview_env;
    /*snapshot param*/
    cmr_u32 quality;
    cmr_u32 thumb_quality;
    cmr_u32 set_encode_rotation;
    struct img_size thum_size;
    cmr_u32 cap_rot;
    cmr_u32 is_cfg_rot_cap;
    cmr_u32 is_dv; /*1 for DV, 0 for DC*/
    cmr_u32 total_cap_num;
    cmr_u32 is_andorid_zsl;

    /*all the above value will be set as 0xFFFFFFFF after inited*/
    cmr_u32 set_end;
    struct cmr_zoom_param zoom_param;
    uint32_t isp_alg_timeout;
    sem_t isp_alg_sem;
    pthread_mutex_t isp_alg_mutex;
};

struct isp_scene_settings {
    cmr_u32 scene_mode;
    cmr_u32 last_isp_scene_mode;//set scene mode to isp mw
    cmr_u32 is_auto_hdr;
    cmr_u32 is_hdr;
    cmr_u32 is_filter;
    cmr_u32 is_facebeauty;
    cmr_u32 is_eis;
    cmr_u32 is_burst;
    cmr_u32 auto_flash;
    cmr_u32 isp_flash_mode;
    cmr_u32 is_zoom;
    cmr_u32 is_night;
    cmr_u32 is_face_num;
    struct cmr_range_fps_param range_fps;
};

typedef enum {
    JPEG_ENCODE_MIN = 0,
    JPEG_ENCODING,
    JPEG_ENCODE_DONE,
    JPEG_ENCODE_STOP,
    JPEG_ENCODE_MAX,
} jpg_encode_status;

typedef enum {
    SCALE_MIN = 0,
    SCALING,
    SCALE_DONE,
    SCALE_STOP,
    SCALE_MAX,
} scale_status;

struct info_4in1 {
    cmr_u32 is_4in1_supported;  /* 191105: 1: software remosaic; 0:other */
    cmr_u32 limited_4in1_width; /* >0: 4in1 sensor, 0: other */
    cmr_u32 limited_4in1_height;
    cmr_u32 *sns_mode; // sensor mode for 4in1
    cmr_u32 dcam_raw_fmt;
    cmr_u32 sensor_raw_fmt;
};

struct camera_context {
    /*for the device OEM layer owned*/
    struct grab_context grab_cxt;
    struct sensor_context sn_cxt;
    struct isp_context isp_cxt;
    struct jpeg_context jpeg_cxt;
    struct scaler_context scaler_cxt;
    struct rotation_context rot_cxt;
    struct dma_context dma_cxt;
    struct preview_context prev_cxt;
    struct snapshot_context snp_cxt;
    struct focus_context focus_cxt;
    struct setting_context setting_cxt;
    struct debug_context dbg_cxt;

#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
    struct mm_dvfs_context mm_dvfs_cxt;
#endif
    /*for the workflow management*/
    cmr_u32 camera_id;
    cmr_u32 err_code;
    camera_cb_of_type camera_cb;
    void *client_data;
    cmr_u32 inited;
    cmr_u32 camera_mode;
    cmr_u32 fastThumb_en;
    cmr_u32 is_ai_sfnr;
    cmr_u32 mfsr_force_off;
    cmr_uint is_discard_frm;
    cmr_u8 bypass_blc_enable;
    sem_t mfsr_sm;
    sem_t ai_scene_flag_sm;

    sem_t cnr_flag_sm;
    sem_t dre_flag_sm;
    sem_t dre_pro_flag_sm;

    sem_t threednr_flag_sm;
    sem_t threednr_proc_sm;
    sem_t filter_sm;
    sem_t share_path_sm;
    sem_t access_sm;
    sem_t sbs_sync_sm;
    sem_t snapshot_sm;
    pthread_mutex_t prealloc_status_mutex;

    cmr_uint share_path_sm_flag;
    cmr_handle init_thread;
    cmr_handle jpeg_async_init_handle;
    cmr_int facing;

    /*wait camera_res_init_done*/
    bool need_wait_init_done_flag;

    /*callback thread to hal*/
    cmr_handle prev_cb_thr_handle;
    cmr_handle snp_cb_thr_handle;
    cmr_handle snp_secondary_thr_handle;
    cmr_handle snp_send_raw_image_handle;
    cmr_handle snp_capture_ctrl_handle;
    cmr_handle iss_handler;

    /*for setting*/
    cmr_u32 ref_camera_id;
    cmr_uint ai_scene_enable;
    struct camera_settings cmr_set;
    cmr_u32 is_support_fd;
    cmr_u32 fd_on_off;
    struct isp_face_area fd_face_area;
    struct isp_scene_settings isp_scene;
    cmr_u32 is_android_zsl;
    cmr_u32 flip_on;
    cmr_u32 is_lls_enable;
    cmr_u32 lls_shot_mode;
    cmr_u32 isp_to_dram;
    cmr_int cap_cnt;
    multiCameraMode is_multi_mode;
    uint8_t master_id;
    cmr_u32 is_refocus_mode;
    cmr_u32 is_3dcalibration_mode;
    cmr_uint is_yuv_callback_mode;
    cmr_uint is_cb_yuv_scaler;

    /*memory func*/
    camera_cb_of_malloc hal_malloc;
    camera_cb_of_free hal_free;
    camera_cb_of_gpu_malloc hal_gpu_malloc;
    void *hal_mem_privdata;

    cmr_u8 flag_highiso_alloc_mem;
    cmr_uint dump_cnt;
    cmr_uint is_start_snapshot;
    cmr_u32 blur_facebeauty_flag;
    cmr_uint is_ultra_wide;
    cmr_uint is_ultra_wide_pro;
    cmr_u8 is_dual_video;
    cmr_uint is_fov_fusion;
    float app_ratio;
    cmr_uint is_multi_camera_id;
    cmr_u32 is_focus;
    struct isp_pos focus_rect;
    cmr_int lcd_flash_highlight;
    cmr_u32 backlight_brightness;
    cmr_u32 max_backlight_brightness;
    cmr_u32 backup_brightness;
    cmr_u16 color_temp;
    cmr_u32 bg_color;
    cmr_u8 control_scene_mode;
    // enhance_device_t *enhance;

    cmr_s64 hdr_capture_timestamp;
    cmr_s64 capture_timestamp;
    cmr_u32 ai_sfnr_capture_state;
    cmr_u32 skip_frame_enable;
    cmr_int skip_frame_cnt;
    enum camera_snapshot_tpye snapshot_type;
    struct img_rect trim_reset_info;
    struct img_addr ai_sfnr_dcam_buffer[AI_SFNR_PIPLINE_NUM];
    cmr_u8 nr_flag;
    cmr_u8 warppro_flag;
    cmr_u8 dre_flag;
    cmr_u8 gtm_flag;
    cmr_u8 predre_flag;
    cmr_u8 skipframe;
    /*for flash skip preview frame*/
    cmr_s64 flash_handle_timestamp;
    cmr_u32 flash_skip_frame_enable;
    cmr_u32 flash_skip_frame_cnt;
    cmr_u32 flash_skip_frame_num;
    struct isp_face_area fd_face_area_capture;
    bool is_capture_face;
    cmr_u32 zsl_enabled; /* 1: zsl,0: non-zsl */
    cmr_u32 is_thumb_cb;

    /* new 4in1 plan, 20191028 */
    //add for sw remosaic alloc dst buff
    struct img_frm sw_remosaic_dst_param;
    struct info_4in1 info_4in1;
    cmr_u32 cap_4in1_dst_size;
    cmr_u32 sw_remosaic_inited;
    cmr_u32 is_4in1_sensor; /* as is_4in1_sensor, should rename later */
    cmr_u32 remosaic_type; /* 1: software, 2: hardware, 0:other(sensor output bin size) */
    cmr_u32 ambient_highlight; /* 4in1: 1:highlight,0:lowlight; other sensor:0 */
    cmr_uint is_high_res_mode;
    cmr_uint is_support_front_16M; /*for l5 pro front picture size from 4M to 16M*/
    /*for ynr room ratio*/
    float zoom_ratio;
    jpg_encode_status jpg_encode;
    scale_status scale;
    cmr_u8 nightscepro_flag;
    bool snp_cancel;
    cmr_u8 night_flag;//night mode
    cmr_u8 nightpro_flag;
    void *aux_param;
    cmr_uint long_expo_enable;
    cmr_u32 longexp_skipnum;
    cmr_u8 longexp_zsl_done;
    cmr_u64 exp_time;
    cmr_uint flash_mode;
    cmr_uint long_expo_cap;
    enum top_app_id app_id;
    /*af roi for mfnr select frame*/
    struct isp_afctrl_roi af_roi;
    struct ae_aux_param_t ae_aux_info; //gain, exp
    cmr_u8 ev_ctrl_capture;
    cmr_u32 cam_type;
    float appRatio;
    cmr_uint gesture_detect_enable;
    int capture_burst; /* 1:burst,0:other */
    cmr_s32 cur_bv; /* callback after ae calc */
    cmr_u8 prev_skip_fd;
    cmr_u32 is_support_md;
    cmr_u32 md_on_off;
    cmr_u32 mp_on_off;
    cmr_u32 is_960fps;

    cmr_u8 ai_scene_type;
    struct scenario_info scen_info;
    cmr_u32 postproc_type;

    bool prealloc_zslbuffer_flag;
    struct ae_callback_param_for_ai_sfnr ai_sfnr;
    bool is_raw_capture;
    uint8_t is_yuv_capture;
    uint8_t video_regular_mode;
    struct img_rect base_rect;
    bool is_raw_stream_capture;
    bool is_sensor_raw_required;
};

struct prev_ai_scene_info {
    struct img_frm frm_preview;
    unsigned long camera_id;
    void *caller_handle;
    struct frm_info data;
};

struct prev_gesture_detect_info {
    struct img_frm frm_preview;
    unsigned long camera_id;
    void *caller_handle;
    bool is_detected;
    struct frm_info data;
    cmr_u32 orientation;
};

cmr_int camera_local_int(cmr_u32 camera_id, camera_cb_of_type callback,
                         void *client_data, cmr_uint is_autotest,
                         cmr_handle *oem_handle, void *cb_of_malloc,
                         void *cb_of_free);

cmr_int camera_local_deinit(cmr_handle oem_handle);

cmr_int camera_local_md_start(cmr_handle oem_handle);

cmr_int camera_local_start_preview(cmr_handle oem_handle,
                                   enum takepicture_mode mode,
                                   cmr_uint is_snapshot);
cmr_int camera_local_stop_preview(cmr_handle oem_handle);

cmr_int camera_local_start_snapshot(cmr_handle oem_handle,
                                    enum takepicture_mode mode,
                                    struct snap_input_data *req);

cmr_int camera_local_stop_snapshot(cmr_handle oem_handle);

cmr_int camera_local_redisplay_data(
    cmr_handle oem_handle, cmr_s32 output_fd, cmr_uint output_addr,
    cmr_uint output_vir_addr, cmr_uint output_width, cmr_uint output_height,
    cmr_s32 input_fd, cmr_uint input_addr_y, cmr_uint input_addr_uv,
    cmr_uint input_vir_addr, cmr_uint input_width, cmr_uint input_height);

cmr_int camera_local_get_prev_rect(cmr_handle oem_handle,
                                   struct img_rect *param_ptr);

cmr_int camera_get_sensor_mode_info(cmr_handle oem_handle,
                                    struct sensor_mode_info *mode_info);

cmr_int camera_get_sensor_mode_trim(cmr_handle oem_handle,
                                    struct img_rect *sn_trim);

cmr_int camera_get_senor_mode_trim2(cmr_handle oem_handle,
                                    struct img_rect *sn_trim);

cmr_uint camera_get_preview_angle(cmr_handle oem_handle);

cmr_uint camera_get_result_exif_info(
    cmr_handle oem_handle, struct exif_spec_pic_taking_cond_tag *exif_pic_info);

cmr_int camera_write_calibration_otp(cmr_handle oem_handle, struct cal_otp_info *param);

cmr_int camera_local_start_focus(cmr_handle oem_handle);

cmr_int camera_local_cancel_focus(cmr_handle oem_handle);

cmr_int prev_set_preview_skip_frame_num(cmr_handle preview_handle,
                                        cmr_u32 camera_id, cmr_uint skip_num,
                                        cmr_uint has_preflashed);

cmr_int camera_isp_set_params(cmr_handle camera_handle,
                              enum camera_param_type id, cmr_uint param);

cmr_int camera_local_set_param(cmr_handle camera_handle,
                               enum camera_param_type id, uint64_t param);

cmr_int camera_local_get_zsl_info(cmr_handle oem_handle, cmr_uint *is_support,
                                  cmr_uint *max_width, cmr_uint *max_height);

cmr_int camera_local_fast_ctrl(cmr_handle oem_handle);

cmr_int camera_local_preflash(cmr_handle oem_handle);

cmr_int camera_local_get_viewangle(cmr_handle oem_handle,
                                   struct sensor_view_angle *view_angle);

cmr_int camera_local_set_preview_buffer(cmr_handle oem_handle,
                                        cmr_uint src_phy_addr,
                                        cmr_uint src_vir_addr, cmr_s32 fd);
cmr_int camera_local_set_zsl_buffer(cmr_handle oem_handle,
                                    cmr_uint src_phy_addr,
                                    cmr_uint src_vir_addr, cmr_s32 fd);
cmr_int camera_local_set_raw_buffer(cmr_handle oem_handle, struct buffer_cfg *buf_cfg);
cmr_int camera_lwp_start_capture(cmr_handle oem_handle,
        struct sprd_img_capture_param *capture_param);
cmr_int camera_local_postproc_raw(cmr_handle oem_handle, uint32_t cmd, struct sprd_img_parm *param);

cmr_int camera_local_pop_zsl_buffer(cmr_handle oem_handle,
                                        struct frm_info *data);
cmr_int camera_local_set_raw_buffer(cmr_handle oem_handle, struct buffer_cfg *buf_cfg);

cmr_s32 local_queue_buffer(cmr_handle camera_handle, cam_buffer_info_t buffer,
                           int steam_type);

cmr_int camera_local_set_video_snapshot_buffer(cmr_handle oem_handle,
                                               cmr_uint src_phy_addr,
                                               cmr_uint src_vir_addr,
                                               cmr_s32 fd);

cmr_int camera_local_set_zsl_snapshot_buffer(cmr_handle oem_handle,
                                             cmr_uint src_phy_addr,
                                             cmr_uint src_vir_addr, cmr_s32 fd);

cmr_int camera_local_nozsl_post_proc(cmr_handle oem_handle,
                                             cmr_uint src_phy_addr,
                                             cmr_uint src_vir_addr,
                                             cmr_s32 fd);

cmr_int camera_local_zsl_snapshot_need_pause(cmr_handle oem_handle,
                                             cmr_int *flag);
cmr_int camera_local_normal_snapshot_need_pause(cmr_handle oem_handle,
                                                cmr_int *flag);
void camera_calibrationconfigure_save(uint32_t start_addr, uint32_t data_size);

cmr_int camera_local_get_last_preflash_time(cmr_handle oem_handle, cmr_s64 *time);

void camera_local_start_burst_notice(cmr_handle oem_handle);
void camera_local_end_burst_notice(cmr_handle oem_handle);

void destruct_scenario_info(cmr_handle oem_handle);

cmr_s32 camera_local_get_iommu_status(cmr_handle oem_handle);

cmr_int camera_set_security(cmr_handle oem_handle,
                            struct sprd_cam_sec_cfg *sec_cfg);
cmr_int camera_set_zsl_param(cmr_handle oem_handle,
                            struct sprd_cap_zsl_param *zsl_param);
cmr_int
camera_isp_set_sensor_info_to_af(cmr_handle oem_handle,
                                 struct cmr_af_aux_sensor_info *sensor_info);
cmr_int cmr_get_sensor_max_fps(cmr_handle oem_handle, cmr_u32 camera_id,
                               cmr_u32 *max_fps);
cmr_int cmr_sensor_init_static_info(cmr_handle oem_handle);
cmr_int cmr_sensor_deinit_static_info(cmr_handle oem_handle);

cmr_int cmr_set_zoom_factor_to_isp(cmr_handle oem_handle, float *zoomFactor);

cmr_int prev_set_preview_touch_info(cmr_handle preview_handle,
                                    cmr_u32 camera_id,
                                    struct touch_coordinate *touch_xy);

cmr_int camera_local_snapshot_is_need_flash(cmr_handle oem_handle,
                                            cmr_u32 camera_id,
                                            cmr_u32 *is_need_flash);
cmr_int camera_get_otpinfo(cmr_handle oem_handle, cmr_u8 dual_flag,
                           struct sensor_otp_cust_info *otp_data);
cmr_int camera_get_onlinebuffer(cmr_handle oem_handle, void *cali_info);
cmr_int prev_set_vcm_step(cmr_handle preview_handle, cmr_u32 camera_id,
                          void *data);
cmr_int cmr_get_sensor_vcm_step(cmr_handle oem_handle, cmr_u32 camera_id,
                                cmr_u32 *max_fps);
cmr_int cmr_get_vcm_range(cmr_handle oem_handle, cmr_u32 camera_id,
                          struct vcm_range_info *vcm_range);
cmr_int cmr_get_ae_fps_range(cmr_handle oem_handle, cmr_u32 camera_id,
                          struct ae_fps_range_info *ae_fps_range);
cmr_int cmr_set_vcm_disc(cmr_handle oem_handle, cmr_u32 camera_id,
                         struct vcm_disc_info *vcm_disc);
int af_state_focus_to_hal(cmr_u32 valid_win);
cmr_int camera_local_set_sensor_close_flag(cmr_handle oem_handle);
cmr_int camera_local_set_cap_size(
    cmr_handle oem_handle, cmr_u32 is_reprocessing, cmr_u32 camera_id,
    cmr_u32 width,
    cmr_u32 height); /**add for 3d capture to reset reprocessing capture size*/

cmr_int camera_local_start_capture(cmr_handle oem_handle, struct snap_input_data *req);
cmr_int camera_local_stop_capture(cmr_handle oem_handle);

void camera_set_oem_multimode(multiCameraMode camera_mode);
void camera_set_oem_masterid(uint8_t master_id);
cmr_int camera_local_set_ref_camera_id(cmr_handle oem_handle,
                                       cmr_u32 *ref_camera_id);
cmr_int camera_local_set_visible_region(cmr_handle oem_handle,
                                        struct visible_region_info *info);
cmr_int camera_local_set_global_zoom_ratio(cmr_handle oem_handle, float *ratio);
cmr_int camera_set_eis_move_info(cmr_handle oem_handle, cmr_u8 *move_info);
cmr_int camera_local_cap_state(cmr_handle oem_handle,
                                       bool *flag);
cmr_int camera_local_dcam_state(cmr_handle oem_handle,
                                       bool *flag);

cmr_int camera_local_get_cover(cmr_handle cmr_handle,
                               struct dual_sensor_luma_info *cover_value);
cmr_int camera_cpat_get_cover(cmr_handle oem_handle, cmr_handle luma_info);

cmr_int camera_stream_ctrl(cmr_handle cmr_handle, cmr_u32 on_off);
cmr_int cmr_get_isp_af_fullscan(cmr_handle oem_handle,
                                struct isp_af_fullscan_info *af_fullscan_info);
cmr_int cmr_set_af_pos(cmr_handle oem_handle, cmr_u32 af_pos);
cmr_int cmr_set_af_bypass(cmr_handle oem_handle, cmr_u32 value);

cmr_int cmr_set_3a_bypass(cmr_handle oem_handle, cmr_u32 value);
cmr_int cmr_get_ae_fps(cmr_handle oem_handle, cmr_u32 *ae_fps);
void cmr_do_postprocss_lwp(cmr_handle oem_handle, struct frm_info *frame);
cmr_int camera_local_reprocess_yuv_for_jpeg(cmr_handle oem_handle,
                                            enum takepicture_mode mode,
                                            cmr_uint yaddr, cmr_uint yaddr_vir,
                                            cmr_uint fd, void *buffer_handle);
cmr_int camera_set_ultra_wide_mode(cmr_handle oem_handle,
                                   cmr_uint is_ultra_wide);
cmr_int camera_set_slowmotion_mode(cmr_handle oem_handle,
                                   cmr_uint is_960fps);
cmr_int camera_set_video_mode(cmr_handle oem_handle, bool flag);
cmr_int camera_set_fov_fusion_app_ratio(cmr_handle oem_handle,
                                   float app_ratio);
cmr_int camera_set_sync_state(cmr_handle oem_handle,
                                   SyncState *mSyncState);
cmr_int cmr_set_snapshot_timestamp(cmr_handle oem_handle, int64_t timestamp);
cmr_int cmr_get_microdepth_param(cmr_handle oem_handle, void *param);
cmr_int camera_local_get_sensor_format(cmr_handle cmr_handle,
                                       cmr_u32 *sensor_format);
cmr_int camera_set_thumb_yuv_proc(cmr_handle oem_handle,
                                  struct snp_thumb_yuv_param *param);
cmr_int camera_set_hal_middleware_info(cmr_handle oem_handle,
                                  struct hal_middleware_info *param);
cmr_int camera_set_snapshot_status(cmr_handle oem_handle, cmr_u8 *move_info);
cmr_int camera_yuv_do_face_beauty_simplify(cmr_handle oem_handle,
                                           struct img_frm *src);
cmr_int camera_jpeg_encode_exif_simplify(cmr_handle oem_handle,
                                         struct enc_exif_param *param);
cmr_int camera_jpeg_decode_simplify(cmr_handle oem_handle,
                                         struct enc_exif_param *param);

cmr_int camera_local_set_gpu_mem_ops(cmr_handle oem_handle, void *cb_of_malloc,
                                     void *cb_of_free);
cmr_int camera_get_grab_capability(cmr_handle oem_handle,
                                   struct cmr_path_capability *capability);
cmr_int camera_get_af_support(cmr_handle oem_handle, cmr_u16 *af_support);

cmr_int camera_local_image_sw_algorithm_processing(
    cmr_handle oem_handle, struct image_sw_algorithm_buf *src_sw_algorithm_buf,
    struct image_sw_algorithm_buf *dst_sw_algorithm_buf,
    sprd_cam_image_sw_algorithm_type_t sw_algorithm_type,
    cam_img_format_t format);
cmr_int camera_local_start_scale(cmr_handle oem_handle,
                                 struct img_frm **scale_param);
cmr_int camera_local_start_rotate(cmr_handle oem_handle,
                                  struct rotate_param *rotate_param);
int dump_image_with_3a_info(cmr_handle oem_handle, uint32_t img_fmt,
                            uint32_t width, uint32_t height, uint32_t dump_size,
                            struct img_addr *addr);
#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
cmr_int camera_local_set_mm_dvfs_policy(cmr_handle oem_handle,
                                        enum DVFS_MM_MODULE module,
                                        enum CamProcessingState camera_state);
#endif
cmr_int camera_3dnr_set_ev(cmr_handle oem_handle, cmr_u32 enable);
cmr_int camera_get_sensor_info(cmr_handle oem_handle, cmr_uint sensor_id,
                               struct sensor_exp_info *exp_info_ptr);
cmr_int camera_sensor_ioctl(cmr_handle oem_handle, cmr_uint cmd_type,
                            struct common_sn_cmd_param *param_ptr);
cmr_int camera_channel_reproc(cmr_handle oem_handle, struct buffer_cfg *buf_cfg);
cmr_int camera_snapshot_set_ev(cmr_handle oem_handle, cmr_u32 value ,enum camera_snapshot_tpye type);
cmr_int camera_snapshot_set_ev_group(cmr_handle oem_handle, float *ev_arr, int size, cmr_u32 value);
void camera_adjust_ev_group_before_snapshot(cmr_handle oem_handle, float *ev_arr, int size);
cmr_int camera_get_tuning_info(cmr_handle oem_handle,
                               struct isp_adgain_exp_info *adgain_exp_info_ptr);
cmr_int camera_get_adgain_exp_info(cmr_handle oem_handle,
                                struct isp_adgain_exp_info *isp_adgain);
cmr_int camera_isp_ioctl(cmr_handle oem_handle, cmr_uint cmd_type,
                                struct common_isp_cmd_param *param_ptr);
cmr_int cmr_get_reboke_data(cmr_handle oem_handle,
                            struct af_relbokeh_oem_data *golden_distance);
cmr_int cmr_get_sensor_soft_landing(cmr_handle oem_handle, cmr_u32 camera_id,
                          struct sensor_af_softlanding_param *soft_landing);
cmr_int cmr_sensor_af_softlanding(cmr_handle sensor_handle, void *sl_param);
cmr_int camera_local_get_tuning_param(cmr_handle oem_handle,
                                      struct tuning_param_info *tuning_info);
cmr_int cmr_get_bokeh_sn_trim(cmr_handle handle,
                              struct sprd_img_path_rect *trim_param);

cmr_int camera_get_remosaic_type(struct sensor_4in1_info *p,
                              cmr_u32 sensor_w, cmr_u32 sensor_h);
cmr_int camera_get_is_4in1_sensor(struct sensor_4in1_info *p);
cmr_int camera_get_ois_info(cmr_handle handle, struct cal_ois_info *param);
cmr_int camera_get_ois_data(void *sensor_handle, void* data);
cmr_int camera_set_ois_func(cmr_handle handle, cmr_u32 *on_off);
cmr_int cmr_sns_ois_set_func(void *sns_handle, int cameraId, cmr_u32 on_off);
cmr_int cmr_sns_ois_calibration(void *sns_handle, int cameraId);
cmr_int cmr_sns_ois_get_data(void *sensor_handle, int cameraId, struct sensor_ois_packet_data_info* data);
cmr_int camera_get_4in1_info(cmr_handle handle, struct fin1_info *param);
cmr_int camera_set_high_res_mode(cmr_handle oem_handle,cmr_uint is_high_res_mode);
cmr_int camera_get_bv_info(cmr_handle oem_handle, cmr_u32 *bv_info);
cmr_int camera_get_ct_info(cmr_handle oem_handle, cmr_u32 *ct_info);
cmr_int camera_get_ae_lum_value(cmr_handle oem_handle, cmr_u32 *luma_info);
cmr_u32 camera_get_watermark_flag(cmr_handle oem_handle);
cmr_u32 camera_get_ai_sfnr_flag(struct camera_context *cxt);
void camera_grab_handle(cmr_int evt, void *data, void *privdata);
cmr_int camera_get_iso_info(cmr_handle oem_handle, cmr_u32 *iso_info);
cmr_int camera_get_adgain_exp_info(cmr_handle oem_handle, struct isp_adgain_exp_info *isp_adgain);
cmr_int camera_get_exp_info(cmr_handle oem_handle, cmr_u32 *exp_info);
int camera_local_get_scaler(uint32_t *scaler);
cmr_int camera_set_ae_params(cmr_handle oem_handle, void *param);
cmr_int camera_set_af_params(cmr_handle oem_handle, void *param);
void camera_local_set_exif_iso_value(cmr_handle oem_handle, cmr_u32 iso_value);
void camera_local_set_exif_exp_time(cmr_handle oem_handle, cmr_s64 exp_time);
cmr_s64 camera_local_get_shutter_skew(cmr_handle oem_handle);
cmr_int camera_local_set_alloc_size(cmr_handle oem_handle, cmr_u16 width, cmr_u16 height);
cmr_int camera_set_flash_calibration(cmr_handle oem_handle, cmr_int * param);
cmr_int camera_get_frames_cur_snapshot(cmr_handle oem_handle);
cmr_int cmr_preview_waitencode(cmr_handle oem_handle);
cmr_int camera_get_flash_cali_data(struct isp_init_param *isp_param);
cmr_int camera_get_flash_capture_flag(cmr_handle oem_handle, uint32_t * param);
cmr_int cmr_grab_960fps_cfg(cmr_handle grab_handle);
void camera_local_update_scenario(cmr_handle oem_handle);
void camera_set_scenario_info_to_isp(cmr_handle oem_handle);
void camera_construct_scenario(cmr_handle oem_handle);
cmr_int camera_local_capture_post_proc(cmr_handle oem_handle, cmr_u32 camera_id);
cmr_int camera_dump_sensor_trace(cmr_handle oem_handle, cmr_u32 camera_id);
cmr_int camera_dump_csi_trace(cmr_handle oem_handle, cmr_u32 camera_id);
void camera_local_set_scene_mode(cmr_handle oem_handle, uint8_t mode);
void camera_local_set_hdr_night_capture(cmr_handle oem_handle, uint8_t num, hdr_ev_info_t* hdr_info);
void camera_local_stop_hdr_capture(cmr_handle oem_handle);
void camera_local_stop_night_capture(cmr_handle oem_handle);
void camera_local_is_raw_stream_capture(cmr_handle oem_handle, bool is_raw_stream_capture,
                                                        bool is_sensor_raw_required);
cmr_int camera_iss_create(cmr_handle oem_handle);
cmr_int camera_iss_deinit(cmr_handle oem_handle);
bool camera_iss_get_yuv_frame(cmr_handle oem_handle, void **ptr, cmr_s32 *num);
#ifdef __cplusplus
}
#endif

#endif
