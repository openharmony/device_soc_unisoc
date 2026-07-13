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

#ifndef _CMR_EXIF_H_
#define _CMR_EXIF_H_
#include "jpeg_exif_header.h"
#include "cmr_setting.h"

#ifdef __cplusplus
extern "C" {
#endif


cmr_int cmr_exif_init(JINF_EXIF_INFO_T *jinf_exif_info_ptr,
                      setting_get_pic_taking_cb setting_cb, void *priv_data);

cmr_int cmr_exifinfo_save(JINF_EXIF_INFO_T *jinf_exif_info_ptr,
                      saved_exif_info_t *dst);
cmr_int cmr_exif_update_ae_params(JINF_EXIF_INFO_T *jinf_exif_info_ptr,
                      struct ae_status_info *ae_info);

#ifdef __cplusplus
}
#endif

#endif
