/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
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

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include "display_common.h"
#include "securec.h"
#include "display_gfx.h"
#include <sys/mman.h>
#include <xf86drm.h>
#include "display_gfx.h"
#include "sprd_drm_gsp.h"
#include "gsp_cfg.h"
#include "gsp_r9p0_cfg.h"
#include "gsp_r9p0_common.h"

#include "v1_0/display_composer_type.h"
namespace OHOS {
namespace HDI {
namespace DISPLAY {
using namespace OHOS::HDI::Display::Composer::V1_0;
constexpr uint32_t BYTES_PER_PIXEL_RGB888 = 4;
constexpr int COMMIT_STATUS_IDLE = 0;
constexpr int COMMIT_STATUS_IN_PROGRESS = 1;
constexpr int SET_TO_LAYER_SUCCESS = 0;
constexpr int SET_TO_LAYER_LAST_IMG = 1;
constexpr int SET_TO_LAYER_FAILED = 2;
constexpr uint32_t LAYER_COUNT_DEFAULT = 4;

#define GSP_DEVICE "/dev/dri/card0"
#define GSP_R9P0_NAME "R9P0"

static struct DrmGspR9p0CfgUser *g_drmMConfigs;
static struct GspR9p0CfgUser *g_mConfigs;
static struct GspR9p0ImgLayerUser mImgConfig[R9P0_IMGL_NUM];
static struct GspR9p0OsdLayerUser mOsdConfig[R9P0_OSDL_NUM];
static struct GspR9p0DesLayerUser mDstConfig;
static struct GspR9p0MiscCfgUser mMiscConfig;

static int g_mDevice;
static int g_mCommitStatus = COMMIT_STATUS_IDLE;

static void ClearGspLayerInfo()
{
    for (int icnt = 0; icnt < R9P0_IMGL_NUM; icnt++) {
        memset_s(&mImgConfig[icnt], sizeof(struct GspR9p0ImgLayerUser), 0, sizeof(struct GspR9p0ImgLayerUser));
    }
    for (int icnt = 0; icnt < R9P0_OSDL_NUM; icnt++) {
        memset_s(&mOsdConfig[icnt], sizeof(struct GspR9p0OsdLayerUser), 0, sizeof(struct GspR9p0OsdLayerUser));
    }
    memset_s(&mDstConfig, sizeof(struct GspR9p0DesLayerUser), 0, sizeof(struct GspR9p0DesLayerUser));
    memset_s(&mMiscConfig, sizeof(struct GspR9p0MiscCfgUser), 0, sizeof(struct GspR9p0MiscCfgUser));
    g_mInputRotMode = false;
    g_mCommitStatus = COMMIT_STATUS_IN_PROGRESS;
}

static int32_t InitGspR9p0Cfg(int fd)
{
    if (strcmp(GSP_R9P0_NAME, mCapability.common.version) == 0) {
        struct DrmGspCapability drm_cap;
        drm_cap.gspId = 0;
        drm_cap.size = sizeof(struct GspR9p0Capability);
        drm_cap.cap = &mCapability;
        drmIoctl(fd, DRM_IOCTL_SPRD_GSP_GET_CAPABILITY, &drm_cap);
        g_mConfigs = (struct GspR9p0CfgUser *)malloc(sizeof(struct GspR9p0CfgUser[mCapability.common.ioCnt]));

        g_drmMConfigs =
        (struct DrmGspR9p0CfgUser *)malloc(sizeof(struct DrmGspR9p0CfgUser));
        g_drmMConfigs->async = false;
        g_drmMConfigs->config = g_mConfigs;
        g_drmMConfigs->size = sizeof(struct GspR9p0CfgUser);
        g_drmMConfigs->num = 1;
        g_drmMConfigs->split = false;
        g_drmMConfigs->gspId = 0;
        g_mDevice = fd;
        memset_s(g_mConfigs, sizeof(struct GspR9p0CfgUser[mCapability.common.ioCnt]), 0,
            sizeof(struct GspR9p0CfgUser[mCapability.common.ioCnt]));
        ClearGspLayerInfo();
        g_mCommitStatus = COMMIT_STATUS_IDLE;
    } else {
        DISPLAY_LOGE("gsp version map fail! version: %{public}s", mCapability.common.version);
        close(fd);
        return DISPLAY_FAILURE;
    }
    return DISPLAY_SUCCESS;
}

int32_t RkInitGfx()
{
    int fd;
    struct DrmGspCapability drm_cap;
    fd = open(GSP_DEVICE, O_RDWR, S_IRWXU);
    if (fd < 0) {
        DISPLAY_LOGE("open gsp device failed fd=%d.", fd);
        return DISPLAY_FAILURE;
    }

    drm_cap.gspId = 0;
    drm_cap.size = sizeof(struct GspCapability);
    drm_cap.cap = &mCapability.common;
    int ret = drmIoctl(fd, DRM_IOCTL_SPRD_GSP_GET_CAPABILITY, &drm_cap);
    if (ret < 0) {
    DISPLAY_LOGE("get gsp device capability failed ret=%d.", ret);
    close(fd);
    return DISPLAY_FAILURE;
    }

    if (mCapability.common.magic != GSP_CAPABILITY_MAGIC) {
        DISPLAY_LOGE("gsp device capability has not been initialized");
        close(fd);
        return DISPLAY_FAILURE;
    }

    DISPLAY_LOGI("gsp device version: %s, io count: %d, fd: %d", mCapability.common.version,
        mCapability.common.ioCnt, fd);

    if (mCapability.common.maxLayer < 1) {
        DISPLAY_LOGE("max layer params error");
        return DISPLAY_FAILURE;
    }

    return InitGspR9p0Cfg(fd);
}

int32_t RkDeinitGfx()
{
    return DISPLAY_SUCCESS;
}

int32_t rkFillRect(ISurface *iSurface, IRect *rect, uint32_t color, GfxOpt *opt)
{
    (void)iSurface;
    (void)rect;
    (void)color;
    (void)opt;

    DISPLAY_LOGE("%{public}s: not support", __func__);
    return DISPLAY_SUCCESS;
}

static bool imgLayerValidate(IRect *srcRect, IRect *dstRect, enum GspRotAngle rot, int32_t format)
{
    if (imgCheckOddBoundary(srcRect, format) == false) {
        if ((srcRect->w & 0x01) || (srcRect->h & 0x01)) {
            srcRect->w = srcRect->w & ~0x01;
            srcRect->h = srcRect->h & ~0x01;
        } else {
            DISPLAY_LOGE("img do not support odd source layer xy.");
            return false;
        }
    }

    if (checkRangeSize(srcRect, dstRect) == false) {
        return false;
    }

    if (checkScale(srcRect, dstRect, rot, 0, -1) == false) {
        DISPLAY_LOGD("img do not support scale size.");
        return false;
    }

    if ((rot != GSP_ROT_ANGLE_0) &&
        (checkInputRotation(srcRect, dstRect, rot, format, -1) == false)) {
        DISPLAY_LOGD("img do not support InputRotation.");
        return false;
    }
    return true;
}

static void imgLayerConfigSize(IRect *srcRect, IRect *dstRect, struct GspR9p0ImgLayerParams *params)
{
    params->clipRect.stX = srcRect->x;
    params->clipRect.stY = srcRect->y;
    params->clipRect.rectW = srcRect->w;
    params->clipRect.rectH = srcRect->h;

    params->desRect.stX = dstRect->x;
    params->desRect.stY = dstRect->y;
    params->desRect.rectW = dstRect->w;
    params->desRect.rectH = dstRect->h;
}

bool imgLayer_set(ISurface *srcSurface, IRect *srcRect, IRect *dstRect, GfxOpt *opt,
    struct GspR9p0ImgLayerParams *params, struct GspLayerUser *common)
{
    if (common->enable) {
        return false;
    }
    enum GspRotAngle rot = rotationTypeConvert(opt->rotateType);
    if (rot) {
        g_mInputRotMode = true;
    }
    if (rot >= GSP_ROT_ANGLE_MAX_NUM) {
        return false;
    }

