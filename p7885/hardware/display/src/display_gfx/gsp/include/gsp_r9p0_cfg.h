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

#ifndef GSP_R9P0_CFG_H_
#define GSP_R9P0_CFG_H_

#include <linux/ioctl.h>
#include <linux/types.h>
#include "gsp_cfg.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {

#define UISOC_R9P0_IMGL_MAX     2
#define UISOC_R9P0_OSDL_MAX     2
#define UISOC_R9P0_IMSEC_MAX    0
#define UISOC_R9P0_OSSEC_MAX    1
#define UISOC_LUT_SIZE          (1024 + 1)

#define UISOC_FREQ_256M         256000000
#define UISOC_FREQ_307M         307200000
#define UISOC_FREQ_384M         384000000
#define UISOC_FREQ_512M         512000000
#define UISOC_FREQ_614M         614400000

enum UisocEndianW {
    UISOC_ENDN_W0 = 0x00,
    UISOC_ENDN_W1,
    UISOC_ENDN_W2,
    UISOC_ENDN_W3,
    UISOC_ENDN_W_MAX,
};

enum UisocEndianDW {
    UISOC_ENDN_DW0 = 0x00,
    UISOC_ENDN_DW1,
    UISOC_ENDN_DW_MAX,
};

enum UisocEndianQW {
    UISOC_ENDN_QW0 = 0x00,
    UISOC_ENDN_QW1,
    UISOC_ENDN_QW_MAX,
};

enum UisocSwapMode {
    UISOC_SWAP_RGB = 0x00,
    UISOC_SWAP_RBG,
    UISOC_SWAP_GRB,
    UISOC_SWAP_GBR,
    UISOC_SWAP_BGR,
    UISOC_SWAP_BRG,
    UISOC_SWAP_MAX,
};

enum UisocAlphaSwap {
    UISOC_ALPHA_ARGB,
    UISOC_ALPHA_RGBA,
    UISOC_ALPHA_MAX,
};

enum UisocImgFormat {
    UISOC_IMG_ARGB888 = 0x00,
    UISOC_IMG_RGB888,
    UISOC_IMG_YUV422_2P,
    UISOC_IMG_RESERVED,
    UISOC_IMG_YUV420_2P,
    UISOC_IMG_YUV420_3P,
    UISOC_IMG_RGB565,
    UISOC_IMG_YV12,
    UISOC_IMG_P010,
    UISOC_IMG_MAX,
};

enum UisocOsdFormat {
    UISOC_OSD_ARGB888 = 0x00,
    UISOC_OSD_RGB888,
    UISOC_OSD_RGB565,
    UISOC_OSD_MAX,
};

enum UisocDesFormat {
    UISOC_DES_ARGB888 = 0x00,
    UISOC_DES_RGB888,
    UISOC_DES_RGB565,
    UISOC_DES_YUV420_2P,
    UISOC_DES_YUV420_3P,
    UISOC_DES_YUV422_2P,
    UISOC_DES_RGB666,
    UISOC_DES_MAX,
};

struct UisocEndian {
    __u32 wEndn;
    __u32 dwEndn;
    __u32 qwEndn;
    __u32 uvWEndn;
    __u32 uvDWEndn;
    __u32 uvQWEndn;
    __u32 swap;
    __u32 aSwap;
};

struct UisocImgParams {
    struct UisocRect clip;
    struct UisocRect dst;
    struct UisocColor grey;
    struct UisocColor ck;
    struct UisocColor plt;
    struct UisocEndian endian;
    __u32 fmt;
    __u32 pitch;
    __u32 height;
    __u32 orient;
    __u8 alpha;
    __u8 ckEn;
    __u8 pltEn;
    __u8 fbcd;
    __u8 pmaEn;
    __u8 scaleEn;
    __u8 pmaMode;
    __u8 zOrder;
    __u8 y2r;
    __u8 y2y;
    struct UisocYuvAdj adj;
    struct UisocScalePara scale;
    __u32 hdrSz;
    __u8 secure;
    uint8_t hdr2rgb;
};

struct UisocImgCfg {
    struct UisocLayerCfg base;
    struct UisocImgParams params;
};

struct UisocOsdParams {
    struct UisocRect clip;
    struct UisocPt pos;
    struct UisocColor grey;
    struct UisocColor ck;
    struct UisocColor plt;
    struct UisocEndian endian;
    __u32 fmt;
    __u32 pitch;
    __u32 height;
    __u8 alpha;
    __u8 ckEn;
    __u8 pltEn;
    __u8 fbcd;
    __u8 pmaEn;
    __u8 pmaMode;
    __u8 zOrder;
    __u32 hdrSz;
    __u8 secure;
};

