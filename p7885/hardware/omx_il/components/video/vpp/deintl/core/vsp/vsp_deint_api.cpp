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

#include "vpp_drv_interface.h"
#include <securec.h>
#include "osal_log.h"
#include <thread>
#include <chrono>
#include <unistd.h>

/* Constants for register configuration */
constexpr uint32 K_CFG0_FRAME_NO_MASK = 0x1;
constexpr uint32 K_CFG0_FRAME_NO_SHIFT = 0;
constexpr uint32 K_CFG0_YUV_FORMAT_MASK = 0x2;
constexpr uint32 K_CFG0_YUV_FORMAT_SHIFT = 1;
constexpr uint32 K_CFG0_THRESHOLD_MASK = 0xFF;
constexpr uint32 K_CFG0_THRESHOLD_SHIFT = 8;

constexpr uint32 K_CFG1_WIDTH_MASK = 0xFFFF;
constexpr uint32 K_CFG1_WIDTH_SHIFT = 0;
constexpr uint32 K_CFG1_HEIGHT_MASK = 0xFFFF;
constexpr uint32 K_CFG1_HEIGHT_SHIFT = 16;

constexpr uint32 K_IMAGE_SIZE_MAX_SHIFT = 12;
constexpr uint32 K_IMAGE_SIZE_MAX_CHECK = 0;

constexpr uint32 K_VSP_INT_CLR_INIT_MASK = 0xFFF;
constexpr uint32 K_VSP_INT_CLR_ALL_MASK = 0x1FF;

constexpr uint32 K_DEINT_START_VALUE = 0x01;

/* Polling parameters */
constexpr int32 K_DEINT_POLL_MAX_COUNT = 20;
constexpr useconds_t K_DEINT_POLL_SLEEP_US = 1000;

/* Address shift values for different platforms */
#ifndef VPP_SHARKL3_DEINT
constexpr uint32 K_ADDR_SHIFT_VALUE = 3;
#else
constexpr uint32 K_ADDR_SHIFT_VALUE = 4;
#endif

/* Default VSP frequency division value */
constexpr uint32 K_VSP_FREQ_DIV_DEFAULT = 3;

/* Default trace enabled state */
constexpr int32 K_VSP_TRACE_ENABLED_DEFAULT = 1;

/* Invalid file descriptor value */
constexpr int32 K_INVALID_FILE_DESCRIPTOR = (-1);

/* Bit positions used by deinterlace status/configuration flow. */
constexpr uint32 K_DEINT_DONE_BIT_DEFAULT = 9;
constexpr uint32 K_DEINT_DONE_BIT_SHARK_L3 = 11;
constexpr uint32 K_ENDIAN_BIT_A = 23;
constexpr uint32 K_ENDIAN_BIT_B = 21;
constexpr uint32 K_ENDIAN_BIT_C = 20;
constexpr uint32 K_ENDIAN_BIT_D = 18;
constexpr uint32 K_ENDIAN_BIT_SHARK_L3_A = 30;
constexpr uint32 K_ENDIAN_BIT_SHARK_L3_B = 28;
constexpr uint32 K_ENDIAN_BIT_SHARK_L3_C = 26;
constexpr uint32 K_ENDIAN_BIT_SHARK_L3_D = 24;
constexpr uint32 K_VSP_MODE_WATCHDOG_DISABLE_BIT = 30;
constexpr uint32 K_ARM_INTERRUPT_MASK_ENABLE_BIT = 2;

static void ClearDeintInterrupts(VPPObject *vo, uint32_t mask)
{
    (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase +
        (GLB_REG_BASE_ADDR + VSP_INT_CLR_OFF)) = mask);
}

static int32 PollDeintCompletion(VPPObject *vo)
{
    uint32 cmd = (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase +
        (GLB_REG_BASE_ADDR + VSP_INT_RAW_OFF)));
    SPRD_CODEC_LOGD("%s, %d, cmd : %d", __FUNCTION__, __LINE__, cmd);
    int count = K_DEINT_POLL_MAX_COUNT;
#ifndef VPP_SHARKL3_DEINT
    while (((cmd & VspBitMask(K_DEINT_DONE_BIT_DEFAULT)) == 0) && count--) {
#else
    while (((cmd & VspBitMask(K_DEINT_DONE_BIT_SHARK_L3)) == 0) && count--) {
#endif
        usleep(K_DEINT_POLL_SLEEP_US);
        cmd = (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase +
            (GLB_REG_BASE_ADDR + VSP_INT_RAW_OFF)));
        SPRD_CODEC_LOGD("%s, %d, cmd : %d", __FUNCTION__, __LINE__, cmd);
    }
    if (count < 0) {
        vo->errorBits = 1;
    }
    return 0;
}

