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

#define R9P0_IMGL_NUM 2
#define R9P0_OSDL_NUM 2
#define R9P0_IMGSEC_NUM 0
#define R9P0_OSDSEC_NUM 1
#define HDR_TM_LUT_SIZE (1024 + 1)
#define GSP_R9P0_FREQ_256M 256000000
#define GSP_R9P0_FREQ_307_2M 307200000
#define GSP_R9P0_FREQ_384M 384000000
#define GSP_R9P0_FREQ_512M 512000000
#define GSP_R9P0_FREQ_614_4M 614400000

/*Original: B3B2B1B0*/
/*Original: B3B2B1B0*/
enum GspR9p0WordEndian {
    GSP_R9P0_WORD_ENDN_0 = 0x00, /*B3B2B1B0*/
    GSP_R9P0_WORD_ENDN_1,        /*B0B1B2B3*/
    GSP_R9P0_WORD_ENDN_2,        /*B1B0B3B2*/
    GSP_R9P0_WORD_ENDN_3,        /*B2B3B0B1*/
    GSP_R9P0_WORD_ENDN_MAX_NUM,
};

enum GspR9p0DwordEndian {
    GSP_R9P0_DWORD_ENDN_0 = 0x00, /*B7B6B5B4_B3B2B1B0*/
    GSP_R9P0_DWORD_ENDN_1,        /*B3B2B1B0_B7B6B5B4*/
    GSP_R9P0_DWORD_ENDN_MAX_NUM,
};

enum GspR9p0QwordEndian {
    /*B15B14B13B12_B11B10B9B8_B7B6B5B4_B3B2B1B0*/
    GSP_R9P0_QWORD_ENDN_0 = 0x00,
    /*B7B6B5B4_B3B2B1B0_B15B14B13B12_B11B10B9B8*/
    GSP_R9P0_QWORD_ENDN_1,
    GSP_R9P0_QWORD_ENDN_MAX_NUM,
};

enum GspR9p0RgbSwapMod {
    GSP_R9P0_RGB_SWP_RGB = 0x00,
    GSP_R9P0_RGB_SWP_RBG,
    GSP_R9P0_RGB_SWP_GRB,
    GSP_R9P0_RGB_SWP_GBR,
    GSP_R9P0_RGB_SWP_BGR,
    GSP_R9P0_RGB_SWP_BRG,
    GSP_R9P0_RGB_SWP_MAX,
};

enum GspR9p0ASwapMod {
    GSP_R9P0_A_SWAP_ARGB,
    GSP_R9P0_A_SWAP_RGBA,
    GSP_R9P0_A_SWAP_MAX,
};

enum GspR9p0ImgLayerFormat {
    GSP_R9P0_IMG_FMT_ARGB888 = 0x00,
    GSP_R9P0_IMG_FMT_RGB888,
    GSP_R9P0_IMG_FMT_YUV422_2P,
    GSP_R9P0_IMG_FMT_RESERVED,
    GSP_R9P0_IMG_FMT_YUV420_2P,
    GSP_R9P0_IMG_FMT_YUV420_3P,
    GSP_R9P0_IMG_FMT_RGB565,
    GSP_R9P0_IMG_FMT_YV12,
    GSP_R9P0_IMG_FMT_YCBCR_P010,
    GSP_R9P0_IMG_FMT_MAX_NUM,
};

enum GspR9p0OsdLayerFormat {
    GSP_R9P0_OSD_FMT_ARGB888 = 0x00,
    GSP_R9P0_OSD_FMT_RGB888,
    GSP_R9P0_OSD_FMT_RGB565,
    GSP_R9P0_OSD_FMT_MAX_NUM,
};

enum GspR9p0DesLayerFormat {
    GSP_R9P0_DST_FMT_ARGB888 = 0x00,
    GSP_R9P0_DST_FMT_RGB888,
    GSP_R9P0_DST_FMT_RGB565,
    GSP_R9P0_DST_FMT_YUV420_2P,
    GSP_R9P0_DST_FMT_YUV420_3P,
    GSP_R9P0_DST_FMT_YUV422_2P,
    GSP_R9P0_DST_FMT_RGB666,
    GSP_R9P0_DST_FMT_MAX_NUM,
};

