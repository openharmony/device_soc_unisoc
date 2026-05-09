/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "drm_plane.h"
#include "drm_device.h"

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

namespace OHOS {
namespace HDI {
namespace DISPLAY {

static uint32_t GetPropertyId(int fd, drmModeObjectProperties *props,
                              const char *name)
{
    drmModePropertyPtr property;
    uint32_t i;
    uint32_t id = 0;

    for (i = 0; i < props->count_props; i++) {
        property = drmModeGetProperty(fd, props->props[i]);
        if (!strcmp(property->name, name)) {
            id = property->prop_id;
        }
        drmModeFreeProperty(property);

        if (id != 0) {
            break;
        }
    }

    return id;
}

DrmPlane::DrmPlane(const drmModePlane &p)
    : mId(p.plane_id), mPossibleCrtcs(p.possible_crtcs), mFormats(p.formats, p.formats + p.count_formats)
{}

DrmPlane::~DrmPlane()
{
    DISPLAY_LOGD();
}

int32_t DrmPlane::Init(DrmDevice &drmDevice)
{
    DISPLAY_LOGD();
    int32_t ret;
    DrmProperty prop;
    ret = drmDevice.GetPlaneProperty(*this, PROP_FBID, &prop);
    mPropFbId = prop.propId;
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("can not get plane fb id"));
    ret = drmDevice.GetPlaneProperty(*this, PROP_IN_FENCE_FD, &prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get plane in fence prop id"));
    mPropFenceInId = prop.propId;
    ret = drmDevice.GetPlaneProperty(*this, PROP_CRTC_ID, &prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane crtc prop id"));
    mPropCrtcId = prop.propId;
    ret = drmDevice.GetPlaneProperty(*this, PROP_TYPE, &prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane crtc prop id"));
    switch (prop.value) {
        case DRM_PLANE_TYPE_OVERLAY:
        case DRM_PLANE_TYPE_PRIMARY:
        case DRM_PLANE_TYPE_CURSOR:
            mType = static_cast<uint32_t>(prop.value);
            break;
        default:
            DISPLAY_LOGE("unknown type value %{public}" PRIu64 "", prop.value);
            return DISPLAY_FAILURE;
    }

    drmModeObjectProperties *props = drmModeObjectGetProperties(drmDevice.GetDrmFd(), mId, DRM_MODE_OBJECT_PLANE);
    propertyCrtcX = GetPropertyId(drmDevice.GetDrmFd(), props, "CRTC_X");
    propertyCrtcY = GetPropertyId(drmDevice.GetDrmFd(), props, "CRTC_Y");
    propertyCrtcW = GetPropertyId(drmDevice.GetDrmFd(), props, "CRTC_W");
    propertyCrtcH = GetPropertyId(drmDevice.GetDrmFd(), props, "CRTC_H");

    propertySrcX = GetPropertyId(drmDevice.GetDrmFd(), props, "SRC_X");
    propertySrcY = GetPropertyId(drmDevice.GetDrmFd(), props, "SRC_Y");
    propertySrcW = GetPropertyId(drmDevice.GetDrmFd(), props, "SRC_W");
    propertySrcH = GetPropertyId(drmDevice.GetDrmFd(), props, "SRC_H");
    propertyBlendMode = GetPropertyId(drmDevice.GetDrmFd(), props, "pixel blend mode");
    propertyRotation = GetPropertyId(drmDevice.GetDrmFd(), props, "rotation");
    propertyAlpha = GetPropertyId(drmDevice.GetDrmFd(), props, "alpha");
    propertyY2rCoef = GetPropertyId(drmDevice.GetDrmFd(), props, "YUV2RGB coef");
    drmModeFreeObjectProperties(props);

    return DISPLAY_SUCCESS;
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