int InitDeintReg(VPPObject *vo, const DEINT_PARAMS_T *deintConfig, uint32 frameIndex)
{
    uint32 val;
#ifndef VPP_SHARKL3_DEINT
    val = VspBitMask(K_ENDIAN_BIT_A) | VspBitMask(K_ENDIAN_BIT_B) |
        VspBitMask(K_ENDIAN_BIT_C) | VspBitMask(K_ENDIAN_BIT_D);
#else
    val = VspBitMask(K_ENDIAN_BIT_SHARK_L3_A) | VspBitMask(K_ENDIAN_BIT_SHARK_L3_B) |
        VspBitMask(K_ENDIAN_BIT_SHARK_L3_C) | VspBitMask(K_ENDIAN_BIT_SHARK_L3_D);
#endif
    (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase +
        (GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF)) = (val));    //VSP and OR endian.
    val = VspBitMask(K_VSP_MODE_WATCHDOG_DISABLE_BIT);
    (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_MODE_OFF)) = (val));

#ifdef USE_INTERRUPT
    (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase +
        (VSP_REG_BASE_ADDR + ARM_INT_MASK_OFF)) = (VspBitMask(K_ARM_INTERRUPT_MASK_ENABLE_BIT)));    //enable int
#endif
    if (frameIndex == 0) {
        val = (1 & K_CFG0_FRAME_NO_MASK) << K_CFG0_FRAME_NO_SHIFT;
    } else {
        val = (0 & K_CFG0_FRAME_NO_MASK) << K_CFG0_FRAME_NO_SHIFT;
    }
    val |= (0 & K_CFG0_YUV_FORMAT_MASK) << K_CFG0_YUV_FORMAT_SHIFT;    //yuv format : 0: UV,       1:u/v
    val |= (deintConfig->threshold & K_CFG0_THRESHOLD_MASK) << K_CFG0_THRESHOLD_SHIFT;
    (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG0_OFF)) = (val));

    if ((deintConfig->width >> K_IMAGE_SIZE_MAX_SHIFT) + (deintConfig->height >> K_IMAGE_SIZE_MAX_SHIFT) >
        K_IMAGE_SIZE_MAX_CHECK) {
        return -1;
    }
    val = (deintConfig->width & K_CFG1_WIDTH_MASK) << K_CFG1_WIDTH_SHIFT;
    val |= (deintConfig->height & K_CFG1_HEIGHT_MASK) << K_CFG1_HEIGHT_SHIFT;
    (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG1_OFF)) = (val));

    return 0;
}

int WriteDeintFrameAddr(VPPObject *vo, unsigned long sourceFrameAddr,
                        unsigned long referenceFrameAddr, unsigned long destinationFrameAddr,
                        const DEINT_PARAMS_T *deintConfig)
{
    uint32 yLen = deintConfig->yLen;
    uint32 cLen = deintConfig->cLen;

    if (deintConfig->interleave != 0) {
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG2_OFF)) =
            ((sourceFrameAddr) >> K_ADDR_SHIFT_VALUE));
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG3_OFF)) =
            ((sourceFrameAddr + yLen) >> K_ADDR_SHIFT_VALUE));
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG4_OFF)) = (0));

        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG5_OFF)) =
            ((destinationFrameAddr) >> K_ADDR_SHIFT_VALUE));
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG6_OFF)) =
            ((destinationFrameAddr + yLen) >> K_ADDR_SHIFT_VALUE));
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG7_OFF)) = (0));

        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG8_OFF)) =
            ((referenceFrameAddr) >> K_ADDR_SHIFT_VALUE));
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG9_OFF)) =
            ((referenceFrameAddr + yLen) >> K_ADDR_SHIFT_VALUE));
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG10_OFF)) = (0));
    } else {
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG2_OFF)) =
            ((sourceFrameAddr) >> K_ADDR_SHIFT_VALUE));
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG3_OFF)) =
            ((sourceFrameAddr + yLen) >> K_ADDR_SHIFT_VALUE));
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG4_OFF)) =
            ((sourceFrameAddr + yLen + cLen) >> K_ADDR_SHIFT_VALUE));

        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG5_OFF)) =
            ((destinationFrameAddr) >> K_ADDR_SHIFT_VALUE));
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG6_OFF)) =
            ((destinationFrameAddr + yLen) >> K_ADDR_SHIFT_VALUE));
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG7_OFF)) =
            ((destinationFrameAddr + yLen + cLen) >> K_ADDR_SHIFT_VALUE));

        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG8_OFF)) =
            ((referenceFrameAddr) >> K_ADDR_SHIFT_VALUE));
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG9_OFF)) =
            ((referenceFrameAddr + yLen) >> K_ADDR_SHIFT_VALUE));
        (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_CFG10_OFF)) =
            ((referenceFrameAddr + yLen + cLen) >> K_ADDR_SHIFT_VALUE));
    }
    return 0;
}

int32 VppDeintInit(VPPObject *vo)
{
    int ret = 0;
    if (vo == nullptr) {
        return -1;
    }
    (void)memset_s(vo, sizeof(VPPObject), 0, sizeof(VPPObject));
    vo->deviceFd = K_INVALID_FILE_DESCRIPTOR;
    vo->clockLevel = K_VSP_FREQ_DIV_DEFAULT;
    vo->traceOn = K_VSP_TRACE_ENABLED_DEFAULT;
    ret = VSP_OPEN_Dev(vo);
    if (ret != 0) {
        SPRD_CODEC_LOGE("vsp_open_dev failed ret: 0x%x\n", ret);
        return -1;
    }
    return 0;
}

int VppDeintGetIova(VPPObject *vo, int dmaBufFd, unsigned long *mappedIova, size_t *mappedSize)
{
    int ret = 0;
    ret = VspGetIova(vo, dmaBufFd, mappedIova, mappedSize);
    return ret;
}

int VppDeintFreeIova(VPPObject *vo, unsigned long mappedIova, size_t mappedSize)
{
    int ret = 0;
    ret = VspFreeIova(vo, mappedIova, mappedSize);
    return ret;
}

int VppDeintGetIommuStatus(VPPObject *vo)
{
    int ret = 0;
    ret = VspGetIommuStatus(vo);
    return ret;
}

int32 VppDeintRelease(VPPObject *vo)
{
    if (vo) {
        VSP_CLOSE_Dev(vo);
    }
    return 0;
}

int32 VppDeintProcess(VPPObject *vo, const VppDeintProcessParams *processParams)
{
    unsigned long sourceFrameAddr = processParams->sourceFrameAddr;
    unsigned long referenceFrameAddr = processParams->referenceFrameAddr;
    unsigned long destinationFrameAddr = processParams->destinationFrameAddr;
    uint32 frameIndex = processParams->frameIndex;
    DEINT_PARAMS_T *deintParams = processParams->deintParams;
    SPRD_CODEC_LOGI("vsp_deint_process: sourceFrameAddr = 0x%lx, referenceFrameAddr = 0x%lx, destinationFrameAddr = "
        "0x%lx, frameIndex = %d\n", sourceFrameAddr, referenceFrameAddr, destinationFrameAddr, frameIndex);
    if (ARM_VSP_RST(vo) < 0) {
        return -1;
    }
    if (InitDeintReg(vo, deintParams, frameIndex) != 0) {
        SPRD_CODEC_LOGE("Init deint config error\n");
        VSP_RELEASE_Dev(vo);
        return -1;
    }

    SPRD_CODEC_LOGD("%s, %d, interleave : %d", __FUNCTION__, __LINE__, deintParams->interleave);
    WriteDeintFrameAddr(vo, sourceFrameAddr, referenceFrameAddr, destinationFrameAddr, deintParams);
    ClearDeintInterrupts(vo, K_VSP_INT_CLR_INIT_MASK);
    (*reinterpret_cast<volatile uint32_t *>((vo)->mappedRegBase + (GLB_REG_BASE_ADDR + VSP_DEINTELACE_OFF)) =
        (K_DEINT_START_VALUE));
#ifdef USING_INTERRUPT
    if (VSP_POLL_COMPLETE(vo)) {
        SPRD_CODEC_LOGE("Error, Get interrupt timeout!!!! \n");
        VSP_RELEASE_Dev(vo);
        return -1;
    }
#else
    PollDeintCompletion(vo);
#endif
    ClearDeintInterrupts(vo, K_VSP_INT_CLR_ALL_MASK);
    VSP_RELEASE_Dev(vo);
    return 0;
}