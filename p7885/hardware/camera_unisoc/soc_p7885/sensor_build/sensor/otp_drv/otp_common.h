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

#ifndef _OTP_COMMON_H_
#define _OTP_COMMON_H_

#include "cmr_common.h"
#include "sensor_drv_u.h"
#include <cmr_property.h>
#include "otp_info.h"

cmr_int sensor_otp_rw_data_from_file(cmr_u8 cmd, char *sensor_name,
                                     void **otp_data, long *format_otp_size);
cmr_int sensor_otp_lsc_decompress(otp_base_info_cfg_t *otp_base_info,
                                  otp_section_info_t *lsc_cal_data);
cmr_int sensor_otp_decompress_gain(cmr_u16 *src, cmr_u32 src_bytes,
                                   cmr_u32 src_uncompensate_bytes, cmr_u16 *dst,
                                   cmr_u32 GAIN_COMPRESSED_BITS,
                                   cmr_u32 GAIN_MASK);
void sensor_otp_change_pattern(cmr_u32 pattern, cmr_u16 *interlaced_gain,
                               cmr_u16 *chn_gain[4], cmr_u16 gain_num);
cmr_int sensor_otp_dump_raw_data(cmr_u8 *buffer, int size, char *dev_name);

cmr_int sensor_otp_dump_to_icap(cmr_u8 *buffer, int size, char *dev_name);

cmr_int sensor_otp_dump_data2txt(cmr_u8 *buffer, int size, char *dev_name);

cmr_int sensor_otp_drv_create(otp_drv_init_para_t *input_para,
                              cmr_handle *sns_af_drv_handle);
cmr_int sensor_otp_drv_delete(void *otp_drv_handle);
cmr_u8 *sensor_otp_get_raw_buffer(cmr_uint size, cmr_u32 sensor_id);
cmr_u8 *sensor_otp_get_formatted_buffer(cmr_uint size, cmr_u32 sensor_id);
void sensor_otp_set_buffer_state(cmr_u32 sensor_id, cmr_u32 state);
cmr_u32 sensor_otp_get_buffer_state(cmr_u32 sensor_id);
cmr_u8 *sensor_otp_copy_raw_buffer(cmr_uint size, cmr_u32 sensor_id,
                                   cmr_u32 sensor_id2);

#endif
