/*
 * Copyright (C) 2018 Spreadtrum Communications Inc.
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

struct GspR9p0Capability mCapability;
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
    if (srcRect->w < mCapability.common.cropMin.rectW ||
        srcRect->h < mCapability.common.cropMin.rectH ||
        srcRect->w > mCapability.common.cropMax.rectW ||
        srcRect->h > mCapability.common.cropMax.rectH || dstRect->w < mCapability.common.outMin.rectW ||
        dstRect->h < mCapability.common.outMin.rectH || dstRect->w > mCapability.common.outMax.rectW ||
        dstRect->h > mCapability.common.outMax.rectH) {
        DISPLAY_LOGD("clip or dst rect is not supported.");
        result = false;
    }

    return result;
}

enum GspRotAngle rotationTypeConvert(TransformType value)
{
    enum GspRotAngle rot = GSP_ROT_ANGLE_0;
    switch (value) {
        case ROTATE_NONE:
            rot = GSP_ROT_ANGLE_0;
            break;
        case ROTATE_90:
            rot = GSP_ROT_ANGLE_270;
            break;
        case ROTATE_180:
            rot = GSP_ROT_ANGLE_180;
            break;
        case ROTATE_270:
            rot = GSP_ROT_ANGLE_90;
            break;
        case MIRROR_H:
            rot = GSP_ROT_ANGLE_180_M;
            break;
        case MIRROR_V:
            rot = GSP_ROT_ANGLE_0_M;
            break;
        case MIRROR_H_ROTATE_90:
            rot = GSP_ROT_ANGLE_270_M;
            break;
        case MIRROR_V_ROTATE_90:
            rot = GSP_ROT_ANGLE_90_M;
            break;
        default:
            rot = GSP_ROT_ANGLE_MAX_NUM;
            break;
    }

    return rot;
}

int convertImgFormat(PixelFormat enColorFmt, struct GspLayerUser *common,
                     struct GspR9p0ImgLayerParams *params, uint32_t w, uint32_t h)
{
    int format = GSP_R9P0_IMG_FMT_MAX_NUM;
    uint32_t pixel_cnt = w * h;
    common->offset.vOffset = common->offset.uvOffset = pixel_cnt;

    params->endian.uvDwordEndn = GSP_R9P0_DWORD_ENDN_0;
    params->endian.uvWordEndn = GSP_R9P0_WORD_ENDN_0;
    params->endian.yRgbDwordEndn = GSP_R9P0_DWORD_ENDN_0;
    params->endian.yRgbWordEndn = GSP_R9P0_WORD_ENDN_0;
    params->endian.aSwapMode = GSP_R9P0_A_SWAP_ARGB;
    switch (enColorFmt) {
        case PIXEL_FMT_RGBA_8888:
            format = GSP_R9P0_IMG_FMT_ARGB888;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_BGR;
            break;
        case PIXEL_FMT_BGRA_8888:
            format = GSP_R9P0_IMG_FMT_ARGB888;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_RGB;
            break;
        case PIXEL_FMT_RGBX_8888:
            format = GSP_R9P0_IMG_FMT_RGB888;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_BGR;
            break;
        case PIXEL_FMT_RGB_565:
            format = GSP_R9P0_IMG_FMT_RGB565;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_RGB;
            break;
        case PIXEL_FMT_YCBCR_420_SP:
            format = GSP_R9P0_IMG_FMT_YUV420_2P;
            params->endian.uvWordEndn = GSP_R9P0_WORD_ENDN_0;
            params->endian.yRgbWordEndn = GSP_R9P0_WORD_ENDN_0;
            params->endian.aSwapMode = GSP_R9P0_A_SWAP_ARGB;
            break;
        case PIXEL_FMT_YCRCB_420_SP:
            format = GSP_R9P0_IMG_FMT_YUV420_2P;
            params->endian.uvWordEndn = GSP_R9P0_WORD_ENDN_3;
            break;
        case PIXEL_FMT_YCRCB_420_P: // YUV420_3P, Y V U
            format = GSP_R9P0_IMG_FMT_YV12;
            common->offset.uvOffset += (h + 1) / 2 * ((w + 1) / 2); // (height + 1) / 2 * ALIGN((width + 1) / 2, 16);
            params->endian.uvWordEndn = GSP_R9P0_WORD_ENDN_0;
            params->endian.yRgbWordEndn = GSP_R9P0_WORD_ENDN_0;
            params->endian.aSwapMode = GSP_R9P0_A_SWAP_ARGB;
            break;
        default:
            return -1;
    }
    return format;
}

int osdFormatConvert(PixelFormat enColorFmt, struct GspR9p0OsdLayerParams *params, struct GspLayerUser *common)
{
    int format = GSP_R9P0_OSD_FMT_MAX_NUM;
    common->offset.vOffset = common->offset.uvOffset = 0;

    params->endian.yRgbWordEndn = GSP_R9P0_WORD_ENDN_0;
    params->endian.yRgbDwordEndn = GSP_R9P0_DWORD_ENDN_0;
    params->endian.yRgbQwordEndn = GSP_R9P0_QWORD_ENDN_0;
    params->endian.aSwapMode = GSP_R9P0_A_SWAP_ARGB;
    switch (enColorFmt) {
        case PIXEL_FMT_RGBA_8888:
            format = GSP_R9P0_OSD_FMT_ARGB888;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_BGR;
            break;
        case PIXEL_FMT_BGRA_8888:
            format = GSP_R9P0_OSD_FMT_ARGB888;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_RGB;
            break;
        case PIXEL_FMT_RGBX_8888:
            format = GSP_R9P0_OSD_FMT_RGB888;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_BGR;
            break;
        case PIXEL_FMT_BGRX_8888:
            format = GSP_R9P0_OSD_FMT_RGB888;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_RGB;
            break;
        case PIXEL_FMT_RGB_565:
            format = GSP_R9P0_OSD_FMT_RGB565;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_RGB;
            break;
        default:
            DISPLAY_LOGD("osd configEndian, unsupport format=0x%x.", format);
            break;
    }

    return format;
}

int dstFormatConvert(PixelFormat enColorFmt, struct GspR9p0DesLayerParams *params, struct GspLayerUser *common,
                     uint32_t w, uint32_t h)
{
    int format = GSP_R9P0_DST_FMT_MAX_NUM;
    params->endian.uvDwordEndn = GSP_R9P0_DWORD_ENDN_0;
    params->endian.uvWordEndn = GSP_R9P0_WORD_ENDN_0;
    params->endian.yRgbDwordEndn = GSP_R9P0_DWORD_ENDN_0;
    params->endian.yRgbWordEndn = GSP_R9P0_WORD_ENDN_0;
    params->endian.aSwapMode = GSP_R9P0_A_SWAP_ARGB;
    switch (enColorFmt) {
        case PIXEL_FMT_YCBCR_420_SP:
            format = GSP_R9P0_DST_FMT_YUV420_2P;
            params->endian.aSwapMode = GSP_R9P0_A_SWAP_ARGB;
            break;
        case PIXEL_FMT_YCBCR_422_P:
            format = GSP_R9P0_DST_FMT_YUV422_2P;
            break;
        case PIXEL_FMT_YCRCB_420_P:
            format = GSP_R9P0_DST_FMT_YUV420_3P;
            params->endian.uvWordEndn = GSP_R9P0_WORD_ENDN_3;
            break;
        case PIXEL_FMT_RGBA_8888:
            format = GSP_R9P0_DST_FMT_ARGB888;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_BGR;
            break;
        case PIXEL_FMT_RGB_565:
            format = GSP_R9P0_DST_FMT_RGB565;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_RGB;
            break;
        case PIXEL_FMT_RGBX_8888:
            format = GSP_R9P0_DST_FMT_RGB888;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_BGR;
            break;
        case PIXEL_FMT_BGRA_8888:
            format = GSP_R9P0_DST_FMT_ARGB888;
            params->endian.rgbSwapMode = GSP_R9P0_RGB_SWP_RGB;
            break;
        default:
            DISPLAY_LOGD("dst configEndian, unsupport format=0x%x.", enColorFmt);
            break;
    }
    common->offset.uvOffset = w * h;
    common->offset.vOffset = w * h;

    return format;
}

bool IsVideoLayerImg(int format)
{
    bool result = false;

    if ((format >= GSP_R9P0_IMG_FMT_YUV422_2P &&
         format <= GSP_R9P0_IMG_FMT_YUV420_3P) ||
        (format == GSP_R9P0_IMG_FMT_YV12)) {
        result = true;
    }

    return result;
}

bool IsVideoLayerOsd(int format)
{
    bool result = false;

    if (format > GSP_R9P0_OSD_FMT_RGB565) {
        result = true;
    }

    return result;
}

bool imgCheckOddBoundary(IRect *srcRect, int32_t format)
{
    bool result = true;

    // if yuvXywhEven == 1, gsp do not support odd source layer.
    if (IsVideoLayerImg(format) == true && !mCapability.yuvXywhEven) {
        if ((srcRect->x & 0x1) || (srcRect->y & 0x1) || (srcRect->w & 0x1) ||
            (srcRect->h & 0x1)) {
            DISPLAY_LOGD("do not support odd source layer xywh.");
            result = false;
        }
    }

    return result;
}

bool IsLandScapeTransform(enum GspRotAngle rot)
{
    bool result = false;

    if (rot == GSP_ROT_ANGLE_90 || rot == GSP_ROT_ANGLE_270 ||
        rot == GSP_ROT_ANGLE_90_M || rot == GSP_ROT_ANGLE_270_M) {
        result = true;
    }

    return result;
}


bool checkScaleSize(IRect *srcRect, IRect *dstRect,
    enum GspRotAngle rot, bool inFBC,
    int protectLayerNum)
{
    bool result = true;
    constexpr int SCALE_LIMIT_6 = 6;
    constexpr uint32_t SCALE_RATIO_4 = 4;

    uint16_t scaleUpLimit = (inFBC ? 24 : (mCapability.scaleRangeUp / 4));
    uint16_t scaleDownLimit = (inFBC ? 4 : (16 / mCapability.scaleRangeDown));

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
                enum GspRotAngle rot, bool inFBC,
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

    if ((mCapability.scaleUpdownSametime == false) &&
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
        case GSP_R9P0_IMG_FMT_YUV420_2P:
        case GSP_R9P0_IMG_FMT_YV12:
        case GSP_R9P0_IMG_FMT_ARGB888:
        case GSP_R9P0_IMG_FMT_RGB888:
        case GSP_R9P0_IMG_FMT_RGB565:
        case GSP_R9P0_IMG_FMT_YCBCR_P010:
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
        case GSP_R9P0_IMG_FMT_ARGB888:
        case GSP_R9P0_IMG_FMT_RGB888:
        case GSP_R9P0_IMG_FMT_RGB565:
        case GSP_R9P0_IMG_FMT_YUV420_2P:
        case GSP_R9P0_IMG_FMT_YV12:
        case GSP_R9P0_IMG_FMT_YCBCR_P010:
            return true;
        default:
            DISPLAY_LOGD("input rotation scaling unsupport img format:0x%x.", format);
            return false;
    }
}

bool checkInputRotation(IRect *srcRect, IRect *dstRect, enum GspRotAngle rot,
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
               enum GspRotAngle rot)
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

void configScale(IRect *srcRect, IRect *dstRect, enum GspRotAngle rot,
                 struct GspR9p0ImgLayerParams *params)
{
    uint32_t dstw = 0;
    uint32_t dsth = 0;

    if (needScale(srcRect, dstRect, rot) == true) {
        params->scalingEn = 1;
        params->scalePara.scaleEn = 1;

        params->scalePara.scaleRectIn.stX = srcRect->x;
        params->scalePara.scaleRectIn.stY = srcRect->y;
        params->scalePara.scaleRectIn.rectW = srcRect->w;
        params->scalePara.scaleRectIn.rectH = srcRect->h;

        params->scalePara.scaleRectOut.stX = dstRect->x;
        params->scalePara.scaleRectOut.stY = dstRect->y;
        params->scalePara.scaleRectOut.rectW = dstRect->w;
        params->scalePara.scaleRectOut.rectH = dstRect->h;

        if (IsLandScapeTransform(rot) == true) {
            dstw = params->scalePara.scaleRectOut.rectH;
            dsth = params->scalePara.scaleRectOut.rectW;
        } else {
            dstw = params->scalePara.scaleRectOut.rectW;
            dsth = params->scalePara.scaleRectOut.rectH;
        }

        params->scalePara.htapMod =
            GetTapVar0(params->scalePara.scaleRectIn.rectW, dstw);

        params->scalePara.vtapMod =
            GetTapVar0(params->scalePara.scaleRectIn.rectH, dsth);
    }

    /* for output rotation dst coordinate calucuate, "rotAdjustSingle" */
    params->scalePara.scaleRectOut.rectW = dstRect->w;
    params->scalePara.scaleRectOut.rectH = dstRect->h;
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

