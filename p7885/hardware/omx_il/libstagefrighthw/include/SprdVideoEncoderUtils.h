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
#ifndef SPRD_VIDEO_ENCODER_UTILS_H_
#define SPRD_VIDEO_ENCODER_UTILS_H_
#include <securec.h>
#include <utils/time_util.h>
#include <utils/omx_log.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#ifndef CONFIG_BIA_SUPPORT
#include <arm_neon.h>
#endif
#define VIDEOENC_CURRENT_OPT
/* RGB to YUV conversion coefficients */
#define RGB2Y_R_COEFF    66
#define RGB2Y_G_COEFF    129
#define RGB2Y_B_COEFF    25
#define RGB2Y_OFFSET     16
#define RGB2CB_R_COEFF   (-38)
#define RGB2CB_G_COEFF   (-74)
#define RGB2CB_B_COEFF   112
#define RGB2CB_OFFSET    128
#define RGB2CR_R_COEFF   112
#define RGB2CR_G_COEFF   (-94)
#define RGB2CR_B_COEFF   (-18)
#define RGB2CR_OFFSET    128
/* Component shift values for UV channels */
#define UV_SHIFT_FACTOR  8
/* Memory alignment boundaries */
#define ALIGN_16_BYTES   16
#define ALIGN_8_BYTES    8
#define ALIGN_4_BYTES    4
#define ALIGN_2_BYTES    2
/* Bit manipulation constants */
#define BIT_MASK_0X_FF    0xFF
#define BIT_MASK_0X_FF00  0xFF00
/* YUV format constants */
#define YUV420_UV_SCALE_FACTOR  2
#define YUV420_UV_DIVISOR       4
/* Color component indices */
#define ARGB_RED_INDEX   2
#define ARGB_GREEN_INDEX 1
#define ARGB_BLUE_INDEX  0
#define ARGB_ALPHA_INDEX 3
/* Loop processing parameters */
#define NEON_PROCESS_ELEMENTS  8
#define NEON_LOAD_BYTES        32
/* Frequency setting for DDR */
#define DDR_FREQ_400MHZ        "400000"
/* RGB lookup table size */
#define RGB_LUT_SIZE           256
/* Loop control constants */
#define LOOP_DECREMENT_ONE     1
#define HALF_WIDTH_ADJUSTMENT  1
#define FULL_WIDTH_ADJUSTMENT  1
#define DIVISOR_TWO            2
#define INITIAL_COUNTER_ZERO   0
/*
 * In case of orginal input height_org is not 16 aligned, we shoud copy original data to a larger space
 * Example: width_org = 640, height_org = 426,
 * We have to copy this data to a width_dst = 640 height_dst = 432 buffer which is 16 aligned.
 * Be careful, when doing this convert we MUST keep UV in their right position.
 */
struct Yuv420PlanarConvertParams {
    uint8_t *input;
    uint8_t *output;
    int32_t widthOrg;
    int32_t heightOrg;
    int32_t widthDst;
    int32_t heightDst;
};

struct ArgbToYvu420SemiPlanarConvertParams {
    uint8_t *inputRgb;
    uint8_t *outputY;
    uint8_t *outputUv;
    int32_t widthOrg;
    int32_t heightOrg;
    int32_t widthDst;
    int32_t heightDst;
};

