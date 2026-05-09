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
 ** File Name:    h265dec.h                                                   *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         3/15/2007                                                   *
 ** Copyright:    2019 Unisoc, Incorporated. All Rights Reserved.             *
 ** Description:  Define data structures for H.265 video decoder.             *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 3/15/2007     Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
#ifndef H265_DEC_H_
#define H265_DEC_H_
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

namespace OHOS {
namespace OMX {

typedef unsigned char BOOLEAN;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef signed long long int64;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;

/**
 * HEVC profile enumeration.
 * Values follow the profileIdc in the sequence parameter set.
 */
typedef enum {
    HEVC_PROFILE_UNKNOWN = 0x0,
    HEVC_PROFILE_MAIN    = 0x1,
    HEVC_PROFILE_MAIN_10 = 0x2,
    HEVC_PROFILE_MAIN_STILL = 0x3,
    HEVC_PROFILE_MAX     = 0x7FFFFFFF
} HEVCProfile;

/**
 * HEVC level enumeration.
 * Values follow the level_idc in the sequence parameter set.
 */
typedef enum {
    HEVC_LEVEL_UNKNOWN    = 0x0,
    HEVC_MAIN_TIER_LEVEL1  = 0x1,
    HEVC_MAIN_TIER_LEVEL2  = 0x4,
    HEVC_MAIN_TIER_LEVEL2_1 = 0x10,
    HEVC_MAIN_TIER_LEVEL3  = 0x40,
    HEVC_MAIN_TIER_LEVEL3_1 = 0x100,
    HEVC_MAIN_TIER_LEVEL4  = 0x400,
    HEVC_MAIN_TIER_LEVEL4_1 = 0x1000,
    HEVC_MAIN_TIER_LEVEL5  = 0x4000,
    HEVC_MAIN_TIER_LEVEL5_1 = 0x10000,
    HEVC_MAIN_TIER_LEVEL5_2 = 0x40000,
} HEVCLevel;

/**
 * Decoder capability structure.
 */
typedef struct {
    HEVCProfile profile;      /* Supported profile */
    HEVCLevel level;          /* Supported level */
    uint32 maxWidth;          /* Maximum supported width */
    uint32 maxHeight;         /* Maximum supported height */
} MMDecCapability;

/**
 * Return codes for decoder functions.
 */
typedef enum {
    MMDEC_OK = 0,
    MMDEC_ERROR = -1,
    MMDEC_PARAM_ERROR = -2,
    MMDEC_MEMORY_ERROR = -3,
    MMDEC_INVALID_STATUS = -4,
    MMDEC_STREAM_ERROR = -5,
    MMDEC_OUTPUT_BUFFER_OVERFLOW = -6,
    MMDEC_HW_ERROR = -7,
    MMDEC_NOT_SUPPORTED = -8,
    MMDEC_FRAME_SEEK_IVOP = -9,
    MMDEC_MEMORY_ALLOCED = -10
} MMDecRet;

/**
 * Video standard enumeration.
 */
typedef enum {
    ITU_H263 = 0,
    MPEG4,
    JPEG,
    FLV_V1,
    H264,
    RV8,
    RV9,
    H265
} VIDEO_STANDARD_E;

/**
 * YUV format enumeration.
 */
typedef enum {
    YUV420P_YU12 = 0,    /* Planar YUV 4:2:0, Y plane followed by U then V */
    YUV420P_YV12 = 1,    /* Planar YUV 4:2:0, Y plane followed by V then U */
    YUV420SP_NV12 = 2,    /* Semi-planar, Y plane + interleaved UV (U first) */
    YUV420SP_NV21 = 3,    /* Semi-planar, Y plane + interleaved VU (V first) */
} MM_YUV_FORMAT_E;

/**
 * Decoder video format structure.
 */
typedef struct {
    int32 videoStd;                /* Video standard (see VIDEO_STANDARD_E) */
    int32 frameWidth;              /* Frame width in pixels */
    int32 frameHeight;             /* Frame height in pixels */
    int32 iExtra;                  /* Size of extra data */
    void *pExtra;                  /* Pointer to extra data (e.g., codec specific data) */
    unsigned long pExtraPhy;       /* Physical address of extra data */
    int32 yuvFormat;               /* YUV format (see MM_YUV_FORMAT_E) */
    int32 useThreadCnt;            /* Number of threads to use (e.g., 1 for VoLTE, 4 otherwise) */
} MMDecVideoFormat;

/**
 * Decoder priority and operating rate structure.
 */
typedef struct {
    int32 priority;                /* Decoder thread priority */
    int32 operatingRate;           /* Operating rate in kHz */
} MMDecCodecPriority;

/**
 * Common codec buffer structure.
 */
typedef struct {
    uint8 *commonBufferPtr;        /* Pointer to common buffer */
    unsigned long commonBufferPtrPhy; /* Physical address of common buffer */
    uint32 size;                   /* Size of buffer in bytes */
    int32 frameBfrNum;            /* Number of YUV frame buffers */
    uint8 *intBufferPtr;           /* Pointer to internal memory */
    int32 intSize;                 /* Internal memory size in bytes */
} MMCodecBuffer;

/**
 * Error packet position structure.
 */
typedef struct {
    uint16 startPos;               /* Start position in bitstream (bytes) */
    uint16 endPos;                 /* End position in bitstream (bytes) */
} ERR_POS_T;

#define MAX_ERR_PKT_NUM 30          /* Maximum number of error packets */

/**
 * Frame buffer compression modes.
 */
enum FBC_MODE {
    FBC_NONE,
    IFBC = 1,
    AFBC = 2
};

/**
 * Meta data information structure (e.g., for HDR).
 */
typedef struct MetaDataInfo {
    uint32 metaDataSz;              /* Size of meta data */
    uint8 metaData[1024];           /* Meta data buffer */
} META_DATA_T;

/**
 * Static meta data for HDR (ST.2086).
 */
typedef struct StaticMetaDataInfo {
    uint16 redX;                    /* Red primary X coordinate */
    uint16 redY;                     /* Red primary Y coordinate */
    uint16 greenX;                   /* Green primary X coordinate */
    uint16 greenY;                   /* Green primary Y coordinate */
    uint16 blueX;                    /* Blue primary X coordinate */
    uint16 blueY;                    /* Blue primary Y coordinate */
    uint16 whiteX;                   /* White point X coordinate */
    uint16 whiteY;                   /* White point Y coordinate */
    uint16 maxLuminance;             /* Maximum luminance (in 0.0001 cd/m²) */
    uint16 minLuminance;             /* Minimum luminance (in 0.0001 cd/m²) */
    uint16 maxCll;                   /* Maximum content light level */
    uint16 maxFall;                  /* Maximum frame-average light level */
} STATIC_META_DATA_T;

/**
 * Scaler crop parameters.
 */
typedef struct SCALER_CROP_T {
    int32 rowOut;                    /* Output row offset */
    int32 colOut;                    /* Output column offset */
    int32 colStart;                  /* Starting column for cropping */
    int32 rowStart;                  /* Starting row for cropping */
    int32 cropW;                     /* Cropped width */
    int32 cropH;                     /* Cropped height */
} SCALER_CROP;

/**
 * Decoder input structure for one frame.
 */
typedef struct {
    uint8 *pStream;                  /* Pointer to compressed bitstream */
    unsigned long pStreamPhy;        /* Physical address of bitstream */
    uint32 dataLength;               /* Length of bitstream in bytes */
    int32 beLastFrame;                /* 1 if this is the last frame, 0 otherwise */
    int32 expectedIVOP;               /* Seek for I-VOP (used for random access) */
    int32 pts;                        /* Presentation timestamp */
    int32 beDisplayed;                 /* 1 if frame should be displayed, 0 otherwise */
    int32 errPktNum;                   /* Number of error packets in this frame */
    ERR_POS_T errPktPos[MAX_ERR_PKT_NUM]; /* Positions of error packets */
    uint8 fbcMode;                     /* Frame buffer compression mode (see FBC_MODE) */
    uint8 ditherEn;                    /* Dithering enable flag */
    uint8 scalerEn;                    /* Scaler enable flag */
    SCALER_CROP sclCrp;                 /* Scaler crop parameters */
} MMDecInput;

/**
 * Decoder output structure for one frame.
 */
typedef struct {
    uint8 *pOutFrameY;                /* Pointer to Y plane of decoded picture */
    uint8 *pOutFrameU;                /* Pointer to U plane of decoded picture */
    uint8 *pOutFrameV;                /* Pointer to V plane of decoded picture */
    uint32 frameWidth;                 /* Width of decoded frame */
    uint32 frameHeight;                /* Height of decoded frame */
    int32 isTransposed;                /* 1 if frame is transposed, 0 otherwise (always 0 for 8800S4) */
    int32 pts;                         /* Presentation timestamp */
    int32 frameEffective;               /* Frame effective flag */
    int32 errMBNum;                     /* Number of erroneous macroblocks */
    void *pBufferHeader;                /* Pointer to buffer header */
    int requestNewBuffer;               /* Flag indicating need for new buffer */
    int32 picId;                        /* Picture ID */
    BOOLEAN hasPic;                     /* Picture is present */
} MMDecOutput;

/**
 * Codec buffer type enumeration.
 */
typedef enum {
    INTER_MEM = 0,      /* Internal memory (SW only) */
    HW_NO_CACHABLE,     /* Physical continuous, non-cachable (HW access) */
    HW_CACHABLE,        /* Physical continuous, cachable (SW write, HW read) */
    SW_CACHABLE,        /* SW only, cachable */
    MAX_MEM_TYPE
} CODEC_BUF_TYPE;

/**
 * Cropping parameters structure.
 */
typedef struct __attribute__((__packed__)) {
    uint32 cropLeftOffset;            /* Left offset for cropping */
    uint32 cropOutWidth;              /* Output width after cropping */
    uint32 cropTopOffset;             /* Top offset for cropping */
    uint32 cropOutHeight;             /* Output height after cropping */
} CropParams;

/**
 * H.265 decoder information structure.
 */
typedef struct __attribute__((__packed__)) {
    uint32 profile;                   /* Profile from SPS */
    uint32 picWidth;                   /* Picture width */
    uint32 picHeight;                  /* Picture height */
    uint32 videoRange;                 /* Video range (0-255 or 16-235) */
    uint32 matrixCoefficients;         /* Matrix coefficients for color conversion */
    uint32 parWidth;                   /* Picture aspect ratio width */
    uint32 parHeight;                  /* Picture aspect ratio height */
    uint32 croppingFlag;                /* Cropping enabled flag */
    CropParams cropParams;              /* Cropping parameters */
    uint32 numFrames;                   /* Number of frames in the sequence */
    uint8 colorPrimaries;               /* Color primaries (u(8)) */
    uint8 transfer;                     /* Transfer characteristics (u(8)) */
    uint8 bindedBuffNum;                 /* Number of bound buffers */
    uint8 high10En;                      /* 0: 8-bit, 1: 10-bit */
    uint8 ctbSize;                       /* CTB size (Coding Tree Block) */
    STATIC_META_DATA_T st2086;           /* Static HDR metadata (ST.2086) */
    uint8 videoSignalTypePresent;        /* Video signal type present flag */
} H265SwDecInfo;

/**
 * Backup buffer structure (for DPB or reference management).
 */
typedef struct BkupBufT {
    int fd;                             /* File descriptor */
    int size;                           /* Buffer size */
    uint8 *imgY;                         /* Pointer to Y plane (virtual) */
    uint8 *imgU;                         /* Pointer to U plane (virtual) */
    unsigned long imgYAddr;              /* Y plane address (physical?) */
    unsigned long imgUAddr;              /* U plane address (physical?) */
    uint8 isUsed;                        /* Usage flag */
} BKUP_BUF;

/* Callback function types */
typedef int (*FunctionTypeBufCb)(void *userdata, void *pHeader);
typedef int (*FunctionTypeMallocCb)(void *aUserData, unsigned int sizeExtra);
typedef int (*FunctionTypeCtuinfoMallocCb)(void *aUserData, unsigned int sizeMbinfo, unsigned long *pPhyAddr);
typedef int (*FunctionTypeBkMallocCb)(void *aUserData, BKUP_BUF *buffer, unsigned int ySize, int cnt);

/**
 * Application handle structure.
 * This structure must be allocated and maintained by the user.
 */
typedef struct TagHevcHandle {
    void *videoDecoderData;            /* Internal decoder data (must be NULL initially) */
#ifdef PV_MEMORY_POOL
    int32 size;
#endif
    void *userdata;                     /* User data for callbacks */
    FunctionTypeBufCb vspBindcb;        /* Callback for binding VSP buffers */
    FunctionTypeBufCb vspUnbindcb;      /* Callback for unbinding VSP buffers */
    FunctionTypeMallocCb vspExtmemcb;   /* Callback for external memory allocation */
    FunctionTypeCtuinfoMallocCb vspCtuinfomemcb; /* Callback for CTU info memory allocation */
    FunctionTypeBkMallocCb vspBkmemcb;  /* Callback for backup buffer allocation */
} HEVCHandle;
/**----------------------------------------------------------------------------*
**                           Function Prototypes                              **
**----------------------------------------------------------------------------*/
// Description:   Parses the NAL unit type and reference IDC from a bitstream.
//   hevcHandle  - Handle to the decoder instance.
//   bitstream   - Pointer to the bitstream data.
//   size        - Size of the bitstream in bytes.
//   nalType     - Output pointer to store the NAL unit type.
//   nalRefIdc   - Output pointer to store the NAL reference IDC.
// Returns:       MMDecRet status code.
MMDecRet H265DecGetNALType(HEVCHandle *hevcHandle, uint8 *bitstream, int size,
                           int *nalType, int *nalRefIdc);

// Description:   Retrieves decoder information from the SPS/PPS.
//   hevcHandle  - Handle to the decoder instance.
//   pDecInfo    - Pointer to H265SwDecInfo structure to be filled.
// Returns:       MMDecRet status code.
MMDecRet H265DecGetInfo(HEVCHandle *hevcHandle, H265SwDecInfo *pDecInfo);
// Description:   Retrieves the decoder capabilities.
//   hevcHandle  - Handle to the decoder instance.
//   capability  - Pointer to MMDecCapability structure to be filled.
// Returns:       MMDecRet status code.
MMDecRet H265GetCodecCapability(HEVCHandle *hevcHandle, MMDecCapability *capability);
// Description:   Sets decoder parameters based on video format.
//   hevcHandle   - Handle to the decoder instance.
//   pVideoFormat - Pointer to MMDecVideoFormat structure.
// Returns:       MMDecRet status code.
MMDecRet H265DecSetParameter(HEVCHandle *hevcHandle, MMDecVideoFormat *pVideoFormat);
// Description:   Initializes the H.265 decoder.
//   hevcHandle   - Handle to the decoder instance.
//   pBuffer      - Pointer to MMCodecBuffer structure for internal memory.
//   pVideoFormat - Pointer to MMDecVideoFormat structure.
//   pPriority    - Pointer to MMDecCodecPriority structure (may be NULL).
// Returns:       MMDecRet status code.
MMDecRet H265DecInit(HEVCHandle *hevcHandle, MMCodecBuffer *pBuffer,
                     MMDecVideoFormat *pVideoFormat, MMDecCodecPriority *pPriority);
// Description:   Initializes decoder memory buffers.
//   hevcHandle   - Handle to the decoder instance.
//   pBuffer      - Pointer to MMCodecBuffer structure for internal memory.
// Returns:       MMDecRet status code.
MMDecRet H265DecMemInit(HEVCHandle *hevcHandle, MMCodecBuffer *pBuffer);
// Description:   Decodes one video frame (VOP).
//   hevcHandle   - Handle to the decoder instance.
//   pInput       - Pointer to MMDecInput structure with input data.
//   pOutput      - Pointer to MMDecOutput structure for decoded output.
// Returns:       MMDecRet status code.
MMDecRet H265DecDecode(HEVCHandle *hevcHandle, MMDecInput *pInput, MMDecOutput *pOutput);
// Description:   Releases the H.265 decoder and frees resources.
//   hevcHandle   - Handle to the decoder instance.
// Returns:       MMDecRet status code.
MMDecRet H265DecRelease(HEVCHandle *hevcHandle);
// Description:   Releases reference buffers held by the decoder.
//   hevcHandle   - Handle to the decoder instance.
void H265DecReleaseRefBuffers(HEVCHandle *hevcHandle);
// Description:   Retrieves the last decoded frame from the DSP.
//   hevcHandle   - Handle to the decoder instance.
//   pOutput      - Output pointer to the frame buffer.
//   pIcId        - Output picture ID.
// Returns:       MMDecRet status code.
MMDecRet H265DecGetLastDspFrm(HEVCHandle *hevcHandle, void **pOutput, int32 *pIcId);
// Description:   Sets the current reconstructed picture for DPB management.
//   hevcHandle    - Handle to the decoder instance.
//   pFrameY       - Pointer to Y plane (virtual address).
//   pFrameYPhy   - Pointer to Y plane (physical address).
//   st2094        - Pointer to meta data (e.g., HDR ST.2094).
//   pBufferHeader - Buffer header pointer.
//   picId         - Picture ID.
//   fd            - File descriptor for buffer (if applicable).
void H265DecSetCurRecPic(HEVCHandle *hevcHandle, uint8 *pFrameY,
                         uint8 *pFrameYPhy, META_DATA_T *st2094,
                         void *pBufferHeader, int32 picId, int32 fd);
// Description:   Retrieves I/O virtual address (IOVA) for a buffer.
//   hevcHandle   - Handle to the decoder instance.
//   fd           - File descriptor.
//   iova         - Output pointer for IOVA.
//   size         - Output pointer for size.
//   needLock     - Flag indicating whether lock is needed.
// Returns:       0 on success, negative error code on failure.
int H265DecGetIova(HEVCHandle *hevcHandle, int fd, unsigned long *iova,
                   size_t *size, BOOLEAN needLock);
// Description:   Frees I/O virtual address (IOVA) for a buffer.
//   hevcHandle   - Handle to the decoder instance.
//   iova         - IOVA to free.
//   size         - Size of mapped region.
//   needLock     - Flag indicating whether lock is needed.
// Returns:       0 on success, negative error code on failure.
int H265DecFreeIova(HEVCHandle *hevcHandle, unsigned long iova, size_t size,
                    BOOLEAN needLock);

#define H265_DEC_GET_IOVA H265DecGetIova
#define H265_DEC_FREE_IOVA H265DecFreeIova
// Description:   Retrieves the current IOMMU status.
//   hevcHandle   - Handle to the decoder instance.
// Returns:       IOMMU status (implementation-defined).
int H265DecGetIommuStatus(HEVCHandle *hevcHandle);
/* Function pointer types for dynamic linking */
typedef MMDecRet (*FT_H265DecGetNALType)(HEVCHandle *hevcHandle, uint8 *bitstream,
                                         int size, int *nalType, int *nalRefIdc);
typedef void (*FT_H265GetBufferDimensions)(HEVCHandle *hevcHandle,
                                           int32 *alignedWidth, int32 *alignedHeight);
typedef MMDecRet (*FT_H265DecInit)(HEVCHandle *hevcHandle, MMCodecBuffer *pBuffer,
                                   MMDecVideoFormat *pVideoFormat, MMDecCodecPriority *pPriority);
typedef MMDecRet (*FT_H265DecGetInfo)(HEVCHandle *hevcHandle, H265SwDecInfo *pDecInfo);
typedef MMDecRet (*FT_H265GetCodecCapability)(HEVCHandle *hevcHandle,
                                              MMDecCapability *capability);
typedef MMDecRet (*FT_H265DecMemInit)(HEVCHandle *hevcHandle, MMCodecBuffer *pBuffer);
typedef MMDecRet (*FT_H265DecDecode)(HEVCHandle *hevcHandle, MMDecInput *pInput,
                                     MMDecOutput *pOutput);
typedef MMDecRet (*FT_H265DecRelease)(HEVCHandle *hevcHandle);
typedef void (*FtH265DecSetCurRecPic)(HEVCHandle *hevcHandle, uint8 *pFrameY,
                                      uint8 *pFrameYPhy, META_DATA_T *st2094,
                                      void *pBufferHeader, int32 picId, int32 fd);
typedef MMDecRet (*FtH265DecGetLastDspFrm)(HEVCHandle *hevcHandle,
                                           void **pOutput, int32 *pIcId);
typedef void (*FtH265DecReleaseRefBuffers)(HEVCHandle *hevcHandle);
typedef MMDecRet (*FT_H265DecSetparam)(HEVCHandle *hevcHandle,
                                       MMDecVideoFormat *pVideoFormat);
typedef int (*FtH265DecGetIova)(HEVCHandle *hevcHandle, int fd,
                                unsigned long *iova, size_t *size, BOOLEAN needLock);
typedef int (*FtH265DecFreeIova)(HEVCHandle *hevcHandle,
                                 unsigned long iova, size_t size, BOOLEAN needLock);
typedef int (*FtH265DecGetIommuStatus)(HEVCHandle *hevcHandle);
typedef int (*FT_H265DecNeedToSwitchToSwDecoder)(HEVCHandle *hevcHandle);

}  // namespace OMX
}  // namespace OHOS
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif  // H265_DEC_H_
// End