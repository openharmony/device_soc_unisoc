/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "hdi_gfx_composition.h"
#include <cinttypes>
#include <dlfcn.h>
#include "v1_0/display_composer_type.h"
using namespace OHOS::HDI::Display::Composer::V1_0;

namespace OHOS {
namespace HDI {
namespace DISPLAY {

namespace {
const uint32_t MASK_SCALING = 0x01;
#ifdef SPRD_7863
const uint32_t MASK_TRANSFORM = 0x02;
const uint32_t MASK_YUV = 0x04;
const uint32_t MASK_SCALING_TRANSFORM = MASK_SCALING | MASK_TRANSFORM;
const uint32_t MASK_ALL = MASK_SCALING | MASK_TRANSFORM | MASK_YUV;
const uint32_t DPU_LAYERS_COMPLEX_LIMIT = 5;
#endif
const uint32_t DPU_LAYERS_MAX = 4;
const uint32_t DPU_7885_LAYERS_MAX = 8;
}
int32_t HdiGfxComposition::Init(void)
{
    DISPLAY_LOGD();
    int32_t ret = GfxModuleInit();
    if ((ret != DISPLAY_SUCCESS) || (mGfxFuncs == nullptr)) {
        DISPLAY_LOGE("GfxModuleInit failed will use client composition always");
        return DISPLAY_SUCCESS;
    }
    ret = mGfxFuncs->InitGfx();
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("gfx init failed"));
    if (ret != DISPLAY_SUCCESS) {
        DISPLAY_LOGE("Failed to init gfx will use client composition always");
        return DISPLAY_SUCCESS;
    }
    valid_ = true;
    return DISPLAY_SUCCESS;
}

int32_t HdiGfxComposition::GfxModuleInit(void)
{
    DISPLAY_LOGD();
    mGfxModule = dlopen(libHdiGfxName, RTLD_NOW | RTLD_NOLOAD);
    if (mGfxModule != nullptr) {
        DISPLAY_LOGI("Module '%{public}s' already loaded", libHdiGfxName);
    } else {
        DISPLAY_LOGI("Loading module '%{public}s'", libHdiGfxName);
        mGfxModule = dlopen(libHdiGfxName, RTLD_NOW);
        if (mGfxModule == nullptr) {
            DISPLAY_LOGE("Failed to load module: %{public}s", dlerror());
            return DISPLAY_FAILURE;
        }
    }

    using InitFunc = int32_t (*)(GfxFuncs **funcs);
    InitFunc func = reinterpret_cast<InitFunc>(dlsym(mGfxModule, libGfxFuncInit));
    if (func == nullptr) {
        DISPLAY_LOGE("Failed to lookup %{public}s function: %s", libGfxFuncInit, dlerror());
        dlclose(mGfxModule);
        return DISPLAY_FAILURE;
    }
    return func(&mGfxFuncs);
}

int32_t HdiGfxComposition::GfxModuleDeinit(void)
{
    DISPLAY_LOGD();
    int32_t ret = DISPLAY_SUCCESS;
    if (mGfxModule == nullptr) {
        using DeinitFunc = int32_t (*)(GfxFuncs *funcs);
        DeinitFunc func = reinterpret_cast<DeinitFunc>(dlsym(mGfxModule, libGfxFuncDeinit));
        if (func == nullptr) {
            DISPLAY_LOGE("Failed to lookup %{public}s function: %s", libGfxFuncDeinit, dlerror());
        } else {
            ret = func(mGfxFuncs);
        }
        dlclose(mGfxModule);
    }
    return ret;
}

int32_t HdiGfxComposition::CheckLayers(std::vector<HdiLayer *> &layers, uint32_t index)
{
    int mask = 0;
    HdiLayer *layer;
    for (uint32_t i = index; i < layers.size(); i++) {
        layer = layers[i];
        if ((layer->GetLayerCrop().w != layer->GetLayerDisplayRect().w) ||
            (layer->GetLayerCrop().h != layer->GetLayerDisplayRect().h)) {
            mask |= MASK_SCALING;
        }
#ifdef SPRD_7863
        if (layer->GetTransFormType()) {
            mask |= MASK_TRANSFORM;
        }
        if (layer->GetCurrentBuffer()->GetFormat() >= PIXEL_FMT_YUV_422_I) {
            mask |= MASK_YUV;
        }
#endif
    }
    return mask;
}

bool HdiGfxComposition::CanHandle(HdiLayer &hdiLayer)
{
    DISPLAY_LOGD();
    (void)hdiLayer;
    return valid_;
}

int32_t HdiGfxComposition::SetLayers(std::vector<HdiLayer *> &layers, HdiLayer &clientLayer)
{
    DISPLAY_LOGD("layers size %{public}zd", layers.size());
    mClientLayer = &clientLayer;
    mCompLayers.clear();
    int32_t mask = CheckLayers(layers, 0);
    mClientLayer->SetAcceleratorType(ACCELERATOR_NON);
    HdiLayer *layer;
    uint32_t dpuSize = 0;
    for (uint32_t i = 0; i < layers.size(); i++) {
        layer = layers[i];
        if (CanHandle(*layer)) {
            if ((layer->GetCompositionType() != COMPOSITION_VIDEO) &&
                (layer->GetCompositionType() != COMPOSITION_CURSOR)) {
                SetLayerAccelerator(layer, layers, i, mask, dpuSize);
            } else {
                layer->SetDeviceSelect(layer->GetCompositionType());
            }
            mCompLayers.push_back(layer);
        } else {
            layer->SetDeviceSelect(COMPOSITION_CLIENT);
            mClientLayer->SetAcceleratorType(ACCELERATOR_DPU);
            dpuSize = 1;
        }
    }
    DISPLAY_LOGD("composer layers size %{public}zd dpuSize=%{public}d mask=%{public}d",
        mCompLayers.size(), dpuSize, mask);
    return DISPLAY_SUCCESS;
}

void HdiGfxComposition::SetLayerAccelerator(HdiLayer *layer, std::vector<HdiLayer *> &layers,
                                            uint32_t i, int32_t mask, uint32_t &dpuSize)
{
#ifdef SPRD_7863
    if (!(mask & MASK_SCALING_TRANSFORM) && (layers.size() < DPU_LAYERS_MAX))
#else
    if ((mask == 0) && (layers.size() < DPU_LAYERS_MAX)) // 直接给DPU处理
#endif
    {
        layer->SetAcceleratorType(ACCELERATOR_DPU);
        layer->SetDeviceSelect(COMPOSITION_DEVICE);
        dpuSize++;
        return;
    }
#ifdef SPRD_7863
    else if ((mask & MASK_ALL) == MASK_SCALING_TRANSFORM) {
        layer->SetAcceleratorType(ACCELERATOR_GPU);
        layer->SetDeviceSelect(COMPOSITION_CLIENT);
        if (mClientLayer->GetAcceleratorType() == ACCELERATOR_NON) {
            dpuSize++;
            mClientLayer->SetAcceleratorType(ACCELERATOR_DPU); // 输出结果给DPU显示
        }
        return;
    }
#endif
    else {
        SetComplexLayerAccelerator(layer, layers, i, dpuSize);
    }
}

void HdiGfxComposition::SetComplexLayerAccelerator(HdiLayer *layer, std::vector<HdiLayer *> &layers,
                                                   uint32_t i, uint32_t &dpuSize)
{
    // GSP+DPU
    int32_t tempMask = CheckLayers(layers, i);
    uint32_t tempSize = layers.size() - i;
#ifdef SPRD_7863
    if (tempMask & MASK_SCALING_TRANSFORM) {
#else
    if (tempMask != 0) { // 复杂场景交给GSP
#endif
        layer->SetAcceleratorType(ACCELERATOR_DPU);
        layer->SetDeviceSelect(COMPOSITION_DEVICE);
        dpuSize++;
        return;
    }

#ifdef SPRD_7863
    bool canInsert = (dpuSize + tempSize) < DPU_LAYERS_COMPLEX_LIMIT;
#else
    bool canInsert = (dpuSize + tempSize) < DPU_7885_LAYERS_MAX;
#endif

    if (canInsert) {
        layer->SetAcceleratorType(ACCELERATOR_DPU);
        layer->SetDeviceSelect(COMPOSITION_DEVICE);
        dpuSize++;
        return;
    }

    layer->SetAcceleratorType(ACCELERATOR_GSP);
    layer->SetDeviceSelect(COMPOSITION_DEVICE);
    if (mClientLayer->GetAcceleratorType() == ACCELERATOR_NON) {
        dpuSize++;
        mClientLayer->SetAcceleratorType(ACCELERATOR_DPU); // 输出结果给DPU显示
    }
}

void HdiGfxComposition::InitGfxSurface(ISurface &surface, HdiLayerBuffer &buffer)
{
    surface.width = buffer.GetWidth();
    surface.height = buffer.GetHeight();
    surface.phyAddr = buffer.GetMemHandle();
    surface.enColorFmt = (PixelFormat)buffer.GetFormat();
    surface.stride = buffer.GetStride();
    surface.bAlphaExt1555 = true;
    surface.bAlphaMax255 = true;
    surface.alpha0 = 0XFF;
    surface.alpha1 = 0XFF;
    DISPLAY_LOGD("surface w:%{public}d h:%{public}d addr:0x%{public}" PRIx64 " fmt:%{public}d stride:%{public}d",
        surface.width, surface.height, surface.phyAddr, surface.enColorFmt, surface.stride);
}

// now not handle the alpha of layer
int32_t HdiGfxComposition::BlitLayer(HdiLayer &src, HdiLayer &dst, uint32_t index, uint32_t max, uint32_t zorder)
{
    ISurface srcSurface = { 0 };
    ISurface dstSurface = { 0 };
    GfxOpt opt = { 0 };
    SprdGfxOpt sprdOpt = { 0 };
    sprdOpt.opt = &opt;
    DISPLAY_LOGD();
    HdiLayerBuffer *srcBuffer = src.GetCurrentBuffer();
    DISPLAY_CHK_RETURN((srcBuffer == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("the srcbuffer is null"));
    DISPLAY_LOGD("init the src surface");
    InitGfxSurface(srcSurface, *srcBuffer);

    HdiLayerBuffer *dstBuffer = dst.GetCurrentBuffer();
    DISPLAY_CHK_RETURN((dstBuffer == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("can not get client layer buffer"));
    DISPLAY_LOGD("init the dst surface");
    InitGfxSurface(dstSurface, *dstBuffer);

    sprdOpt.opt->blendType = src.GetLayerBlenType();
    DISPLAY_LOGD("blendType %{public}d", opt.blendType);
    sprdOpt.opt->enPixelAlpha = true;
    sprdOpt.opt->enableScale = true;

    sprdOpt.index = index;
    sprdOpt.maxCnt = max;
    sprdOpt.zOrder = zorder;

    if (src.GetAlpha().enGlobalAlpha) { // is alpha is 0xff we not set it
        sprdOpt.opt->enGlobalAlpha = true;
        srcSurface.alpha0 = src.GetAlpha().gAlpha;
        DISPLAY_LOGD("src alpha %{public}x", src.GetAlpha().gAlpha);
    }
    sprdOpt.opt->rotateType = src.GetTransFormType();
    DISPLAY_LOGD(" the roate type is %{public}d", opt.rotateType);
    IRect crop = src.GetLayerCrop();
    IRect displayRect = src.GetLayerDisplayRect();
    DISPLAY_LOGD("crop x: %{public}d y : %{public}d w : %{public}d h: %{public}d", crop.x, crop.y, crop.w, crop.h);
    DISPLAY_LOGD("displayRect x: %{public}d y : %{public}d w : %{public}d h : %{public}d", displayRect.x, displayRect.y,
        displayRect.w, displayRect.h);
    DISPLAY_CHK_RETURN(mGfxFuncs == nullptr, DISPLAY_FAILURE, DISPLAY_LOGE("Blit: mGfxFuncs is null"));
    return mGfxFuncs->Blit(&srcSurface, &crop, &dstSurface, &displayRect, (GfxOpt*)&sprdOpt);
}

int32_t HdiGfxComposition::ClearRect(HdiLayer &src, HdiLayer &dst)
{
    ISurface dstSurface = { 0 };
    GfxOpt opt = { 0 };
    DISPLAY_LOGD();
    HdiLayerBuffer *dstBuffer = dst.GetCurrentBuffer();
    DISPLAY_CHK_RETURN((dstBuffer == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("can not get client layer buffer"));
    InitGfxSurface(dstSurface, *dstBuffer);
    IRect rect = src.GetLayerDisplayRect();
    DISPLAY_CHK_RETURN(mGfxFuncs == nullptr, DISPLAY_FAILURE, DISPLAY_LOGE("Rect: mGfxFuncs is null"));
    return mGfxFuncs->FillRect(&dstSurface, &rect, 0, &opt);
}

int32_t HdiGfxComposition::Apply(bool modeSet)
{
#ifdef HIHOPE_OS_DEBUG
    HITRACE_METER_NAME(HITRACE_TAG_GRAPHIC_AGP, "HdiGfxComposition::Apply");
#endif
    int32_t ret;
    DISPLAY_LOGD("composer layers size %{public}zd", mCompLayers.size());
    for (uint32_t i = 0; i < mCompLayers.size(); i++) {
        HdiLayer *layer = mCompLayers[i];
        CompositionType compType = layer->GetDeviceSelect();
        switch (compType) {
            case COMPOSITION_VIDEO:
                ret = ClearRect(*layer, *mClientLayer);
                DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE,
                    DISPLAY_LOGE("clear layer %{public}d failed", i));
                break;
            case COMPOSITION_DEVICE:
                ret = BlitLayer(*layer, *mClientLayer, i, mCompLayers.size(), layer->GetZorder());
                DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE,
                    DISPLAY_LOGE("blit layer %{public}d failed ", i));
                break;
            default:
                DISPLAY_LOGE("the gfx composition can not surpport the type %{public}d", compType);
                break;
        }
    }
    return DISPLAY_SUCCESS;
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
