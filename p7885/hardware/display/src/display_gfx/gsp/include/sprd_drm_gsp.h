/*
 * Copyright (C) 2023 HiHope Open Source Organization.
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

#ifndef UAPI_SPRD_DRM_GSP_H_
#define UAPI_SPRD_DRM_GSP_H_

#include <drm/drm.h>
#include "gsp_cfg.h"

#define DRM_SPRD_GSP_GET_CAPABILITY 0
#define DRM_SPRD_GSP_TRIGGER 1

struct DrmGspCfgUser {
    __u8 gspId;
    bool async;
    __u32 size;
    __u32 num;
    bool split;
    void *config;
};

struct DrmGspCapability {
    __u8 gspId;
    __u32 size;
    void *cap;
};

#define DRM_IOCTL_SPRD_GSP_GET_CAPABILITY                                      \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SPRD_GSP_GET_CAPABILITY,                     \
             struct DrmGspCapability)

#define DRM_IOCTL_SPRD_GSP_TRIGGER                                             \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SPRD_GSP_TRIGGER, struct DrmGspCfgUser)

#endif /* UAPI_SPRD_DRM_GSP_H_ */
