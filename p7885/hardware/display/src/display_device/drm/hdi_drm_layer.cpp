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

#include "hdi_drm_layer.h"
#include <cinttypes>
#include <cerrno>
#include <limits>
#include "drm_device.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
namespace {
enum Yuv420PlaneIndex {
    yuv420YPlane,
    yuv420UPlane,
    yuv420VPlane,
};

bool IsValidLayerBuffer(const HdiLayerBuffer *hdl)
{
    if (hdl == nullptr) {
        DISPLAY_LOGE("layer buffer is nullptr");
        return false;
    }

    if (hdl->GetFb() < 0) {
        DISPLAY_LOGE("layer buffer fd is invalid %{public}d", hdl->GetFb());
        return false;
    }

    if ((hdl->GetWidth() <= 0) || (hdl->GetHeight() <= 0) || (hdl->GetStride() <= 0)) {
        DISPLAY_LOGE("invalid layer buffer size w:%{public}d h:%{public}d stride:%{public}d",
            hdl->GetWidth(), hdl->GetHeight(), hdl->GetStride());
        return false;
    }

    if (DrmDevice::ConvertToDrmFormat(static_cast<PixelFormat>(hdl->GetFormat())) == DRM_FORMAT_INVALID) {
        DISPLAY_LOGE("unsupported layer buffer format:%{public}d", hdl->GetFormat());
        return false;
    }

    return true;
}
}

DrmGemBuffer::DrmGemBuffer(int drmfd, HdiLayerBuffer &hdl) : mDrmFd(drmfd)
{
    DISPLAY_LOGD();
    Init(mDrmFd, hdl);
}

void DrmGemBuffer::Init(int drmFd, HdiLayerBuffer &hdl)
{
    int ret;
    constexpr uint32_t bytesPerPixelRgB888 = 4;
    constexpr int maxPlaneCount = 4;
    uint32_t pitches[maxPlaneCount] = {0};
    uint32_t gemHandles[maxPlaneCount] = {0};
    uint32_t offsets[maxPlaneCount] = {0};
    DISPLAY_LOGD("hdl %{public}" PRIx64 "", hdl.GetMemHandle());
    DISPLAY_CHK_RETURN_NOT_VALUE((drmFd < 0), DISPLAY_LOGE("can not init drmfd %{public}d", drmFd));
    DISPLAY_CHK_RETURN_NOT_VALUE((!IsValidLayerBuffer(&hdl)), DISPLAY_LOGE("can not init invalid layer buffer"));
    mDrmFormat = DrmDevice::ConvertToDrmFormat(static_cast<PixelFormat>(hdl.GetFormat()));
    ret = drmPrimeFDToHandle(drmFd, hdl.GetFb(), &mGemHandle);
    DISPLAY_CHK_RETURN_NOT_VALUE((ret != 0), DISPLAY_LOGE("can not get handle errno %{public}d", errno));

    int widthStride = hdl.GetStride() / bytesPerPixelRgB888; // 其它格式默认处理
    if ((hdl.GetFormat() >= PIXEL_FMT_YUV_422_I) && (hdl.GetFormat() <= PIXEL_FMT_VYUY_422_PKG)) {
        widthStride = hdl.GetStride();
    }
    DISPLAY_CHK_RETURN_NOT_VALUE((widthStride <= 0), DISPLAY_LOGE("invalid width stride %{public}d", widthStride));
    DISPLAY_CHK_RETURN_NOT_VALUE(((hdl.GetFormat() == PIXEL_FMT_YCBCR_420_P) && (widthStride < 2)),
        DISPLAY_LOGE("invalid yuv420 planar width stride %{public}d", widthStride));
    DISPLAY_CHK_RETURN_NOT_VALUE((widthStride > (std::numeric_limits<int>::max() / hdl.GetHeight())),
        DISPLAY_LOGE("buffer size overflow widthStride:%{public}d height:%{public}d", widthStride, hdl.GetHeight()));
    int bpf = widthStride * hdl.GetHeight();
    pitches[0] = hdl.GetStride();
    gemHandles[0] = mGemHandle;
    offsets[0] = 0;
    FbConfig cfg = { pitches, gemHandles, offsets };
    AllocateFbParams(hdl, widthStride, bpf, cfg);
    ret = drmModeAddFB2(drmFd, hdl.GetWidth(), hdl.GetHeight(), mDrmFormat, gemHandles, pitches, offsets, &mFdId, 0);
    DISPLAY_LOGD("mGemHandle %{public}d  mFdId %{public}d", mGemHandle, mFdId);
    DISPLAY_LOGD("w: %{public}d  h: %{public}d mDrmFormat : %{public}d gemHandles: %{public}d pitches: %{public}d "
        "offsets: %{public}d",
        hdl.GetWidth(), hdl.GetHeight(), mDrmFormat, gemHandles[0], pitches[0], offsets[0]);
    DISPLAY_CHK_RETURN_NOT_VALUE((ret != 0), DISPLAY_LOGE("can not add fb errno %{public}d", errno));
}