static int imgLayerRotAdjust(struct GspR9p0CfgUser *cmdInfo, uint32_t transform)
{
    int32_t ret = 0;
    struct GspR9p0ImgLayerUser *imgInfo = cmdInfo->limg;
    for (int icnt = 0; icnt < R9P0_IMGL_NUM; icnt++) {
        if (imgInfo[icnt].common.enable == 1) {
            RotRect rect = {
                imgInfo[icnt].params.desRect.stX,
                imgInfo[icnt].params.desRect.stY,
                imgInfo[icnt].params.scalePara.scaleRectOut.rectW,
                imgInfo[icnt].params.scalePara.scaleRectOut.rectH
            };
            ret = rotAdjustSingle(&rect, cmdInfo->ld1.params.pitch, cmdInfo->ld1.params.height, transform);
            imgInfo[icnt].params.desRect.stX = rect.x;
            imgInfo[icnt].params.desRect.stY = rect.y;
            imgInfo[icnt].params.scalePara.scaleRectOut.rectW = rect.w;
            imgInfo[icnt].params.scalePara.scaleRectOut.rectH = rect.h;
            if (ret) {
                DISPLAY_LOGD("rotAdjust img layer[%d] rotation adjust failed, ret=%d.", icnt, ret);
                return ret;
            }
        }
    }
    return 0;
}

