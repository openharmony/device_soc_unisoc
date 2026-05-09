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
// VSP deinterlace driver interface definitions.
#ifndef VSP_DRV_S_C8830_H_
#define VSP_DRV_S_C8830_H_

// Dependencies.
#include "vsp_deint.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cerrno>
#include <unistd.h>
#include <ctime>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

// C ABI exports.
#ifdef __cplusplus
extern "C"
{
#endif

// Typed utilities.
constexpr uint32 VspBitMask(uint32 bit)
{
    return static_cast<uint32>(1U << bit);
}

constexpr uint32 K_VSP_ERROR_BIT_STREAM = 0;
constexpr uint32 K_VSP_ERROR_BIT_MEMORY = 1;
constexpr uint32 K_VSP_ERROR_BIT_REF_FRAME = 2;
constexpr uint32 K_VSP_ERROR_BIT_HARDWARE = 3;
constexpr uint32 K_VSP_ERROR_BIT_FORMAT = 4;
constexpr uint32 K_VSP_ERROR_BIT_SPS_PPS = 5;

// Forward declaration for ioctl payload typing.
typedef struct VspIommuMapData VspIommuMapDataT;

// Device and ioctl constants.
constexpr const char SPRD_VSP_DRIVER[] = "/dev/sprd_vsp";
constexpr uint32 SPRD_VSP_MAP_SIZE = 0xA000;  // 40kbyte
constexpr uint32 SPRD_MAX_VSP_FREQ_LEVEL = 3;
constexpr uint32 SPRD_VSP_FREQ_BASE_LEVEL = 0;
constexpr char SPRD_VSP_IOCTL_MAGIC = 'm';

constexpr unsigned long VSP_CONFIG_FREQ = _IOW(SPRD_VSP_IOCTL_MAGIC, 1, unsigned int);
constexpr unsigned long VSP_GET_FREQ = _IOR(SPRD_VSP_IOCTL_MAGIC, 2, unsigned int);
constexpr unsigned long VSP_ENABLE = _IO(SPRD_VSP_IOCTL_MAGIC, 3);
constexpr unsigned long VSP_DISABLE = _IO(SPRD_VSP_IOCTL_MAGIC, 4);
constexpr unsigned long VSP_ACQUAIRE = _IO(SPRD_VSP_IOCTL_MAGIC, 5);
constexpr unsigned long VSP_RELEASE = _IO(SPRD_VSP_IOCTL_MAGIC, 6);
constexpr unsigned long VSP_COMPLETE = _IO(SPRD_VSP_IOCTL_MAGIC, 7);
constexpr unsigned long VSP_RESET = _IO(SPRD_VSP_IOCTL_MAGIC, 8);
constexpr unsigned long VSP_HW_INFO = _IO(SPRD_VSP_IOCTL_MAGIC, 9);
constexpr unsigned long VSP_VERSION = _IO(SPRD_VSP_IOCTL_MAGIC, 10);
constexpr unsigned int K_VSP_GET_IOVA_CMD_ID = 11;
constexpr unsigned int K_VSP_FREE_IOVA_CMD_ID = 12;
constexpr unsigned long VSP_GET_IOMMU_STATUS = _IO(SPRD_VSP_IOCTL_MAGIC, 13);
constexpr unsigned long VSP_SET_CODEC_ID = _IOW(SPRD_VSP_IOCTL_MAGIC, 14, unsigned int);
constexpr unsigned long VSP_GET_CODEC_COUNTER = _IOWR(SPRD_VSP_IOCTL_MAGIC, 15, unsigned int);
constexpr unsigned long VSP_SET_SCENE = _IO(SPRD_VSP_IOCTL_MAGIC, 16);
constexpr unsigned long VSP_GET_SCENE = _IO(SPRD_VSP_IOCTL_MAGIC, 17);
constexpr uint32 K_REG_POLL_TIMEOUT_CLK = 0xffff;
constexpr uint32 K_FRAME_POLL_TIMEOUT_CLK = 0x7fffff;
constexpr uint32 TIME_OUT_CLK = K_REG_POLL_TIMEOUT_CLK;
constexpr uint32 TIME_OUT_CLK_FRAME = K_FRAME_POLL_TIMEOUT_CLK;
/* -----------------------------------------------------------------------
** VSP versions
** ----------------------------------------------------------------------- */
typedef enum VspCodecInstanceTag : uint32 {
    K_VSP_CODEC_H264_DECODER = 0,
    K_VSP_CODEC_H264_ENCODER,
    K_VSP_CODEC_MPEG4_DECODER,
    K_VSP_CODEC_MPEG4_ENCODER,
    K_VSP_CODEC_H265_DECODER,
    K_VSP_CODEC_H265_ENCODER,
    K_VSP_CODEC_VPX_DECODER,
    K_VSP_CODEC_ENCODER,
    K_VSP_CODEC_INSTANCE_COUNT,
} VSP_CODEC_INSTANCE_E;

typedef enum VspSceneModeTag : uint32 {
    K_VSP_SCENE_NORMAL = 0,
    K_VSP_SCENE_VOLTE = 1,
    K_VSP_SCENE_WFD = 2,
    K_VSP_SCENE_MODE_COUNT,
} VSP_SCENE_MODE_E;

typedef enum VspVersionTag : uint32 {
    K_VSP_VERSION_SHARK = 0,
    K_VSP_VERSION_DOLPHIN = 1,
    K_VSP_VERSION_TSHARK = 2,
    K_VSP_VERSION_SHARK_L = 3,
    K_VSP_VERSION_PIKE = 4,
    K_VSP_VERSION_PIKEL = 5,
    K_VSP_VERSION_SHARK_L64 = 6,
    K_VSP_VERSION_SHARK_LT8 = 7,
    K_VSP_VERSION_WHALE = 8,
    K_VSP_VERSION_WHALE2 = 9,
    K_VSP_VERSION_I_WHALE2 = 10,
    K_VSP_VERSION_I_SHARK_L2 = 11,
    K_VSP_VERSION_SHARK_L2 = 12,
    K_VSP_VERSION_COUNT,
} VSP_VERSION_E;
/* -----------------------------------------------------------------------
** Standard Types
** ----------------------------------------------------------------------- */
enum : uint32 {
    K_STREAM_ID_H263 = 0,
    K_STREAM_ID_MPEG4 = 1,
    K_STREAM_ID_VP8 = 2,
    K_STREAM_ID_FLV_H263 = 3,
    K_STREAM_ID_H264 = 4,
    K_STREAM_ID_REAL8 = 5,
    K_STREAM_ID_REAL9 = 6,
    K_STREAM_ID_H265 = 7,
};
enum : uint32 {
    STREAM_ID_H263 = K_STREAM_ID_H263,
    STREAM_ID_MPEG4 = K_STREAM_ID_MPEG4,
    STREAM_ID_VP8 = K_STREAM_ID_VP8,
    STREAM_ID_FLVH263 = K_STREAM_ID_FLV_H263,
    STREAM_ID_H264 = K_STREAM_ID_H264,
    STREAM_ID_REAL8 = K_STREAM_ID_REAL8,
    STREAM_ID_REAL9 = K_STREAM_ID_REAL9,
    STREAM_ID_H265 = K_STREAM_ID_H265,
};
constexpr uint32 K_SHARED_RAM_SIZE_BYTES = 0x100;
constexpr uint32 K_CODEC_MODE_ENCODE = 1;
constexpr uint32 K_CODEC_MODE_DECODE = 0;
constexpr uint32 ENC = K_CODEC_MODE_ENCODE;
constexpr uint32 DEC = K_CODEC_MODE_DECODE;
/* -----------------------------------------------------------------------
** Control Register Address on ARM
** ----------------------------------------------------------------------- */
constexpr uint32 K_GLB_REG_SIZE_BYTES = 0x200;
constexpr uint32 K_PPA_SLICE_INFO_SIZE_BYTES = 0x200;
constexpr uint32 K_DCT_IQW_TABLE_SIZE_BYTES = 0x400;
constexpr uint32 K_FRAME_ADDR_TABLE_SIZE_BYTES = 0x200;
constexpr uint32 K_VLC_TABLE0_SIZE_BYTES = 2624;
constexpr uint32 K_VLC_TABLE1_SIZE_BYTES = 1728;
constexpr uint32 K_BSM_CTRL_REG_SIZE_BYTES = 0x1000;
constexpr uint32 SHARE_RAM_SIZE = K_SHARED_RAM_SIZE_BYTES;
constexpr uint32 GLB_REG_SIZE = K_GLB_REG_SIZE_BYTES;
constexpr uint32 PPA_SLICE_INFO_SIZE = K_PPA_SLICE_INFO_SIZE_BYTES;
constexpr uint32 DCT_IQW_TABLE_SIZE = K_DCT_IQW_TABLE_SIZE_BYTES;
constexpr uint32 FRAME_ADDR_TABLE_SIZE = K_FRAME_ADDR_TABLE_SIZE_BYTES;
constexpr uint32 VLC_TABLE0_SIZE = K_VLC_TABLE0_SIZE_BYTES;
constexpr uint32 VLC_TABLE1_SIZE = K_VLC_TABLE1_SIZE_BYTES;
constexpr uint32 BSM_CTRL_REG_SIZE = K_BSM_CTRL_REG_SIZE_BYTES;
constexpr uint32 VSP_REG_BASE_ADDR = 0x60900000;  // AHB
constexpr uint32 AHB_CTRL_BASE_ADDR = (VSP_REG_BASE_ADDR + 0x0);
constexpr uint32 SHARE_RAM_BASE_ADDR = (VSP_REG_BASE_ADDR + 0x200);
constexpr uint32 OR_BOOTROM_BASE_ADDR = (VSP_REG_BASE_ADDR + 0x400);
constexpr uint32 GLB_REG_BASE_ADDR = (VSP_REG_BASE_ADDR + 0x1000);
constexpr uint32 PPA_SLICE_INFO_BASE_ADDR = (VSP_REG_BASE_ADDR + 0x1200);
constexpr uint32 DCT_IQW_TABLE_BASE_ADDR = (VSP_REG_BASE_ADDR + 0x1400);
constexpr uint32 FRAME_ADDR_TABLE_BASE_ADDR = (VSP_REG_BASE_ADDR + 0x1800);
constexpr uint32 CABAC_CONTEXT_BASE_ADDR = (VSP_REG_BASE_ADDR + 0x1a00);
constexpr uint32 VLC_TABLE0_BASE_ADDR = (VSP_REG_BASE_ADDR + 0x2000);
constexpr uint32 VLC_TABLE1_BASE_ADDR = (VSP_REG_BASE_ADDR + 0x3000);
constexpr uint32 EIS_GRID_DATA_TABLE = (VSP_REG_BASE_ADDR + 0x7000);
constexpr uint32 BSM_CTRL_REG_BASE_ADDR = (VSP_REG_BASE_ADDR + 0x8000);
constexpr uint32 BSM1_CTRL_REG_BASE_ADDR = (VSP_REG_BASE_ADDR + 0x8100);
// AHB control register offsets.
constexpr int32 ARM_ACCESS_CTRL_OFF = 0x0;
constexpr int32 ARM_ACCESS_STATUS_OFF = 0x04;
constexpr int32 MCU_CTRL_SET_OFF = 0x08;
constexpr int32 ARM_INT_STS_OFF = 0x10;  // Interrupt from IMCU/OpenRISC side.
constexpr int32 ARM_INT_MASK_OFF = 0x14;
constexpr int32 ARM_INT_CLR_OFF = 0x18;
constexpr int32 ARM_INT_RAW_OFF = 0x1C;
constexpr int32 WB_ADDR_SET0_OFF = 0x20;
constexpr int32 WB_ADDR_SET1_OFF = 0x24;
///#define USE_INTERRUPT
typedef struct VspIommuMapData {
    int bufferFd;
    size_t bufferSize;
    unsigned long mappedIova;
}VspIommuMapDataT;
constexpr unsigned long VSP_GET_IOVA = _IOWR(SPRD_VSP_IOCTL_MAGIC, K_VSP_GET_IOVA_CMD_ID, VspIommuMapDataT);
constexpr unsigned long VSP_FREE_IOVA = _IOW(SPRD_VSP_IOCTL_MAGIC, K_VSP_FREE_IOVA_CMD_ID, VspIommuMapDataT);

typedef struct VspAhbmRegTag {
    // [2] globalRegOwner: 0 ARM controls GLB regs, 1 IMCU/accelerator side controls GLB regs.
    // [1] sharedRamOwner: 0 ARM accesses shared RAM, 1 IMCU accesses shared RAM.
    // [0] bootRamOwner: 0 ARM accesses boot RAM, 1 IMCU accesses boot RAM.
    volatile uint32 busAccessCtrl;
    // [1] sharedRamOwnerStatus: 0 ARM currently owns shared RAM, 1 IMCU owns shared RAM.
    // [0] bootRamOwnerStatus: 0 ARM currently owns boot RAM, 1 IMCU owns boot RAM.
    volatile uint32 busAccessStatus;
    // [1] imcuWakePulse: rising edge wakes IMCU/OpenRISC and triggers its interrupt.
    volatile uint32 imcuControl;
    // [0] imcuClockEnable: 1 enables IMCU/OpenRISC clock.
    volatile uint32 reserved0;
    // ARM-facing interrupt status, valid when IMCU controls the core.
    // [2] acceleratorInterrupt, [1] frameError, [0] imcuDone.
    volatile uint32 armInterruptStatus;
    volatile uint32 armInterruptMask;
    volatile uint32 armInterruptClear;
    volatile uint32 armInterruptRaw;
    // [28:0] instruction WB base address for OpenRISC fetch; dword aligned.
    // [28:0] data WB base address for OpenRISC data access; dword aligned.
    volatile uint32 wbInstructionBase;
    volatile uint32 wbDataBase;
} VSP_AHBM_REG_T;
// Global register offsets.
constexpr int32 VSP_INT_SYS_OFF = 0x0;  // Interrupt source from VSP core.
constexpr int32 VSP_INT_MASK_OFF = 0x04;
constexpr int32 VSP_INT_CLR_OFF = 0x08;
constexpr int32 VSP_INT_RAW_OFF = 0x0c;
constexpr int32 AXIM_ENDIAN_OFF = 0x10;
constexpr int32 AXIM_BURST_GAP_OFF = 0x14;
constexpr int32 AXIM_PAUSE_OFF = 0x18;
constexpr int32 AXIM_STS_OFF = 0x1c;
constexpr int32 VSP_MODE_OFF = 0x20;
constexpr int32 IMG_SIZE_OFF = 0x24;
constexpr int32 RAM_ACC_SEL_OFF = 0x28;
constexpr int32 VSP_INT_GEN_OFF = 0x2c;
constexpr int32 VSP_START_OFF = 0x30;
constexpr int32 VSP_SIZE_SET_OFF = 0x34;
constexpr int32 BSM0_FRM_ADDR_OFF = 0x60;
constexpr int32 BSM1_FRM_ADDR_OFF = 0x64;
constexpr int32 VSP_ADDRIDX0_OFF = 0x68;
constexpr int32 VSP_ADDRIDX1_OFF = 0x6c;
constexpr int32 VSP_ADDRIDX2_OFF = 0x70;
constexpr int32 VSP_ADDRIDX3_OFF = 0x74;
constexpr int32 VSP_ADDRIDX4_OFF = 0x78;
constexpr int32 VSP_ADDRIDX5_OFF = 0x7c;
constexpr int32 MCU_START_OFF = 0x80;  // Read-only IMCU start state.
constexpr int32 VSP_CFG11_OFF = 0x8c;
constexpr int32 VSP_DBG_STS0_OFF = 0x100;  // mbx mby
#ifndef VPP_SHARKL3_DEINT
constexpr int32 VSP_CFG0_OFF = 0x3c;
constexpr int32 VSP_CFG1_OFF = 0x40;
constexpr int32 VSP_CFG2_OFF = 0x44;
constexpr int32 VSP_CFG3_OFF = 0x48;
constexpr int32 VSP_CFG4_OFF = 0x4c;
constexpr int32 VSP_CFG5_OFF = 0x50;
constexpr int32 VSP_CFG6_OFF = 0x54;
constexpr int32 VSP_CFG7_OFF = 0x58;
constexpr int32 VSP_CFG8_OFF = 0x5c;
constexpr int32 VSP_CFG9_OFF = 0x84;   // Write-only config.
constexpr int32 VSP_CFG10_OFF = 0x88;  // Write-only config.
constexpr int32 VSP_DEINTELACE_OFF = 0x1c0;
#else
constexpr int32 VSP_DEINTELACE_OFF = (0x0800 - 0x1000);
constexpr int32 VSP_CFG0_OFF = (0x080c - 0x1000);
constexpr int32 VSP_CFG1_OFF = (0x0810 - 0x1000);
constexpr int32 VSP_CFG2_OFF = (0x0814 - 0x1000);
constexpr int32 VSP_CFG3_OFF = (0x0818 - 0x1000);
constexpr int32 VSP_CFG4_OFF = (0x081c - 0x1000);
constexpr int32 VSP_CFG5_OFF = (0x0820 - 0x1000);
constexpr int32 VSP_CFG6_OFF = (0x0824 - 0x1000);
constexpr int32 VSP_CFG7_OFF = (0x0828 - 0x1000);
constexpr int32 VSP_CFG8_OFF = (0x082c - 0x1000);
constexpr int32 VSP_CFG9_OFF = (0x0830 - 0x1000);
constexpr int32 VSP_CFG10_OFF = (0x0834 - 0x1000);
#endif
typedef struct VspGlobalRegTag {
    // Interrupt status and control.
    volatile uint32 interruptStatus;
    volatile uint32 interruptMask;
    volatile uint32 interruptClear;
    volatile uint32 interruptRaw;
    // AXI bus format, pacing and runtime status.
    volatile uint32 axiEndianConfig;
    volatile uint32 axiBurstGapConfig;
    volatile uint32 axiPauseControl;
    volatile uint32 axiBusStatus;

    // Core working mode and image geometry.
    volatile uint32 coreModeConfig;
    volatile uint32 imageSizeConfig;

    // RAM owner select and software-generated interrupts.
    volatile uint32 ramAccessOwner;
    volatile uint32 softwareInterruptGen;
    // Start controls, size limiter and timeout threshold.
    volatile uint32 startControl;
    volatile uint32 sizeConstraint;
    volatile uint32 watchdogTimeout;

    // Codec-dependent control words (shared by encode/decode paths).
    volatile uint32 codecControl0;
    volatile uint32 codecControl1;
    volatile uint32 codecControl2;
    volatile uint32 codecControl3;
    volatile uint32 codecControl4;
    volatile uint32 codecControl5;
    volatile uint32 codecControl6;
    volatile uint32 codecControl7;
    volatile uint32 codecControl8;
    // Bitstream frame addresses and reference index maps.
    volatile uint32 bitstreamBuffer0Addr;
    volatile uint32 bitstreamBuffer1Addr;
    volatile uint32 refIndexMap00;
    volatile uint32 refIndexMap01;
    volatile uint32 refIndexMap02;
    volatile uint32 refIndexMap03;
    volatile uint32 refIndexMap04;
    volatile uint32 refIndexMap05;

    // IMCU state and extra codec control words.
    volatile uint32 imcuRunStatus;
    volatile uint32 codecControl9;
    volatile uint32 codecControl10;

    // Hardware debug snapshots.
    volatile uint32 debugSnapshot0;
    volatile uint32 debugSnapshot1;
    volatile uint32 debugSnapshot2;
    volatile uint32 debugSnapshot3;
    volatile uint32 debugSnapshot4;
    volatile uint32 debugSnapshot5;
    volatile uint32 debugSnapshot6;
    volatile uint32 debugSnapshot7;
    volatile uint32 debugSnapshot8;
    volatile uint32 debugSnapshot9;
    volatile uint32 debugSnapshot10;
    volatile uint32 debugSnapshot11;
    volatile uint32 debugSnapshot12;
    volatile uint32 debugSnapshot13;
} VSP_GLB_REG_T;
// BSM register offsets.
constexpr int32 BSM_CFG0_OFF = 0x0;
constexpr int32 BSM_CFG1_OFF = 0x4;
constexpr int32 BSM_OP_OFF = 0x8;
constexpr int32 BSM_WDATA_OFF = 0xc;
constexpr int32 BSM_RDATA_OFF = 0x10;
constexpr int32 TOTAL_BITS_OFF = 0x14;
constexpr int32 BSM_DBG0_OFF = 0x18;
constexpr int32 BSM_DBG1_OFF = 0x1c;
constexpr int32 BSM_RDY_OFF = 0x20;
constexpr int32 USEV_RD_OFF = 0x24;
constexpr int32 USEV_RDATA_OFF = 0x28;
constexpr int32 DSTUF_NUM_OFF = 0x2c;
constexpr int32 BSM_NAL_LEN = 0x34;
constexpr int32 BSM_NAL_DATA_LEN = 0x38;
// IQW table offsets.
constexpr int32 INTE_R4X4_Y_OFF = 0x00;
constexpr int32 INTE_R4X4_U_OFF = 0x10;
constexpr int32 INTE_R4X4_V_OFF = 0x20;
constexpr int32 INTR_A4X4_Y_OFF = 0x40;
constexpr int32 INTR_A4X4_U_OFF = 0x50;
constexpr int32 INTR_A4X4_V_OFF = 0x60;
constexpr int32 INTE_R8X8_OFF = 0x80;
constexpr int32 INTR_A8X8_OFF = 0x100;
typedef struct  BsmrControlRegisterTag {
    // Control word 0:
    // [31] buffer0Active, [30] buffer1Active. 1 means active (set by SW, cleared by HW).
    // Normal path uses buffer0; both buffers may be active for exceptional recovery paths.
    // [21:0] bitstreamBufferSizeBytes. Reset value is 22'h3fffff.
    volatile uint32 controlWord0;
    // Control word 1:
    // [31] destuffEnable. In encoder mode (H.264), stuffing is auto-managed and this bit is ignored.
    // [30] startCodeCheckMode. 1 enables H.264 decode start-code check, 0 selects normal mode.
    // [29:24] flushBitsCount. Number of bits to flush in decode path (valid range 1..32).
    // [21:0] bitstreamStartOffsetBytes.
    volatile uint32 controlWord1;
    volatile uint32 operationControl;
    // operationControl bits:
    // [3] byteAlign, [2] clearBitCounter, [1] clearBsmFifo, [0] flushBitstreamBits (decode only).
    // clearBsmFifo behavior differs by mode: decoder clears FIFO; encoder may drain or discard pending data.
    volatile uint32 writeDataWindow;  // [31:0] write window payload injected into bitstream.
    volatile uint32 readDataWindow;   // [31:0] current 32-bit capture window.
    volatile uint32 bitCounter;       // [24:0] total bits injected/removed.
    // debugState0 bits:
    // [31] bsmActive, [30:28] bsmState, [27] dataTransferBusy,
    // [16:12] shiftRegisterDepth, [9:8] destuffRemainingWords,
    // [4] pingPongBufferSelect, [3:0] fifoDepth.
    volatile uint32 debugState0;
    // debugState1 bits: [2] startCodeFound, [1] ahbmTransferReq, [0] ahbmReadFifoEmpty.
    volatile uint32 debugState1;
    // fifoAccessState bits: [1] prefixSearchBusy, [0] fifoSwAccessEnable.
    // Check bit[0] before software reads/writes internal FIFO via operation register.
    volatile uint32 fifoAccessState;
    // ueSeDecodeState bits: [2] ueSeDecodeError, [1] seValid, [0] ueValid.
    volatile uint32 ueSeDecodeState;
    volatile uint32 ueSeDecodedValue;  // [31:0] signed UE/SE decoded result.
    volatile uint32 stuffByteCount;    // [7:0] stuffing or de-stuffing byte count.
    volatile uint32 reserved;
    // startCodeTotalLen [21:0]: distance to next detected start code in check mode.
    volatile uint32 startCodePayloadLen;
    // startCodePayloadLen [21:0]: payload bytes excluding stuffing and start-code prefix.
} VSP_BSM_REG_T;
// RGB to YUV conversion mode constants.
constexpr uint32 B_T601_FULL = 0x0;
constexpr uint32 B_T601_REDUCE = 0x4;
/* -----------------------------------------------------------------------
** Structs
** ----------------------------------------------------------------------- */
typedef struct TagVspObject {
    uint32or64 mappedRegBase;
    int32 deviceFd ;
    uint32 clockLevel;
    int32 errorBits;
    int32 hwVersion;
    int32 codecType;
    int32 traceOn;
    int32 vsyncOn;
} VSPObject, VPPObject;

typedef struct {
    unsigned long sourceFrameAddr;
    unsigned long referenceFrameAddr;
    unsigned long destinationFrameAddr;
    uint32 frameIndex;
    DEINT_PARAMS_T *deintParams;
} VppDeintProcessParams;

typedef struct {
    uint32 registerOffset;
    uint32 maskData;
    uint32 expectedValue;
    uint32 timeoutCount;
    const char *debugTag;
} VspRegPollParams;
constexpr uint32 K_VSP_ERROR_STREAM_ID = VspBitMask(K_VSP_ERROR_BIT_STREAM);
constexpr uint32 K_VSP_ERROR_MEMORY_ID = VspBitMask(K_VSP_ERROR_BIT_MEMORY);
constexpr uint32 K_VSP_ERROR_REF_FRAME_ID = VspBitMask(K_VSP_ERROR_BIT_REF_FRAME);
constexpr uint32 K_VSP_ERROR_HARDWARE_ID = VspBitMask(K_VSP_ERROR_BIT_HARDWARE);
constexpr uint32 K_VSP_ERROR_FORMAT_ID = VspBitMask(K_VSP_ERROR_BIT_FORMAT);
constexpr uint32 K_VSP_ERROR_SPS_PPS_ID = VspBitMask(K_VSP_ERROR_BIT_SPS_PPS);
/* -----------------------------------------------------------------------
** Global
** ----------------------------------------------------------------------- */
int32 VppDeintInit(VPPObject *vo);
int32 VppDeintRelease(VPPObject *vo);
int32 VppDeintProcess(VPPObject *vo, const VppDeintProcessParams *processParams);
int VppDeintGetIova(VPPObject *vo, int dmaBufFd, unsigned long *mappedIova, size_t *mappedSize);
int VppDeintFreeIova(VPPObject *vo, unsigned long mappedIova, size_t mappedSize);
int VppDeintGetIommuStatus(VPPObject *vo);
int32 VSP_CFG_FREQ(VSPObject *vo, uint32 frameSize);
int32 VSP_OPEN_Dev(VSPObject *vo);
int32 VSP_CLOSE_Dev(VSPObject *vo);
int32 VSP_POLL_COMPLETE(VSPObject *vo);
int32 VSP_ACQUIRE_Dev(VSPObject *vo);
int32 VSP_RELEASE_Dev(VSPObject *vo);
int32 ARM_VSP_RST(VSPObject *vo);
int32 VSP_SET_SCENE_MODE(VSPObject *vo, uint32* sceneProfile);
int32 VSP_GET_SCENE_MODE(VSPObject *vo, uint32* sceneProfile);
int32 VspGetCodecCounter(VSPObject *vo, uint32* codecCounter);
int VspGetIova(VSPObject *vo, int dmaBufFd, unsigned long *mappedIova, size_t *mappedSize);
int VspFreeIova(VSPObject *vo, unsigned long mappedIova, size_t mappedSize);
int VspGetIommuStatus(VSPObject *vo);
int VSP_NeedAlign(const VSPObject *vo);
int32 VspReadRegPoll(VSPObject *vo, const VspRegPollParams *pollParams);
[[maybe_unused]] static void VspWriteReg(VSPObject *vo, uint32 registerOffset, uint32 value, const char *debugTag)
{
    (void)(debugTag);
    VSPObject* voObj = static_cast<VSPObject*>(vo);
    uintptr_t registerOffsetUint = static_cast<uintptr_t>(registerOffset);
    uintptr_t registerAddress = static_cast<uintptr_t>(registerOffsetUint + voObj->mappedRegBase);
    volatile uint32_t* registerPtr = reinterpret_cast<volatile uint32_t*>(reinterpret_cast<void*>(registerAddress));
    *registerPtr = static_cast<uint32_t>(value);
}

[[maybe_unused]] static uint32 VspReadReg(const VSPObject *vo, uint32 registerOffset, const char *debugTag)
{
    (void)(debugTag);
    return (*reinterpret_cast<volatile uint32*>(registerOffset + vo->mappedRegBase));
}

// End of C ABI exports.
#ifdef __cplusplus
}
#endif
// End of header.
#endif  //VSP_DRV_S_C8830_H_