void DrmGemBuffer::AllocateFbParams(HdiLayerBuffer &hdl, int widthStride, int bpf, const FbConfig &cfg)
{
    constexpr int subsampleFactor = 2;
    constexpr int alignment = 16;
    constexpr int bpp3 = 3;
    constexpr int bpp2 = 2;

    switch (hdl.GetFormat()) {
        case PIXEL_FMT_RGBA_8888:
        case PIXEL_FMT_BGRA_8888:
        case PIXEL_FMT_BGRX_8888:
        case PIXEL_FMT_RGBX_8888:
            break;
        case PIXEL_FMT_RGB_888:
            cfg.pitches[0] = widthStride * bpp3;
            break;
        case PIXEL_FMT_RGB_565:
            cfg.pitches[0] = widthStride * bpp2;
            break;
        case PIXEL_FMT_YCBCR_420_SP:
        case PIXEL_FMT_YCRCB_420_SP:
        case PIXEL_FMT_YCBCR_422_SP:
        case PIXEL_FMT_YCRCB_422_SP:
            cfg.gemHandles[yuv420UPlane] = mGemHandle;
            cfg.offsets[yuv420UPlane] = bpf;
            cfg.pitches[yuv420YPlane] = widthStride * 1;
            cfg.pitches[yuv420UPlane] = cfg.pitches[yuv420YPlane];
            break;
        case PIXEL_FMT_YCBCR_420_P:
            cfg.gemHandles[yuv420UPlane] = mGemHandle;
            cfg.gemHandles[yuv420VPlane] = mGemHandle;
            cfg.offsets[yuv420UPlane] = bpf;
            cfg.offsets[yuv420VPlane] = bpf + (((widthStride / subsampleFactor + alignment) - 1) /
                ((widthStride / subsampleFactor) * alignment)) * hdl.GetHeight() / subsampleFactor;
            cfg.pitches[yuv420YPlane] = widthStride * 1;
            cfg.pitches[yuv420UPlane] = (((widthStride / subsampleFactor + alignment) - 1) /
                ((widthStride / subsampleFactor) * alignment));
            cfg.pitches[yuv420VPlane] = cfg.pitches[yuv420UPlane];
            break;
        case PIXEL_FMT_YUV_422_I:
            cfg.pitches[0] = widthStride * bpp2;
            break;
        default:
            DISPLAY_LOGE("dont support format:%x", hdl.GetFormat());
            break;
    }
}

DrmGemBuffer::~DrmGemBuffer()
{
    DISPLAY_LOGD();
    if (mFdId > 0) {
        if (drmModeRmFB(mDrmFd, mFdId) != 0) {
            DISPLAY_LOGE("can not free fdid %{public}d errno %{public}d", mFdId, errno);
        }
    }

    if (mGemHandle > 0) {
        struct drm_gem_close gemClose = { 0 };
        gemClose.handle = mGemHandle;
        if (drmIoctl(mDrmFd, DRM_IOCTL_GEM_CLOSE, &gemClose)) {
            DISPLAY_LOGE("can not free gem handle %{public}d errno : %{public}d", mGemHandle, errno);
        }
    }
}

bool DrmGemBuffer::IsValid()
{
    DISPLAY_LOGD();
    return (mGemHandle != INVALID_DRM_ID) && (mFdId != INVALID_DRM_ID);
}

DrmGemBuffer *HdiDrmLayer::GetGemBuffer()
{
    DISPLAY_LOGD();
    HdiLayerBuffer *layerBuffer = GetCurrentBuffer();
    if (!IsValidLayerBuffer(layerBuffer)) {
        DISPLAY_LOGE("skip invalid current layer buffer");
        return nullptr;
    }

    // 保存旧的 Buffer，等待 DRM 使用完毕后再释放
    // 这样可以避免 DRM 访问已释放的 Framebuffer
    mLastBuffer = std::move(mCurrentBuffer);

    // 创建新的 Buffer
    std::unique_ptr<DrmGemBuffer> ptr = std::make_unique<DrmGemBuffer>(DrmDevice::GetDrmFd(), *layerBuffer);
    if ((ptr == nullptr) || !ptr->IsValid()) {
        DISPLAY_LOGE("create DrmGemBuffer failed, drop this frame");
        return nullptr;
    }
    mCurrentBuffer = std::move(ptr);

    return mCurrentBuffer.get();
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
