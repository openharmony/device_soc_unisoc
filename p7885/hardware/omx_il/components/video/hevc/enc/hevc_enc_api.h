/*
 * Copyright 2016-2026 Unisoc (Shanghai) Technologies Co., Ltd.
 *
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
/******************************************************************************
 ** File Name:    hevcEncApi.h                                               *
 ** Author:       Xiaowei.Luo                                                *
 ** DATE:         6/17/2013                                                  *
 ** Copyright:    2019 Unisoc, Incorporated. All Rights Reserved.            *
 ** Description:  Declaration of interface between HEVC encoder library and  *
 **               its user.                                                   *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 6/17/2013     Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
#ifndef HEVC_ENC_API_H_
#define HEVC_ENC_API_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char BOOLEAN;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;

/**
 * This enumeration is for profiles. The value follows the profileIdc in
 * sequence parameter set. See HEVC specification.
 */
typedef enum {
    AVC_BASELINE = 66,
    AVC_MAIN = 77,
    AVC_EXTENDED = 88,
    AVC_HIGH = 100,
    AVC_HIGH10 = 110,
    AVC_HIGH422 = 122,
    AVC_HIGH444 = 144
} HEVCProfile;

/**
 * This enumeration is for levels. The value follows the level_idc in
 * sequence parameter set. See HEVC specification.
 */
typedef enum {
    AVC_LEVEL_AUTO = 0,
    AVC_LEVEL1_B = 9,
    AVC_LEVEL1 = 10,
    AVC_LEVEL1_1 = 11,
    AVC_LEVEL1_2 = 12,
    AVC_LEVEL1_3 = 13,
    AVC_LEVEL2 = 20,
    AVC_LEVEL2_1 = 21,
    AVC_LEVEL2_2 = 22,
    AVC_LEVEL3 = 30,
    AVC_LEVEL3_1 = 31,
    AVC_LEVEL3_2 = 32,
    AVC_LEVEL4 = 40,
    AVC_LEVEL4_1 = 41,
    AVC_LEVEL4_2 = 42,
    AVC_LEVEL5 = 50,
    AVC_LEVEL5_1 = 51
} HEVCLevel;

/* Constants used in the structures below */
#define MAXIMUMVALUEO_FCPB_CNT                     32   /* used in HRDParams */
#define MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE 255  /* used in SeqParamSet */
#define MAX_NUM_SLICE_GROUP                        8   /* used in PicParamSet */
#define MAX_REF_PIC_LIST_REORDERING                32  /* max according to Annex A, SliceHeader */
#define MAX_DEC_REF_PIC_MARKING                    64  /* max possible given max num ref pictures to 31 */
#define MAX_FS                                     (16 + 1)    /* pre-defined size of frame store array */
#define MAX_LEVEL_IDX                              15  /* only 15 levels defined for now */
#define MAX_REF_PIC_LIST                           33  /* max size of RefPicList0 and RefPicList1 */
#define MAX_SRC_FRAME_BUFFER                        32

/**
 * This enumerates the status of certain flags.
 */
typedef enum {
    HEVC_OFF = 0,
    HEVC_ON = 1
} HEVCFlag;

/**
 * Encoding parameters structure.
 */
