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
#include "display_common.h"
#include "securec.h"
#include "sprd_drm_gsp.h"
#include "gsp_cfg.h"
#include "gsp_r9p0_cfg.h"
#include "gsp_r9p0_common.h"
#include "v1_0/display_composer_type.h"
namespace OHOS {
namespace HDI {
namespace DISPLAY {
using namespace OHOS::HDI::Display::Composer::V1_0;
#define GSP_QOGIRN6PRO "qogirn6pro"
#define GSP_QOGIRN6L "qogirn6l"

struct UisocGspCapa mCapability;
uint16_t mLayerCount = 0;
bool g_mInputRotMode = false;

constexpr int GSP_TAP_2 = 2;
constexpr int GSP_TAP_4 = 4;
constexpr int GSP_TAP_6 = 6;
constexpr int GSP_TAP_8 = 8;
constexpr int SCALE_FACTOR_3 = 3;
constexpr int SCALE_FACTOR_4 = 4;
constexpr int SCALE_FACTOR_6 = 6;
constexpr int SCALE_FACTOR_8 = 8;
constexpr int SCALE_FACTOR_12 = 12;
constexpr int SCALE_FACTOR_16 = 16;
constexpr int TAP_MOD_DIVISOR = 2;

int GetTapVar0(int srcPara, int destPara)
{
    int retTap = 0;
    if ((srcPara < SCALE_FACTOR_3 * destPara) && (destPara <= SCALE_FACTOR_4 * srcPara)) {
        retTap = GSP_TAP_4;
    } else if ((srcPara >= SCALE_FACTOR_3 * destPara) && (srcPara < SCALE_FACTOR_4 * destPara)) {
        retTap = GSP_TAP_6;
    } else if (srcPara == SCALE_FACTOR_4 * destPara) {
        retTap = GSP_TAP_8;
    } else if ((srcPara > SCALE_FACTOR_4 * destPara) && (srcPara < SCALE_FACTOR_6 * destPara)) {
        retTap = GSP_TAP_4;
    } else if ((srcPara >= SCALE_FACTOR_6 * destPara) && (srcPara < SCALE_FACTOR_8 * destPara)) {
        retTap = GSP_TAP_6;
    } else if (srcPara == SCALE_FACTOR_8 * destPara) {
        retTap = GSP_TAP_8;
    } else if ((srcPara > SCALE_FACTOR_8 * destPara) && (srcPara < SCALE_FACTOR_12 * destPara)) {
        retTap = GSP_TAP_4;
    } else if ((srcPara >= SCALE_FACTOR_12 * destPara) && (srcPara < SCALE_FACTOR_16 * destPara)) {
        retTap = GSP_TAP_6;
    } else if (srcPara == SCALE_FACTOR_16 * destPara) {
        retTap = GSP_TAP_8;
    } else {
        retTap = GSP_TAP_2;
    }

    retTap = (GSP_TAP_8 - retTap) / TAP_MOD_DIVISOR;
    // retTap = 3 is null for htap & vtap on r9p0

    return retTap;
}

bool checkRangeSize(IRect *srcRect, IRect *dstRect)
{
    bool result = true;
    // Source and destination rectangle size check.
    if (srcRect->w < mCapability.base.cropMin.w ||
        srcRect->h < mCapability.base.cropMin.h ||
        srcRect->w > mCapability.base.cropMax.w ||
        srcRect->h > mCapability.base.cropMax.h || dstRect->w < mCapability.base.outMin.w ||
        dstRect->h < mCapability.base.outMin.h || dstRect->w > mCapability.base.outMax.w ||
        dstRect->h > mCapability.base.outMax.h) {
        DISPLAY_LOGD("clip or dst rect is not supported.");
        result = false;
    }

    return result;
}

UisocRotation rotationTypeConvert(TransformType value)
{
    UisocRotation rot = UISOC_ROT_0;
    switch (value) {
        case ROTATE_NONE:
            rot = UISOC_ROT_0;
            break;
        case ROTATE_90:
            rot = UISOC_ROT_270;
            break;
        case ROTATE_180:
            rot = UISOC_ROT_180;
            break;
        case ROTATE_270:
            rot = UISOC_ROT_90;
            break;
        case MIRROR_H:
            rot = UISOC_ROT_180_FLIP;
            break;
        case MIRROR_V:
            rot = UISOC_ROT_FLIP_H;
            break;
        case MIRROR_H_ROTATE_90:
            rot = UISOC_ROT_270_FLIP;
            break;
        case MIRROR_V_ROTATE_90:
            rot = UISOC_ROT_FLIP_V;
            break;
        default:
            rot = UISOC_ROT_MAX;
            break;
    }

    return rot;
}

int convertImgFormat(PixelFormat enColorFmt, struct UisocLayerCfg *common,
                     struct UisocImgParams *params, uint32_t w, uint32_t h)
{
    int format = UISOC_IMG_MAX;
    uint32_t pixel_cnt = w * h;
    common->off.v = common->off.uv = pixel_cnt;