struct GspR9p0Endian {
    __u32 yRgbWordEndn;
    __u32 yRgbDwordEndn;
    __u32 yRgbQwordEndn;
    __u32 uvWordEndn;
    __u32 uvDwordEndn;
    __u32 uvQwordEndn;
    __u32 rgbSwapMode;
    __u32 aSwapMode;
};

struct GspR9p0ImgLayerParams {
    struct GspRect clipRect;
    struct GspRect desRect;
    struct GspRgb grey;
    struct GspRgb colorkey;
    struct GspRgb pallet;
    struct GspR9p0Endian endian;
    __u32 imgFormat;
    __u32 pitch;
    __u32 height;
    __u32 rotAngle;
    __u8 alpha;
    __u8 colorkeyEn;
    __u8 palletEn;
    __u8 fbcdMod;
    __u8 pmargbEn;
    __u8 scalingEn;
    __u8 pmargbMod;
    __u8 zOrder;
    __u8 y2rMod;
    __u8 y2yMod;
    struct GspYuvAdjustPara yuvAdjust;
    struct GspScalePara scalePara;
    __u32 headerSizeR;
    __u8 secureEn;
    uint8_t hdr2rgbMod;
};

struct GspR9p0ImgLayerUser {
    struct GspLayerUser common;
    struct GspR9p0ImgLayerParams params;
};

struct GspR9p0OsdLayerParams {
    struct GspRect clipRect;
    struct GspPos desPos;
    struct GspRgb grey;
    struct GspRgb colorkey;
    struct GspRgb pallet;
    struct GspR9p0Endian endian;
    __u32 osdFormat;
    __u32 pitch;
    __u32 height;
    __u8 alpha;
    __u8 colorkeyEn;
    __u8 palletEn;
    __u8 fbcdMod;
    __u8 pmargbEn;
    __u8 pmargbMod;
    __u8 zOrder;
    __u32 headerSizeR;
    __u8 secureEn;
};

struct GspR9p0OsdLayerUser {
    struct GspLayerUser common;
    struct GspR9p0OsdLayerParams params;
};

struct GspR9p0DesLayerParams {
    __u32 pitch;
    __u32 height;
    struct GspR9p0Endian endian;
    __u32 imgFormat;
    __u32 rotAngle;
    __u8 r2yMod;
    __u8 fbcMod;
    __u8 ditherEn;
    struct GspBackgroundPara bkPara;
    __u32 headerSizeR;
};

struct GspR9p0DesLayerUser {
    struct GspLayerUser common;
    struct GspR9p0DesLayerParams params;
};

struct GspR9p0Hdr10Cfg {
    int videoRange; //0: narrow range, 1: full range
    int transferChar;

    int maxcll;
    int maxscl[3];
    int maxMaxscl;
    int maxpanel;

    int toneMapEn;
    int smEn;
    int profile; //0: profile A; 1: profile B

    __u8 numBezierCurveAnchors;
    __u16 bezierCurveAnchors[15];

    bool regHdrSlp;
    bool regHdrBypassCsc1;
    bool regHdrBypassDegamma;
    bool regHdrBypassCsc2;
    bool regHdrMaxcllGainBypass;
    bool regHdrBypassGamma;
    bool regHdrBypassCsc3;
    bool regHdrForceInRangeCsc1;
    bool regHdrForceInRangeCsc3;
    bool regHdrGamutMapEn;
    bool regHdrCsc1ClEn;
    bool regHdrAvgEn;
    int regHdrAlphaGain;
    int regHdrSatThr;

    int regHdrCsc1Ycr;
    int regHdrCsc1Ucr;
    int regHdrCsc1Vcr;
    int regHdrCsc1Ycg;
    int regHdrCsc1Ucg;
    int regHdrCsc1Vcg;
    int regHdrCsc1Ycb;
    int regHdrCsc1Ucb;
    int regHdrCsc1Vcb;
    int regHdrCsc1Ucb2;
    int regHdrCsc1Vcr2;
    int regHdrCsc1Yls;
    int regHdrCsc1Uls;
    int regHdrCsc1Vls;

    int regHdrDgmlutAddr;
    int regHdrRgmlutAddr;
    int regHdrDgmlutData;
    int regHdrRgmlutData;

