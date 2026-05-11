# WUKONG100 UIS7885芯片启动适配

## 产品配置和目录规划

### 产品配置

在产品//vendor/revoview/wukong100目录下创建以wukong100名字命名的json文件，并指定CPU的架构,配置如下：

```json
{
  "product_name": "wukong100",
  "device_company": "revoview",
  "device_build_path": "device/board/revoview/wukong100",
  "target_cpu": "arm64",
  "type": "standard",
  "version": "3.0",
  "board": "wukong100",
  "api_version": 8,
  "enable_ramdisk": true,
  "enable_absystem": false,
  "build_selinux": true,
  "build_seccomp": false,
  "device_stack_size": 8388608,
  "inherit": [ "productdefine/common/inherit/rich.json", "productdefine/common/inherit/chipset_common.json" ],
  "subsystems": [......]
}
```

主要的配置内容包括：    
device_build_path：指定device路径。   
target_cpu：cpu架构。   
type：配置系统的级别， 这里直接standard即可。   
inherit：可以继承已有的json配置文件。  

>已定义的子系统可以在//build/subsystem_config.json中找到。您也可以自定义子系统。

### 目录规划

参考Board和SoC解耦的设计思路，并把芯片适配目录规划为：

```sh
device
├── board                                --- 单板厂商目录
│   └── revoview                         --- 单板厂商名字：
│       └── wukong100                       --- 单板名：wukong100,主要放置开发板相关的驱动业务代码
└── soc                                  --- SoC厂商目录
    └── unisoc                       --- SoC厂商名字：unisoc
        └── p7885                      --- SoC Series名：uis7885,主要为芯片原厂提供的一些方案，以及闭源库等
```

```sh
vendor
└── revoview
    └── wukong100                           --- 产品名字：产品、hcs以及demo相关
```

## 内核启动

### 二级启动

二级启动简单来说就是将之前直接挂载sytem,从system下的init启动，改成先挂载ramdsik,从ramdsik中的init 启动，做些必要的初始化动作，如挂载system,vendor等分区，然后切到system下的init 。

oriole适配主要是将主线编译出来的ramdisk.img 打包到boot.img和vendor_boot.img中，主要有以下工作：

1.使能二级启动

在vendor/revoview/wukong100/config.json 中使能enable_ramdisk。

```json
{
......
  "enable_ramdisk": true,  
......
} 
```
2.把主线编译出来的ramdsik.img 打包到vendor_boot.img

配置路径：

device/board/revoview/wukong100/kernel/BUILD.gn

```
action("make_boot_image") {
  script = "make-build.sh"
  sources = [ kernel_source_dir ]
  outputs = [ "$root_build_dir/packages/phone/images/boot.img" ]
  args = [
    rebase_path("$root_build_dir"),
    "ramdisk",
  ]
  # must wait ramdisk image build
  deps = [ "//build/ohos/images:make_images" ]
}
```



### 打包

增加了打包boot镜像的脚本make_boot.sh，供编译完ramdisk,打包boot 镜像时调用, 主要内容:

由于wukong100启动lk支持从ramdisk 启动，通过内核编译脚本make_boot.sh将编译出来的ramdisk.img 打包到boot.img和vendor_boot.img中，具体脚本实现在:
```
ramdisk_name=ramdisk.img
boot_name=boot.img
vendor_boot_name=vendor_boot.img

if [ $# -gt 0 ] && [ "${1}" == "updater" ] ; then
    vendor_boot_name=vendor_boot_updater.img
    ramdisk_name=updater.img
    boot_name=boot-updater.img
else
    #build and copy dtbo.img to out directory
    dtbo_name=mkboot/dist/dtbo.img
	dtb_name=mkboot/dist/dtb.img
    rm -f ${dtbo_name}
	rm -f ${dtb_name}
    ./mkboot/bin/mkdtimg create ./${dtbo_name} ./mkboot/dist/uis7885-2h10-overlay.dtbo --id=0x220000
    cp -f ${dtbo_name} ../../../packages/phone/images/.
    
    ./mkboot/bin/mkdtimg create ./${dtb_name} ./mkboot/dist/*.dtb
    cp -f ${dtb_name} ../../../packages/phone/images/.
fi

ramdisk_path=./mkboot/dist/${ramdisk_name}
rm -f ${boot_name}

./mkboot/bin/mkbootimg --kernel ./mkboot/dist/Image \
--ramdisk ${ramdisk_path} --base 0x00000000 --pagesize 4096 \
--cmdline "console=ttyS1,115200n8 buildvariant=engdebug" \
--os_version 13 --os_patch_level 2023-03-05 --kernel_offset 0x00008000 \
--ramdisk_offset 0x05400000 --header_version 4 -o ./${boot_name}

./mkboot/bin/mkbootimg --dtb ./mkboot/dist/dtb.img --base 0x00000000 --pagesize 4096 \
--vendor_cmdline "console=ttyS1,115200n8 buildvariant=engdebug" \
--kernel_offset 0x00008000 --ramdisk_offset 0x05400000 --header_version 4 \
--pagesize=4096 --vendor_ramdisk ${ramdisk_path} --vendor_boot ./${vendor_boot_name}

```
## INIT配置

init相关配置请参考启动子系统的规范要求即可