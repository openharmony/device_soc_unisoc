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
 ** File Name:    h264dec.h                                                   *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         3/15/2007                                                   *
 ** Copyright:    2019 Unisoc, Incorporated. All Rights Reserved.             *
 ** Description:  Define data structures for H.264 video decoder.             *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 3/15/2007     Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
#ifndef H264_DEC_H_
#define H264_DEC_H_
#include <cstddef>

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **---------------------------------------------------------------------------*/
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
 * This enumeration is for profiles. The value follows the profileIdc in
 * sequence parameter set rbsp. See Annex A of H.264 specification.
 */
typedef enum {
    AVC_BASELINE = 66,
    AVC_MAIN = 77,
    AVC_EXTENDED = 88,
    AVC_HIGH = 100,
    AVC_HIGH10 = 110,
    AVC_HIGH422 = 122,
    AVC_HIGH444 = 144
} AVCProfile;

/**
 * This enumeration is for levels. The value follows the level_idc in sequence
 * parameter set rbsp. See Annex A of H.264 specification.
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
} AVCLevel;

/**
 * Decoder capability structure.
 */
typedef struct {
    AVCProfile profile;      /* Supported profile */
    AVCLevel level;          /* Supported level */
    uint32 maxWidth;         /* Maximum supported width */
    uint32 maxHeight;        /* Maximum supported height */
    uint8 support1080i;      /* Support for 1080i interlaced content */
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
    RV9
} VIDEO_STANDARD_E;

/**
 * YUV format enumeration.
 */