    params->endian.uvDWEndn = UISOC_ENDN_DW0;
    params->endian.uvWEndn = UISOC_ENDN_W0;
    params->endian.dwEndn = UISOC_ENDN_DW0;
    params->endian.wEndn = UISOC_ENDN_W0;
    params->endian.aSwap = UISOC_ALPHA_ARGB;
    switch (enColorFmt) {
        case PIXEL_FMT_RGBA_8888:
            format = UISOC_IMG_ARGB888;
            params->endian.swap = UISOC_SWAP_BGR;
            break;
        case PIXEL_FMT_BGRA_8888:
            format = UISOC_IMG_ARGB888;
            params->endian.swap = UISOC_SWAP_RGB;
            break;
        case PIXEL_FMT_RGBX_8888:
            format = UISOC_IMG_RGB888;
            params->endian.swap = UISOC_SWAP_BGR;
            break;
        case PIXEL_FMT_RGB_565:
            format = UISOC_IMG_RGB565;
            params->endian.swap = UISOC_SWAP_RGB;
            break;
        case PIXEL_FMT_YCBCR_420_SP:
            format = UISOC_IMG_YUV420_2P;
            params->endian.uvWEndn = UISOC_ENDN_W0;
            params->endian.wEndn = UISOC_ENDN_W0;
            params->endian.aSwap = UISOC_ALPHA_ARGB;
            break;
        case PIXEL_FMT_YCRCB_420_SP:
            format = UISOC_IMG_YUV420_2P;
            params->endian.uvWEndn = UISOC_ENDN_W3;
            break;
        case PIXEL_FMT_YCRCB_420_P: // YUV420_3P, Y V U
            format = UISOC_IMG_YV12;
            common->off.uv += (h + 1) / 2 * ((w + 1) / 2); // (height + 1) / 2 * ALIGN((width + 1) / 2, 16);
            params->endian.uvWEndn = UISOC_ENDN_W0;
            params->endian.wEndn = UISOC_ENDN_W0;
            params->endian.aSwap = UISOC_ALPHA_ARGB;
            break;
        default:
            return -1;
    }
    return format;
}

int osdFormatConvert(PixelFormat enColorFmt, struct UisocOsdParams *params)
{
    int format = UISOC_OSD_MAX;

    params->endian.wEndn = UISOC_ENDN_W0;
    params->endian.dwEndn = UISOC_ENDN_DW0;
    params->endian.qwEndn = UISOC_ENDN_QW0;
    params->endian.aSwap = UISOC_ALPHA_ARGB;
    switch (enColorFmt) {
        case PIXEL_FMT_RGBA_8888:
            format = UISOC_OSD_ARGB888;
            params->endian.swap = UISOC_SWAP_BGR;
            break;
        case PIXEL_FMT_BGRA_8888:
            format = UISOC_OSD_ARGB888;
            params->endian.swap = UISOC_SWAP_RGB;
            break;
        case PIXEL_FMT_RGBX_8888:
            format = UISOC_OSD_RGB888;
            params->endian.swap = UISOC_SWAP_BGR;
            break;
        case PIXEL_FMT_BGRX_8888:
            format = UISOC_OSD_RGB888;
            params->endian.swap = UISOC_SWAP_RGB;
            break;
        case PIXEL_FMT_RGB_565:
            format = UISOC_OSD_RGB565;
            params->endian.swap = UISOC_SWAP_RGB;
            break;
        default:
            DISPLAY_LOGD("osd configEndian, unsupport format=0x%x.", format);
            break;
    }

    return format;
}

int dstFormatConvert(PixelFormat enColorFmt, struct UisocDesParams *params, struct UisocLayerCfg *common,
                     uint32_t w, uint32_t h)
{
    int format = UISOC_DES_MAX;
    params->endian.uvDWEndn = UISOC_ENDN_DW0;
    params->endian.uvWEndn = UISOC_ENDN_W0;
    params->endian.dwEndn = UISOC_ENDN_DW0;
    params->endian.wEndn = UISOC_ENDN_W0;
    params->endian.aSwap = UISOC_ALPHA_ARGB;
    switch (enColorFmt) {
        case PIXEL_FMT_YCBCR_420_SP:
            format = UISOC_DES_YUV420_2P;
            params->endian.aSwap = UISOC_ALPHA_ARGB;
            break;
        case PIXEL_FMT_YCBCR_422_P:
            format = UISOC_DES_YUV422_2P;
            break;
        case PIXEL_FMT_YCRCB_420_P:
            format = UISOC_DES_YUV420_3P;
            params->endian.uvWEndn = UISOC_ENDN_W3;
            break;
        case PIXEL_FMT_RGBA_8888:
            format = UISOC_DES_ARGB888;
            params->endian.swap = UISOC_SWAP_BGR;
            break;
        case PIXEL_FMT_RGB_565:
            format = UISOC_DES_RGB565;
            params->endian.swap = UISOC_SWAP_RGB;
            break;
        case PIXEL_FMT_RGBX_8888:
            format = UISOC_DES_RGB888;
            params->endian.swap = UISOC_SWAP_BGR;
            break;
        case PIXEL_FMT_BGRA_8888:
            format = UISOC_DES_ARGB888;
            params->endian.swap = UISOC_SWAP_RGB;
            break;
        default:
            DISPLAY_LOGD("dst configEndian, unsupport format=0x%x.", enColorFmt);
            break;
    }
    common->off.uv = w * h;
    common->off.v = w * h;

    return format;
}

bool IsVideoLayerImg(int format)
{
    bool result = false;

    if ((format >= UISOC_IMG_YUV422_2P &&
         format <= UISOC_IMG_YUV420_3P) ||
        (format == UISOC_IMG_YV12)) {
        result = true;
    }

    return result;
}

bool IsVideoLayerOsd(int format)
{
    bool result = false;

    if (format > UISOC_OSD_RGB565) {
        result = true;
    }

    return result;
}

bool imgCheckOddBoundary(IRect *srcRect, int32_t format)
{
    bool result = true;

    // if yuvEven == 1, gsp do not support odd source layer.
    if (IsVideoLayerImg(format) == true && !mCapability.yuvEven) {
        if ((srcRect->x & 0x1) || (srcRect->y & 0x1) || (srcRect->w & 0x1) ||
            (srcRect->h & 0x1)) {
            DISPLAY_LOGD("do not support odd source layer xywh.");
            result = false;
        }
    }

    return result;
}

bool IsLandScapeTransform(enum UisocRotation rot)
{
    bool result = false;

    if (rot == UISOC_ROT_90 || rot == UISOC_ROT_270 ||
        rot == UISOC_ROT_FLIP_V || rot == UISOC_ROT_270_FLIP) {
        result = true;
    }

    return result;
}


bool checkScaleSize(IRect *srcRect, IRect *dstRect,
    enum UisocRotation rot, bool inFBC,
    int protectLayerNum)
{
    bool result = true;
    constexpr int SCALE_LIMIT_6 = 6;
    constexpr uint32_t SCALE_RATIO_4 = 4;

    uint16_t scaleUpLimit = (inFBC ? 24 : (mCapability.upMax / 4));
    uint16_t scaleDownLimit = (inFBC ? 4 : (16 / mCapability.dnMax));

    uint32_t srcw = 0;
    uint32_t srch = 0;
    uint32_t dstw = dstRect->w;
    uint32_t dsth = dstRect->h;

    if (protectLayerNum >= 0) {
        scaleDownLimit = SCALE_FACTOR_12;
        scaleUpLimit = SCALE_FACTOR_12;
    }

    if ((mLayerCount >= SCALE_LIMIT_6) && (inFBC == 0)) {
        scaleDownLimit = SCALE_FACTOR_12;
    }

    if (IsLandScapeTransform(rot) == true) {
        srcw = srcRect->h;
        srch = srcRect->w;
    } else {
        srcw = srcRect->w;
        srch = srcRect->h;
    }

    if (scaleUpLimit * srcw < dstw || scaleUpLimit * srch < dsth ||
        scaleDownLimit * dstw < srcw || scaleDownLimit * dsth < srch) {
        // gsp support [1/16-gsp_scaling_up_limit] scaling
        DISPLAY_LOGD("GSP only support %d-%d scaling!", scaleDownLimit,
                     scaleUpLimit);
        result = false;
    } else {
        if ((protectLayerNum >= 0) && (srcw * SCALE_RATIO_4 < dstw || srch * SCALE_RATIO_4 < dsth ||
                                       dstw * SCALE_RATIO_4 < srcw || dsth * SCALE_RATIO_4 < srch)) {
            DISPLAY_LOGD("GSP need scale twice");
        }
    }

    return result;
}

bool checkScale(IRect *srcRect, IRect *dstRect,
                enum UisocRotation rot, bool inFBC,
                int protectLayerNum)
{
    uint32_t srcw = 0;
    uint32_t srch = 0;
    uint32_t dstw = dstRect->w;
    uint32_t dsth = dstRect->h;

    if (IsLandScapeTransform(rot) == true) {
        srcw = srcRect->h;
        srch = srcRect->w;
    } else {
        srcw = srcRect->w;
        srch = srcRect->h;
    }

    if ((mCapability.dualScale == false) &&
        ((srcw < dstw && srch > dsth) || (srcw > dstw && srch < dsth))) {
        DISPLAY_LOGD("need scale up and down at same time, which not support");
        return false;
    }

    if (checkScaleSize(srcRect, dstRect, rot, inFBC, protectLayerNum) == false) {
        return false;
    }

    return true;
}

static bool checkInputRotationFormat(int32_t format)
{
    switch (format) {
        case UISOC_IMG_YUV420_2P:
        case UISOC_IMG_YV12:
        case UISOC_IMG_ARGB888:
        case UISOC_IMG_RGB888:
        case UISOC_IMG_RGB565:
        case UISOC_IMG_P010:
            return true;
        default:
            DISPLAY_LOGD("input rotation unsupport img format:0x%x.", format);
            return false;
    }
}

static bool checkInputRotationScaling(int32_t format, uint32_t srcw, uint32_t srch, uint32_t dstw, uint32_t dsth)
{
    if (srcw == dstw && srch == dsth) {
        return true;
    }
    switch (format) {
        case UISOC_IMG_ARGB888:
        case UISOC_IMG_RGB888:
        case UISOC_IMG_RGB565:
        case UISOC_IMG_YUV420_2P:
        case UISOC_IMG_YV12:
        case UISOC_IMG_P010:
            return true;
        default:
            DISPLAY_LOGD("input rotation scaling unsupport img format:0x%x.", format);
            return false;
    }
}

bool checkInputRotation(IRect *srcRect, IRect *dstRect, enum UisocRotation rot,
    int32_t format, int protectLayerNum)
{
    uint16_t scaleUpLimit = 24;
    uint16_t scaleDownLimit = 4;

    if (protectLayerNum >= 0) {
        scaleDownLimit = SCALE_FACTOR_16;
        scaleUpLimit = SCALE_FACTOR_16;
    }

    uint32_t srcw = 0;
    uint32_t srch = 0;
    uint32_t dstw = dstRect->w;
    uint32_t dsth = dstRect->h;

    if (IsLandScapeTransform(rot) == true) {
        srcw = srcRect->h;
        srch = srcRect->w;
    } else {
        srcw = srcRect->w;
        srch = srcRect->h;
    }

    if (scaleUpLimit * srcw < dstw || scaleUpLimit * srch < dsth ||
        scaleDownLimit * dstw < srcw || scaleDownLimit * dsth < srch) {
        DISPLAY_LOGD("GSP input rotation only support %d-%d scaling!",
            scaleDownLimit, scaleUpLimit);
        return false;
    }

    if (rot != 0) {
        if (!checkInputRotationFormat(format)) {
            return false;
        }
        if (!checkInputRotationScaling(format, srcw, srch, dstw, dsth)) {
            return false;
        }
    }

    return true;
}

bool needScale(IRect *srcRect, IRect *dstRect,
               UisocRotation rot)
{
    bool result = false;

    if (IsLandScapeTransform(rot) == true) {
        if (srcRect->w != dstRect->h || srcRect->h != dstRect->w) {
            result = true;
        }
    } else {
        if (srcRect->w != dstRect->w || srcRect->h != dstRect->h) {
            result = true;
        }
    }

    return result;
}

void configScale(IRect *srcRect, IRect *dstRect, UisocRotation rot,
                 struct UisocImgParams *params)
{
    uint32_t dstw = 0;
    uint32_t dsth = 0;