struct UisocOsdCfg {
    struct UisocLayerCfg base;
    struct UisocOsdParams params;
};

struct UisocDesParams {
    __u32 pitch;
    __u32 height;
    struct UisocEndian endian;
    __u32 fmt;
    __u32 orient;
    __u8 r2y;
    __u8 fbc;
    __u8 dither;
    struct UisocBgPara bg;
    __u32 hdrSz;
};

struct UisocDesCfg {
    struct UisocLayerCfg base;
    struct UisocDesParams params;
};

struct UisocHdrConfig {
    int vRange;
    int trfChar;
    int maxCll;
    int maxScl[3];
    int maxmScl;
    int maxPnl;
    int tmEn;
    int smEn;
    int prof;
    __u8 bzAnchCnt;
    __u16 bzAnchors[15];
    bool slp;
    bool bCsc1;
    bool bDegamma;
    bool bCsc2;
    bool bCllGain;
    bool bGamma;
    bool bCsc3;
    bool fInCsc1;
    bool fInCsc3;
    bool gamutEn;
    bool csc1ClEn;
    bool avgEn;
    int aGain;
    int sThr;
    int c1Ycr;
    int c1Ucr;
    int c1Vcr;
    int c1Ycg;
    int c1Ucg;
    int c1Vcg;
    int c1Ycb;
    int c1Ucb;
    int c1Vcb;
    int c1Ucb2;
    int c1Vcr2;
    int c1Yls;
    int c1Uls;
    int c1Vls;
    int dgAddr;
    int rgAddr;
    int dgData;
    int rgData;
    int c2C11;
    int c2C12;
    int c2C13;
    int c2C21;
    int c2C22;
    int c2C23;
    int c2C31;
    int c2C32;
    int c2C33;
    int c2C112;
    int c2C122;
    int c2C132;
    int c2C212;
    int c2C222;
    int c2C232;
    int c2C312;
    int c2C322;
    int c2C332;
    int c2OffR;
    int c2OffG;
    int c2OffB;
    int c2Gain;
    int rgS1;
    int rgS2;
    int rgS3;
    int rgS4;
    bool bGammaPre;
    int dSatThr;
    int c3A11;
    int c3A12;
    int c3A13;
    int c3A21;
    int c3A22;
    int c3A23;
    int c3A31;
    int c3A32;
    int c3A33;
    bool tmBypass;
    bool tmForceIn;
    int tmRwSel;
    int tmUseSel;
    bool tmForceRw;
    bool tm1En;
    int tmS1;
    int tmS2;
    int tmS3;
    int tmS4;
    int tmNGain;
    int tm1BGain;
    int tm2BGain;
    int tm3BGain;
    int tmLutAddr;
    int tmLutData;
    __u32 tmLut[UISOC_LUT_SIZE];
};

struct UisocCommonCfg {
    uint8_t gap;
    uint8_t cores;
    uint8_t co0;
    uint8_t co1;
    uint8_t mode;
    uint8_t pmaEn;
    uint8_t secure;
    bool hdr[UISOC_R9P0_IMGL_MAX];
    bool f10b[UISOC_R9P0_IMGL_MAX];
    bool h10p[UISOC_R9P0_IMGL_MAX];
    uint32_t freq;
    struct UisocRect workSrc;
    struct UisocPt workDst;
    struct UisocHdrConfig hdr10[UISOC_R9P0_IMGL_MAX];
};

struct UisocGspConfig {
    struct UisocImgCfg img[UISOC_R9P0_IMGL_MAX];
    struct UisocOsdCfg osd[UISOC_R9P0_OSDL_MAX];
    struct UisocDesCfg dst;
    struct UisocCommonCfg common;
};

struct UisocDrmConfig {
    __u8 id;
    bool async;
    __u32 size;
    __u32 count;
    bool split;
    struct UisocGspConfig *data;
};

struct UisocGspCapa {
    struct UisocCapa base;
    char board[32];
    __u32 upMax;
    __u32 dnMax;
    __u32 yuvEven;
    __u32 videoMax;
    __u32 videoCpy;
    __u32 blendOsd;
    __u32 osdScale;
    __u32 dualScale;
    __u32 yuvLyrCnt;
    __u32 sclLyrCnt;
    __u32 seq0Up;
    __u32 seq0Dn;
    __u32 seq1Up;
    __u32 seq1Dn;
    __u32 yuvEvenLim;
    __u32 cscIn;
    __u32 cscOut;
    __u32 alphaLim;
    __u32 tpMax;
    __u32 mmuMax;
    __u32 bwMax;
};

} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS

#endif /* GSP_R9P0_CFG_H_ */
