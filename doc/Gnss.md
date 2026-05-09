# Gnss驱动接口适配

### 1、GNSS驱动框架介绍

位置服务组件用于确定用户设备在哪里,使用经纬度表示用户设备的位置，OpenHarmony使用多种定位技术提供定位服务，有如GNSS定位、基站定位、WLAN/蓝牙定位（基站定位、WLAN/蓝牙定位后续统称“网络定位技术”）

#### 1.1源码框架介绍

```
/base/location/location      # 源代码目录结构：
  ├── figures       # 存放readme中的架构图
  ├── frameworks    # 框架代码
  ├── interfaces    # 对外接口 
  ├── sa_profile    # SA的配置文件
  ├── services      # 定位服务各个SA代码目录
  ├── test          # 测试代码目录
device/soc/unisoc/p7885/location/gnss #驱动接口匹配
  ├── vendorGnssAdapter.cpp # 编译成vendorGnssAdapter.so
drivers/peripheral/location # HDI接口层
drivers/interface/location/gnss/v1_0 #idl接口
device/board/revoview/wukong100\init.uis7885.gnss.cfg #init配置文件
vendor/revoview/wukong100/hdf_config/uhdf #HCS配置文件
device/soc/unisoc/p7885/hardware/location/lib64/libgnssmgt.so # 闭源库
```

HDF GNSS框架总体框图

![](figures/gnss.png)

 GNSS框架总体框图说明：

- NAPI组件是一套对外接口基于Node.js N-API规范开发的原生模块扩展开发框架
- C++ SDK主要用于服务端C++应用，如C++应用的后台服务
- 所有与基础定位能力相关的功能API，都是通过Locator提供的（其中入参需要提供当前应用程序的AbilityInfo信息，便于系统管理应用定位请求）
- 所有与（逆）地理编码转化能力相关的功能API，都是通过GeoConvert提供的

### 2、NAPI使用说明

#### 2.1、NAPI接口说明

表1 位置信息API表