    if (needScale(srcRect, dstRect, rot) == true) {
        params->scaleEn = 1;
        params->scale.enable = 1;

        params->scale.in.x = srcRect->x;
        params->scale.in.y = srcRect->y;
        params->scale.in.w = srcRect->w;
        params->scale.in.h = srcRect->h;

        params->scale.out.x = dstRect->x;
        params->scale.out.y = dstRect->y;
        params->scale.out.w = dstRect->w;
        params->scale.out.h = dstRect->h;

        if (IsLandScapeTransform(rot) == true) {
            dstw = params->scale.out.h;
            dsth = params->scale.out.w;
        } else {
            dstw = params->scale.out.w;
            dsth = params->scale.out.h;
        }

        params->scale.hTap =
            GetTapVar0(params->scale.in.w, dstw);

        params->scale.vTap =
            GetTapVar0(params->scale.in.h, dsth);
    }

    /* for output rotation dst coordinate calucuate, "rotAdjustSingle" */
    params->scale.out.w = dstRect->w;
    params->scale.out.h = dstRect->h;
}


struct RotRect {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

static int rotAdjustSingle(RotRect *rect, uint32_t pitch,
                           uint32_t height, uint32_t transform)
{
    uint32_t x = rect->x;
    uint32_t y = rect->y;

    /* first adjust dest x y */
    switch (transform) {
        case 0:
            break;
        case MIRROR_H: // 1
            rect->x = pitch - x - rect->w;
            break;
        case MIRROR_V: // 2
            rect->y = height - y - rect->h;
            break;
        case ROTATE_180: // 3
            rect->x = pitch - x - rect->w;
            rect->y = height - y - rect->h;
            break;
        case ROTATE_90: // 4
            rect->x = y;
            rect->y = pitch - x - rect->w;
            break;
        case MIRROR_H_ROTATE_90: // 5
            rect->x = height - y - rect->h;
            rect->y = pitch - x - rect->w;
            break;
        case MIRROR_V_ROTATE_90: // 6
            rect->x = y;
            rect->y = x;
            break;
        case ROTATE_270: // 7
            rect->x = height - y - rect->h;
            rect->y = x;
            break;
        default:
            DISPLAY_LOGD("rotAdjustSingle, unsupport angle=%d.", transform);
            break;
    }

    /* then adjust dest width height */
    if (transform & ROTATE_90) {
        uint16_t tmp = rect->w;
        rect->w = rect->h;
        rect->h = tmp;
    }

    return 0;
}

static int imgLayerRotAdjust(struct UisocGspConfig *cmdInfo, uint32_t transform)
{
    int32_t ret = 0;
    struct UisocImgCfg *imgInfo = cmdInfo->img;
    for (int icnt = 0; icnt < UISOC_R9P0_IMGL_MAX; icnt++) {
        if (imgInfo[icnt].base.enable == 1) {
            RotRect rect = {
                imgInfo[icnt].params.dst.x,
                imgInfo[icnt].params.dst.y,
                imgInfo[icnt].params.scale.out.w,
                imgInfo[icnt].params.scale.out.h
            };
            ret = rotAdjustSingle(&rect, cmdInfo->dst.params.pitch, cmdInfo->dst.params.height, transform);
            imgInfo[icnt].params.dst.x = rect.x;
            imgInfo[icnt].params.dst.y = rect.y;
            imgInfo[icnt].params.scale.out.w = rect.w;
            imgInfo[icnt].params.scale.out.h = rect.h;
            if (ret) {
                DISPLAY_LOGD("rotAdjust img layer[%d] rotation adjust failed, ret=%d.", icnt, ret);
                return ret;
            }
        }
    }
    return 0;
}

static int osdLayerRotAdjust(struct UisocGspConfig *cmdInfo, uint32_t transform)
{
    int32_t ret = 0;
    struct UisocOsdCfg *osdInfo = cmdInfo->osd;
    for (int icnt = 0; icnt < UISOC_R9P0_OSDL_MAX; icnt++) {
        if (osdInfo[icnt].base.enable == 1) {
            uint16_t w = osdInfo[icnt].params.clip.w;
            uint16_t h = osdInfo[icnt].params.clip.h;
            if (transform & ROTATE_90) {
                uint16_t tmp = w;
                w = h;
                h = tmp;
            }
            RotRect rect = {
                osdInfo[icnt].params.pos.x,
                osdInfo[icnt].params.pos.y,
                w,
                h
            };
            ret = rotAdjustSingle(&rect, cmdInfo->dst.params.pitch, cmdInfo->dst.params.height, transform);
            osdInfo[icnt].params.pos.x = rect.x;
            osdInfo[icnt].params.pos.y = rect.y;
            // Note: w and h are discarded as original logic
            if (ret) {
                DISPLAY_LOGD("rotAdjust OSD[%d] rotation adjust failed, ret=%d.", icnt, ret);
                return ret;
            }
        }
    }
    return 0;
}

int rotAdjust(struct UisocGspConfig *cmdInfo, uint32_t transform)
{
    int32_t ret = imgLayerRotAdjust(cmdInfo, transform);
    if (ret) return ret;

    ret = osdLayerRotAdjust(cmdInfo, transform);
    if (ret) return ret;

    if (transform & ROTATE_90) {
        uint16_t tmp = cmdInfo->dst.params.pitch;
        cmdInfo->dst.params.pitch = cmdInfo->dst.params.height;
        cmdInfo->dst.params.height = tmp;
        tmp = cmdInfo->common.workSrc.w;
        cmdInfo->common.workSrc.w = cmdInfo->common.workSrc.h;
        cmdInfo->common.workSrc.h = tmp;
    }
    return ret;
}


bool miscCfgParcel(struct UisocCommonCfg *miscCfg, int modeType, uint32_t transform,
    uint32_t w, uint32_t h)
{
    bool status = false;
    uint32_t freq = UISOC_FREQ_256M;

    switch (modeType) {
        case 0: {
            /* run_mod = 0, scale_seq = 0 */
            miscCfg->mode = 0;
            miscCfg->cores = 0;

            miscCfg->workSrc.x = 0;
            miscCfg->workSrc.y = 0;
            miscCfg->workSrc.w = w;
            miscCfg->workSrc.h = h;

            miscCfg->secure = 0;

            freq = UISOC_FREQ_512M;

            /* set gsp freq = 512M when gsp open dual core */
            if (miscCfg->cores) {
                freq = UISOC_FREQ_512M;
            }

            if (strcmp(GSP_QOGIRN6L, mCapability.board) == 0) {
                freq = UISOC_FREQ_614M;
            }

            miscCfg->freq = freq;
            DISPLAY_LOGD("config frequency Index : 0x%d", freq);
        }
            miscCfg->workDst.x = 0;
            miscCfg->workDst.y = 0;
            status = true;
            break;
        default:
            DISPLAY_LOGD("gsp r9p0 not implement other mode(%d) yet! ", modeType);
            break;
    }

    return status;
}
} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS
