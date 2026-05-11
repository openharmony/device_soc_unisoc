# Display Adaptation

-   [Introduction](#section01)
-   [Display HDI Adaptation](#section02)
-   [GPU Adaptation](#section03)

## Introduction<a name="section01"></a>

This repository is used to store the display adaptation content for Spreadtrum chips.

The display adaptation requires completion of the following tasks: LCD driver adaptation, graphics service HDI interface adaptation, and GPU adaptation.

The LCD driver adaptation for WUKONG100 uses the native Linux driver. For details, refer to the kernel adaptation process.

## Display HDI Adaptation<a name="section02"></a>

[显示HDI](https://gitee.com/openharmony/drivers_peripheral/blob/master/display/README_zh.md)The Display HDI provides display driver capabilities to the graphics service, including display layer management, display memory management, and hardware acceleration. The Display HDI requires adaptation in two parts: gralloc and display_device. The display_device also includes hardware composition management such as display_gfx. The overall architecture is shown in the following figure:

![](figures/display框架图.png)

The directory structure is as follows:

```shell
display
├── BUILD.gn
├── include
└── src
    ├── display_device         # 显示设备管理、layer管理等
    │   ├── core
    │   ├── drm
    │   ├── fbdev
    │   └── vsync
    ├── display_gfx            # gsp硬件合成
    │   ├── display_gfx.cpp
    │   └── gsp
    │       ├── include
    │       └── src
    ├── display_gralloc        # allocator显示内存管理
    ├── display_layer_video
    └── utils
```

### Gralloc Adaptation

The gralloc module provides display memory management functionality. OpenHarmony provides a reference implementation for Hi3516DV300, which vendors can refer to for adaptation based on their actual situation. This implementation is based on drm development. [Source code link.](https://gitee.com/openharmony/drivers_peripheral/tree/master/display/hal/default_standard)。

The display memory management for Spreadtrum P7885 refers to the implementation of RK3568, relying on Spreadtrum's proprietary allocator implementation. The proprietary allocator is encapsulated to provide libdisplay_buffer_vdi_impl.z.so for the allocator_host service to call.

###  Display Device Adaptation

The display device module provides display device management, layer management, hardware acceleration, and other functions.

OpenHarmony provides[a reference implementation based on drm for the Hi3516DV300 chip](https://gitee.com/openharmony/drivers_peripheral/tree/master/display/hal/default_standard/src/display_device),which supports hardware composition by default.

The display device module for Spreadtrum P7885 refers to the implementation of RK3568 and supports hardware composition by default.

OpenHarmony has initially implemented hardware composition using GSP and DPU for Spreadtrum P7885, but GPU hardware composition is not yet supported. The hardware composition strategy can be referred to in the set_layers method in the file:

//drivers_peripheral/display/hal/default_standard/src/display_device/hdi_gfx_composition.cpp

```
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
                (layer->GetCompositionType() != COMPOSITION_POINTER)) {
#ifdef SPRD_7863
                if(!(mask & 0x03) && (layers.size() < 4))
#else
                if((mask == 0) && (layers.size() < 4))
#endif
                {
                    layer->SetAcceleratorType(ACCELERATOR_DPU);
                    layer->SetDeviceSelect(COMPOSITION_DEVICE);
                    dpuSize++;
                    continue;
                }
#ifdef SPRD_7863
                else if ((mask & 0x07) == 0x03)
                {   
                    layer->SetAcceleratorType(ACCELERATOR_GPU);
                    layer->SetDeviceSelect(COMPOSITION_CLIENT);
                    if(mClientLayer->GetAcceleratorType() == ACCELERATOR_NON)
                    {
                        dpuSize++;
                        mClientLayer->SetAcceleratorType(ACCELERATOR_DPU);
                    }
                    continue;
                }
#endif
                else
                {
                    //GSP+DPU
                    int32_t tempMask = CheckLayers(layers, i);
                    uint32_t tempSize = layers.size() - i;
#ifdef SPRD_7863
                    if(tempMask & 0x03)
#else
                    if(tempMask)
#endif
                    {
                        layer->SetAcceleratorType(ACCELERATOR_GSP);
                        layer->SetDeviceSelect(COMPOSITION_DEVICE);
                        if(mClientLayer->GetAcceleratorType() == ACCELERATOR_NON)
                        {
                            dpuSize++;
                            mClientLayer->SetAcceleratorType(ACCELERATOR_DPU);
                        }
                    }
                    else
                    {
#ifdef SPRD_7863
                        if((dpuSize + tempSize) < 5)
#else
                        if((dpuSize + tempSize) < 7)
#endif
                        {
                            layer->SetAcceleratorType(ACCELERATOR_DPU);
                            layer->SetDeviceSelect(COMPOSITION_DEVICE);
                            dpuSize++;
                            continue;
                        }
                        else
                        {
                            layer->SetAcceleratorType(ACCELERATOR_GSP);
                            layer->SetDeviceSelect(COMPOSITION_DEVICE);
                            if(mClientLayer->GetAcceleratorType() == ACCELERATOR_NON)
                            {
                                dpuSize++;
                                mClientLayer->SetAcceleratorType(ACCELERATOR_DPU);
                            }
                        }
                    }
                }
                
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
    DISPLAY_LOGD("composer layers size %{public}zd dpuSize=%{public}d mask=%{public}d", mCompLayers.size(), dpuSize, mask);
    return DISPLAY_SUCCESS;
}
```

Currently, layers with rotation or scaling use GSP hardware for serial composition, which may affect performance.

The implementation of GSP composition relies on Spreadtrum's closed-source library. gfx_composition implements calls to the closed-source library. Refer to the GfxModuleInit method in the file:

```c++
int32_t HdiGfxComposition::GfxModuleInit(void)
{
    DISPLAY_LOGD();
    mGfxModule = dlopen(LIB_HDI_GFX_NAME, RTLD_NOW | RTLD_NOLOAD);
    if (mGfxModule != nullptr) {
        DISPLAY_LOGI("Module '%{public}s' already loaded", LIB_HDI_GFX_NAME);
    } else {
        DISPLAY_LOGI("Loading module '%{public}s'", LIB_HDI_GFX_NAME);
        mGfxModule = dlopen(LIB_HDI_GFX_NAME, RTLD_NOW);
        if (mGfxModule == nullptr) {
            DISPLAY_LOGE("Failed to load module: %{public}s", dlerror());
            return DISPLAY_FAILURE;
        }
    }

    using InitFunc = int32_t (*)(GfxFuncs **funcs);
    InitFunc func = reinterpret_cast<InitFunc>(dlsym(mGfxModule, LIB_GFX_FUNC_INIT));
    if (func == nullptr) {
        DISPLAY_LOGE("Failed to lookup %{public}s function: %s", LIB_GFX_FUNC_INIT, dlerror());
        dlclose(mGfxModule);
        return DISPLAY_FAILURE;
    }
    return func(&mGfxFuncs);
}
```

### Test Verification

hello_composer test module: A test program provided by the Rosen graphics framework, primarily used to verify whether the main display process and HDI interfaces are functioning properly. It is compiled with the system by default.

```
foundation/graphic/graphic_2d/rosen/samples/composer
├── BUILD.gn
├── hello_composer.cpp
├── hello_composer.h
├── layer_context.cpp
├── layer_context.h
└── main.cpp
```

Specific verification steps:

1. Stop the render service

  ```
  service_control stop render_service
  ```

2. Stop the foundation process

  ```
  service_control stop foundation
  ```

3. Run hello_composer to test the relevant interfaces

```
./hello_composer
```



## GPU Adaptation<a name="section03"></a>

Spreadtrum P7885 provides a closed-source implementation of the GPU driver. Referring to the implementation of RK3568, the closed-source library is linked. Reference example:

```shell
ohos_prebuilt_shared_library( "GLES_mali" ){
    source = "${GPU_LIB_PATH}/lib64/libGLES_mali.z.so"
    relative_install_dir = "chipsetsdk"
    symlink_target_name = [
        "libEGL_impl.so",
        "libGLESv1_impl.so",
        "libGLESv2_impl.so",
        "libGLESv3_impl.so",
        "libmali.so.0",
        "libmali.so.1",
    ]
    install_enable = true
    install_images = [ chipset_base_dir ]
    part_name = "${PART_NAME}"
    subsystem_name = "${SUBSYSTEM_NAME}"
}
```