static int osdLayerRotAdjust(struct GspR9p0CfgUser *cmdInfo, uint32_t transform)
{
    int32_t ret = 0;
    struct GspR9p0OsdLayerUser *osdInfo = cmdInfo->losd;
    for (int icnt = 0; icnt < R9P0_OSDL_NUM; icnt++) {
        if (osdInfo[icnt].common.enable == 1) {
            uint16_t w = osdInfo[icnt].params.clipRect.rectW;
            uint16_t h = osdInfo[icnt].params.clipRect.rectH;
            if (transform & ROTATE_90) {
                uint16_t tmp = w;
                w = h;
                h = tmp;
            }
            RotRect rect = {
                osdInfo[icnt].params.desPos.ptX,
                osdInfo[icnt].params.desPos.ptY,
                w,
                h
            };
            ret = rotAdjustSingle(&rect, cmdInfo->ld1.params.pitch, cmdInfo->ld1.params.height, transform);
            osdInfo[icnt].params.desPos.ptX = rect.x;
            osdInfo[icnt].params.desPos.ptY = rect.y;
            // Note: w and h are discarded as original logic
            if (ret) {
                DISPLAY_LOGD("rotAdjust OSD[%d] rotation adjust failed, ret=%d.", icnt, ret);
                return ret;
            }
        }
    }
    return 0;
}

