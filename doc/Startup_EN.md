# WUKONG100 UIS7885 Chip Startup Adaptation

## Product Configuration and Directory Planning

### Product Configuration

Create a JSON file named wukong100 in the product directory //vendor/revoview/wukong100, and specify the CPU architecture. The configuration is as follows:

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

Main configuration content includes:    
device_build_path: Specifies the device path.   
target_cpu: CPU architecture.   
type: Configures the system level, standard is used here.   
inherit: Can inherit existing JSON configuration files.  

>Defined subsystems can be found in //build/subsystem_config.json. You can also customize subsystems.

### Directory Planning

Following the design concept of Board and SoC decoupling, the chip adaptation directory is planned as:

```sh
device
├── board                                --- Board vendor directory
│   └── revoview                         --- Board vendor name:
│       └── wukong100                       --- Board name: wukong100, mainly contains board-related driver business code
└── soc                                  --- SoC vendor directory
    └── unisoc                       --- SoC vendor name: unisoc
        └── p7885                      --- SoC Series name: uis7885, mainly for solutions provided by chip manufacturers and closed-source libraries
```

```sh
vendor
└── revoview
    └── wukong100                           --- Product name: product, hcs, and demo related
```

## Kernel Startup

### Two-stage Boot

Two-stage boot simply means changing from directly mounting system and starting from init under system, to first mounting ramdisk and starting from init in ramdisk, performing necessary initialization actions such as mounting system, vendor and other partitions, then switching to init under system.

The main work for oriole adaptation is to package the ramdisk.img compiled from the mainline into boot.img and vendor_boot.img, mainly including the following work:

1. Enable two-stage boot

Enable enable_ramdisk in vendor/revoview/wukong100/config.json.

```json
{
......
  "enable_ramdisk": true,  
......
} 
```
2. Package the ramdisk.img compiled from the mainline into vendor_boot.img

Configuration path:

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



### Packaging

Added the make_boot.sh script for packaging boot images, which is called after compiling ramdisk to package the boot image. Main content:

Since wukong100 startup lk supports booting from ramdisk, the kernel compilation script make_boot.sh packages the compiled ramdisk.img into boot.img and vendor_boot.img. The specific script implementation is in:
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
## INIT Configuration

For init-related configuration, please refer to the specification requirements of the startup subsystem.