typedef struct TagHevcEncParam {
    /* If profile/level is set to zero, encoder will choose the closest one */
    HEVCProfile profile;            /* Profile of the bitstream to be compliant with */
    HEVCLevel level;                /* Level of the bitstream to be compliant with */
    int width;                      /* Width of an input frame in pixels */
    int height;                     /* Height of an input frame in pixels */
    int pocType;                    /* Picture order count mode: 0, 1 or 2 */
    /* For pocType == 0 */
    uint32 log2MaxPocLsbMinus4;     /* Maximum value of POC Lsb, range 0..12 */
    /* For pocType == 1 */
    uint32 deltaPocZeroFlag;        /* Delta POC always zero */
    int offsetPocNonRef;            /* Offset for non-reference picture */
    int offsetTopBottom;            /* Offset between top and bottom field */
    uint32 numRefInCycle;           /* Number of reference frames in one cycle */
    int *offsetPocRef;              /* Array of offsets for reference pictures, dimension [numRefInCycle] */
    int numRefFrame;                 /* Number of reference frames used */
    int numSliceGroup;               /* Number of slice groups */
    int fmoType;                     /* 0: interleave, 1: dispersed, 2: foreground with left-over,
                                        3: box-out, 4: raster scan, 5: wipe, 6: explicit */

    /* For fmoType == 0 */
    uint32 runLengthMinus1[MAX_NUM_SLICE_GROUP];   /* Array of size numSliceGroup, in round robin fashion */
    /* For fmoType == 2 */
    uint32 topLeft[MAX_NUM_SLICE_GROUP - 1];         /* Coordinates of each slice group except last */
    uint32 bottomRight[MAX_NUM_SLICE_GROUP - 1];     /* Background is the last group */
    /* For fmoType == 3, 4, 5 */
    HEVCFlag changeDirFlag;          /* Slice group change direction flag */
    uint32 changeRateMinus1;

    /* For fmoType == 6 */
    uint32 *sliceGroup;              /* Array of size MBWidth*MBHeight */
    HEVCFlag dbFilter;                /* Enable deblocking loop filter */
    int32 disableDbIdc;               /* 0: filter everywhere, 1: no filter, 2: no filter across slice boundary */
    int32 alphaOffset;                /* Alpha offset range -6 .. 6 */
    int32 betaOffset;                 /* Beta offset range -6 .. 6 */
    HEVCFlag constrainedIntraPred;    /* Constrained intra prediction flag */
    HEVCFlag autoScd;                 /* Scene change detection on/off */
    int idrPeriod;                    /* IDR frame refresh rate in number of target encoded frames */
    int intrambRefresh;                /* Minimum number of intra MBs per frame */
    HEVCFlag dataPar;                  /* Enable data partitioning */
    HEVCFlag fullsearch;               /* Enable full-pel full-search mode */
    int searchRange;                   /* Motion vector search range in (-searchRange, +searchRange) pixels */
    HEVCFlag subPel;                   /* Enable sub-pel prediction */
    HEVCFlag submbPred;                /* Enable sub-MB partition mode */
    HEVCFlag rdoptMode;                /* RD-optimal mode selection */
    HEVCFlag bidirPred;                /* Enable bi-directional prediction for B slices.
                                          Forces encoder to treat frames with POC less than previous
                                          encoded frame as B frames; if off, they remain P frames. */
    HEVCFlag rateControl;              /* Rate control enable: on = RC, off = constant QP */
    int initQP;                        /* Initial QP */
    uint32 bitrate;                    /* Target encoding bit rate in bits/second */
    uint32 cpbSize;                    /* Coded picture buffer size in bits */
    uint32 initCBPRemovalDelay;        /* Initial CBP removal delay in msec */
    uint32 frameRate;                   /* Frame rate in units of frames per 1000 seconds */
    /* Note: frameRate is only needed by rate control; HEVC is timestamp agnostic. */
    HEVCFlag outOfBandParamSet;        /* Retrieve parameter sets up front or not */
    HEVCFlag useOverrunBuffer;         /* Do not discard frame if output buffer insufficient;
                                          copy excess bits to overrun buffer. */
} HEVCEncParams;

/**
 * Encoder video capability structure.
 */
typedef struct {
    HEVCProfile profile;
    HEVCLevel level;
    int32 maxWidth;
    int32 maxHeight;
} MMEncCapability;

/**
 * Return codes for encoder functions.
 */
typedef enum {
    MMENC_OK = 0,
    MMENC_ERROR = -1,
    MMENC_PARAM_ERROR = -2,
    MMENC_MEMORY_ERROR = -3,
    MMENC_INVALID_STATUS = -4,
    MMENC_OUTPUT_BUFFER_OVERFLOW = -5,
    MMENC_HW_ERROR = -6
} MMEncRet;

/**
 * Common codec buffer structure (for internal/external memory).
 */
typedef struct {
    uint8 *commonBufferPtr;        /* Pointer to common buffer */
    unsigned long commonBufferPtrPhy;   /* Physical address of common buffer */
    uint32 size;                   /* Size of buffer in bytes */
    int32 frameBfrNum;            /* YUV frame buffer number */
    uint8 *intBufferPtr;           /* Pointer to internal memory */
    int32 intSize;                 /* Internal memory size */
} MMCodecBuffer;

/**
 * Bitstream buffer structure (multiple buffers).
 */
typedef struct {
    uint8 *commonBufferPtr[MAX_SRC_FRAME_BUFFER];   /* Array of pointers to bitstream buffers */
    unsigned long commonBufferPtrPhy[MAX_SRC_FRAME_BUFFER];   /* Physical addresses */
    uint32 size;                     /* Size of each buffer in bytes */
} MMCodecBSBuffer;

typedef MMCodecBuffer MMEncBuffer;

/**
 * YUV format enumeration.
 */
