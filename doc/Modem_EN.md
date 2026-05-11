# Spreadtrum 7885 Chip Modem Adaptation

## Telephony Layered Architecture Diagram

OpenHarmony's telephony architecture diagram is as follows

- Framework:
  Provides telephony business-related functions, including call management, cellular calls, cellular data, SMS/MMS, etc.
- Service Layer:
  Telephony service core layer, including SIM card, network search, and underlying interface calls.
- Vendor:
  RIL adaptation, providing a bridge for communication between telephony services and modem hardware. Implementation includes chip adaptation, vendor library loading, time scheduling management, business abstraction interfaces, interface implementation, etc. OpenHarmony's telephony architecture diagram is as follows. On December 30, 2025, the full-stack adaptation of Spreadtrum P7885-Wukong100 was completed (including Kernel, HDF, ArkUI, system services).

![core_service framework diagram](figures/core_service框架图.png)

## Modem Adaptation Introduction

The work that needs to be completed for modem adaptation is the adaptation of the modem's HDI interface. That is, the "**RIL adaptation**" part in the diagram below.

In the way RIL interacts with modem hardware, different modem chips have different methods.

The modem of Spreadtrum 7885 interacts with the underlying hardware through the service program provided by Spreadtrum. The "**modem service**" in the diagram below.

![telephony flow diagram](figures/telephony流程图.png)

The modem service structure diagram of Spreadtrum 7885 is as follows:

![](figures/ril_adapter.png)

### Dependencies

Spreadtrum modem depends on Spreadtrum underlying services, with the following programs

**Service Programs**

modem\_control.bin

cp\_diskserver.bin

channelmanger.bin

### **SO Library Files**

libmodem-utils.z.so

libmodem_halo.z.so

libreadfixednv_lib.z.so

libsprd7885_tele_adapter_1.0.z.so

### **Configuration Files**

modem\_ch\_info.xml

modem\_cp\_info.xml

modem\_sp\_info.xml

tele_adapter.para.dac

tele_adapter.para

nvitem.para

modem_control.para

channelmanager.para

channelmanager.cfg

modem_control.cfg

nvitem.cfg

### Startup Script

Spreadtrum modem functionality depends on Spreadtrum underlying services modem\_control.bin and cp\_diskserver.bin, which need to be set to start automatically at boot.

By configuring the startup script modem.cfg, set modem\_control.bin and cp\_diskserver.bin to start automatically at boot.

Note: The startup of modem\_control.bin, cp\_diskserver.bin, and channelmanger.bin depends on **SO library files** and **configuration files**

```
{
    "jobs": [
        {
            "name" : "init",
            "cmds" : [
                "setparam ro.vendor.radio.modemtype tl",
                "setparam ro.vendor.modem.tmctl /dev/sprd_time_sync",
                "mkdir /mnt/data",
                "chmod 0755 /mnt/data",
                "mkdir /mnt/vendor",
                "chmod 0755 /mnt/vendor"
            ]
        }, {
            "name": "post-fs-data",
            "cmds": [
                "start channelmanager_service"
            ]
        }
    ],
    "services": [
        {
            "name": "channelmanager_service",
            "start-mode": "condition",
            "path": [
                "/vendor/bin/channelmanager.bin"
            ],
            "uid": "root",
            "gid": [ "root", "system", "radio" ],
            "secon" : "u:r:chipset_init:s0"
        }
    ]
}

```

```
{
    "jobs": [
        {
            "name" : "init",
            "cmds": [
				"mkdir /mnt/data",
				"chmod 0755 /mnt/data"
            ]
        },
        {
            "name": "post-fs-data",
            "cmds": [
                "start cp_diskserver_service"
            ]
        }
    ],
    "services": [
		{
            "name": "cp_diskserver_service",
            "start-mode" : "condition",
            "path": [
                "/vendor/bin/cp_diskserver.bin"
            ],
            "uid": "root",
            "gid": [ "root", "system", "radio" ],
			"secon" : "u:r:chipset_init:s0"
        }
    ]
}
  
```

```
{
    "jobs": [
        {
            "name" : "init",
            "cmds": [
				"mkdir /mnt/data",
				"chmod 0755 /mnt/data"
            ]
        },
        {
            "name": "post-fs-data",
            "cmds": [
                "start modem_control_service"
            ]
        }
    ],
    "services": [
		{
            "name": "modem_control_service",
            "start-mode" : "condition",
            "path": [
                "/vendor/bin/modem_control.bin"
            ],
            "uid": "root",
            "gid": [ "root", "system", "radio" ],
			"secon" : "u:r:chipset_init:s0"
        }
    ]
}


```

### Startup

When the init process starts, it first completes system initialization work, then begins parsing configuration files. When the system parses configuration files, it divides them into three categories:

1. init.cfg default configuration file, defined by the init system, parsed first.
2. /system/etc/init/\*.cfg configuration files defined by each subsystem.
3. /vendor/etc/init/\*.cfg configuration files defined by vendors.

When you need to add configuration files, users can define their own configuration files according to their needs and copy them to the corresponding directory.

**Startup configuration files are channelmanager.cfg, modem**\_control.cfg, nvitem.cfg

Reference document

[https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/subsystems/subsys-boot-init-cfg.md](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/subsystems/subsys-boot-init-cfg.md)

### Verification

Verification 1: Check if startup is successful

Verify if modem\_control.bin started successfully

![](figures/modem_control.png)

Verify if cp\_diskserver.bin started successfully

![](figures/cp_diskservice.png)

Verify if channelmanger.bin started successfully

![](figures/chan.png)

Check if the serial port provided by modem exists

![](figures/stty_nr31.png)

### Logs

The print logs of modem service programs can be obtained from hilog. Developers can analyze the modem status from the logs.

![](figures/modemlog.png)