static void ConvertYUV420PlanarToYVU420SemiPlanar(const Yuv420PlanarConvertParams &params)
{
    int32_t inYsize = params.widthOrg * params.heightOrg;
    uint32_t *outy =  (uint32_t *) params.output;
    uint16_t *incb = (uint16_t *) (params.input + inYsize);
    uint16_t *incr = (uint16_t *) (params.input + inYsize + (inYsize >> YUV420_UV_SCALE_FACTOR));
    /* Y copying */
    memmove_s(outy, params.widthDst * params.heightDst, params.input, inYsize);
    /* U & V copying, Make sure uv data is in their right position*/
    uint32_t *outUV = (uint32_t *) (params.output + params.widthDst * params.heightDst);
    for (int32_t i = params.heightOrg >> YUV420_UV_SCALE_FACTOR; i > 0; --i) {
        for (int32_t j = params.widthOrg >> YUV420_UV_SCALE_FACTOR; j > 0; --j) {
            uint32_t tempU = *incb++;
            uint32_t tempV = *incr++;
            tempU = (tempU & BIT_MASK_0X_FF) | ((tempU & BIT_MASK_0X_FF00) << UV_SHIFT_FACTOR);
            tempV = (tempV & BIT_MASK_0X_FF) | ((tempV & BIT_MASK_0X_FF00) << UV_SHIFT_FACTOR);
            uint32_t temp = tempV | (tempU << UV_SHIFT_FACTOR);
            // Flip U and V
            *outUV++ = temp;
        }
    }
}
#ifdef CONFIG_BIA_SUPPORT
static int RGB_r_y[RGB_LUT_SIZE];
static int RGB_r_cb[RGB_LUT_SIZE];
static int RGB_r_cr_b_cb[RGB_LUT_SIZE];
static int RGB_g_y[RGB_LUT_SIZE];
static int RGB_g_cb[RGB_LUT_SIZE];
static int RGB_g_cr[RGB_LUT_SIZE];
static int RGB_b_y[RGB_LUT_SIZE];
static int RGB_b_cr[RGB_LUT_SIZE];
static  bool mConventFlag = false;
//init the convert table, the Transformation matrix is as:
// Y  =  ((66 * (_r)  + 129 * (_g)  + 25    * (_b)) >> 8) + 16
// Cb = ((-38 * (_r) - 74   * (_g)  + 112  * (_b)) >> 8) + 128
// Cr =  ((112 * (_r) - 94   * (_g)  - 18    * (_b)) >> 8) + 128
static void inittable()
{
    OMX_LOGI("init table");
    int i = INITIAL_COUNTER_ZERO;
    for (i = INITIAL_COUNTER_ZERO; i < RGB_LUT_SIZE; i++) {
        RGB_r_y[i] =  (RGB2Y_R_COEFF * i);
        RGB_r_cb[i] = (RGB2CB_R_COEFF * i);
        RGB_r_cr_b_cb[i] = (RGB2CR_R_COEFF * i);
        RGB_g_y[i] = (RGB2Y_G_COEFF * i);
        RGB_g_cb[i] = (RGB2CB_G_COEFF * i) ;
        RGB_g_cr[i] = (RGB2CR_G_COEFF * i);
        RGB_b_y[i] =  (RGB2Y_B_COEFF * i);
        RGB_b_cr[i] = (RGB2CR_B_COEFF * i);
    }
}
static uint8_t rgb2YFunc(uint8_t r, uint8_t g, uint8_t b)
{
    return ((*(RGB_r_y + r) + *(RGB_g_y + g) + *(RGB_b_y + b)) >> UV_SHIFT_FACTOR) + RGB2Y_OFFSET;
}

static uint8_t rgb2CbFunc(uint8_t r, uint8_t g, uint8_t b)
{
    return ((-*(RGB_r_cb + r) - *(RGB_g_cb + g) + \
                               *(RGB_r_cr_b_cb + b)) >> UV_SHIFT_FACTOR) + RGB2CB_OFFSET;
}

static uint8_t rgb2CrFunc(uint8_t r, uint8_t g, uint8_t b)
{
    return ((*(RGB_r_cr_b_cb + r) - *(RGB_g_cr + g) - \
                               *(RGB_b_cr + b)) >> UV_SHIFT_FACTOR) + RGB2CR_OFFSET;
}

static void ConvertArgbLineWithUv(uint8_t *&argbPtr, uint8_t *&yPtr, uint8_t *&vuPtr, int32_t widthOrg)
{
    uint32_t i = widthOrg / YUV420_UV_SCALE_FACTOR + HALF_WIDTH_ADJUSTMENT;
    while (--i) {
        *yPtr++ = rgb2YFunc(*argbPtr, *(argbPtr + ARGB_GREEN_INDEX), *(argbPtr + ARGB_RED_INDEX));
        *vuPtr++ = rgb2CrFunc(*argbPtr, *(argbPtr + ARGB_GREEN_INDEX), *(argbPtr + ARGB_RED_INDEX));
        *vuPtr++ = rgb2CbFunc(*argbPtr, *(argbPtr + ARGB_GREEN_INDEX), *(argbPtr + ARGB_RED_INDEX));
        *yPtr++ = rgb2YFunc(*(argbPtr + ALIGN_4_BYTES),
            *(argbPtr + ARGB_GREEN_INDEX + ALIGN_4_BYTES), *(argbPtr + ARGB_RED_INDEX + ALIGN_4_BYTES));
        argbPtr += (ALIGN_4_BYTES * YUV420_UV_SCALE_FACTOR);
    }
}

