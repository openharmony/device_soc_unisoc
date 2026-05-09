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

#define UISOC_GSP_DEV "/dev/dri/card0"
#define UISOC_R9P0_NAME "R9P0"

static struct UisocDrmConfig *g_drmMConfigs;
static struct UisocGspConfig *g_mConfigs;
static struct UisocImgCfg mImgConfig[UISOC_R9P0_IMGL_MAX];
static struct UisocOsdCfg mOsdConfig[UISOC_R9P0_OSDL_MAX];
static struct UisocDesCfg mDstConfig;
static struct UisocCommonCfg mMiscConfig;

static int g_mDevice;
static int g_mCommitStatus = COMMIT_STATUS_IDLE;

static void ClearUisocLayerInfo()
{
    for (int icnt = 0; icnt < UISOC_R9P0_IMGL_MAX; icnt++) {
        memset_s(&mImgConfig[icnt], sizeof(struct UisocImgCfg), 0, sizeof(struct UisocImgCfg));
    }
    for (int icnt = 0; icnt < UISOC_R9P0_OSDL_MAX; icnt++) {
        memset_s(&mOsdConfig[icnt], sizeof(struct UisocOsdCfg), 0, sizeof(struct UisocOsdCfg));
    }
    memset_s(&mDstConfig, sizeof(struct UisocDesCfg), 0, sizeof(struct UisocDesCfg));
    memset_s(&mMiscConfig, sizeof(struct UisocCommonCfg), 0, sizeof(struct UisocCommonCfg));
    g_mInputRotMode = false;
    g_mCommitStatus = COMMIT_STATUS_IN_PROGRESS;
}

static int32_t InitGspR9p0Cfg(int fd)
{
    if (strcmp(UISOC_R9P0_NAME, mCapability.base.ver) == 0) {
        struct UisocDrmGspCapa drm_cap;
        drm_cap.id = 0;
        drm_cap.size = sizeof(struct UisocGspCapa);
        drm_cap.ptr = &mCapability;
        drmIoctl(fd, UISOC_IOCTL_GSP_GET_CAP, &drm_cap);
        g_mConfigs = (struct UisocGspConfig *)malloc(sizeof(struct UisocGspConfig[mCapability.base.ioCnt]));

        g_drmMConfigs =
        (struct UisocDrmConfig *)malloc(sizeof(struct UisocDrmConfig));
        g_drmMConfigs->async = false;
        g_drmMConfigs->data = g_mConfigs;
        g_drmMConfigs->size = sizeof(struct UisocGspConfig);
        g_drmMConfigs->count = 1;
        g_drmMConfigs->split = false;
        g_drmMConfigs->id = 0;
        g_mDevice = fd;
        memset_s(g_mConfigs, sizeof(struct UisocGspConfig[mCapability.base.ioCnt]), 0,
            sizeof(struct UisocGspConfig[mCapability.base.ioCnt]));
        ClearUisocLayerInfo();
        g_mCommitStatus = COMMIT_STATUS_IDLE;
    } else {
        DISPLAY_LOGE("gsp version map fail! version: %{public}s", mCapability.base.ver);
        close(fd);
        return DISPLAY_FAILURE;
    }
    return DISPLAY_SUCCESS;
}