    params->imgFormat = convertImgFormat(srcSurface->enColorFmt, common, params,
        srcSurface->stride, srcSurface->height);

    if (!imgLayerValidate(srcRect, dstRect, rot, params->imgFormat)) {
        return false;
    }

    if (opt->blendType == BLEND_NONE && GSP_R9P0_IMG_FMT_ARGB888 == params->imgFormat) {
        params->imgFormat = GSP_R9P0_IMG_FMT_RGB888;
    }

    imgLayerConfigSize(srcRect, dstRect, params);

    params->pitch = srcSurface->stride / BYTES_PER_PIXEL_RGB888;
    params->height = srcSurface->height;
    if (IsVideoLayerImg(params->imgFormat) == true && mCapability.yuvXywhEven == false) {
        if (needScale(srcRect, dstRect, rot) == true) {
            params->clipRect.stX &= 0xfffe;
            params->clipRect.stY &= 0xfffe;
            params->clipRect.rectW &= 0xfffe;
            params->clipRect.rectH &= 0xfffe;
        }
    }

    if (IsVideoLayerImg(params->imgFormat)) {
        params->y2rMod = 1;
        params->pitch = srcSurface->stride;
    }
    params->y2yMod = 1;
    params->pmargbMod = (opt->blendType == BLEND_SRCOVER || opt->blendType == BLEND_NONE) ? 1 : 0;
    params->pmargbEn = 0;