typedef enum {
    MMENC_YUV420P_YU12 = 0,   /* Planar YUV 4:2:0, Y plane followed by U then V */
    MMENC_YUV420P_YV12 = 1,   /* Planar YUV 4:2:0, Y plane followed by V then U */
    MMENC_YUV420SP_NV12 = 2,  /* Semi-planar, Y plane + interleaved UV (U first) */
    MMENC_YUV420SP_NV21 = 3,  /* Semi-planar, Y plane + interleaved VU (V first) */
    MMENC_ARGB32 = 4,
    MMENC_RGBA32 = 5,
} MMENC_YUV_FORMAT_E;

/**
 * Encoder video format structure.
 */
typedef struct {
    int32 frameWidth;      /* Target frame width */
    int32 frameHeight;     /* Target frame height */
    int32 orgWidth;        /* Original input frame width (e.g., 1.2 * frameWidth) */
    int32 orgHeight;       /* Original input frame height (e.g., 1.2 * frameHeight) */
    int32 isH263;          /* 1: H.263, 0: MP4 (legacy) */
    int32 timeScale;       /* Time scale for timestamps */
    int32 yuvFormat;       /* YUV format (see MMENC_YUV_FORMAT_E) */
    int32 cabacEn;         /* CABAC enable flag */
    BOOLEAN eisMode;       /* Electronic Image Stabilization mode */
    uint8 bitDepth;        /* Bit depth per sample */
} MMEncVideoInfo;

/**
 * Encoder priority and operating rate structure.
 */
typedef struct {
    int32 priority;
    int32 operatingRate;
} MMEncCodecPriority;

/**
 * Color aspects structure for VUI.
 */
typedef struct ColorAspectsTag {
    BOOLEAN videoSignalTypePresentFlag;   /* u(1) */
    uint8 videoFormat;                    /* u(3) */
    BOOLEAN videoFullRangeFlag;           /* u(1) */
    BOOLEAN colourDescriptionPresentFlag;    /* u(1) */
    uint8 colourPrimaries;                /* u(8) */
    uint8 transferCharacteristics;        /* u(8) */
    uint8 matrixCoefficients;             /* u(8) */
} ColorAspectsT;

/**
 * Encoder configuration structure.
 */
typedef struct {
    uint32 rateCtrlEnable;      /* 0: disable, 1: enable rate control */
    uint32 targetBitRate;       /* Target bit rate in bits/s (>=400) */
    uint32 frameRate;           /* Frame rate */
    uint32 pFrames;             /* Number of P frames between I frames */
    uint32 vbvBufSize;          /* VBV buffer size (max transfer delay) */
    uint32 qpIvop;              /* QP for first I frame (1..31); default if RC disabled */
    uint32 qpPvop;              /* QP for first P frame (1..31); default if RC disabled */
    uint32 h263En;              /* 1: H.263, 0: MP4 (legacy) */
    uint32 profileAndLevel;     /* Profile and level combined */
    uint32 encSceneMode;        /* Encoding scene mode */
    ColorAspectsT vuiColorAspects;   /* Color aspects for VUI */
    uint32 prependSpsPpsEnable;   /* 0: disable, 1: enable prepending SPS/PPS (currently only disable) */
    /*** Only for H.265 ***/
    uint32 colorFormat;         /* 0: 420SP, 1: 420P, 2: ARGB32 */
    uint32 ctuSizeSlice;        /* CTU size (height) per slice */
} MMEncConfig;

/**
 * Source YUV buffer addresses (virtual and physical).
 */
typedef struct {
    uint8 *pSrcY;
    uint8 *pSrcU;
    uint8 *pSrcV;
    uint8 *pSrcYPhy;    /* Physical address of Y plane */
    uint8 *pSrcUPhy;    /* Physical address of U plane */
    uint8 *pSrcVPhy;    /* Physical address of V plane */
} SrcYuvBufAddr;

/**
 * Encoder input structure for one frame.
 */
typedef struct {
    SrcYuvBufAddr srcYuvAddr[MAX_SRC_FRAME_BUFFER]; /* Source YUV buffer addresses */
    int32 timeStamp;          /* Timestamp */
    int32 bsRemainLen;        /* Remaining bitstream length */
    int32 channelQuality;     /* 0: good, 1: ok, 2: poor */
    int32 orgImgWidth;        /* Original image width */
    int32 orgImgHeight;       /* Original image height */
    int32 cropX;              /* Crop X offset */
    int32 cropY;              /* Crop Y offset */
    int32 bitrate;            /* Requested bitrate (if dynamic change) */
    int32 yuvFormat;          /* YUV format (see MMENC_YUV_FORMAT_E) */
    double matrx[3][3];       /* Transformation matrix (e.g., for EIS) */
    BOOLEAN isChangeBitrate;  /* Flag indicating bitrate change request */
    BOOLEAN needIvoP;         /* Force I or P frame */
    uint8 fbcMode;            /* Frame buffer compression mode (reserved) */
} MMEncIn;