int rotAdjust(struct GspR9p0CfgUser *cmdInfo, uint32_t transform)
{
    int32_t ret = imgLayerRotAdjust(cmdInfo, transform);
    if (ret) return ret;

    ret = osdLayerRotAdjust(cmdInfo, transform);
    if (ret) return ret;

    if (transform & ROTATE_90) {
        uint16_t tmp = cmdInfo->ld1.params.pitch;
        cmdInfo->ld1.params.pitch = cmdInfo->ld1.params.height;
        cmdInfo->ld1.params.height = tmp;
        tmp = cmdInfo->misc.workareaSrcRect.rectW;
        cmdInfo->misc.workareaSrcRect.rectW = cmdInfo->misc.workareaSrcRect.rectH;
        cmdInfo->misc.workareaSrcRect.rectH = tmp;
    }
    return ret;
}


bool miscCfgParcel(struct GspR9p0MiscCfgUser *miscCfg, int modeType, uint32_t transform,
    uint32_t w, uint32_t h)
{
    bool status = false;
    uint32_t freq = GSP_R9P0_FREQ_256M;

    switch (modeType) {
        case 0: {
            /* run_mod = 0, scale_seq = 0 */
            miscCfg->workMod = 0;
            miscCfg->coreNum = 0;

            miscCfg->workareaSrcRect.stX = 0;
            miscCfg->workareaSrcRect.stY = 0;
            miscCfg->workareaSrcRect.rectW = w;
            miscCfg->workareaSrcRect.rectH = h;

            miscCfg->secureEn = 0;

            freq = GSP_R9P0_FREQ_512M;

            /* set gsp freq = 512M when gsp open dual core */
            if (miscCfg->coreNum) {
                freq = GSP_R9P0_FREQ_512M;
            }

            if (strcmp(GSP_QOGIRN6L, mCapability.board) == 0) {
                freq = GSP_R9P0_FREQ_614_4M;
            }

            miscCfg->workFreq = freq;
            DISPLAY_LOGD("config frequency Index : 0x%d", freq);
        }
            miscCfg->workareaDesPos.ptX = 0;
            miscCfg->workareaDesPos.ptY = 0;
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
