# OpenHarmony WiFi Architecture

<img src="figures/WiFi_frame.png" style="zoom: 50%;" />

- Application: Mainly applications developed by developers for WiFi-related functions. Control the device's WiFi and implement its functions by calling the APIs provided by the WiFi SDK. This layer will provide relevant API call examples for reference.

- WiFi Native JS: The JS layer is developed using the NAPI mechanism, connecting the APP layer and Framework layer, encapsulating WiFi functions into JS interfaces for application calls, and supporting both Promise and Callback asynchronous callbacks.

- WiFi Framework: WiFi core function implementation. Directly provides services to upper-layer applications. According to different working modes, it is divided into four business service modules: STA service, AP service, P2P service, and Aware service, along with DHCP functionality.

- WiFi Hal: Provides unified interface services for the Framework layer to operate WiFi hardware, achieving separation between the application framework and hardware operations. Mainly includes Hal adapter, extended Hal modules, and binary library modules provided by WiFi hardware manufacturers.

- Wpa Supplicant: Includes two sub-modules: wpa_supplicant and hosapd. wpa_supplicant and hostapd implement the defined driver API and provide control interfaces externally, allowing the framework to implement various WiFi operations through its control interfaces. wpa_supplicant supports STA and P2P modes, while hostapd supports AP mode.

- HDF: The HDF driver framework mainly consists of four parts: driver basic framework, driver programs, driver configuration files, and driver interfaces, implementing WiFi driver functions, driver loading, driver interface deployment, etc.

- WiFi Kernel: Includes WiFi drivers, including some basic read and write operations on devices, compiled into the kernel by WiFi driver porting personnel.

# WiFi Driver Framework

WLAN driver framework composition:

<img src="figures/WLAN_driver_frame.png" style="zoom:80%;" />

The driver architecture mainly consists of seven parts: HAL, Client, Module, NetDevice, NetBuf, BUS, and Message.

- HAL

The HAL component provides standard WIFI-HDI interfaces and data format definitions to the WiFiService module, with capabilities including: setting MAC address, setting transmission power, obtaining device MAC address, etc.

- Client

The Client component implements user-kernel interaction. By adapting sbuf and nl80211 differently and performing configuration compilation based on products, it provides unified interface calls upwards. The framework is shown in the following diagram:

<img src="figures/CLIENT.png" style="zoom: 50%;" />

- Message

The Message component provides separate business interfaces for each service. This module supports running in user mode, kernel mode, and MCU environments, achieving full decoupling between components.

<img src="figures/MSG.png" style="zoom:50%;" />

- Module

Module implements WiFi framework startup loading, configuration file parsing, device driver initialization, and chip driver initialization based on the HDF driver framework. According to WLAN functional characteristics, it divides into Base, AP, STA and other components, providing unified management of control flow commands and events.

- BUS

The BUS driver module provides unified bus abstraction interfaces upwards. By calling sdio interfaces provided by the Platform layer downwards and encapsulating and adapting usb and pcie interfaces, it shields differences between different operating systems; by uniformly encapsulating operations on different types of buses, it shields differences between different chips, providing complete bus driver functions to different chip manufacturers, allowing different manufacturers to share this module interface, making manufacturer development more convenient and unified. The framework is shown in the following diagram:

<img src="figures/BUS.png" style="zoom:50%;" />

- NetDevice

NetDevice is used to establish dedicated network devices, shielding differences between different OSs, providing unified interfaces to WIFI drivers, providing unified HDF NetDevice data structures, and unified management, registration, and deregistration capabilities; interfacing with the Linux network device layer on rich devices; interfacing with the Linux network device layer on lite devices.

- NetBuf

The NetBuf component provides encapsulation of unified data structures for Linux or LiteOS native network data buffers and encapsulation of operation interfaces for network data for WLAN drivers. The framework is shown in the following diagram:

<img src="figures/NETBUF.png" style="zoom:50%;" />



# WiFi Driver Adaptation

Currently, the method for adapting WLAN is based on the WPA third-party framework, directly using CONFIG_DRIVER_NL80211 with the nl80211 protocol, directly connecting to the chip driver, which is the nl80211 protocol flow in the Client flow, as follows.

<img src="figures/flow.png" style="zoom:70%;" />

## Adaptation Preparation

- Original manufacturer code and firmware

## Adaptation at Vendor Layer

Create a wlan directory under the device_soc_sprd\common\wcn directory, migrate the manufacturer's bin files and C code into it, then modify the build_modules.sh and build.gn code according to the specific board situation.


1. Add socket type and chip model in build_modules.sh


~~~
#wcn bt driver config
export BSP_BOARD_UNISOC_WCN_SOCKET="sdio"

#wcn module version config
export BSP_BOARD_WLAN_DEVICE="sc2355"
~~~

2. In build.gn


~~~
outputs += [ "$root_build_dir/modules/sprd_wlan_combo/sprd_wlan_combo.ko" ]
#  outputs += [ "$root_build_dir/kernel/OBJ/linux-5.15/drivers/unisoc_platform/wlan/sc2355/sc2355_sdio_wlan.ko" ]
~~~

~~~
ohos_prebuilt_executable("wifi_board_config") {
  source = "wcn/wifi_board_config.ini"
  module_install_dir = "lib/firmware"
  install_images = [ "system" ]
  part_name = "product_oriole"
  install_enable = true
}
~~~

~~~
ohos_prebuilt_executable("sprd_wlan_combo") {
  deps = [ ":build_modules" ]
  source = "$root_build_dir/modules/sprd_wlan_combo/sprd_wlan_combo.ko"
  module_install_dir = "modules"
  install_images = [ chipset_base_dir ]
  part_name = "product_oriole"
  install_enable = true
}
~~~

3. Add in vendor\hys\oriole\config.json

~~~
 {
      "subsystem": "thirdparty",
      "components": [
        {
          "component": "wpa_supplicant",
          "features": [
            "wpa_supplicant_driver_nl80211 = true",
            "wpa_supplicant_driver_nl80211_sprd = true"
          ]
        }
      ]
    },
  ... ...
~~~