/**
 * Encoder output structure for one frame.
 */
typedef struct {
    uint8 *pOutBuf[MAX_SRC_FRAME_BUFFER];       /* Output bitstream buffers */
    unsigned long pOutBufPhy[MAX_SRC_FRAME_BUFFER];    /* Physical addresses */
    int32 strmSize[MAX_SRC_FRAME_BUFFER];       /* Encoded stream size per buffer; 0 means skip frame */
    int32 vopType[MAX_SRC_FRAME_BUFFER];        /* VOP type: 0=I, 1=P, 2=B */
    uint8 *pheaderBuf;                           /* Pointer to header buffer (e.g., SPS/PPS) */
} MMEncOut;

typedef struct TagvideoEncControls {
    void *videoEncoderData;
    int videoEncoderInit;
} VideoEncControls;

/* Callback function types */
typedef void (*FunctionTypeFlushBsCache)(const void *aUserData);
typedef void (*FunctionTypeBeginCpuAccess)(const void *aUserData);
typedef void (*FunctionTypeEndCpuAccess)(const void *aUserData);
typedef int (*FunctionTypeMallocCb)(void *aUserData, unsigned int sizeExtra, MMCodecBuffer *extraMemBfr);

/**
 * This structure must be allocated and maintained by the user.
 * It serves as a handle to the library object.
 */
typedef struct TagHevcHandle {
    void *videoEncoderData;               /* Internal data; must be NULL initially */
    void *userData;                        /* User object for callbacks */
    uint32 debugEnable;                     /* Debug flag */
    FunctionTypeFlushBsCache vspFlushBSCache;    /* Callback to flush cache */
    FunctionTypeBeginCpuAccess vspBeginCPUAccess;    /* Callback before CPU access */
    FunctionTypeEndCpuAccess vspEndCPUAccess;     /* Callback after CPU access */
    FunctionTypeMallocCb vspExtmemcb;            /* Callback for external memory allocation */
} HEVCHandle;

/**----------------------------------------------------------------------------*
**                           Function Prototypes                              **
**----------------------------------------------------------------------------*/
// Description:   Retrieves the capabilities of the H.265 encoder, including
//                supported profiles, levels, and maximum resolution.
//   hevcHandle  - Handle to the encoder instance.
//   capability  - Pointer to a MMEncCapability structure to be filled.
// Returns:       MMEncRet status code (MMENC_OK on success, otherwise error).
MMEncRet H265EncGetCodecCapability(HEVCHandle *hevcHandle, MMEncCapability *capability);
// Description:   Performs pre-initialization of the H.265 encoder,
//                allocating internal memory buffers.
//   hevcHandle   - Handle to the encoder instance.
//   pinterMemBfr - Pointer to a MMCodecBuffer structure for internal memory.
// Returns:        MMEncRet status code.
MMEncRet H265EncPreInit(HEVCHandle *hevcHandle, MMCodecBuffer *pinterMemBfr);
// Description:   Calculates the number of frames per batch based on width,
//                height, and frame rate.
//   hevcHandle - Handle to the encoder instance.
//   width      - Frame width in pixels.
//   height     - Frame height in pixels.
//   framerate  - Frame rate (frames per second).
// Returns:      Number of frames per batch.
uint32 H265EncGetFnum(HEVCHandle *hevcHandle, uint32 width, uint32 height, uint32 framerate);
// Description:   Initializes the H.265 encoder with the provided video format,
//                bitstream buffer, and priority settings.
//   hevcHandle     - Handle to the encoder instance.
//   pBitstreamBfr  - Pointer to MMCodecBSBuffer structure for output bitstream.
//   pVideoFormat   - Pointer to MMEncVideoInfo describing video format.
//   pPriority      - Pointer to MMEncCodecPriority (may be NULL).
// Returns:         MMEncRet status code.
MMEncRet H265EncInit(HEVCHandle *hevcHandle,
                     MMCodecBSBuffer *pBitstreamBfr,
                     MMEncVideoInfo *pVideoFormat,
                     MMEncCodecPriority *pPriority);
