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
#ifndef __GSP_R9P0_COMMON_H__
#define __GSP_R9P0_COMMON_H__
#include "v1_0/display_composer_type.h"
#include "gsp_cfg.h"
namespace OHOS {
namespace HDI {
namespace DISPLAY {
using namespace OHOS::HDI::Display::Composer::V1_0;
int GetTapVar0(int srcPara, int destPara);
bool checkRangeSize(IRect *srcRect, IRect *dstRect);
GspRotAngle rotationTypeConvert(TransformType value);
int convertImgFormat(PixelFormat enColorFmt, struct GspLayerUser *common,
    struct GspR9p0ImgLayerParams *params, uint32_t w, uint32_t h);
int osdFormatConvert(PixelFormat enColorFmt, struct GspR9p0OsdLayerParams *params, struct GspLayerUser *common);
int dstFormatConvert(PixelFormat enColorFmt, struct GspR9p0DesLayerParams *params, struct GspLayerUser *common,
    uint32_t w, uint32_t h);
bool IsVideoLayerImg(int format);
bool IsVideoLayerOsd(int format);
bool imgCheckOddBoundary(IRect *srcRect, int32_t format);
bool IsLandScapeTransform(enum GspRotAngle rot);
bool checkScaleSize(IRect *srcRect, IRect *dstRect,
    enum GspRotAngle rot, bool inFBC,
    int protectLayerNum);
bool checkScale(IRect *srcRect, IRect *dstRect,
                enum GspRotAngle rot, bool inFBC,
                int protectLayerNum);
bool checkInputRotation(IRect *srcRect, IRect *dstRect, enum GspRotAngle rot,
    int32_t format, int protectLayerNum);
bool needScale(IRect *srcRect, IRect *dstRect,
    enum GspRotAngle rot);
void configScale(IRect *srcRect, IRect *dstRect, enum GspRotAngle rot,
    struct GspR9p0ImgLayerParams *params);
int rotAdjust(struct GspR9p0CfgUser *cmdInfo, uint32_t transform);
bool miscCfgParcel(struct GspR9p0MiscCfgUser *miscCfg, int mode_type, uint32_t transform,
    uint32_t w, uint32_t h);

extern struct GspR9p0Capability mCapability;
extern uint16_t mLayerCount;
extern bool g_mInputRotMode;
} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS
#endif