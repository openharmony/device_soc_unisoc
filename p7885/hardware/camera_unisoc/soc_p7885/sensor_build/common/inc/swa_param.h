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

#ifndef _SWA_PARAM_API_H_
#define _SWA_PARAM_API_H_

#include <stdint.h>
#include "cmr_types.h"
#ifdef __cplusplus
extern "C" {
#endif



/*========= for HDR ========*/
#define HDR3_CAP_MAX 7
#define HDR_CAP_MAX 3

struct swa_hdr_param {
	uint32_t pic_w;
	uint32_t pic_h;
	uint32_t fmt;
	uint32_t hdr_version;
	float ev[HDR3_CAP_MAX];
	cmr_s32 tuning_param_size;
	void *tuning_param_ptr;
	void *hdr_callback;
	void *ae_exp_gain_info;
	void* (*heap_mem_malloc)(size_t size, char* type);
	void (*heap_mem_free)(void* addr);
	void *out_exif_ptr;
	uint32_t out_exif_size;
};

/*========= for HDR end  ========*/



/*========= for MFSR   =========*/
struct swa_mfsr_info {
	void *data;
	uint32_t data_size;
	struct img_size frame_size;
	struct img_rect frame_crop;
	void *out_exif_ptr;
	uint32_t out_exif_size;
};
/*========= for MFSR end  =========*/



/*========= for ultra-wide WARP  ========*/
struct isp_warp_info {
	void *otp_data;
	uint32_t otp_size;

	uint32_t cap_tag; /* 1 : capture; 0: non-capture */
	uint32_t binning_factor;

	struct img_size src_size;
	struct img_rect src_crop;
	struct img_rect dst_crop;
	struct img_size in_size;
	struct img_size out_size;
};

/*========= for ultra-wide WARP end ========*/

/*========= for ultra-wide WARPPRO  ========*/
struct isp_warppro_info {
	void *data;
	uint32_t data_size;
	cmr_uint is_ultra_wide_pro;//get apk flag
};
/*========= for ultra-wide WARPPRO end ========*/

/*========= for MFNR ========*/

/* sw 3DNR param */
/* used to pass sw3dnr param from tuning array to HAL->3dnr adapt
  * must keep consistent with struct ( sensor_sw3dnr_level) in sensor_raw_xxx.h
  * should not be modified except sensor_raw_xxx.h changes corresponding structure */
struct isp_mfnr_info {
	cmr_s32 threshold[4];
	cmr_s32 slope[4];
	cmr_u16 searchWindow_x;
	cmr_u16 searchWindow_y;
	cmr_s32 recur_str;
	cmr_s32 match_ratio_sad;
	cmr_s32 match_ratio_pro;
	cmr_s32 feat_thr;
	cmr_s32 zone_size;
	cmr_s32 luma_ratio_high;
	cmr_s32 luma_ratio_low;
	cmr_s32 reserverd[16];
};

//get parameter for mfnr4 without parse
struct isp_mfnrv4_img_pm {
	void *ptr;
	cmr_u32 size;
};

struct isp_sw3dnr_info {
	cmr_s32 threshold[4];
	cmr_s32 slope[4];
	cmr_u16 searchWindow_x;
	cmr_u16 searchWindow_y;
	cmr_s32 recur_str;
	cmr_s32 match_ratio_sad;
	cmr_s32 match_ratio_pro;
	cmr_s32 feat_thr;
	cmr_s32 zone_size;
	cmr_s32 luma_ratio_high;
	cmr_s32 luma_ratio_low;
	cmr_s32 reserverd[16];
};
/*========= for MFNR  end ========*/


/*========= for YNRs ========*/
struct isp_sw_ynrs_params {
	uint8_t lumi_thresh[2];
	cmr_u8 gf_rnr_ratio[5];
	cmr_u8 gf_addback_enable[5];
	cmr_u8 gf_addback_ratio[5];
	cmr_u8 gf_addback_clip[5];
	cmr_u16 Radius_factor;
	cmr_u16 imgCenterX;
	cmr_u16 imgCenterY;
	cmr_u16 gf_epsilon[5][3];
	cmr_u16 gf_enable[5];
	cmr_u16 gf_radius[5];
	cmr_u16 gf_rnr_offset[5];
	cmr_u16 bypass;
	cmr_u8 reserved[2];
};

struct isp_ynrs_info {
	cmr_u16 bypass;
	cmr_u16 radius_base;
	struct isp_sw_ynrs_params ynrs_param;
};
/*========= for YNRs end ========*/


/*========= for CNR2.0 =========*/
struct isp_sw_filter_weights
{
	cmr_u8 distWeight[9];
	cmr_u8 rangWeight[128];
};

struct isp_sw_cnr2_info {
	cmr_u8 filter_en[4];
	cmr_u8 rangTh[4][2];
	struct isp_sw_filter_weights weight[4][2];
};

struct isp_cnr2_info {
	cmr_u32 bypass;
	struct isp_sw_cnr2_info param;
};
/*========= for CNR2.0 end =========*/


/*========= for CNR3.0 =========*/
#define CNR3_LAYER_NUM 5

struct isp_sw_multilayer_param {
	cmr_u8 lowpass_filter_en;
	cmr_u8 denoise_radial_en;
	cmr_u8 order[3];
	cmr_u16 imgCenterX;
	cmr_u16 imgCenterY;
	cmr_u16 slope;
	cmr_u16 baseRadius;
	cmr_u16 minRatio;
	cmr_u16 luma_th[2];
	float sigma[3];
};

struct isp_sw_cnr3_info {
	cmr_u16 bypass;
	cmr_u16 baseRadius;
	struct isp_sw_multilayer_param param_layer[CNR3_LAYER_NUM];
};
/*========= for CNR3.0 end =========*/


/*========= for DRE   =========*/
struct isp_predre_param {
	cmr_s32 enable;
	cmr_s32 imgKey_setting_mode;
	cmr_s32 tarNorm_setting_mode;
	cmr_s32 target_norm;
	cmr_s32 imagekey;
	cmr_s32 min_per;
	cmr_s32 max_per;
	cmr_s32 stat_step;
	cmr_s32 low_thresh;
	cmr_s32 high_thresh;
	cmr_s32 tarCoeff;
};

struct isp_postdre_param {
	cmr_s32 enable;
	cmr_s32 strength;
	cmr_s32 texture_counter_en;
	cmr_s32 text_point_thres;
	cmr_s32 text_prop_thres;
	cmr_s32 tile_num_auto;
	cmr_s32 tile_num_x;
	cmr_s32 tile_num_y;
};

struct isp_dre_level {
	struct isp_predre_param predre_param;
	struct isp_postdre_param postdre_param;
};
/*========= for DRE end =========*/

/*========= for DRE_pro  =========*/
struct isp_predre_pro_param {
	cmr_s32 enable;
	cmr_s32 imgKey_setting_mode;
	cmr_s32 tarNorm_setting_mode;
	cmr_s32 target_norm;
	cmr_s32 imagekey;
	cmr_s32 min_per;
	cmr_s32 max_per;
	cmr_s32 stat_step ;
	cmr_s32 low_thresh;
	cmr_s32 high_thresh;
	cmr_s32 uv_gain_ratio;
	cmr_s32 tarCoeff;
};

struct isp_postdre_pro_param {
	cmr_s32 enable;
	cmr_s32 strength;
	cmr_s32 texture_counter_en;
	cmr_s32 text_point_thres;
	cmr_s32 text_prop_thres;
	cmr_s32 tile_num_auto;
	cmr_s32 tile_num_x;
	cmr_s32 tile_num_y;
	cmr_s32 text_point_alpha;
};

struct isp_dre_pro_level {
	struct isp_predre_pro_param predre_pro_param;
	struct isp_postdre_pro_param postdre_pro_param;
};
/*========= for DRE_pro end =========*/



/*========= for filter ========*/
struct swa_filter_param {
	uint32_t version;
	uint32_t filter_type;
	uint32_t orientation;
	uint32_t flip_on;
	uint32_t is_front;
	uint32_t pic_w;
	uint32_t pic_h;
};
/*========= for filter end ========*/



/*========= for face beauty ========*/
enum {
	ISP_FB_SKINTONE_DEFAULT,
	ISP_FB_SKINTONE_YELLOW,
	ISP_FB_SKINTONE_WHITE,
	ISP_FB_SKINTONE_BLACK,
	ISP_FB_SKINTONE_INDIAN,
	ISP_FB_SKINTONE_NUM
};

struct isp_fb_level {
	cmr_u8 skinSmoothLevel[11];
	cmr_u8 skinSmoothDefaultLevel;
	cmr_u8 skinTextureHiFreqLevel[11];
	cmr_u8 skinTextureHiFreqDefaultLevel;
	cmr_u8 skinTextureLoFreqLevel[11];
	cmr_u8 skinTextureLoFreqDefaultLevel;
	cmr_u8 skinSmoothRadiusCoeff[11];
	cmr_u8 skinSmoothRadiusCoeffDefaultLevel;
	cmr_u8 skinBrightLevel[11];
	cmr_u8 skinBrightDefaultLevel;
	cmr_u8 largeEyeLevel[11];
	cmr_u8 largeEyeDefaultLevel;
	cmr_u8 slimFaceLevel[11];
	cmr_u8 slimFaceDefaultLevel;
	cmr_u8 skinColorLevel[11];
	cmr_u8 skinColorDefaultLevel;
	cmr_u8 lipColorLevel[11];
	cmr_u8 lipColorDefaultLevel;
};

struct isp_fb_param {
	cmr_u8 removeBlemishFlag;
	cmr_u8 blemishSizeThrCoeff;
	cmr_u8 skinColorType;
	cmr_u8 lipColorType;
	struct isp_fb_level fb_layer;
};

struct isp_fb_param_info {
	struct isp_fb_param fb_param[ISP_FB_SKINTONE_NUM];
};

struct isp_beauty_levels {
	uint8_t blemishLevel;
	uint8_t smoothLevel;
	uint8_t skinColor;
	uint8_t skinLevel;
	uint8_t brightLevel;
	uint8_t lipColor;
	uint8_t lipLevel;
	uint8_t slimLevel;
	uint8_t largeLevel;
	uint8_t reserved[3];
};

struct swa_img_addr {
    cmr_uint addr_y;
    cmr_uint addr_u;
    cmr_uint addr_v;
};

struct swa_img_data_end {
    cmr_u8 y_endian;
    cmr_u8 uv_endian;
    cmr_u8 reserved0;
    cmr_u8 reserved1;
    // cmr_u32                                 padding;
};


struct swa_img_frm {
    cmr_u32 base_id;
    cmr_uint sec;
    cmr_uint usec;
    cmr_u32 fmt;
    cmr_u32 buf_size;
    struct img_rect rect;
    struct img_size size;
    struct swa_img_addr addr_phy;
    struct swa_img_addr addr_vir;
    cmr_s32 fd;
    void *gpu_handle;
    struct swa_img_data_end data_end;
    cmr_u32 format_pattern;
    void *reserved;
    cmr_s64 monoboottime;
    cmr_u32 frame_number;
};

struct swa_face_info_t {
    int startX;
    int starty;
    int width;
    int height;      /* Face rectangle*/
    int yawAngle;    /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees;  */
    int rollAngle;   /* In-plane rotation angle (Roll); In (-180, +180] degrees;   */
    int score;       /* Confidence score; In [0, 1000]*/
    int humanId;     /* Human ID Number*/
    int reserv[8];   //For extend
};


struct isp_fd_info {
    struct swa_img_frm fd_small;
    cmr_handle fd_handle;
    void * hDT;
    cmr_int work_mode;
    cmr_int face_count;
    void *alloc_addr;
    void *alloc_addr_u;
    struct swa_face_info_t  face_info[10];
};


struct isp_fb_info {
	uint16_t bypass;
	uint16_t param_valid;
	struct isp_fb_param_info param;
	struct isp_beauty_levels levels;
};
/*========= for face beauty end ========*/


/*========= for face detect ========*/
#define FA_SHAPE_POINTNUM 7
#define FASHAPE_MAXFACENUM 20
#define FACEANALYZE_ATTR_MOUTH_LABELNUM 3
#define FACEANALYZE_ATTR_EYE_GLASSES_LABELNUM 3
#define FACEANALYZE_ATTR_FACE_OCC_LABELNUM 2

/*detected face attributes*/
typedef struct
{
	int mouthStatus;                                                /*0: no occlusion, 1: mouth with mask; 2: other occlusion*/
	int mouthScores[FACEANALYZE_ATTR_MOUTH_LABELNUM];               /*confidence score of each label,[0,100]*/
	int eyeGlassesStatus;                                           /*0: no glasses, 1: normal glasses; 2: sunglasses*/
	int eyeGlassesScores[FACEANALYZE_ATTR_EYE_GLASSES_LABELNUM];    /*confidence score of each label,[0,100]*/
	int faceOccStatus;                                              /*0: no occlusion, 1: occlusion*/
	int faceOccScores[FACEANALYZE_ATTR_FACE_OCC_LABELNUM];          /*confidence score of each label,[0,100]*/
}FA_ATTRIBUTE_OUT;

struct isp_face_info {
	cmr_s32 sx;
	cmr_s32 sy;
	cmr_s32 ex;
	cmr_s32 ey;
	cmr_u32 brightness;      /* 3A do not need */
	cmr_s32 pose;            /* yaw_angle */
	cmr_s32 angle;           /* roll_angle */
	cmr_s32 yaw_angle;       /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees;   */
	cmr_s32 roll_angle;      /* In-plane rotation angle (Roll); In (-180, +180] degrees;   */
	cmr_u32 score;
	cmr_u32 id;
	cmr_s32 rotateBoxValid;
	cmr_s32 rotateBox[8];
	cmr_s32 landmarkValid;
	cmr_s32 landmark[14];
	cmr_u32 fid;
	FA_ATTRIBUTE_OUT faceAttr;
};

typedef struct
{
	int data[FA_SHAPE_POINTNUM*2];   /* Coordinates of landmarks [x0,y0,x1,y1, ..., xn,yn]   */
	int score;                       /* Confidence score of face shape                       */
	int yawAngle;
	FA_ATTRIBUTE_OUT attribute;
	int faceID;
} FASHAPE_OUT;

struct fashape_vec {
	FASHAPE_OUT faceShape[FASHAPE_MAXFACENUM];
	int faceNum;
};

struct isp_face_area {
	cmr_u16 type;
	cmr_u16 face_num;
	cmr_u16 frame_width;
	cmr_u16 frame_height;
	struct isp_face_info face_info[10];
	cmr_u32 frame_id;
	cmr_s64 timestamp;
	struct fashape_vec fafacealign_out; /* lwp not use, need delete */
	void *faceanalyze_out;              /* lwp not use, need delete */
	void *faceattribute_out;            /* lwp not use, need delete */
};
/*========= for face detect end ========*/

/*========= for videoStable start ========*/
#define MAX_SENSOR_DATA_NUM 128
#define MAX_EIS_PIPE_TYPE 2

typedef enum {
    SENSOR_TYPE_GYRO,
    SENSOR_TYPE_ACCEL,
    SENSOR_TYPE_HALL,
} VideoStable_Sensor_Type;

// for eis adapter catch sensor data
typedef enum {
    PREVIEW_PIPE_TYPE,
    VIDEO_PIPE_TYPE,
} VideoStable_PIPE_Type;

typedef struct {
    double x;
    double y;
    double z;
    cmr_s64 timestamp;
} SensorData_t;

typedef struct
{
    double x_data[MAX_SENSOR_DATA_NUM];
    double y_data[MAX_SENSOR_DATA_NUM];
    double z_data[MAX_SENSOR_DATA_NUM];
    cmr_s64 timestamp[MAX_SENSOR_DATA_NUM];
    int num;
} VideoStable_SensorData;

typedef int (*acquire_sensor_data) (int sensor_type, int frame_type, VideoStable_SensorData* sensor_data, void* user_data);

/*========= for videoStable end ========*/

struct swa_filter_info {
	uint32_t version;
	uint32_t filter_type;
};

struct swa_watermark_info {
	int flag;
	time_t time_cap;
};

struct swa_init_data {
	uint32_t cam_id;
	struct img_size sensor_max;
	struct img_size sensor_size;
	struct img_size frame_size;
	struct img_rect frame_crop;
	uint32_t pri_data_size;
	uint32_t sn_fmt;
	uint32_t pic_fmt;
	uint32_t frm_total_num;
	uint32_t ae_again;
	void *pri_data;
	void* (*heap_mem_malloc)(size_t size, char* type);
	void (*heap_mem_free)(void* addr);
	struct img_rect af_ctrl_roi;
};

struct swa_common_info {
	int32_t cam_id;
	int32_t dgain;
	int32_t again;
	int32_t total_gain;
	int32_t sn_gain;
	int32_t exp_time;
	int32_t bv;
	int32_t iso;
	int32_t ct;
	uint32_t angle;
	uint32_t sensor_orientation;
	uint32_t flip_on;
	uint32_t is_front;
	float zoom_ratio;
};

struct swa_frame_param {
	struct swa_common_info common_param;
	struct swa_hdr_param hdr_param;
	struct swa_mfsr_info mfsr_param;
	struct isp_warp_info warp_info;
	struct isp_warppro_info warppro_info;
	struct swa_filter_info filter_param;
	struct swa_watermark_info wm_param;
	struct isp_ynrs_info ynrs_info;
	struct isp_cnr2_info cnr2_info;
	struct isp_sw_cnr3_info cnr3_info;
	struct isp_mfnr_info mfnr_param;
	struct isp_mfnrv4_img_pm mfnr4_img_param;
	struct isp_dre_level dre_param;
	struct isp_dre_pro_level dre_pro_param;
	struct isp_face_area face_param;
	struct isp_fb_info fb_info;
	struct img_rect af_ctrl_roi;
	struct isp_fd_info fd_info;
};


#ifdef __cplusplus
}
#endif
#endif