// Description:   Sets the configuration parameters for the H.265 encoder.
//   hevcHandle - Handle to the encoder instance.
//   pConf      - Pointer to MMEncConfig structure containing new parameters.
// Returns:      MMEncRet status code.
MMEncRet H265EncSetConf(HEVCHandle *hevcHandle, MMEncConfig *pConf);
// Description:   Retrieves the current configuration parameters of the
//                H.265 encoder.
//   hevcHandle - Handle to the encoder instance.
//   pConf      - Pointer to MMEncConfig structure to be filled.
// Returns:      MMEncRet status code.
MMEncRet H265EncGetConf(HEVCHandle *hevcHandle, MMEncConfig *pConf);
// Description:   Encodes one frame of video and outputs the compressed
//                bitstream.
//   hevcHandle - Handle to the encoder instance.
//   pInput     - Pointer to MMEncIn structure with input frame data.
//   pOutput    - Pointer to MMEncOut structure to receive encoded output.
// Returns:     MMEncRet status code.
MMEncRet H265EncStrmEncode(HEVCHandle *hevcHandle, MMEncIn *pInput, MMEncOut *pOutput);
// Description:   Generates the Sequence Parameter Set (SPS) or Picture
//                Parameter Set (PPS) header.
//   hevcHandle - Handle to the encoder instance.
//   pOutput    - Pointer to MMEncOut structure to receive the header.
//   isSps      - 1 to generate SPS, 0 to generate PPS.
// Returns:     MMEncRet status code.
MMEncRet H265EncGenHeader(HEVCHandle *hevcHandle, MMEncOut *pOutput, int isSps);
// Description:   Releases the H.265 encoder and frees all associated resources.
//   hevcHandle - Handle to the encoder instance.
// Returns:     MMEncRet status code.
MMEncRet H265EncRelease(HEVCHandle *hevcHandle);
// Description:   Retrieves the I/O virtual address (IOVA) for a given file
//                descriptor.
//   hevcHandle - Handle to the encoder instance.
//   fd         - File descriptor.
//   iova       - Pointer to store the IOVA.
//   size       - Pointer to store the size.
// Returns:      0 on success, negative error code on failure.
int H265Enc_get_iova(HEVCHandle *hevcHandle, int fd, unsigned long *iova, size_t *size);
// Description:   Frees a previously allocated I/O virtual address (IOVA).
//   hevcHandle - Handle to the encoder instance.
//   iova       - IOVA to free.
//   size       - Size of the mapped region.
// Returns:      0 on success, negative error code on failure.
int H265Enc_free_iova(HEVCHandle *hevcHandle, unsigned long iova, size_t size);
// Description:   Retrieves the current IOMMU status.
//   hevcHandle - Handle to the encoder instance.
// Returns:      IOMMU status (implementation-defined).
int H265EncGetIommuStatus(HEVCHandle *hevcHandle);

/* Function pointer types for dynamic linking */
typedef unsigned long (*FT_H265EncGetFnum)(HEVCHandle *hevcHandle, uint32 width, uint32 height, uint32 framerate);
typedef MMEncRet (*FT_H265EncGetCodecCapability)(HEVCHandle *hevcHandle, MMEncCapability *capability);
typedef MMEncRet (*FT_H265EncPreInit)(HEVCHandle *hevcHandle, MMCodecBuffer *pinterMemBfr);
typedef MMEncRet (*FT_H265EncInit)(HEVCHandle *hevcHandle,
                                   MMCodecBSBuffer *pBitstreamBfr,
                                   MMEncVideoInfo *pVideoFormat,
                                   MMEncCodecPriority *pPriority);
typedef MMEncRet (*FT_H265EncSetConf)(HEVCHandle *hevcHandle, MMEncConfig *pConf);
typedef MMEncRet (*FT_H265EncGetConf)(HEVCHandle *hevcHandle, MMEncConfig *pConf);
typedef MMEncRet (*FT_H265EncStrmEncode)(HEVCHandle *hevcHandle, MMEncIn *pInput, MMEncOut *pOutput);
typedef MMEncRet (*FT_H265EncGenHeader)(HEVCHandle *hevcHandle, MMEncOut *pOutput, int isSps);
typedef MMEncRet (*FT_H265EncRelease)(HEVCHandle *hevcHandle);
typedef int (*FtH265EncGetIova)(HEVCHandle *hevcHandle, int fd, unsigned long *iova, size_t *size);
typedef int (*FtH265EncFreeIova)(HEVCHandle *hevcHandle, unsigned long iova, size_t size);
typedef int (*FtH265EncGetIommuStatus)(HEVCHandle *hevcHandle);
typedef int (*FtH265EncNeedAlign)(HEVCHandle *hevcHandle);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

/**---------------------------------------------------------------------------*/
#endif /* HEVC_ENC_API_H_ */
// End
