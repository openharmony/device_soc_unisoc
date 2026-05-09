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
#include "drm_device.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
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
    mDrmFormat = DrmDevice::ConvertToDrmFormat(static_cast<PixelFormat>(hdl.GetFormat()));
    ret = drmPrimeFDToHandle(drmFd, hdl.GetFb(), &mGemHandle);
    DISPLAY_CHK_RETURN_NOT_VALUE((ret != 0), DISPLAY_LOGE("can not get handle errno %{public}d", errno));

    int widthStride = hdl.GetStride() / bytesPerPixelRgB888; // 其它格式默认处理
    if ((hdl.GetFormat() >= PIXEL_FMT_YUV_422_I) && (hdl.GetFormat() <= PIXEL_FMT_VYUY_422_PKG)) {
        widthStride = hdl.GetStride();
    }
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
    constexpr int plane2 = 2;
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
            cfg.gemHandles[1] = mGemHandle;
            cfg.offsets[1] = bpf;
            cfg.pitches[0] = widthStride * 1;
            cfg.pitches[1] = cfg.pitches[0];
            break;
        case PIXEL_FMT_YCBCR_420_P:
            cfg.gemHandles[1] = mGemHandle;
            cfg.gemHandles[plane2] = mGemHandle;
            cfg.offsets[1] = bpf;
            cfg.offsets[plane2] = bpf + (((widthStride / subsampleFactor + alignment) - 1) /
                ((widthStride / subsampleFactor) * alignment)) * hdl.GetHeight() / subsampleFactor;
            cfg.pitches[0] = widthStride * 1;
            cfg.pitches[1] = (((widthStride / subsampleFactor + alignment) - 1) /
                ((widthStride / subsampleFactor) * alignment));
            cfg.pitches[plane2] = cfg.pitches[1];
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

    // 保存旧的 Buffer，等待 DRM 使用完毕后再释放
    // 这样可以避免 DRM 访问已释放的 Framebuffer
    mLastBuffer = std::move(mCurrentBuffer);

    // 创建新的 Buffer
    std::unique_ptr<DrmGemBuffer> ptr = std::make_unique<DrmGemBuffer>(DrmDevice::GetDrmFd(), *GetCurrentBuffer());
    mCurrentBuffer = std::move(ptr);

    return mCurrentBuffer.get();
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