| 接口名                                                                                                                                                                                   | 功能描述                                          |
| ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------- |
| on(type:&nbsp;'locationChange',&nbsp;request:&nbsp;LocationRequest,&nbsp;callback:&nbsp;Callback&lt;Location&gt;)&nbsp;:&nbsp;void                                                    | 开启位置变化订阅，并发起定位请求。                             |
| off(type:&nbsp;'locationChange',&nbsp;callback?:&nbsp;Callback&lt;Location&gt;)&nbsp;:&nbsp;void                                                                                      | 关闭位置变化订阅，并删除对应的定位请求。                          |
| on(type:&nbsp;'locationServiceState',&nbsp;callback:&nbsp;Callback&lt;boolean&gt;)&nbsp;:&nbsp;void                                                                                   | 订阅位置服务状态变化。                                   |
| off(type:&nbsp;'locationServiceState',&nbsp;callback:&nbsp;Callback&lt;boolean&gt;)&nbsp;:&nbsp;void                                                                                  | 取消订阅位置服务状态变化。                                 |
| on(type:&nbsp;'cachedGnssLocationsReporting',&nbsp;<br />request:&nbsp;CachedGnssLocationsRequest,&nbsp;callback:&nbsp;Callback&lt;Array&lt;Location&gt;&gt;)&nbsp;:&nbsp;void;       | 订阅缓存GNSS位置上报。                                 |
| off(type:&nbsp;'cachedGnssLocationsReporting',&nbsp;callback?:&nbsp;Callback&lt;Array&lt;Location&gt;&gt;)&nbsp;:&nbsp;void;                                                          | 取消订阅缓存GNSS位置上报。                               |
| on(type:&nbsp;'gnssStatusChange',&nbsp;callback:&nbsp;Callback&lt;SatelliteStatusInfo&gt;)&nbsp;:&nbsp;void;                                                                          | 订阅卫星状态信息更新事件。                                 |
| off(type:&nbsp;'gnssStatusChange',&nbsp;callback?:&nbsp;Callback&lt;SatelliteStatusInfo&gt;)&nbsp;:&nbsp;void;                                                                        | 取消订阅卫星状态信息更新事件。                               |
| on(type:&nbsp;'nmeaMessageChange',&nbsp;callback:&nbsp;Callback&lt;string&gt;)&nbsp;:&nbsp;void;                                                                                      | 订阅GNSS&nbsp;NMEA信息上报。                         |
| off(type:&nbsp;'nmeaMessageChange',&nbsp;callback?:&nbsp;Callback&lt;string&gt;)&nbsp;:&nbsp;void;                                                                                    | 取消订阅GNSS&nbsp;NMEA信息上报。                       |
| on(type:&nbsp;'fenceStatusChange',&nbsp;request:&nbsp;GeofenceRequest,&nbsp;want:&nbsp;WantAgent)&nbsp;:&nbsp;void;                                                                   | 添加围栏，并订阅该围栏事件上报。                              |
| off(type:&nbsp;'fenceStatusChange',&nbsp;request:&nbsp;GeofenceRequest,&nbsp;want:&nbsp;WantAgent)&nbsp;:&nbsp;void;                                                                  | 删除围栏，并取消订阅该围栏事件。                              |
| getCurrentLocation(request:&nbsp;CurrentLocationRequest,&nbsp;callback:&nbsp;AsyncCallback&lt;Location&gt;)&nbsp;:&nbsp;void                                                          | 获取当前位置，使用callback回调异步返回结果。                    |
| getCurrentLocation(request?:&nbsp;CurrentLocationRequest)&nbsp;:&nbsp;Promise&lt;Location&gt;                                                                                         | 获取当前位置，使用Promise方式异步返回结果。                     |
| getLastLocation(callback:&nbsp;AsyncCallback&lt;Location&gt;)&nbsp;:&nbsp;void                                                                                                        | 获取上一次位置，使用callback回调异步返回结果。                   |
| getLastLocation()&nbsp;:&nbsp;Promise&lt;Location&gt;                                                                                                                                 | 获取上一次位置，使用Promise方式异步返回结果。                    |
| isLocationEnabled(callback:&nbsp;AsyncCallback&lt;boolean&gt;)&nbsp;:&nbsp;void                                                                                                       | 判断位置服务是否已经打开，使用callback回调异步返回结果。              |
| isLocationEnabled()&nbsp;:&nbsp;Promise&lt;boolean&gt;                                                                                                                                | 判断位置服务是否已经开启，使用Promise方式异步返回结果。               |
| requestEnableLocation(callback:&nbsp;AsyncCallback&lt;boolean&gt;)&nbsp;:&nbsp;void                                                                                                   | 请求打开位置服务，使用callback回调异步返回结果。                  |
| requestEnableLocation()&nbsp;:&nbsp;Promise&lt;boolean&gt;                                                                                                                            | 请求打开位置服务，使用Promise方式异步返回结果。                   |
| enbleLocation(callback:&nbsp;AsyncCallback&lt;boolean&gt;)&nbsp;:&nbsp;void                                                                                                           | 打开位置服务，使用callback回调异步返回结果。                    |
| enableLocation()&nbsp;:&nbsp;Promise&lt;boolean&gt;                                                                                                                                   | 打开位置服务，使用Promise方式异步返回结果。                     |
| disableLocation(callback:&nbsp;AsyncCallback&lt;boolean&gt;)&nbsp;:&nbsp;void                                                                                                         | 关闭位置服务，使用callback回调异步返回结果。                    |
| disableLocation()&nbsp;:&nbsp;Promise&lt;boolean&gt;                                                                                                                                  | 关闭位置服务，使用Promise方式异步返回结果。                     |
| getCachedGnssLocationsSize(callback:&nbsp;AsyncCallback&lt;number&gt;)&nbsp;:&nbsp;void;                                                                                              | 获取缓存GNSS位置的个数，使用callback回调异步返回结果。             |
| getCachedGnssLocationsSize()&nbsp;:&nbsp;Promise&lt;number&gt;;                                                                                                                       | 获取缓存GNSS位置的个数，使用Promise方式异步返回结果。              |
| flushCachedGnssLocations(callback:&nbsp;AsyncCallback&lt;boolean&gt;)&nbsp;:&nbsp;void;                                                                                               | 获取所有的GNSS缓存位置，并清空GNSS缓存队列，使用callback回调异步返回结果。 |
| flushCachedGnssLocations()&nbsp;:&nbsp;Promise&lt;boolean&gt;;                                                                                                                        | 获取所有的GNSS缓存位置，并清空GNSS缓存队列，使用Promise方式异步返回结果。  |
| sendCommand(command:&nbsp;LocationCommand,&nbsp;callback:&nbsp;AsyncCallback&lt;boolean&gt;)&nbsp;:&nbsp;void;                                                                        | 给位置服务子系统发送扩展命令，使用callback回调异步返回结果。            |
| sendCommand(command:&nbsp;LocationCommand)&nbsp;:&nbsp;Promise&lt;boolean&gt;;                                                                                                        | 给位置服务子系统发送扩展命令，使用Promise方式异步返回结果。             |
| isLocationPrivacyConfirmed(type&nbsp;:&nbsp;LocationPrivacyType,&nbsp;callback:&nbsp;AsyncCallback&lt;boolean&gt;)&nbsp;:&nbsp;void;                                                  | 查询用户是否同意定位服务的隐私申明，使用callback回调异步返回结果。         |
| isLocationPrivacyConfirmed(type&nbsp;:&nbsp;LocationPrivacyType,)&nbsp;:&nbsp;Promise&lt;boolean&gt;;                                                                                 | 查询用户是否同意定位服务的隐私申明，使用Promise方式异步返回结果。          |
| setLocationPrivacyConfirmStatus(type&nbsp;:&nbsp;LocationPrivacyType,&nbsp;isConfirmed&nbsp;:&nbsp;boolean,&nbsp;<br />callback:&nbsp;AsyncCallback&lt;boolean&gt;)&nbsp;:&nbsp;void; | 设置并记录用户是否同意定位服务的隐私申明，使用callback回调异步返回结果。      |
| setLocationPrivacyConfirmStatus(type&nbsp;:&nbsp;LocationPrivacyType,&nbsp;isConfirmed&nbsp;:&nbsp;boolean)&nbsp;:&nbsp;Promise&lt;boolean&gt;;                                       | 设置并记录用户是否同意定位服务的隐私申明，使用Promise方式异步返回结果。       |

#### 2.2、应用层获取设备位置信息步骤

##### 2.2.1、获取位置权限

需申请ohos.permission.LOCATION和ohos.permission.LOCATION_IN_BACKGROUND权限，具体参考[accesstoken-guidelines.md ](https://gitee.com/openharmony/docs/blob/OpenHarmony-3.2-Release/zh-cn/application-dev/security/accesstoken-guidelines.md)

##### 2.2.2、导入geolocation模块

```js
import geolocation from '@ohos.geolocation'
```

##### 2.2.3、实例化LocationRequest对象，用于告知系统该向应用提供何种类型的位置服务，以及位置结果上报频率

- 方式一
  
  面向开发者提供贴近使用场景的API使用方式
  
  ```c
  export enum LocationRequestScenario {
     UNSET = 0x300,
     NAVIGATION,//导航场景
     TRAJECTORY_TRACKING,//轨迹跟踪场景
     CAR_HAILING,//出行约车场景
     DAILY_LIFE_SERVICE,//生活服务场景
     NO_POWER,//无功耗场景
  }
  ```
  
  以导航场景为例
  
  ```c
  var requestInfo = {
      'scenario':0x301,
      'timeInterval':0,
      'distanceInterval':0,
      'maxAccuracy':0
  };
  ```

- 方式二
  
  提供定位优先级策略API
  
  ```c
  export enum LocationRequestPriority {
    UNSET = 0x200,
    ACCURACY,//定位精度优先策略
    LOW_POWER,//快速定位优先策略
    FIRST_FIX,//低功耗定位优先策略
  }
  ```
  
  以定位精度优先策略为例
  
  ```c
  var requestInfo = {
     'priority':0x201,
     'timeInterval':0,
     'distanceInterval':0,
     'maxAccuracy':0
  };
  ```

##### 2.2.4、实例化Callback对象 ，用于向系统提供位置上报途径

应用需要自行实现系统定义好的回调接口，并将其实例化。系统在定位成功确定设备的实时定位结果时，会通过该接口上报给应用

```js
var locationChange = (location) => {
    console.log('locationChanger:data' + JSON.stringify(location));
}
```

##### 2.2.5、启动定位

```js
geolocation.on('locationChange',requestInfo,locationChange);
```

##### 2.2.6、结束定位

```c
geolocation.on('locationChange',locationChange);
```

##### 2.2.7、获取最近一次的历史定位(可选)

```c
geolocation.getLastLocation((data) => {
    console.log('locationChanger:data' + JSON.stringify(location)); 
}
)
```

#### 2.3、应用层进行坐标与地理编码转换步骤

##### 2.3.1、导入geolocation模块

```js
import geolocation from '@ohos.geolocation'
```

##### 2.3.2、坐标转化地理位置

```js
var reverseGeocodeRequest = {"latitude": 31.12, "longitude": 121.11, "maxItems": 1};
geolocation.getAddressesFromLocation(reverseGeocodeRequest, (data) => {
   console.log('getAddressesFromLocation: ' + JSON.stringify(data));
});
```

##### 2.3.3、位置描述转化坐标

```js
var geocodeRequest = {"description": "上海市浦东新区xx路xx号", "maxItems": 1};
geolocation.getAddressesFromLocationName(geocodeRequest, (data) => {
   console.log('getAddressesFromLocationName: ' + JSON.stringify(data));
});
```

### 2、OH中GNSS框架HDI接口适配说明：

GNSS的HDI接口与GNSS原生驱动接口（制造商提供的Linux或Android驱动）在逻辑上一一对应，但具体参数和函数名字需要进行转化对应。

HDI的四个接口函数*EnableGnss*、*DisableGnss*、*StartGnss*、*StopGnss*

GNSS原生驱动的四个接口函数*gnss_enable*、*gnss_disable*、*gnss_start*、*gnss_stop*

在vendorGnssAdapter.cpp中将原生驱动接口函数的参数转化并封装成HDI接口函数，随后编译成so文件供HDI层调用

### 3、适配OH的GNSS框架前提条件

- 厂商提供的GNSS的闭源驱动（例如：libgnssmgt.so）

- 测试原生GNSS是否正常,切换当前用户为location_host,修改/etc/passwd将
  
  ```bash
  base/startup/init/services/etc/group:location_host:x:1022:
  base/startup/init/services/etc/passwd:loation_host:x:1022:1022:::/bin/false
  ```

### 4、GNSS适配过程

#### 4.1、配置文件修改

vendor/XXX/XXX/hdf_config/uhdf/device_info.hcs

```json
location :: host {
    hostName = "location_host";
    priority = 50;
    uid = "location_host";
    gid = ["location_host"];
    location_gnss_device :: device {
        device0 :: deviceNode {
            policy = 2;
            priority = 100;
            preload = 2;
            moduleName = "liblocation_gnss_hdi_driver.z.so";
            serviceName = "gnss_interface_service";
        }
    }
    location_agnss_device :: device {
        device0 :: deviceNode {
            policy = 2;
            priority = 100;
            preload = 2;
            moduleName = "liblocation_agnss_hdi_driver.z.so";
            serviceName = "agnss_interface_service";
        }
    }
    location_geofence_device :: device {
        device0 :: deviceNode {
            policy = 2;
            priority = 100;
            preload = 2;
            moduleName = "liblocation_geofence_hdi_driver.z.so";
            serviceName = "geofence_interface_service";
        }
    }
}
```

#### 4.2、修改GNSS节点权限

gnss驱动在运行过程中可能涉及到读写配置文件以及设备文件或者系统状态,需要将对应的文件修改所有者为location_host,每次开机启动时都会自动修改文件权限
/vendor/etc/init.产品名称.cfg

```c
"name":"boot", "cmds":[
..........................,//添加如下字段,名称根据实际文件名称修改
chown location_host location_host /dev/gnss0,
chmod 660 /dev/gnss0,
chwon location_host location_host ..., 
chmod 660 ....
]
```

例如init.xxx.gnss.cfg
![](figures/init.gnss.cfg.png)

#### 4.3、编译vendorGnssAdapter.so

将源码文件vendorGnssAdapter.cpp,vendorGnssAdapter.h放在device/board/公司名称/${device_name}/gnss目录下
vendorGnssAdapter.cpp中将原生驱动接口函数的参数转化并封装成HDI接口函数

```c
#define GNSSMGT "/vendor/soc_platform/lib64/libgnssmgt.z.so"
...
int gnss_enable(GnssCallbackStruct *callbacks) {
  LBSLOGE(GNSS, "%{public}s entered", GNSSMGT);
  if (callbacks == nullptr){
    LBSLOGE(GNSS, "%{public}s callbacks == nullptr return", GNSSMGT);
  }
  DL_RET ret=SO_OK;
  g_GCS_ = *callbacks;
  g_Handle = dlopen(GNSSMGT, RTLD_LAZY);
  LBSLOGE(GNSS, "%{public}s GNSSMGT handle addr is %x", GNSSMGT, g_Handle);
  if (g_Handle == NULL) 
  {
    LBSLOGE(GNSS, "%{public}s load failed", GNSSMGT);
    return LOAD_NOK;
  }
  gps_init = (pGnssmgt_init)dlsym(g_Handle, "gnssmgt_init");
  LBSLOGE(GNSS, "%{public}s gps_init addr is %{public}x", GNSSMGT, gps_init);
  gps_start = (pGnssmgt_start)dlsym(g_Handle, "gnssmgt_start");
  LBSLOGE(GNSS, "%{public}s gps_start addr is %{public}x", GNSSMGT, gps_start);
  gps_stop = (pGnssmgt_stop)dlsym(g_Handle, "gnssmgt_stop");
  LBSLOGE(GNSS, "%{public}s gps_stop addr is %{public}x", GNSSMGT, gps_stop);
  gps_cleanup = (pGnssmgt_cleanup)dlsym(g_Handle, "gnssmgt_cleanup");
  LBSLOGE(GNSS, "%{public}s gps_cleanup addr is %{public}x", GNSSMGT, gps_cleanup);
  ...
  gps_init(&sGpsCallbacks);
  return ret;
}
```

修改ohos.build,添加依赖的so模块及源码模块

```json
{
    "subsystem": "子系统名称",
    "parts": {
        "组件名称": {
        "module_list": [
.....
        "//device/board/公司名称/${device_name}/gnss:vendorGnssAdapter",
.....
            ]
        }
    }
}
```

编写BUILD.gn，配置依赖模块的具体路径，源码

```json
import("//build/ohos.gni")
import("//vendor/$product_company/$product_name/product.gni")
ohos_shared_library("vendorGnssAdapter") {
    output_name = "vendorGnssAdapter"
    cflags = [
        "-w",
        "-g",
        "-O",
        "-fPIC",
    ]
defines += [
    "__USER__"
]
include_dirs = [
    "//drivers/peripheral/location/gnss/hdi_service",
    "//base/location/interfaces/inner_api/include",
]
sources = [
    "vendorGnssAdapter.cpp",
]
deps = [
]
external_deps = [
    "hiviewdfx_hilog_native:libhilog",
]
install_images = [ chipset_base_dir ]
part_name = "组件名称"
subsystem_name = "子系统名称"
install_enable = true
output_prefix_override = true
output_extension = "so"
}
```

#### 4.4、HDI层加载vendorGnssAdapter.so

```c
  ```c
  const std::string VENDOR_NAME = "vendorGnssAdapter.so";//供应商驱动
  void LocationVendorInterface::Init()//初始化加载驱动
  {
      ...
      vendorHandle_ = dlopen(VENDOR_NAME.c_str(), RTLD_LAZY);
      ...
      GnssVendorDevice *gnssDevice = static_cast<GnssVendorDevice *>(dlsym(vendorHandle_, "GnssVendorInterface"));
      ...
      vendorInterface_ = gnssDevice->get_gnss_interface();
      ...
  }
```

```
### 5、测试验证

#### 5.1、编译

​```bash
./build.sh --product-name yangfan --ccache -T LocatorFuzzTest
./build.sh --product-name yangfan --ccache -T gnss_test
./build.sh --product-name yangfan --ccache -T LocatorServiceAbilityTest
```

#### 5.2、驱动测试

```bash
hdc file send gnss_test /data/
hdc shell
cd /data
chmod 777 gnss_test 
./gnss_test
```

![](figures/gnss驱动测试.png)

#### 5.3、服务测试

```bash
hdc file send LocatorServiceAbilityTest /data/
hdc shell
cd /data
chmod 777 LocatorServiceAbilityTest 
./LocatorServiceAbilityTest
```

查看log
![](figures/gnss服务测试.png)

#### 5.4、location app测试

```bash
hdc app install -r Location.hap
```

### 相关仓库

[base_location](https://gitee.com/openharmony/base_location/blob/master/README.md)
