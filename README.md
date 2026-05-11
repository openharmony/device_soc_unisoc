# device_soc_unisoc

-   [简介](#section01)
-   [目录结构](#section02)
-   [许可说明](#section03)
-   [使用说明](#section04)
-   [参与贡献](#section05)
-   [相关仓](#section06)

## 简介<a name="section01"></a>

本仓用于存放展锐unisoc芯片适配内容。

### uis7885芯片
|硬件特性	|技术能力|
|-----------|--------|
|处理器     |uis7885|
|架构       |ARM|
|CPU        |1\*A76@2.7GHz+3\*A76@2.3GHz+4\*A55@2.1GHz的8核架构|
|GPU        |ARM NATT-4core@850MHz图像处理器|
|MODEM      |2G/3G/4G/5G,5G全场景覆盖,3GPP R15,NSA/SA,2CC,SUL,LTE DL Cat15+UL Cat18|
|VDSP       |VQ7@2.1GHz;NPU 8TOPS AI算力|
|视频解码器 |4K@60fps|

## 目录结构<a name="section02"></a>

```
├── p7885
│   ├── bootloader          # bootloader源码
│   ├── hardware            # HDI 硬件适配层
│   │   ├── camera_unisoc   # camera HDI 适配层实现
│   │   ├── display         # display HDI 适配层实现
│   │   ├── gpu             # gpu HDI 适配层实现
│   │   ├── modem           # modem HDI 适配层实现
│   │   └── npu             # npu HDI 适配层实现
│   ├── modules             # 内核driver *.ko文件
│   │   ├── audio           # audio模块driver相关.ko文件, 固件
│   │   ├── gnss            # gnss模块driver相关.ko文件
│   │   ├── gpu             # gpu模块driver相关.ko文件
│   │   ├── location        # gnss HDI 适配层实现, 配置文件
│   │   ├── input           # input模块driver相关.ko文件
│   │   ├── npu             # npu模块driver相关.ko文件
│   │   ├── video           # video模块driver相关.ko文件
│   │   ├── vpu             # vpu模块driver相关.ko文件
│   │   ├── wcn             # wcn模块driver相关.ko文件, 配置文件, 固件
│   │   └── ws73            # 星闪ws73模块driver相关.ko文件,  配置文件, 固件
│   ├── pac                 # boot.img, dtbo.img内核镜像, fdl1-sign.bin，modem相关等unisoc bin文件
│   └── third_kit           # 三方源码
└── README.md
```

## 许可说明<a name="section03"></a>

参见对应目录的LICENSE文件及代码声明

## 使用说明<a name="section04"></a>

### 环境准备

[编译构建方法](https://gitcode.com/openharmony-sig/device_board_revoview/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/wukong100/README_zh.md#section03)

[烧录方法](https://gitcode.com/openharmony-sig/device_board_revoview/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/wukong100/README_zh.md#section04)

### 适配方法

[启动适配](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Startup.md)

[Display适配](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Display.md)

[Screen适配](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Screen.md)

[NPU适配](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Nnrt.md)

[Modem适配](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Modem.md)

[BT框架适配](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/BT.md)

[Wifi适配](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/WIFI.md)

[Audio适配](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Audio.md)

[Gnss适配](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Gnss.md)

[面部识别](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/FaceNet.md)

## 参与贡献<a name="section05"></a>

[如何参与](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/contribute/%E5%8F%82%E4%B8%8E%E8%B4%A1%E7%8C%AE.md)

[Commit message规范](https://gitee.com/openharmony/device_qemu/wikis/Commit%20message%E8%A7%84%E8%8C%83?sort_id=4042860)

## 相关仓<a name="section06"></a>

[device_board_revoview](https://gitcode.com/openharmony-sig/device_board_revoview)

[vendor_revoview](https://gitcode.com/openharmony-sig/vendor_revoview)