static void ConvertArgbLineYOnly(uint8_t *&argbPtr, uint8_t *&yPtr, int32_t widthOrg)
{
    uint32_t i = widthOrg + FULL_WIDTH_ADJUSTMENT;
    while (--i) {
        *yPtr++ = rgb2YFunc(*argbPtr, *(argbPtr + ARGB_GREEN_INDEX), *(argbPtr + ARGB_RED_INDEX));
        argbPtr += ALIGN_4_BYTES;
    }
}

static void ConvertARGB888ToYVU420SemiPlanar_c(const ArgbToYvu420SemiPlanarConvertParams &params)
{
    uint8_t *argb_ptr = params.inputRgb;
    uint8_t *y_p = params.outputY;
    uint8_t *vu_p = params.outputUv;
    int32_t width_org = params.widthOrg;
    int32_t height_org = params.heightOrg;
    int32_t width_dst = params.widthDst;
    if (params.inputRgb == nullptr || params.outputY == nullptr || params.outputUv == nullptr) {
        return;
    }
    if ((height_org & ALIGN_2_BYTES) != 0) {
        height_org &= ~ALIGN_2_BYTES;
    }
    if ((width_org & ALIGN_2_BYTES) != 0) {
        OMX_LOGE("width_org:%d is not supported", width_org);
        return;
    }
    if (!mConventFlag) {
        mConventFlag = true;
        inittable();
    }
    OMX_LOGI("rgb2yuv start");
    int64_t startEncode = OHOS::OMX::systemTime();
    for (uint32_t row = height_org + LOOP_DECREMENT_ONE; --row;) {
        uint8_t *y_ptr = y_p;
        y_p += width_dst;
        if (!(row & LOOP_DECREMENT_ONE)) {
            uint8_t *vu_ptr = vu_p;
            vu_p += width_dst;
            ConvertArgbLineWithUv(argb_ptr, y_ptr, vu_ptr, width_org);
        } else {
            ConvertArgbLineYOnly(argb_ptr, y_ptr, width_org);
        }
    }
    int64_t endEncode = OHOS::OMX::systemTime();
    OMX_LOGI("rgb2yuv time: %d", (unsigned int)((endEncode - startEncode) / 1000000L));
}
static void swap_uv_component(uint32_t width, uint32_t height, uint8_t* p_uv)
{
    uint32_t uv_size = (width * height) >> YUV420_UV_SCALE_FACTOR;
    uint32_t i = 0;
    uint8_t tmp = 0;
    for (i = INITIAL_COUNTER_ZERO; i < uv_size / YUV420_UV_SCALE_FACTOR; i ++) {
        tmp = p_uv[YUV420_UV_SCALE_FACTOR * i];
        p_uv[YUV420_UV_SCALE_FACTOR * i] = p_uv[YUV420_UV_SCALE_FACTOR*i + LOOP_DECREMENT_ONE];
        p_uv[YUV420_UV_SCALE_FACTOR*i + LOOP_DECREMENT_ONE] = tmp;
    }
}
#else
/*this is neon assemble function.It is need width_org align in 16Bytes.height_org align in 2Bytes*/
/*in cpu not busy status, it deal with 1280*720 rgb data in 5-6ms */
extern "C" void neon_intrinsics_ARGB888ToYVU420Semi(uint8_t *inrgb, uint8_t* outy, uint8_t* outuv,
                                                    int32_t width_org, int32_t height_org,
                                                    int32_t width_dst, int32_t height_dst);
/*this is neon c function.It is need width_org align in 2Bytes.height_org align in 2Bytes*/
/*like ConvertARGB888ToYVU420SemiPlanar function parameters requirement*/
/*in cpu not busy status, it deal with 1280*720 rgb data in 5-6ms */
struct NeonArgbToYvuCoefficients {
    uint8x8_t r1fac;
    uint8x8_t g1fac;
    uint8x8_t b1fac;
    uint8x8_t r2fac;
    uint8x8_t g2fac;
    uint8x8_t b2fac;
    uint8x8_t g3fac;
    uint8x8_t b3fac;
    uint8x8_t yBase;
    uint8x8_t uvBase;
};

