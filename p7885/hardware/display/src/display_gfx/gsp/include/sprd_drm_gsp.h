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

#include "gsp_cfg.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {

#define UISOC_GSP_CMD_CAP       0
#define UISOC_GSP_CMD_TRIG      1

struct UisocDrmGspCfg {
    __u8 id;
    bool async;
    __u32 size;
    __u32 count;
    bool split;
    void *data;
};

struct UisocDrmGspCapa {
    __u8 id;
    __u32 size;
    void *ptr;
};

#define UISOC_IOCTL_GSP_GET_CAP                                                \
    DRM_IOWR(DRM_COMMAND_BASE + UISOC_GSP_CMD_CAP, struct UisocDrmGspCapa)

#define UISOC_IOCTL_GSP_TRIGGER                                                \
    DRM_IOWR(DRM_COMMAND_BASE + UISOC_GSP_CMD_TRIG, struct UisocDrmGspCfg)

} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS

#endif /* UAPI_SPRD_DRM_GSP_H_ */
