# device_soc_unisoc

-   [Introduction](#section01)
-   [Directory Structure](#section02)
-   [License](#section03)
-   [Usage](#section04)
-   [Contribution](#section05)
-   [Related Repositories](#section06)

## Introduction<a name="section01"></a>

This repository is used to store Unisoc chip adaptation content.

### uis7885 Chip
|Hardware Features	|Technical Capabilities|
|-----------|--------|
|Processor     |uis7885|
|Architecture       |ARM|
|CPU        |1\*A76@2.7GHz+3\*A76@2.3GHz+4\*A55@2.1GHz 8-core architecture|
|GPU        |ARM NATT-4core@850MHz graphics processor|
|MODEM      |2G/3G/4G/5G, 5G full scenario coverage, 3GPP R15, NSA/SA, 2CC, SUL, LTE DL Cat15+UL Cat18|
|VDSP       |VQ7@2.1GHz; NPU 8TOPS AI computing power|
|Video Decoder |4K@60fps|

## Directory Structure<a name="section02"></a>

```
├── p7885
│   ├── bootloader          # bootloader source code
│   ├── hardware            # HDI hardware adaptation layer
│   │   ├── camera_unisoc   # camera HDI adaptation layer implementation
│   │   ├── display         # display HDI adaptation layer implementation
│   │   ├── gpu             # gpu HDI adaptation layer implementation
│   │   ├── modem           # modem HDI adaptation layer implementation
│   │   └── npu             # npu HDI adaptation layer implementation
│   ├── modules             # kernel driver *.ko files
│   │   ├── audio           # audio module driver related .ko files, firmware
│   │   ├── gnss            # gnss module driver related .ko files
│   │   ├── gpu             # gpu module driver related .ko files
│   │   ├── location        # gnss HDI adaptation layer implementation, configuration files
│   │   ├── input           # input module driver related .ko files
│   │   ├── npu             # npu module driver related .ko files
│   │   ├── video           # video module driver related .ko files
│   │   ├── vpu             # vpu module driver related .ko files
│   │   ├── wcn             # wcn module driver related .ko files, configuration files, firmware
│   │   └── ws73            # NearLink ws73 module driver related .ko files, configuration files, firmware
│   ├── pac                 # boot.img, dtbo.img kernel images, fdl1-sign.bin, modem-related and other Unisoc bin files
│   └── third_kit           # third-party source code
└── README.md
```

## License<a name="section03"></a>

See the LICENSE file and code declarations in the corresponding directories.

## Usage<a name="section04"></a>

### Environment Preparation

[Build and Compilation Method](https://gitcode.com/openharmony-sig/device_board_revoview/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/wukong100/README_zh.md#section03)

[Flashing Method](https://gitcode.com/openharmony-sig/device_board_revoview/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/wukong100/README_zh.md#section04)

### Adaptation Methods

[Startup Adaptation](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Startup.md)

[Display Adaptation](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Display.md)

[Screen Adaptation](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Screen.md)

[NPU Adaptation](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Nnrt.md)

[Modem Adaptation](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Modem.md)

[BT Framework Adaptation](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/BT.md)

[WiFi Adaptation](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/WIFI.md)

[Audio Adaptation](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Audio.md)

[GNSS Adaptation](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/Gnss.md)

[Face Recognition](https://gitcode.com/openharmony-sig/device_soc_unisoc/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/doc/FaceNet.md)

## Contribution<a name="section05"></a>

[How to Contribute](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/contribute/%E5%8F%82%E4%B8%8E%E8%B4%A1%E7%8C%AE.md)

[Commit Message Specification](https://gitee.com/openharmony/device_qemu/wikis/Commit%20message%E8%A7%84%E8%8C%83?sort_id=4042860)

## Related Repositories<a name="section06"></a>

[device_board_revoview](https://gitcode.com/openharmony-sig/device_board_revoview)

[vendor_revoview](https://gitcode.com/openharmony-sig/vendor_revoview)