typedef enum {
    YUV420P_YU12 = 0,   /* Planar YUV 4:2:0, Y plane followed by U then V */
    YUV420P_YV12 = 1,   /* Planar YUV 4:2:0, Y plane followed by V then U */
    YUV420SP_NV12 = 2,  /* Semi-planar, Y plane + interleaved UV (U first) */
    YUV420SP_NV21 = 3,  /* Semi-planar, Y plane + interleaved VU (V first) */
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
    int32 thumbnail;               /* Thumbnail mode flag */
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
    int32 frameBufferNum;          /* Number of YUV frame buffers */
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
 * Scaler crop parameters.
 */
typedef struct SCALER_CROP_T {
    int32 rowOut;                   /* Output row offset */
    int32 colOut;                   /* Output column offset */
    int32 colStart;                  /* Starting column for cropping */
    int32 rowStart;                  /* Starting row for cropping */
    int32 cropWidth;                 /* Cropped width */
    int32 cropHeight;                /* Cropped height */
} SCALER_CROP;

/**
 * Decoder input structure for one frame.
 */
typedef struct {
    uint8 *pStream;                  /* Pointer to compressed bitstream */
    unsigned long pStreamPhy;        /* Physical address of bitstream */
    uint32 dataLength;               /* Length of bitstream in bytes */
    int32 beLastFrame;               /* 1 if this is the last frame, 0 otherwise */
    int32 expectedIVOP;              /* Seek for I-VOP (used for random access) */
    uint64 nTimeStamp;               /* Timestamp (PTS or DTS) */
    int32 beDisplayed;                /* 1 if frame should be displayed, 0 otherwise */
    int32 errPktNum;                  /* Number of error packets in this frame */
    ERR_POS_T errPacketPos[MAX_ERR_PKT_NUM]; /* Positions of error packets */
    uint8 fbcMode;                    /* Frame buffer compression mode (see FBC_MODE) */
    uint8 ditherEnable;                /* Dithering enable flag */
    uint8 scalerEnable;                /* Scaler enable flag */
    SCALER_CROP scalerCrop;            /* Scaler crop parameters */
} MMDecInput;

#define MAX_REF_FRAME_NUMBER 16        /* Maximum number of reference frames */

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
    uint64 pts;                        /* Presentation timestamp */
    int32 frameEffective;               /* Frame effective flag */
    int32 errMBNum;                     /* Number of erroneous macroblocks */
    void *pBufferHeader;                /* Pointer to buffer header */
    int requestNewBuffer;               /* Flag indicating need for new buffer */
    int32 picId;                        /* Picture ID */
    BOOLEAN sawSPS;                     /* SPS was seen in this frame */
    BOOLEAN sawPPS;                     /* PPS was seen in this frame */
    BOOLEAN hasPic;                      /* Picture is present */
    int32 releaseBufferId[MAX_REF_FRAME_NUMBER]; /* Buffer IDs to release (not displayed) */
    int releaseBufferIdCount;            /* Number of buffer IDs to release */
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
 * Frame buffer compression modes.
 */
enum FBC_MODE {
    FBC_NONE,
    IFBC = 1,
    AFBC = 2
};

/**
 * Cropping parameters structure.
 */
struct CropParamsT {
    uint32 cropLeftOffset;            /* Left offset for cropping */
    uint32 cropOutWidth;              /* Output width after cropping */
    uint32 cropTopOffset;             /* Top offset for cropping */
    uint32 cropOutHeight;             /* Output height after cropping */
} __attribute__((__packed__));
typedef struct CropParamsT CropParams;

/**
 * H.264 decoder information structure.
 */
struct H264SwDecInfoT {
    uint32 profile;                   /* Profile from SPS */
    uint32 picWidth;                  /* Picture width */
    uint32 picHeight;                 /* Picture height */
    uint32 videoRange;                 /* Video range (0-255 or 16-235) */
    uint32 matrixCoefficients;         /* Matrix coefficients for color conversion */
    uint32 parWidth;                   /* Picture aspect ratio width */
    uint32 parHeight;                  /* Picture aspect ratio height */
    uint32 croppingFlag;                /* Cropping enabled flag */
    CropParams cropParams;              /* Cropping parameters */
    uint32 numRefFrames;                /* Number of reference frames */
    uint32 hasBFrames;                  /* Presence of B frames */
    int32 needDeinterlace;              /* Deinterlacing required flag */
    uint8 colorPrimaries;               /* Color primaries (u(8)) */
    uint8 transfer;                      /* Transfer characteristics (u(8)) */
    uint8 frameMBOnly;                   /* Frame contains only macroblocks (no fields) */
    uint8 bindedBuffNum;                  /* Number of bound buffers */
    uint8 high10En;                       /* High 10 profile enabled */
} __attribute__((__packed__));
typedef struct H264SwDecInfoT H264SwDecInfo;

/* Callback function types */
typedef int (*FunctionTypeBufCb)(void *userdata, void *pHeader);
typedef int (*FunctionTypeMallocCb)(void *aUserData, unsigned int sizeExtra);
typedef int (*FunctionTypeMbinfoMallocCb)(void *aUserData,
                                          unsigned int sizeMbinfo,
                                          unsigned long *pPhyAddr);
typedef int (*FunctionTypeClearDpbCb)(void *aUserData);

/**
 * Application handle structure.
 * This structure must be allocated and maintained by the user.
 */
typedef struct TagAvcHandle {
    void *videoDecoderData;            /* Internal decoder data (must be NULL initially) */
#ifdef PV_MEMORY_POOL
    int32 size;
#endif
    void *userdata;                     /* User data for callbacks */
    FunctionTypeBufCb vspBindcb;        /* Callback for binding VSP buffers */
    FunctionTypeBufCb vspUnbindcb;      /* Callback for unbinding VSP buffers */
    FunctionTypeMallocCb vspExtmemcb;   /* Callback for external memory allocation */
    FunctionTypeMbinfoMallocCb vspMbinfomemcb; /* Callback for macroblock info memory allocation */
} AVCHandle;

/**----------------------------------------------------------------------------*
**                           Function Prototypes                              **
**----------------------------------------------------------------------------*/
// Description:   Parses the NAL unit type and reference IDC from a bitstream.
//   avcHandle   - Handle to the decoder instance.
//   bitstream   - Pointer to the bitstream data.
//   size        - Size of the bitstream in bytes.
//   nalType     - Output pointer to store the NAL unit type.
//   nalRefIdc   - Output pointer to store the NAL reference IDC.
// Returns:       MMDecRet status code.
MMDecRet H264DecGetNALType(AVCHandle *avcHandle, uint8 *bitstream, int size,
                           int *nalType, int *nalRefIdc);

// Description:   Retrieves decoder information from the SPS/PPS.
//   avcHandle   - Handle to the decoder instance.
//   pDecInfo    - Pointer to H264SwDecInfo structure to be filled.
// Returns:       MMDecRet status code.
MMDecRet H264DecGetInfo(AVCHandle *avcHandle, H264SwDecInfo *pDecInfo);
// Description:   Retrieves the decoder capabilities.
//   avcHandle   - Handle to the decoder instance.
//   capability  - Pointer to MMDecCapability structure to be filled.
// Returns:       MMDecRet status code.
MMDecRet H264GetCodecCapability(AVCHandle *avcHandle,
                                MMDecCapability *capability);

// Description:   Sets decoder parameters based on video format.
//   avcHandle   - Handle to the decoder instance.
//   pVideoFormat- Pointer to MMDecVideoFormat structure.
// Returns:       MMDecRet status code.
MMDecRet H264DecSetParameter(AVCHandle *avcHandle,
                             MMDecVideoFormat *pVideoFormat);
// Description:   Initializes the H.264 decoder.
//   avcHandle   - Handle to the decoder instance.
//   pBuffer     - Pointer to MMCodecBuffer structure for internal memory.
//   pVideoFormat- Pointer to MMDecVideoFormat structure.
//   pPriority   - Pointer to MMDecCodecPriority structure (may be NULL).
// Returns:       MMDecRet status code.
MMDecRet H264DecInit(AVCHandle *avcHandle, MMCodecBuffer *pBuffer,
                     MMDecVideoFormat *pVideoFormat,
                     MMDecCodecPriority *pPriority);
// Description:   Initializes decoder memory buffers.
//   avcHandle   - Handle to the decoder instance.
//   pBuffer     - Pointer to MMCodecBuffer structure for internal memory.
// Returns:       MMDecRet status code.
MMDecRet H264DecMemInit(AVCHandle *avcHandle, MMCodecBuffer *pBuffer);
// Description:   Decodes one video frame (VOP).
//   avcHandle   - Handle to the decoder instance.
//   pInput      - Pointer to MMDecInput structure with input data.
//   pOutput     - Pointer to MMDecOutput structure for decoded output.
// Returns:       MMDecRet status code.
MMDecRet H264DecDecode(AVCHandle *avcHandle, MMDecInput *pInput,
                       MMDecOutput *pOutput);
// Description:   Releases the H.264 decoder and frees resources.
//   avcHandle   - Handle to the decoder instance.
// Returns:       MMDecRet status code.
MMDecRet H264DecRelease(AVCHandle *avcHandle);
// Description:   Releases reference buffers held by the decoder.
//   avcHandle   - Handle to the decoder instance.
void H264DecReleaseRefBuffers(AVCHandle *avcHandle);
// Description:   Retrieves the last decoded frame from the DSP.
//   avcHandle   - Handle to the decoder instance.
//   pOutput     - Output pointer to the frame buffer.
//   picId       - Output picture ID.
//   pTs         - Output timestamp.
// Returns:       MMDecRet status code.
MMDecRet H264DecGetLastDspFrm(AVCHandle *avcHandle, void **pOutput,
                              int32 *picId, uint64 *pts);
// Description:   Sets the current reconstructed picture for DPB management.
//   avcHandle   - Handle to the decoder instance.
//   pFrameY     - Pointer to Y plane (virtual address).
//   pFrameYPhy - Pointer to Y plane (physical address).
//   pBufferHeader - Buffer header pointer.
//   picId       - Picture ID.
//   fd          - File descriptor for buffer (if applicable).
void H264DecSetCurRecPic(AVCHandle *avcHandle, uint8 *pFrameY,
                         uint8 *pFrameYPhy, void *pBufferHeader,
                         int32 picId, int32 fd);
// Description:   Retrieves I/O virtual address (IOVA) for a buffer.
//   avcHandle   - Handle to the decoder instance.
//   fd          - File descriptor.
//   iova        - Output pointer for IOVA.
//   size        - Output pointer for size.
//   needLock    - Flag indicating whether lock is needed.
// Returns:       0 on success, negative error code on failure.
int H264DecGetIova(AVCHandle *avcHandle, int fd, unsigned long *iova,
                   size_t *size, BOOLEAN needLock);
// Description:   Frees I/O virtual address (IOVA) for a buffer.
//   avcHandle   - Handle to the decoder instance.
//   iova        - IOVA to free.
//   size        - Size of mapped region.
//   needLock    - Flag indicating whether lock is needed.
// Returns:       0 on success, negative error code on failure.
int H264DecFreeIova(AVCHandle *avcHandle, unsigned long iova, size_t size,
                    BOOLEAN needLock);

#define H264_DEC_GET_IOVA H264DecGetIova
#define H264_DEC_FREE_IOVA H264DecFreeIova
// Description:   Retrieves the current IOMMU status.
//   avcHandle   - Handle to the decoder instance.
// Returns:       IOMMU status (implementation-defined).
int H264DecGetIommuStatus(AVCHandle *avcHandle);
// Description:   Initializes internal structures for a new sequence.
//   avcHandle   - Handle to the decoder instance.
// Returns:       MMDecRet status code.
MMDecRet H264DecInitStructForNewSeq(AVCHandle *avcHandle);

/* Function pointer types for dynamic linking */
typedef MMDecRet (*FT_H264DecGetNALType)(AVCHandle *avcHandle, uint8 *bitstream,
                                         int size, int *nalType,
                                         int *nalRefIdc);
typedef void (*FT_H264GetBufferDimensions)(AVCHandle *avcHandle,
                                           int32 *alignedWidth,
                                           int32 *alignedHeight);
typedef MMDecRet (*FT_H264DecInit)(AVCHandle *avcHandle, MMCodecBuffer *pBuffer,
                                   MMDecVideoFormat *pVideoFormat,
                                   MMDecCodecPriority *pPriority);
typedef MMDecRet (*FT_H264DecGetInfo)(AVCHandle *avcHandle,
                                      H264SwDecInfo *pDecInfo);
typedef MMDecRet (*FT_H264GetCodecCapability)(AVCHandle *avcHandle,
                                              MMDecCapability *capability);
typedef MMDecRet (*FT_H264DecMemInit)(AVCHandle *avcHandle,
                                      MMCodecBuffer *pBuffer);
typedef MMDecRet (*FT_H264DecDecode)(AVCHandle *avcHandle, MMDecInput *pInput,
                                     MMDecOutput *pOutput);
typedef MMDecRet (*FT_H264DecRelease)(AVCHandle *avcHandle);
typedef void (*FtH264DecSetCurRecPic)(AVCHandle *avcHandle, uint8 *pFrameY,
                                      uint8 *pFrameYPhy, void *pBufferHeader,
                                      int32 picId, int32 fd);
typedef MMDecRet (*FtH264DecGetLastDspFrm)(AVCHandle *avcHandle,
                                           void **pOutput, int32 *picId,
                                           uint64 *pts);
typedef void (*FtH264DecReleaseRefBuffers)(AVCHandle *avcHandle);
typedef MMDecRet (*FT_H264DecSetparam)(AVCHandle *avcHandle,
                                       MMDecVideoFormat *pVideoFormat);
typedef int (*FtH264DecGetIova)(AVCHandle *avcHandle, int fd,
                                unsigned long *iova, size_t *size,
                                BOOLEAN needLock);
typedef int (*FtH264DecFreeIova)(AVCHandle *avcHandle, unsigned long iova,
                                 size_t size, BOOLEAN needLock);
typedef int (*FtH264DecGetIommuStatus)(AVCHandle *avcHandle);
typedef MMDecRet (*FtH264DecInitStructForNewSeq)(AVCHandle *avcHandle);
typedef int (*FT_H264DecNeedToSwitchToSwDecoder)(AVCHandle *mp4Handle);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
}  // namespace OMX
}  // namespace OHOS

#ifdef __cplusplus
}
#endif

/**---------------------------------------------------------------------------*/
#endif  // H264_DEC_H_
// End