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

#ifndef HDI_GFX_COMPOSITION_H
#define HDI_GFX_COMPOSITION_H
#include "display_gfx.h"
#include "hdi_composer.h"
namespace OHOS {
namespace HDI {
namespace DISPLAY {
enum ScaleCapType {
    SCALE_CAP_DPU = 0,    // <= 2x scale-down, DPU can handle
    SCALE_CAP_GSP = 1,    // > 2x and <= 4x scale-down, DPU cannot handle, GSP can handle
    SCALE_CAP_CLIENT = 2  // > 4x scale-down, neither DPU nor GSP can handle, fallback to Client
};

class HdiGfxComposition : public HdiComposition {
public:
    int32_t Init(void) override;
    int32_t SetLayers(std::vector<HdiLayer *> &layers, HdiLayer &clientLayer) override;
    int32_t Apply(bool modeSet) override;
    ~HdiGfxComposition() override
    {
        (void)GfxModuleDeinit();
    }

private:
    bool CanHandle(HdiLayer &hdiLayer);
    int32_t CheckLayers(std::vector<HdiLayer *> &layers, uint32_t index);
    ScaleCapType CheckLayerScaleCapability(const HdiLayer &layer);
    void SetLayerAccelerator(HdiLayer *layer, std::vector<HdiLayer *> &layers,
                             uint32_t i, int32_t mask, uint32_t &dpuSize);
    void SetComplexLayerAccelerator(HdiLayer *layer, std::vector<HdiLayer *> &layers,
                                    uint32_t i, int32_t mask, uint32_t &dpuSize);
    void InitGfxSurface(ISurface &surface, HdiLayerBuffer &buffer);
    int32_t BlitLayer(HdiLayer &src, HdiLayer &dst, uint32_t index, uint32_t max, uint32_t zorder);
    int32_t ClearRect(HdiLayer &src, HdiLayer &dst);
    int32_t GfxModuleInit(void);
    int32_t GfxModuleDeinit(void);
    void *mGfxModule = nullptr;
    GfxFuncs *mGfxFuncs = nullptr;
    HdiLayer *mClientLayer;
    static constexpr const char* LIB_HDI_GFX_NAME = "libdisplay_gfx.z.so";
    static constexpr const char* LIB_GFX_FUNC_INIT = "GfxInitialize";
    static constexpr const char* LIB_GFX_FUNC_DEINIT = "GfxUninitialize";
    static constexpr float DPU_MAX_SCALE_DOWN_FACTOR = 2.0f;
    static constexpr float GSP_MAX_SCALE_DOWN_FACTOR = 2.0f;
    bool valid_ = false;
};
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY

#endif // HDI_GFX_COMPOSITION_H