    int regHdrCsc2C11;
    int regHdrCsc2C12;
    int regHdrCsc2C13;
    int regHdrCsc2C21;
    int regHdrCsc2C22;
    int regHdrCsc2C23;
    int regHdrCsc2C31;
    int regHdrCsc2C32;
    int regHdrCsc2C33;
    int regHdrCsc2C112;
    int regHdrCsc2C122;
    int regHdrCsc2C132;
    int regHdrCsc2C212;
    int regHdrCsc2C222;
    int regHdrCsc2C232;
    int regHdrCsc2C312;
    int regHdrCsc2C322;
    int regHdrCsc2C332;
    int regHdrCsc2OffsetR;
    int regHdrCsc2OffsetG;
    int regHdrCsc2OffsetB;
    int regHdrCsc2Gain;

    int regHdrRgStep1;
    int regHdrRgStep2;
    int regHdrRgStep3;
    int regHdrRgStep4;
    bool regHdrBeforeGamma;
    int regHdrDiffSatThr;
    int regHdrCsc3A11;
    int regHdrCsc3A12;
    int regHdrCsc3A13;
    int regHdrCsc3A21;
    int regHdrCsc3A22;
    int regHdrCsc3A23;
    int regHdrCsc3A31;
    int regHdrCsc3A32;
    int regHdrCsc3A33;

    bool regHdrTmBypass;
    bool regHdrTmForceInRange;
    int regHdrTmRwSel;
    int regHdrTmUseSel;
    bool regHdrTmForceRwCur;
    bool regHdrTm1En;
    int regHdrTmStep1;
    int regHdrTmStep2;
    int regHdrTmStep3;
    int regHdrTmStep4;
    int regHdrTmNormGain;
    int regHdrTm1BetaGain;
    int regHdrTm2BetaGain;
    int regHdrTm3BetaGain;
    int regHdrTmlutAddr;
    int regHdrTmlutData;

    __u32 hdrToneMappingLutTable[HDR_TM_LUT_SIZE];
};

struct GspR9p0MiscCfgUser {
    uint8_t gspGap;
    uint8_t coreNum;
    uint8_t coWork0;
    uint8_t coWork1;
    uint8_t workMod;
    uint8_t pmargbEn;
    uint8_t secureEn;
    bool hdrFlag[R9P0_IMGL_NUM];
    bool first10bitFrame[R9P0_IMGL_NUM];
    bool hdr10plusUpdate[R9P0_IMGL_NUM];
    uint32_t workFreq;
    struct GspRect workareaSrcRect;
    struct GspPos workareaDesPos;
    struct GspR9p0Hdr10Cfg hdr10Para[R9P0_IMGL_NUM];
};

struct GspR9p0CfgUser {
    struct GspR9p0ImgLayerUser limg[R9P0_IMGL_NUM];
    struct GspR9p0OsdLayerUser losd[R9P0_OSDL_NUM];
    struct GspR9p0DesLayerUser ld1;
    struct GspR9p0MiscCfgUser misc;
};

struct DrmGspR9p0CfgUser {
    __u8 gspId;
    bool async;
    __u32 size;
    __u32 num;
    bool split;
    struct GspR9p0CfgUser *config;
};

struct GspR9p0Capability {
    struct GspCapability common;
    char board[32];
    /* 1: means 1/16, 64 means 4*/
    __u32 scaleRangeUp;
    /* 1: means 1/16, 64 means 4*/
    __u32 scaleRangeDown;
    __u32 yuvXywhEven;
    __u32 maxVideoSize;
    __u32 videoNeedCopy;
    __u32 blendVideoWithOsd;
    __u32 osdScaling;
    __u32 scaleUpdownSametime;
    __u32 maxYuvLayerCnt;
    __u32 maxScaleLayerCnt;
    __u32 seq0ScaleRangeUp;
    __u32 seq0ScaleRangeDown;
    __u32 seq1ScaleRangeUp;
    __u32 seq1ScaleRangeDown;
    __u32 srcYuvXywhEvenLimit;
    __u32 cscMatrixIn;
    __u32 cscMatrixOut;

    __u32 blockAlphaLimit;
    __u32 maxThroughput;

    __u32 maxGspmmuSize;
    __u32 maxGspBandwidth;
};

#endif // GSP_R9P0_CFG_H_