int32_t RkInitGfx()
{
    int fd;
    struct UisocDrmGspCapa drm_cap;
    fd = open(UISOC_GSP_DEV, O_RDWR, S_IRWXU);
    if (fd < 0) {
        DISPLAY_LOGE("open gsp device failed fd=%d.", fd);
        return DISPLAY_FAILURE;
    }

    drm_cap.id = 0;
    drm_cap.size = sizeof(struct UisocGspCapa);
    drm_cap.ptr = &mCapability;
    int ret = drmIoctl(fd, UISOC_IOCTL_GSP_GET_CAP, &drm_cap);
    if (ret < 0) {
    DISPLAY_LOGE("get gsp device capability failed ret=%d.", ret);
    close(fd);
    return DISPLAY_FAILURE;
    }

    if (mCapability.base.magic != UISOC_GSP_CAP_MAGIC) {
        DISPLAY_LOGE("gsp device capability has not been initialized");
        close(fd);
        return DISPLAY_FAILURE;
    }

    DISPLAY_LOGI("gsp device version: %s, io count: %d, fd: %d", mCapability.base.ver,
        mCapability.base.ioCnt, fd);

    if (mCapability.base.lyrMax < 1) {
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

static bool imgLayerValidate(IRect *srcRect, IRect *dstRect, UisocRotation rot, int32_t format)
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

    if ((rot != UISOC_ROT_0) &&
        (checkInputRotation(srcRect, dstRect, rot, format, -1) == false)) {
        DISPLAY_LOGD("img do not support InputRotation.");
        return false;
    }
    return true;
}

static void imgLayerConfigSize(IRect *srcRect, IRect *dstRect, struct UisocImgParams *params)
{
    params->clip.x = srcRect->x;
    params->clip.y = srcRect->y;
    params->clip.w = srcRect->w;
    params->clip.h = srcRect->h;

    params->dst.x = dstRect->x;
    params->dst.y = dstRect->y;
    params->dst.w = dstRect->w;
    params->dst.h = dstRect->h;
}

bool imgLayer_set(ISurface *srcSurface, IRect *srcRect, IRect *dstRect, GfxOpt *opt,
    struct UisocImgParams *params, struct UisocLayerCfg *base)
{
    if (base->enable) {
        return false;
    }
    UisocRotation rot = rotationTypeConvert(opt->rotateType);
    if (rot) {
        g_mInputRotMode = true;
    }
    if (rot >= UISOC_ROT_MAX) {
        return false;
    }

    params->fmt = convertImgFormat(srcSurface->enColorFmt, base, params,
        srcSurface->stride, srcSurface->height);

    if (!imgLayerValidate(srcRect, dstRect, rot, params->fmt)) {
        return false;
    }

    if (opt->blendType == BLEND_NONE && UISOC_IMG_ARGB888 == params->fmt) {
        params->fmt = UISOC_IMG_RGB888;
    }

    imgLayerConfigSize(srcRect, dstRect, params);

    params->pitch = srcSurface->stride / BYTES_PER_PIXEL_RGB888;
    params->height = srcSurface->height;
    if (IsVideoLayerImg(params->fmt) == true && mCapability.yuvEven == false) {
        if (needScale(srcRect, dstRect, rot) == true) {
            params->clip.x &= 0xfffe;
            params->clip.y &= 0xfffe;
            params->clip.w &= 0xfffe;
            params->clip.h &= 0xfffe;
        }
    }

    if (IsVideoLayerImg(params->fmt)) {
        params->y2r = 1;
        params->pitch = srcSurface->stride;
    }
    params->y2y = 1;
    params->pmaMode = (opt->blendType == BLEND_SRCOVER || opt->blendType == BLEND_NONE) ? 1 : 0;
    params->pmaEn = 0;

    configScale(srcRect, dstRect, rot, params);
    params->zOrder = g_mCommitStatus++ - 1;
    params->orient = rot;
    params->secure = 0;
    params->pltEn = 0;
    params->alpha = srcSurface->alpha0;

    base->type = UISOC_LYR_IMG;
    base->enable = 1;
    base->fdWait = -1;
    base->fdShare = srcSurface->phyAddr;
    return true;
}


static bool osdLayerValidate(IRect *srcRect, IRect *dstRect, UisocRotation rot, int32_t format)
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

static void osdLayerConfigSize(IRect *srcRect, IRect *dstRect, struct UisocOsdParams *params)
{
    params->clip.x = srcRect->x;
    params->clip.y = srcRect->y;
    params->clip.w = srcRect->w;
    params->clip.h = srcRect->h;

    params->pos.x = dstRect->x;
    params->pos.y = dstRect->y;
}

bool osdLayer_set(ISurface *dstSource, IRect *srcRect, IRect *dstRect, GfxOpt *opt,
    struct UisocOsdCfg *osdConfig)
{
    if (osdConfig->base.enable) {
        return false;
    }
    UisocRotation rot = rotationTypeConvert(opt->rotateType);

    osdConfig->base.off.v = osdConfig->base.off.uv = 0;
    osdConfig->params.fmt = osdFormatConvert(dstSource->enColorFmt, &osdConfig->params);

    if (!osdLayerValidate(srcRect, dstRect, rot, osdConfig->params.fmt)) {
        return false;
    }

    if (opt->blendType == BLEND_NONE && UISOC_OSD_ARGB888 == osdConfig->params.fmt) {
        osdConfig->params.fmt = UISOC_OSD_RGB888;
    }

    osdLayerConfigSize(srcRect, dstRect, &osdConfig->params);

    osdConfig->params.pitch = dstSource->stride / BYTES_PER_PIXEL_RGB888;
    osdConfig->params.height = dstSource->height;
    osdConfig->params.pmaMode =
        (opt->blendType == BLEND_SRCOVER || opt->blendType == BLEND_NONE) ? 1 : 0;
    osdConfig->params.pmaEn = 0;
    osdConfig->params.alpha = dstSource->alpha0;
    osdConfig->params.zOrder = g_mCommitStatus++ - 1;
    osdConfig->params.pltEn = 0;

    osdConfig->base.type = UISOC_LYR_OSD;
    osdConfig->base.enable = 1;
    osdConfig->base.fdWait = -1;
    osdConfig->base.fdShare = dstSource->phyAddr;
    return true;
}

static void DstLayerConfigBg(struct UisocDesParams *params)
{
    struct UisocBgPara bgPara;
    bgPara.enable = 1;
    bgPara.mode = 0;
    bgPara.color.a = 0;
    bgPara.color.r = 0;
    bgPara.color.g = 0;
    bgPara.color.b = 0;
    params->bg.enable = bgPara.enable;
    params->bg.mode = bgPara.mode;
    params->bg.color = bgPara.color;
}

bool dstLayer_set(ISurface *dstSurface, TransformType rotate, struct UisocDesCfg *dstLayerUser)
{
    if (dstLayerUser->base.enable) {
        return false;
    }
    dstLayerUser->params.orient = rotationTypeConvert(rotate);
    dstLayerUser->base.type = UISOC_LYR_DES;
    dstLayerUser->base.enable = 1;
    dstLayerUser->base.fdWait = -1;
    dstLayerUser->base.fdShare = dstSurface->phyAddr;
    dstLayerUser->params.pitch = dstSurface->stride / BYTES_PER_PIXEL_RGB888;
    dstLayerUser->params.height = dstSurface->height;

    dstLayerUser->params.fmt = dstFormatConvert(dstSurface->enColorFmt,
        &dstLayerUser->params, &dstLayerUser->base,
        dstSurface->stride / BYTES_PER_PIXEL_RGB888, dstSurface->height);

    DstLayerConfigBg(&dstLayerUser->params);
    dstLayerUser->params.r2y = 0;
    return true;
}

int32_t DoSet(int w, int h)
{
    if (!g_mInputRotMode) {
        rotAdjust(&g_mConfigs[0], 0);
    }
    miscCfgParcel(&mMiscConfig, 0, mDstConfig.params.orient, w, h);
    for (int icnt = 0; icnt < UISOC_R9P0_IMGL_MAX; icnt++) {
        g_mConfigs[0].img[icnt] = mImgConfig[icnt];
    }

    for (int icnt = 0; icnt < UISOC_R9P0_OSDL_MAX; icnt++) {
        g_mConfigs[0].osd[icnt] = mOsdConfig[icnt];
    }

    g_mConfigs[0].dst = mDstConfig;
    g_mConfigs[0].common = mMiscConfig;

    int ret = drmIoctl(g_mDevice, UISOC_IOCTL_GSP_TRIGGER, g_drmMConfigs);
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
    for (int icnt = 0; icnt < UISOC_R9P0_OSDL_MAX; icnt++) {
        if (osdLayer_set(srcSurface, srcRect, dstRect, opt, &mOsdConfig[icnt])) {
            return SET_TO_LAYER_SUCCESS;
        }
    }
    for (int icnt = 0; icnt < UISOC_R9P0_IMGL_MAX; icnt++) {
        if (imgLayer_set(srcSurface, srcRect, dstRect, opt, &mImgConfig[icnt].params, &mImgConfig[icnt].base)) {
            if ((icnt + 1) == UISOC_R9P0_IMGL_MAX) {
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
    for (int32_t icnt = 0; icnt < UISOC_R9P0_IMGL_MAX; icnt++) {
        if (!mImgConfig[icnt].base.enable) return false;
    }
    return true;
}

int32_t doFlit(ISurface *srcSurface, IRect *srcRect, ISurface *dstSurface, IRect *dstRect, GfxOpt *opt)
{
    if (g_mCommitStatus == 0) {
        ClearUisocLayerInfo();
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
    if (srcSurface == nullptr) {
        DISPLAY_LOGE("srcSurface is null");
        return DISPLAY_NULL_PTR;
    }
    if (srcRect == nullptr) {
        DISPLAY_LOGE("srcRect is null");
        return DISPLAY_NULL_PTR;
    }
    if (dstSurface == nullptr) {
        DISPLAY_LOGE("dstSurface is null");
        return DISPLAY_NULL_PTR;
    }
    if (dstRect == nullptr) {
        DISPLAY_LOGE("dstRect is null");
        return DISPLAY_NULL_PTR;
    }
    if (opt == nullptr) {
        DISPLAY_LOGE("opt is null");
        return DISPLAY_NULL_PTR;
    }

    if (doFlit(srcSurface, srcRect, dstSurface, dstRect, opt) < 0) {
        return DISPLAY_FAILURE;
    } else {
        return DISPLAY_SUCCESS;
    }
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
    if (funcs == nullptr) {
        DISPLAY_LOGE("funcs is null");
        return DISPLAY_NULL_PTR;
    }
    free(funcs);
    DISPLAY_LOGI("%{public}s: gfx uninitialize success", __func__);
    return DISPLAY_SUCCESS;
}
} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS
