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
 ** File Name:    vsp_drv_sc8830.c
 *
 ** Author:       Xiaowei Luo                                                  *
 ** DATE:         06/20/2013                                                  *
 ** Copyright:    2019 Unisoc, Incoporated. All Rights Reserved.              *
 ** Description:  VSP (Video Signal Processor) driver implementation for SC8830*
 *****************************************************************************/
/******************************************************************************
 **                   Edit History                                            *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/20/2013    Xiaowei Luo      Initial creation.                          *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vpp_drv_interface.h"
#include "osal_log.h"
#include <thread>
#include <chrono>
#include <unistd.h>

/* Note: uint32or64 is typedef'd as unsigned long in vsp_deint.h */

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif
/* Magic number definitions */
constexpr uint32 K_VSP_QVGA_SIZE = (320 * 240);
constexpr uint32 K_VSP_D1_SIZE = (720 * 576);
constexpr uint32 K_VSP_720P_SIZE = (1280 * 720);
constexpr uint32 K_VSP_DEFAULT_FREQ_DIV = 3;
constexpr uint32 K_VSP_HIGH_FREQ_DIV = 4;
constexpr useconds_t K_VSP_ERROR_SLEEP_US = (20 * 1000);
constexpr int32 K_VSP_POLL_TIMEOUT_10 = 10;
constexpr uint32 K_VSP_AXIM_ENDIAN_VALUE = 0x30828;
constexpr uint32 K_VSP_DMA_DONE_INT_BIT = 8;
constexpr uint32 K_VSP_DMA_MODE_BIT = 6;
constexpr uint32 K_VSP_DMA_CTRL_MASK = (VspBitMask(K_VSP_DMA_DONE_INT_BIT) | VspBitMask(K_VSP_DMA_MODE_BIT));
constexpr uint32 K_VSP_DMA_DIR_MEM_TO_IRAM = 0;
constexpr uint32 K_VSP_DMA_START = 0x1;
constexpr uint32 K_VSP_AXIM_PAUSE_ENABLE = 0x1;
constexpr useconds_t K_VSP_AXIM_PAUSE_DELAY_US = 1;
/* Frequency divider levels */
constexpr uint32 K_VSP_FREQ_DIV_LEVEL0 = 0;
constexpr uint32 K_VSP_FREQ_DIV_LEVEL1 = 1;
constexpr uint32 K_VSP_FREQ_DIV_LEVEL2 = 2;
constexpr uint32 K_VSP_VERSION_NAME_MAX_LEN = 10;
constexpr uint32 K_VSP_POLL_BUSY_BIT = 30;
constexpr uint32 K_VSP_POLL_DONE_BIT = 1;
constexpr uint32 K_VSP_POLL_ERROR_BIT = 2;
constexpr uint32 K_VSP_AXIM_WRITE_BUSY_BIT = 1;
constexpr uint32 K_VSP_AXIM_READ_BUSY_BIT = 2;
constexpr uint32 K_VSP_RAM_ACCESS_SHARED_BIT = 1;
constexpr uint32 K_VSP_RAM_ACCESS_BOOT_BIT = 0;
/* IOVA related definitions */
constexpr unsigned long K_VSP_INVALID_IOVA = 0;
/* Alignment requirements */

static int32 HandleRegPollTimeout(VSPObject *vo, uint32 vspTimeOutCnt, const char *debugTag)
{
    uint32 mmEbReg;
    ioctl(vo->deviceFd, VSP_HW_INFO, &mmEbReg);
    vo->errorBits |= K_VSP_ERROR_HARDWARE_ID;
    SPRD_CODEC_LOGE("vspTimeoutCount %d, MM_CLK_REG (0x402e0000): 0x%0x, debugTag: %s",
        vspTimeOutCnt, mmEbReg, debugTag);
    return 1;
}

static int32 WaitForRegPoll(VSPObject *vo, const VspRegPollParams *pollParams)
{
    uint32 tmp = 0;
    uint32 vspTimeOutCnt = 0;
    volatile uint32* regPtr = reinterpret_cast<volatile uint32*>(
        pollParams->registerOffset + vo->mappedRegBase);
    tmp = (*regPtr) & pollParams->maskData;
    while ((tmp != pollParams->expectedValue) && (vspTimeOutCnt < pollParams->timeoutCount)) {
        tmp = (*regPtr) & pollParams->maskData;
        vspTimeOutCnt++;
    }
    if (vspTimeOutCnt >= pollParams->timeoutCount) {
        return HandleRegPollTimeout(vo, vspTimeOutCnt, pollParams->debugTag);
    }
    return 0;
}
constexpr int32 K_VSP_ALIGN_NOT_REQUIRED = 0;
constexpr int32 K_VSP_ALIGN_REQUIRED = 1;
/* Register value definitions */
constexpr uint32 K_VSP_REG_VALUE_ZERO = 0;
constexpr uint32 K_VSP_REG_AXIM_STS_ZERO = 0x0;
constexpr uint32 K_VSP_REG_ARM_ACCESS_STS_ZERO = 0x00000000;
constexpr uint32 K_VSP_REG_INT_MASK_ZERO = 0x0;
constexpr uint32 K_VSP_REG_ARM_INT_CLR_VALUE = 0x7;
PUBLIC int32 VSP_CFG_FREQ(VSPObject *vo, uint32 frameSize)
{
    if (frameSize <= K_VSP_QVGA_SIZE) {
        vo->clockLevel = K_VSP_FREQ_DIV_LEVEL0;
    } else if (frameSize <= K_VSP_D1_SIZE) {
        vo->clockLevel = K_VSP_FREQ_DIV_LEVEL1;
    } else if (frameSize < K_VSP_720P_SIZE) {
        vo->clockLevel = K_VSP_FREQ_DIV_LEVEL2;
    } else {
        vo->clockLevel = K_VSP_DEFAULT_FREQ_DIV;
    }
    vo->clockLevel += SPRD_VSP_FREQ_BASE_LEVEL;
    // Use the highest frequency for VSP
    if (SPRD_VSP_FREQ_BASE_LEVEL != 0) {
        vo->clockLevel = K_VSP_HIGH_FREQ_DIV;
    } else {
        vo->clockLevel = K_VSP_DEFAULT_FREQ_DIV;
    }
#ifdef VPP_SHARKL3_DEINT
    vo->clockLevel = K_VSP_HIGH_FREQ_DIV;
#endif
    return 0;
}
PUBLIC int32 VSP_OPEN_Dev(VSPObject *vo)
{
    int32 ret = 0;
    SPRD_CODEC_LOGE("VSP_OPEN_Dev is called");
    char versionNameTable[K_VSP_VERSION_COUNT][K_VSP_VERSION_NAME_MAX_LEN] = { "SHARK", "DOLPHIN", "TSHARK",
        "SHARKL", "PIKE", "PIKEL", "SHARKL64", "SHARKLT8", "WHALE", "WHALE2", "IWHALE2", "ISHARKL2", "SHARKL2"};
    if (vo->deviceFd == -1) {
        if ((vo->deviceFd = open(SPRD_VSP_DRIVER, O_RDWR)) < 0) {
            SPRD_CODEC_LOGE("---yw VSP_OPEN_Dev ERR\n");
            SPRD_CODEC_LOGE("open SPRD_VSP_DRIVER ERR\n");
            return -1;
        } else {
            void* mmapResult = mmap(nullptr, SPRD_VSP_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, vo->deviceFd, 0);
            if (mmapResult == MAP_FAILED) {
                SPRD_CODEC_LOGE("%s: mmap failed, errno=%d", __FUNCTION__, errno);
                close(vo->deviceFd);
                vo->deviceFd = -1;
                return -1;
            }
            vo->mappedRegBase = reinterpret_cast<uint32or64>(mmapResult);
            vo->mappedRegBase -= VSP_REG_BASE_ADDR;
        }
        ioctl(vo->deviceFd, VSP_VERSION, &(vo->hwVersion));
        ioctl(vo->deviceFd, VSP_SET_CODEC_ID, &(vo->codecType));
    }
    SPRD_CODEC_LOGD("%s, %d, vsp addr %lx, hwVersion: %s\n",
        __FUNCTION__, __LINE__, vo->mappedRegBase, versionNameTable[vo->hwVersion]);
    return ret;
}
PUBLIC int32 VSP_CLOSE_Dev(VSPObject *vo)
{
    if (vo->deviceFd > 0) {
        void* munmapAddr = reinterpret_cast<void*>(vo->mappedRegBase + VSP_REG_BASE_ADDR);
        if (munmap(munmapAddr, SPRD_VSP_MAP_SIZE) != 0) {
            SPRD_CODEC_LOGE("%s, %d, %d", __FUNCTION__, __LINE__, errno);
            return -1;
        }
        close(vo->deviceFd);
        return 0;
    } else {
        SPRD_CODEC_LOGD ("%s, error\n", __FUNCTION__);
        return -1;
    }
}
PUBLIC int32 VSP_GET_DEV_FREQ(VSPObject *vo, uint32*  vspClkPtr)
{
    if (vo->deviceFd > 0) {
        ioctl(vo->deviceFd, VSP_GET_FREQ, vspClkPtr);
        return 0;
    } else {
        SPRD_CODEC_LOGE ("%s, error", __FUNCTION__);
        return -1;
    }
}
PUBLIC int32 VSP_CONFIG_DEV_FREQ(VSPObject *vo, uint32*  vspClkPtr)
{
    if (vo->deviceFd > 0) {
        int32 ret = ioctl(vo->deviceFd, VSP_CONFIG_FREQ, vspClkPtr);
        if (ret < 0) {
            SPRD_CODEC_LOGE ("%s, VSP_CONFIG_FREQ failed", __FUNCTION__);
            return -1;
        }
    } else {
        SPRD_CODEC_LOGE ("%s, error", __FUNCTION__);
        return -1;
    }
    return 0;
}
PUBLIC int32 VSP_SET_SCENE_MODE(VSPObject *vo, uint32* sceneProfile)
{
    if (vo->deviceFd > 0) {
        int32 ret;
        if (*sceneProfile > K_VSP_SCENE_MODE_COUNT) {
            SPRD_CODEC_LOGE ("%s, Invalid sceneProfile value", __FUNCTION__);
            return -1;
        }
        ret = ioctl(vo->deviceFd, VSP_SET_SCENE, sceneProfile);
        if (ret < 0) {
            SPRD_CODEC_LOGE ("%s, VSP_SET_SCENE_MODE failed", __FUNCTION__);
            return -1;
        }
    } else {
        SPRD_CODEC_LOGE ("%s, error", __FUNCTION__);
        return -1;
    }
    return 0;
}
PUBLIC int32 VSP_GET_SCENE_MODE(VSPObject *vo, uint32* sceneProfile)
{
    if (vo->deviceFd > 0) {
        int32 ret = ioctl(vo->deviceFd, VSP_GET_SCENE, sceneProfile);
        if (ret < 0 || *sceneProfile > K_VSP_SCENE_MODE_COUNT) {
            SPRD_CODEC_LOGE ("%s, VSP_GET_SCENE_MODE failed", __FUNCTION__);
            return -1;
        }
    } else {
        SPRD_CODEC_LOGE ("%s, error", __FUNCTION__);
        return -1;
    }
    return 0;
}
constexpr int32 K_MAX_POLL_COUNT = K_VSP_POLL_TIMEOUT_10;
PUBLIC int32 VSP_POLL_COMPLETE(VSPObject *vo)
{
    if (vo->deviceFd > 0) {
        int32 ret;
        int32 cnt = 0;
        do {
            ioctl(vo->deviceFd, VSP_COMPLETE, &ret);
            cnt++;
        } while ((ret & VspBitMask(K_VSP_POLL_BUSY_BIT)) &&  (cnt < K_MAX_POLL_COUNT));
        if (!(ret == VspBitMask(K_VSP_POLL_DONE_BIT) || ret == VspBitMask(K_VSP_POLL_ERROR_BIT))) {
            SPRD_CODEC_LOGD("%s, %d, int_ret: %0x\n", __FUNCTION__, __LINE__, ret);
        }
        return ret;
    } else {
        SPRD_CODEC_LOGE ("%s, error", __FUNCTION__);
        return -1;
    }
}
PUBLIC int32 VSP_ACQUIRE_Dev(VSPObject *vo)
{
    int32 ret ;
    if (vo->deviceFd <  0) {
        SPRD_CODEC_LOGE("%s: failed :fd <  0", __FUNCTION__);
        return -1;
    }
    ret = ioctl(vo->deviceFd, VSP_ACQUAIRE, nullptr);
    if (ret != 0) {
        SPRD_CODEC_LOGE("%s: VSP_ACQUAIRE failed,  %d\n", __FUNCTION__, ret);
        return -1;
    }
    if (VSP_CONFIG_DEV_FREQ(vo, &(vo->clockLevel)) != 0) {
        return -1;
    }
    ret = ioctl(vo->deviceFd, VSP_ENABLE, nullptr);
    if (ret < 0) {
        SPRD_CODEC_LOGE("%s: VSP_ENABLE failed %d\n", __FUNCTION__, ret);
        return -1;
    }
    ret = ioctl(vo->deviceFd, VSP_RESET, nullptr);
    if (ret < 0) {
        SPRD_CODEC_LOGE("%s: VSP_RESET failed %d\n", __FUNCTION__, ret);
        return -1;
    }
    SPRD_CODEC_LOGD("%s, %d\n", __FUNCTION__, __LINE__);
    return 0;
}

static void PauseAximIfNeeded(VSPObject *vo)
{
    if (vo->errorBits == 0) {
        return;
    }
    VspWriteReg(vo, GLB_REG_BASE_ADDR + AXIM_PAUSE_OFF, K_VSP_AXIM_PAUSE_ENABLE,
        "AXIM_PAUSE: stop DATA TRANS to AXIM");
    usleep(K_VSP_AXIM_PAUSE_DELAY_US);
}

static void PollAximIdle(VSPObject *vo)
{
    VspRegPollParams aximWritePoll = {
        GLB_REG_BASE_ADDR + AXIM_STS_OFF, VspBitMask(K_VSP_AXIM_WRITE_BUSY_BIT), K_VSP_REG_AXIM_STS_ZERO,
        K_FRAME_POLL_TIMEOUT_CLK, "Polling AXIM_STS: not Axim_wch_busy"};
    if (VspReadRegPoll(reinterpret_cast<VSPObject*>(vo), &aximWritePoll) != 0) {
        SPRD_CODEC_LOGE("%s, %d, Axim_wch_busy\n", __FUNCTION__, __LINE__);
    }
    VspRegPollParams aximReadPoll = {
        GLB_REG_BASE_ADDR + AXIM_STS_OFF, VspBitMask(K_VSP_AXIM_READ_BUSY_BIT), K_VSP_REG_AXIM_STS_ZERO,
        K_FRAME_POLL_TIMEOUT_CLK, "Polling AXIM_STS: not Axim_rch_busy"};
    if (VspReadRegPoll(reinterpret_cast<VSPObject*>(vo), &aximReadPoll) != 0) {
        SPRD_CODEC_LOGE("%s, %d, Axim_wch_busy", __FUNCTION__, __LINE__);
    }
}

static int32 IoctlOrFail(VSPObject *vo, int request, const char *requestName)
{
    int32 ret = ioctl(vo->deviceFd, request, nullptr);
    if (ret >= 0) {
        return 0;
    }
    SPRD_CODEC_LOGE("%s: %s failed, %d\n", __FUNCTION__, requestName, ret);
    return -1;
}

PUBLIC int32 VSP_RELEASE_Dev(VSPObject *vo)
{
    int32 ret = 0;
    if (vo->deviceFd <= 0) {
        SPRD_CODEC_LOGE("%s: failed :fd <  0", __FUNCTION__);
        ret = -1;
    } else {
        PauseAximIfNeeded(vo);
        PollAximIdle(vo);
        if (IoctlOrFail(vo, VSP_RESET, "VSP_RESET") != 0 ||
            IoctlOrFail(vo, VSP_DISABLE, "VSP_DISABLE") != 0 ||
            IoctlOrFail(vo, VSP_RELEASE, "VSP_RELEASE") != 0) {
            ret = -1;
        }
    }
    if (vo->errorBits || (ret < 0)) {
        usleep(K_VSP_ERROR_SLEEP_US);
    }
    SPRD_CODEC_LOGD("%s, %d, ret: %d\n", __FUNCTION__, __LINE__, ret);
    return ret;
}
int32 VspGetCodecCounter(VSPObject *vo, uint32* codecCounter)
{
    int ret = 0;
    ret = ioctl(vo->deviceFd, VSP_GET_CODEC_COUNTER, codecCounter);
    return ret;
}
PUBLIC int VspGetIova(VSPObject *vo, int dmaBufFd, unsigned long *mappedIova, size_t *mappedSize)
{
    int ret = 0;
    VspIommuMapDataT mapData;
    mapData.bufferFd = dmaBufFd;
    ret = ioctl(vo->deviceFd, VSP_GET_IOVA, &mapData);
    if (ret < 0) {
        SPRD_CODEC_LOGE("vsp_get_iova failed ret %d \n", ret);
    } else {
        *mappedIova = mapData.mappedIova;
        *mappedSize = mapData.bufferSize;
    }
    return ret;
}
PUBLIC int VspFreeIova(VSPObject *vo, unsigned long mappedIova, size_t mappedSize)
{
    int ret = 0;
    VspIommuMapDataT unmapData;
    if (mappedIova == K_VSP_INVALID_IOVA) {
        return -1;
    }
    unmapData.mappedIova = mappedIova;
    unmapData.bufferSize = mappedSize;
    ret = ioctl(vo->deviceFd, VSP_FREE_IOVA, &unmapData);
    if (ret < 0) {
        SPRD_CODEC_LOGE("vsp_free_iova failed ret %d \n", ret);
    }
    return ret;
}
PUBLIC int VspGetIommuStatus(VSPObject *vo)
{
    int ret = 0;
    ret = ioctl(vo->deviceFd, VSP_GET_IOMMU_STATUS);
    return ret;
}
PUBLIC int VSP_NeedAlign(const VSPObject *vo)
{
    if ((vo->hwVersion == K_VSP_VERSION_WHALE2) || (vo->hwVersion == K_VSP_VERSION_I_WHALE2)) {
        return K_VSP_ALIGN_NOT_REQUIRED;
    }
    return K_VSP_ALIGN_REQUIRED;
}
PUBLIC int32 ARM_VSP_RST(VSPObject *vo)
{
    if (VSP_ACQUIRE_Dev(vo) < 0) {
        return -1;
    }
#ifndef VPP_SHARKL3_DEINT
    VspWriteReg(vo, AHB_CTRL_BASE_ADDR + ARM_ACCESS_CTRL_OFF, K_VSP_REG_VALUE_ZERO, "RAM_ACC_by arm");
    VspRegPollParams armAccessPoll = {
        AHB_CTRL_BASE_ADDR + ARM_ACCESS_STATUS_OFF,
        (VspBitMask(K_VSP_RAM_ACCESS_SHARED_BIT) | VspBitMask(K_VSP_RAM_ACCESS_BOOT_BIT)),
        K_VSP_REG_ARM_ACCESS_STS_ZERO, K_REG_POLL_TIMEOUT_CLK,
        reinterpret_cast<const char*>("ARM_ACCESS_STATUS_OFF")};
    if (VspReadRegPoll(reinterpret_cast<VSPObject*>(vo), &armAccessPoll) != 0) {
        return -1;
    }
    // Disable Openrisc interrrupt
    VspWriteReg(vo, AHB_CTRL_BASE_ADDR + ARM_INT_MASK_OFF, K_VSP_REG_VALUE_ZERO, "arm INT mask set");
    VspWriteReg(vo, GLB_REG_BASE_ADDR + VSP_INT_MASK_OFF, K_VSP_REG_VALUE_ZERO,
        "VSP_INT_MASK, enable vlc_slice_done, time_out");
#endif
    // VSP and OR endian.
    VspWriteReg(vo, GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF, K_VSP_AXIM_ENDIAN_VALUE, "axim endian set");
    return 0;
}
PUBLIC  int32 VspReadRegPoll(VSPObject *vo, const VspRegPollParams *pollParams)
{
    return WaitForRegPoll(vo, pollParams);
}
/**
 * @brief Trigger VSP DMA transfer for table data
 * @param vo: Pointer to VSPObject instance
 * @param srcAddr: Source memory address
 * @param dstAddr: Destination IRAM address
 * @param size: Transfer size in 128-bit units
 */