    configScale(srcRect, dstRect, rot, params);
    params->zOrder = g_mCommitStatus++ - 1;
    params->rotAngle = rot;
    params->secureEn = 0;
    params->palletEn = 0;
    params->alpha = srcSurface->alpha0;

    common->type = GSP_IMG_LAYER;
    common->enable = 1;
    common->waitFd = -1;
    common->shareFd = srcSurface->phyAddr;
    return true;
}


static bool osdLayerValidate(IRect *srcRect, IRect *dstRect, enum GspRotAngle rot, int32_t format)
{
    if (rot) {
        return false;
    }
    if (needScale(srcRect, dstRect, rot)) {
        DISPLAY_LOGD("osd do not support scale layer.");
        return false;
    }
    if (IsVideoLayerOsd(format)) {
        DISPLAY_LOGD("osd do not support video layer.");
        return false;
    }
    if (checkRangeSize(srcRect, dstRect) == false) {
        return false;
    }
    return true;
}

static void osdLayerConfigSize(IRect *srcRect, IRect *dstRect, struct GspR9p0OsdLayerParams *params)
{
    params->clipRect.stX = srcRect->x;
    params->clipRect.stY = srcRect->y;
    params->clipRect.rectW = srcRect->w;
    params->clipRect.rectH = srcRect->h;

    params->desPos.ptX = dstRect->x;
    params->desPos.ptY = dstRect->y;
}

bool osdLayer_set(ISurface *dstSource, IRect *srcRect, IRect *dstRect, GfxOpt *opt,
    struct GspR9p0OsdLayerUser *osdLayerUser)
{
    if (osdLayerUser->common.enable) {
        return false;
    }
    enum GspRotAngle rot = rotationTypeConvert(opt->rotateType);

    osdLayerUser->params.osdFormat = osdFormatConvert(dstSource->enColorFmt,
        &osdLayerUser->params, &osdLayerUser->common);

    if (!osdLayerValidate(srcRect, dstRect, rot, osdLayerUser->params.osdFormat)) {
        return false;
    }

    if (opt->blendType == BLEND_NONE && GSP_R9P0_OSD_FMT_ARGB888 == osdLayerUser->params.osdFormat) {
        osdLayerUser->params.osdFormat = GSP_R9P0_OSD_FMT_RGB888;
    }

    osdLayerConfigSize(srcRect, dstRect, &osdLayerUser->params);

