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

#ifndef _CMR_ISPTOOL_H_
#define _CMR_ISPTOOL_H_
#include "cmr_common.h"
#include "cmr_snapshot.h"

#ifdef __cplusplus
extern "C" {
#endif

cmr_int cmr_isp_simulation_proc(cmr_handle oem_handle,
                                struct snapshot_param *param_ptr);
#ifdef __cplusplus
}
#endif

#endif