PUBLIC void VSP_DMA_TRANSFER(VSPObject *vo, uint32or64 srcAddr,  uint32 dstAddr, uint32 size)
{
#if defined CHIP_WHALE
    (void)vo;
    (void)srcAddr;
    (void)dstAddr;
    (void)size;
    
    VspWriteReg(vo, GLB_REG_BASE_ADDR + VSP_INT_MASK_OFF, VspBitMask(K_VSP_DMA_DONE_INT_BIT), "VSP_INT_MASK");
#ifdef CHIP_IWHALE2
    VspWriteReg(vo, GLB_REG_BASE_ADDR + VSP_INT_MASK_OFF, VSP_REG_VALUE_ZERO, "VSP_INT_MASK");
#endif
    VspWriteReg(vo, GLB_REG_BASE_ADDR + DMA_SET0_OFF, VSP_DMA_DIR_MEM2IRAM, "DMA trans dir 0, from EXT to IRAM");
    VspWriteReg(vo, GLB_REG_BASE_ADDR + DMA_SET1_OFF, size, "DMA trans size");   // unit=128-bit, 72 words
    VspWriteReg(vo, GLB_REG_BASE_ADDR + DMA_SET2_OFF, srcAddr, "DMA EXT start addr");
    VspWriteReg(vo, GLB_REG_BASE_ADDR + DMA_SET3_OFF, dstAddr, "DMA VLC table start addr");
    VspWriteReg(vo, GLB_REG_BASE_ADDR + VSP_MODE_OFF, K_VSP_DMA_CTRL_MASK, "DMA_access_mode=1&& watchdog_disable");
    VspWriteReg(vo, GLB_REG_BASE_ADDR + VSP_START_OFF, K_VSP_DMA_START, "DMA_START");
    VspRegPollParams dmaDonePoll = {
        GLB_REG_BASE_ADDR + VSP_INT_RAW_OFF, VspBitMask(K_VSP_DMA_DONE_INT_BIT),
        VspBitMask(K_VSP_DMA_DONE_INT_BIT), K_REG_POLL_TIMEOUT_CLK,
        reinterpret_cast<const char*>("DMA_TRANS_DONE int")};
    VspReadRegPoll(reinterpret_cast<VSPObject*>(vo), &dmaDonePoll);    //check HW int
    VspWriteReg(vo, GLB_REG_BASE_ADDR + VSP_INT_CLR_OFF, VspBitMask(K_VSP_DMA_DONE_INT_BIT), "clear VSP_INT_RAW");
    VspWriteReg(vo, VSP_REG_BASE_ADDR+ARM_INT_CLR_OFF, K_VSP_REG_ARM_INT_CLR_VALUE, "clear ARM_INT_RAW");
    VspWriteReg(vo, GLB_REG_BASE_ADDR + VSP_INT_MASK_OFF, K_VSP_REG_INT_MASK_ZERO, "VSP_INT_MASK");
#else
    // Suppress unused parameter warnings when CHIP_WHALE is not defined
    (void)vo;
    (void)srcAddr;
    (void)dstAddr;
    (void)size;
#endif
}
#ifdef   __cplusplus
}
#endif
// End of file