static NeonArgbToYvuCoefficients InitNeonArgbToYvuCoefficients()
{
    NeonArgbToYvuCoefficients coeffs;
    coeffs.r1fac = vdup_n_u8(RGB2Y_R_COEFF);
    coeffs.g1fac = vdup_n_u8(RGB2Y_G_COEFF);
    coeffs.b1fac = vdup_n_u8(RGB2Y_B_COEFF);
    coeffs.r2fac = vdup_n_u8(-RGB2CB_R_COEFF);
    coeffs.g2fac = vdup_n_u8(-RGB2CB_G_COEFF);
    coeffs.b2fac = vdup_n_u8(RGB2CB_B_COEFF);
    coeffs.g3fac = vdup_n_u8(-RGB2CR_G_COEFF);
    coeffs.b3fac = vdup_n_u8(-RGB2CR_B_COEFF);
    coeffs.yBase = vdup_n_u8(RGB2Y_OFFSET);
    coeffs.uvBase = vdup_n_u8(RGB2CB_OFFSET);
    return coeffs;
}

static void ConvertOneNeonBlock(uint8_t *&argbPtr, uint8_t *&yPtr, uint8_t *&uvPtr, int32_t linecount,
    const NeonArgbToYvuCoefficients &coeffs)
{
    uint16x8_t temp;
    uint8x8_t result;
    uint8x8x2_t result_uv;
    uint8x8_t result_u;
    uint8x8_t result_v;
    uint8x8x4_t argb = vld4_u8(argbPtr);
    temp = vmull_u8(argb.val[ARGB_RED_INDEX], coeffs.r1fac);
    temp = vmlal_u8(temp, argb.val[ARGB_GREEN_INDEX], coeffs.g1fac);
    temp = vmlal_u8(temp, argb.val[ARGB_BLUE_INDEX], coeffs.b1fac);
    result = vshrn_n_u16(temp, UV_SHIFT_FACTOR);
    result = vadd_u8(result, coeffs.yBase);
    vst1_u8(yPtr, result);
    if (linecount % YUV420_UV_SCALE_FACTOR == INITIAL_COUNTER_ZERO) {
        temp = vmull_u8(argb.val[ARGB_BLUE_INDEX], coeffs.b2fac);
        temp = vmlsl_u8(temp, argb.val[ARGB_GREEN_INDEX], coeffs.g2fac);
        temp = vmlsl_u8(temp, argb.val[ARGB_RED_INDEX], coeffs.r2fac);
        result_u = vshrn_n_u16(temp, UV_SHIFT_FACTOR);
        result_u = vadd_u8(result_u, coeffs.uvBase);
        temp = vmull_u8(argb.val[ARGB_RED_INDEX], coeffs.b2fac);
        temp = vmlsl_u8(temp, argb.val[ARGB_GREEN_INDEX], coeffs.g3fac);
        temp = vmlsl_u8(temp, argb.val[ARGB_BLUE_INDEX], coeffs.b3fac);
        result_v = vshrn_n_u16(temp, UV_SHIFT_FACTOR);
        result_v = vadd_u8(result_v, coeffs.uvBase);
        result_uv = vtrn_u8(result_v, result_u);
        vst1_u8(uvPtr, result_uv.val[INITIAL_COUNTER_ZERO]);
        uvPtr += NEON_PROCESS_ELEMENTS;
    }
    yPtr += NEON_PROCESS_ELEMENTS;
    argbPtr += NEON_LOAD_BYTES;
}