    osdLayerUser->params.pitch = dstSource->stride / BYTES_PER_PIXEL_RGB888;
    osdLayerUser->params.height = dstSource->height;
    osdLayerUser->params.pmargbMod =
        (opt->blendType == BLEND_SRCOVER || opt->blendType == BLEND_NONE) ? 1 : 0;
    osdLayerUser->params.pmargbEn = 0;
    osdLayerUser->params.alpha = dstSource->alpha0;
    osdLayerUser->params.zOrder = g_mCommitStatus++ - 1;
    osdLayerUser->params.palletEn = 0;

    osdLayerUser->common.type = GSP_OSD_LAYER;
    osdLayerUser->common.enable = 1;
    osdLayerUser->common.waitFd = -1;
    osdLayerUser->common.shareFd = dstSource->phyAddr;
    return true;
}

static void DstLayerConfigBg(struct GspR9p0DesLayerParams *params)
{
    struct GspBackgroundPara bgPara;
    bgPara.bkEnable = 1;
    bgPara.bkBlendMod = 0;
    bgPara.backgroundRgb.aVal = 0;
    bgPara.backgroundRgb.rVal = 0;
    bgPara.backgroundRgb.gVal = 0;
    bgPara.backgroundRgb.bVal = 0;
    params->bkPara.bkEnable = bgPara.bkEnable;
    params->bkPara.bkBlendMod = bgPara.bkBlendMod;
    params->bkPara.backgroundRgb = bgPara.backgroundRgb;
}

bool dstLayer_set(ISurface *dstSurface, TransformType rotate, struct GspR9p0DesLayerUser *dstLayerUser)
{
    if (dstLayerUser->common.enable) {
        return false;
    }
    dstLayerUser->params.rotAngle = rotationTypeConvert(rotate);
    dstLayerUser->common.type = GSP_DES_LAYER;
    dstLayerUser->common.enable = 1;
    dstLayerUser->common.waitFd = -1;
    dstLayerUser->common.shareFd = dstSurface->phyAddr;
    dstLayerUser->params.pitch = dstSurface->stride / BYTES_PER_PIXEL_RGB888;
    dstLayerUser->params.height = dstSurface->height;

    dstLayerUser->params.imgFormat = dstFormatConvert(dstSurface->enColorFmt,
        &dstLayerUser->params, &dstLayerUser->common,
        dstSurface->stride / BYTES_PER_PIXEL_RGB888, dstSurface->height);

    DstLayerConfigBg(&dstLayerUser->params);
    dstLayerUser->params.r2yMod = 0;
    return true;
}

int32_t DoSet(int w, int h)
{
    if (!g_mInputRotMode) {
        rotAdjust(&g_mConfigs[0], 0);
    }
    miscCfgParcel(&mMiscConfig, 0, mDstConfig.params.rotAngle, w, h);
    for (int icnt = 0; icnt < R9P0_IMGL_NUM; icnt++) {
        g_mConfigs[0].limg[icnt] = mImgConfig[icnt];
    }

    for (int icnt = 0; icnt < R9P0_OSDL_NUM; icnt++) {
        g_mConfigs[0].losd[icnt] = mOsdConfig[icnt];
    }

    g_mConfigs[0].ld1 = mDstConfig;
    g_mConfigs[0].misc = mMiscConfig;

    int ret = drmIoctl(g_mDevice, DRM_IOCTL_SPRD_GSP_TRIGGER, g_drmMConfigs);
    if (ret < 0) {
        DISPLAY_LOGE("trigger gsp device failed ret=%d.", ret);
        return DISPLAY_FAILURE;
    } else {
        DISPLAY_LOGD("trigger gsp device success");
    }
    return ret;
}

int setToLayer(ISurface *srcSurface, IRect *srcRect, ISurface *dstSurface, IRect *dstRect, GfxOpt *opt)
{
    for (int icnt = 0; icnt < R9P0_OSDL_NUM; icnt++) {
        if (osdLayer_set(srcSurface, srcRect, dstRect, opt, &mOsdConfig[icnt])) {
            return SET_TO_LAYER_SUCCESS;
        }
    }
    for (int icnt = 0; icnt < R9P0_IMGL_NUM; icnt++) {
        if (imgLayer_set(srcSurface, srcRect, dstRect, opt, &mImgConfig[icnt].params, &mImgConfig[icnt].common)) {
            if ((icnt + 1) == R9P0_IMGL_NUM) {
                return SET_TO_LAYER_LAST_IMG;
            } else {
                return SET_TO_LAYER_SUCCESS;
            }
        }
    }
    return SET_TO_LAYER_FAILED;
}

static bool ImgIsFull()
{
    for (int32_t icnt = 0; icnt < R9P0_IMGL_NUM; icnt++) {
        if (!mImgConfig[icnt].common.enable) return false;
    }
    return true;
}

int32_t doFlit(ISurface *srcSurface, IRect *srcRect, ISurface *dstSurface, IRect *dstRect, GfxOpt *opt)
{
    if (g_mCommitStatus == 0) {
        ClearGspLayerInfo();
        dstLayer_set(dstSurface, ROTATE_NONE, &mDstConfig);
    }

    SprdGfxOpt *sprdOpt = (SprdGfxOpt *)opt;
    int ret = setToLayer(srcSurface, srcRect, dstSurface, dstRect, sprdOpt->opt);
    if (ret || ((sprdOpt->index + 1) >= sprdOpt->maxCnt)) {
        DoSet(mDstConfig.params.pitch, mDstConfig.params.height);
        g_mCommitStatus = COMMIT_STATUS_IDLE;
        if ((ret == SET_TO_LAYER_FAILED) && ImgIsFull()) {
            doFlit(srcSurface, srcRect, dstSurface, dstRect, opt);
        } else if ((ret == SET_TO_LAYER_LAST_IMG) && ((sprdOpt->index + 1) < sprdOpt->maxCnt)) {
            sprdOpt->opt->rotateType = ROTATE_NONE;
            IRect rect = {0, 0, dstSurface->width, dstSurface->height};
            doFlit(dstSurface, &rect, dstSurface, &rect, opt);
        }
    }
    return DISPLAY_SUCCESS;
}

int32_t rkBlit(ISurface *srcSurface, IRect *srcRect, ISurface *dstSurface, IRect *dstRect, GfxOpt *opt)
{
    mLayerCount = LAYER_COUNT_DEFAULT;
    CHECK_NULLPOINTER_RETURN_VALUE(srcSurface, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(srcRect, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(dstSurface, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(dstRect, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(opt, DISPLAY_NULL_PTR);

    if (doFlit(srcSurface, srcRect, dstSurface, dstRect, opt) < 0)
        return DISPLAY_FAILURE;
    else
        return DISPLAY_SUCCESS;
}

int32_t RkSync(int32_t timeOut)
{
    (void)timeOut;

    return DISPLAY_SUCCESS;
}

extern "C" int32_t GfxInitialize(GfxFuncs **funcs)
{
    DISPLAY_CHK_RETURN((funcs == NULL), DISPLAY_PARAM_ERR, DISPLAY_LOGE("info is null"));
    GfxFuncs *gfxFuncs = (GfxFuncs *)malloc(sizeof(GfxFuncs));
    DISPLAY_CHK_RETURN((gfxFuncs == NULL), DISPLAY_NULL_PTR, DISPLAY_LOGE("gfxFuncs is nullptr"));
    errno_t eok = memset_s((void *)gfxFuncs, sizeof(GfxFuncs), 0, sizeof(GfxFuncs));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        free(gfxFuncs);
        return DISPLAY_FAILURE;
    }
    gfxFuncs->InitGfx = RkInitGfx;
    gfxFuncs->DeinitGfx = RkDeinitGfx;
    gfxFuncs->FillRect = rkFillRect;
    gfxFuncs->Blit = rkBlit;
    gfxFuncs->Sync = RkSync;
    *funcs = gfxFuncs;

    return DISPLAY_SUCCESS;
}

extern "C" int32_t GfxUninitialize(GfxFuncs *funcs)
{
    CHECK_NULLPOINTER_RETURN_VALUE(funcs, DISPLAY_NULL_PTR);
    free(funcs);
    DISPLAY_LOGI("%{public}s: gfx uninitialize success", __func__);
    return DISPLAY_SUCCESS;
}
} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS