# 展锐7885芯片 NPU适配OpenHarmony

## OpenHarmony AI整体架构

### AI架构图

![oh-AI](figures/oh-AI.png)

### AI架构介绍

整个架构主要分为三层：

- AI应用位于应用层；
- AI推理框架MindSpore Liet和NNRt位于系统层；
- 设备服务位于芯片层。

AI应用要在专用加速芯片上完成模型推理，需要经过AI推理框架和NNRt才能调用到底层的芯片设备，NNRt负责适配底层各种芯片设备，它开放了标准统一的南向接口，第三方芯片设备都可以通过HDI接口接入OHOS。

## 端侧推理引擎 -- MindSpore Lite

MindSpore Lite是OpenHarmony内置的端侧AI推理引擎，为开发者提供统一的AI推理接口，可以完成在手机等端侧设备中的模型推理过程。目前已经在图像分类、目标识别、文字识别等应用中广泛使用。

基于MindSpore Lite进行AI应用的开发有两种方式：

- 方式一：[使用MindSpore Lite JS API开发AI应用](https://gitee.com/openharmony/docs/blob/OpenHarmony-4.0-Release/zh-cn/application-dev/ai/mindspore-guidelines-based-js.md)，直接在UI代码中调用 MindSpore Lite JS API 加载模型并进行AI模型推理，此方式可快速验证效果。

- 方式二：[使用MindSpore Lite Native API开发AI应用](https://gitee.com/openharmony/docs/blob/OpenHarmony-4.0-Release/zh-cn/application-dev/ai/mindspore-guidelines-based-native.md)，将算法模型和调用 MindSpore Lite Native API 的代码封装成动态库，并通过N-API封装成JS接口，供UI调用。

 MindSpore Lite已经通过Delegate机制接入Neural Network Runtime，MindSpore Lite可以调度支持NNRt的硬件加速芯片参与到推理过程。

## Neural Network Runtime

Neural Network Runtime（NNRt, 神经网络运行时）作为上层AI推理引擎和底层加速芯片的桥梁，对上为AI推理引擎提供精简的Native接口，对下提供统一AI芯片驱动接口，满足推理引擎通过加速芯片执行端到端推理的需求，实现AI模型的跨芯片推理计算，使AI芯片驱动能够接入OpenHarmony系统。

### Neural Network Runtime HDI接口
![nnrt-hdi](figures/nnrt-hdi.png)

NNRt通过HDI接口跨进程通信实现与设备芯片的对接。程序运行时，AI应用、AI推理框架和NNRt都在同一个进程，底层设备服务在另一个进程，进程间是通过IPC的机制通信，NNRt根据南向HDI接口实现了HDI Client，服务端也需要根据南向HDI接口实现HDI Service。

NNRt  HDI接口文件的位置如下：

```
/drivers/interface/nnrt/
├── bundle.json
├── v1_0
│   ├── BUILD.gn
│   ├── INnrtDevice.idl
│   ├── IPreparedModel.idl
│   ├── ModelTypes.idl
│   ├── NnrtTypes.idl
│   └── NodeAttrTypes.idl
└── v2_0
    ├── BUILD.gn
    ├── INnrtDevice.idl       # 定义设备相关接口
    ├── IPreparedModel.idl    # 定义AI模型相关接口
    ├── ModelTypes.idl        # AI模型相关结构体的定义
    ├── NnrtTypes.idl         # Nnrt数据类型的定义
    └── NodeAttrTypes.idl     # AI模型算子属性定义
```

接口介绍文档目录路径：[NNRt HDI 接口](https://gitee.com/openharmony/drivers_interface/tree/master/nnrt)

> 注：使用的是v2.0接口。

对于需要接入OpenHarmony的AI芯片设备，需要根据实际的硬件能力适配上述HDI接口，实现NNRt的HDI服务。

## 展锐NPU的HDI适配

### NNRt HDI服务的实现

在```/device/soc/unisoc/p7885/hardware/npu/```目录下新建名为nnrt的目录，用于实现HDI服务。

目录结构如下所示

```
device/soc/unisoc/p7885/hardware/npu/nnrt
├── BUILD.gn
├── include
│   ├── innrt_device_vdi.h              # NNRt对接NPU芯片的接口头文件
│   └── nnrt_common.h                   # 主要定义NNRt hilog的头文件
├── nnrt_device_vdi_impl.cpp
├── prepared_model_impl_2_0.cpp
└── prepared_model_impl_2_0.h
```
```
device/soc/unisoc/p7885/hardware/npu/nnrt_drivers
└── v2_0
    ├── BUILD.gn
    ├── include
    │   └── nnrt_device_service.h       # 设备服务端头文件
    └── src
        ├── nnrt_device_driver.cpp      # 设备驱动实现文件
        └── nnrt_device_service.cpp     # 设备服务端实现文件      
```

这里按照HDF框架实现了nnrt_host进程，向上层提供了NPU硬件推理的服务。

通过调用HDI接口libnnrt_vdi_impl.z.so中对应的接口来完成从Openharmony NNRt HDI接口到展锐7885 NPU芯片功能的转换。

在nnrt_device_service.cpp中，NnrtDeviceService的构造函数中通过dlopen方式加载了libnnrt_vdi_impl.z.so。NnrtDeviceService提供的所有HDI接口都是调用vdi的函数来完成的。

```
NnrtDeviceService::NnrtDeviceService()
NnrtDeviceService::NnrtDeviceService()
    : libHandle_(nullptr),
    createVdiFunc_(nullptr),
    destroyVdiFunc_(nullptr),
    vdiImpl_(nullptr)
{
    int32_t ret = LoadVdi();
    if (ret == HDF_SUCCESS) {
        vdiImpl_ = createVdiFunc_();
        if (NnrtCheckNullPointerOrReturnValue(vdiImpl_, __func__, __FILE__, __LINE__)) {
            return;
        }
    } else {
        HDF_LOGE("Load nnrt device VDI failed, lib: %{public}s", NNRT_DEVICE_VDI_LIBRARY);
    }
}
```

### 编译驱动和HDI服务的实现文件为共享库

在/device/soc/unisoc/p7885/hardware/npu/nnrt_drivers/v2_0/下新建BUILD.gn文件，文件内容如下所示

```
import("//build/ohos.gni")
import("//device/soc/unisoc/p7885/soc_common.gni")
import("//drivers/hdf_core/adapter/uhdf2/uhdf.gni")

ohos_shared_library("libnnrt_device_service_2.0") {
  sources = [ "src/nnrt_device_service.cpp" ]

  include_dirs = [
    "include",
    "../../nnrt/include",
  ]

  external_deps = [
    "c_utils:utils",
    "drivers_interface_nnrt:libnnrt_stub_2.0",
    "hdf_core:libhdf_utils",
    "hdf_core:libhdi",
    "hilog:libhilog",
    "ipc:ipc_core",
  ]

  install_images = [ chipset_base_dir ]
  subsystem_name = "soc_p7885"
  part_name = "soc_p7885"
}

ohos_shared_library("libnnrt_driver") {
  include_dirs = [ "include" ]
  sources = [ "src/nnrt_device_driver.cpp" ]
  deps = []

  external_deps = [
    "c_utils:utils",
    "drivers_interface_nnrt:libnnrt_stub_2.0",
    "drivers_interface_nnrt:nnrt_idl_headers",
    "hdf_core:libhdf_host",
    "hdf_core:libhdf_ipc_adapter",
    "hdf_core:libhdf_utils",
    "hdf_core:libhdi",
    "hilog:libhilog",
    "hitrace:hitrace_meter",
    "init:libbegetutil",
    "ipc:ipc_single",
  ]

  shlib_type = "hdi"
  install_images = [ chipset_base_dir ]
  subsystem_name = "soc_p7885"
  part_name = "soc_p7885"
}

group("hdf_nnrt_service") {
  deps = [
    ":libnnrt_device_service_2.0",
    ":libnnrt_driver",
  ]
}
```

将group("hdf_nnrt_service")添加到/device/soc/spreadtrum/common/nnrt/BUILD.gn文件中，这样上级目录层级就能引用。

```
group("nnrt_entry") {
  deps = [ 
    "./v2_0:hdf_nnrt_service",
  ]
}
```

### 声明HDI服务

在对应产品的uhdf hcs配置文件中声明NNRt的用户态驱动与服务。

服务需要在```/vendor/revoview/wukong100/hdf_config/uhdf/device_info.hcs```文件中新增如下配置：

```
nnrt::host {
            hostName = "nnrt_host";
            priority = 50;
            uid = "";
            gid = "";
            caps = ["DAC_OVERRIDE", "DAC_READ_SEARCH"];
            nnrt_device :: device {
                device0 :: deviceNode {
                    policy = 2;
                    priority = 100;
                    preload = 0;
                    moduleName = "libnnrt_driver.z.so";
                    serviceName = "nnrt_device_service";
                }
            }
        }
```

\> 注意：修改hcs文件需要删除out目录重新编译，才能生效。

### 配置nnrt host进程的用户ID和组ID

对于新增的nnrt_host进程的场景，需要配置对应进程的用户ID和组ID。 进程的用户ID在文件```/vendor/revoview/wukong100/etc/passwd```中配置，进程的组ID在文件```/vendor/revoview/wukong100/etc/group```中配置，编译时会拷贝到```/vendor/revoview/wukong100/etc/BUILD.gn```中配置的对应路径中。

```text
# 在/vendor/revoview/wukong100/etc/passwd新增
nnrt_host:x:3311:3311:::/bin/false

# 在/vendor/revoview/wukong100/etc/group新增
nnrt_host:x:3311:
```

### 调测验证
适配完成后，开发者可通过以下步骤进行验证：

1. 验证nnrt_host服务进程是否存在

    使用命令```hdc shell "ps -A | grep nnrt_host"```查看进程是否存在。

2. 验证so文件是否存在

    使用命令```hdc shell "ls /vendor/lib64 | grep nnrt"```查看是否存在以下so文件。

    ```shell
    libnnrt_device_service_2.0.z.so
    libnnrt_driver.z.so
    libnnrt_stub_1.0.z.so
    libnnrt_stub_2.0.z.so
    ```

## 相关仓

[**device\_board\_revoview**](https://gitcode.com/openharmony-sig/device_board_revoview)

[**vendor\_revoview**](https://gitcode.com/openharmony-sig/vendor_revoview)