static void NeonIntrinsicsArgB888ToYvU420SemiC(const ArgbToYvu420SemiPlanarConvertParams &params)
{
    int32_t i = 0;
    int32_t j = 0;
    uint8_t *argb_ptr = params.inputRgb;
    uint8_t *y_ptr = params.outputY;
    uint8_t *uv_ptr = params.outputUv;
    int32_t width_org = params.widthOrg;
    int32_t height_org = params.heightOrg;
    int32_t width_dst = params.widthDst;
    NeonArgbToYvuCoefficients coeffs = InitNeonArgbToYvuCoefficients();
    bool needadjustPos = true;
    //due to width_dst is align in 16Bytes.so if width_org is align in 16bytes.no need adjust pos.
    if (width_org%ALIGN_16_BYTES == INITIAL_COUNTER_ZERO) {
        needadjustPos = false;
    }
    int32_t linecount=INITIAL_COUNTER_ZERO;
    for (i = height_org; i > INITIAL_COUNTER_ZERO; i--) {  /////  line
        for (j = (width_org >> (YUV420_UV_SCALE_FACTOR + LOOP_DECREMENT_ONE)); j > INITIAL_COUNTER_ZERO; j--) {  // col
            ConvertOneNeonBlock(argb_ptr, y_ptr, uv_ptr, linecount, coeffs);
        }
        linecount++;
        if (needadjustPos) {
            //note:let y, uv, argb get correct position before operating next line.
            y_ptr = params.outputY + width_dst * linecount;
            uv_ptr = params.outputUv + width_dst * (linecount / YUV420_UV_SCALE_FACTOR);
            argb_ptr = params.inputRgb + ALIGN_4_BYTES * width_org * linecount;
        }
    }
}
static void ConvertArgB888ToYvU420SemiPlanarNeon(const ArgbToYvu420SemiPlanarConvertParams &params)
{
    int32_t width_org = params.widthOrg;
    int32_t height_org = params.heightOrg;
    if (params.inputRgb == nullptr || params.outputUv == nullptr || params.outputY == nullptr) {
        return;
    }
        
    if ((height_org & ALIGN_2_BYTES) != INITIAL_COUNTER_ZERO) {
        height_org &= ~ALIGN_2_BYTES;
    }
        
    if ((width_org & ALIGN_2_BYTES) != INITIAL_COUNTER_ZERO) {
        OMX_LOGE("width_org:%d is not supported", width_org);
        return;
    }
    int64_t startEncode = OHOS::OMX::systemTime();
    NeonIntrinsicsArgB888ToYvU420SemiC(params);
    int64_t endEncode = OHOS::OMX::systemTime();
    OMX_LOGI("wfd: ConvertArgB888ToYvU420SemiPlanarNeon:  rgb2yuv cost time: %d",
        (unsigned int)((endEncode - startEncode) / 1000000L));
}
static void swap_uv_component(uint32_t width, uint32_t height, uint8_t* p_uv)
{
    uint32_t uv_size = (width * height) >> YUV420_UV_SCALE_FACTOR;
    uint8x16_t data0 = vdupq_n_u8(0);
    uint8x16_t data1 = vdupq_n_u8(0);
    uint32_t i = 0;
    for (i = INITIAL_COUNTER_ZERO; i < uv_size; i += ALIGN_16_BYTES) {
        data0 = vld1q_u8(p_uv+i);
        data1 = vrev16q_u8(data0);
        vst1q_u8(p_uv+i, data1);
    }
}
#endif
static void ConvertARGB888ToYVU420SemiPlanar(const ArgbToYvu420SemiPlanarConvertParams &params)
{
#ifdef CONFIG_BIA_SUPPORT
    ConvertARGB888ToYVU420SemiPlanar_c(params);
#else
    ConvertArgB888ToYvU420SemiPlanarNeon(params);
#endif
}
#ifdef VIDEOENC_CURRENT_OPT
/**
 * @brief Set DDR frequency by writing to sysfs control file
 *
 * @param freqInKhz Frequency value in kHz as string (e.g., "933000")
 * @return true if frequency was successfully set, false otherwise
 */
static bool SetDdrFrequency(const char *freqInKhz)
{
    // Validate input parameter
    if (freqInKhz == nullptr || freqInKhz[0] == '\0') {
        OMX_LOGE("Invalid frequency parameter: null or empty string");
        return false;
    }
    // Basic validation: ensure it's a numeric string
    for (const char *p = freqInKhz; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            OMX_LOGE("Invalid frequency value '%s': must contain only digits", freqInKhz);
            return false;
        }
    }
    const char *const setFreq = "/sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/setFreq";
    FILE* fp = fopen(setFreq, "w");
    if (fp == nullptr) {
        OMX_LOGE("Failed to open %s: %s", setFreq, strerror(errno));
        return false;
    }
    bool success = false;
    int written = fprintf(fp, "%s", freqInKhz);
    if (written <= 0) {
        OMX_LOGE("Failed to write frequency to %s: %s", setFreq, strerror(errno));
    } else {
        OMX_LOGI("Successfully set DDR frequency to %skHz", freqInKhz);
        success = true;
    }
    // Always close the file
    if (fclose(fp) != 0) {
        OMX_LOGW("Failed to close %s: %s", setFreq, strerror(errno));
        // Don't treat close failure as operation failure if write succeeded
    }
    
    return success;
}

// Maintain backward compatibility with old function name
static void SetDdrFreq(const char *freqInKhz)
{
    SetDdrFrequency(freqInKhz);
}
#endif
#endif  // SPRD_VIDEO_ENCODER_UTILS_H_
