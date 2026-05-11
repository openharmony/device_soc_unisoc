# Unisoc 7885 Chip Screen Adaptation

## LCD Adaptation
The 7885 platform supports one MIPI interface LCD screen by default.

The following diagram shows the Display driver model hierarchy:
![Hierarchy Diagram](figures/lcd.png)

The current model is mainly deployed in kernel mode, providing libdrm interfaces upwards to assist HDI implementation. The display driver exposes display screen driver capabilities to graphics services through the Display-HDI layer; downwards, it interfaces with LCD panel devices, driving the screen to work normally, and connecting the entire display process from top to bottom.

Therefore, LCD adaptation mainly focuses on the adaptation of LCD panel device drivers.

Device driver adaptation is divided into two parts: panel driver and dts configuration.

The location of libpanel.z.so on the product is:
/system/lib64/module/inputmethod/libpanel.z.so

### Panel Driver
Device drivers mainly revolve around the following interfaces:
~~~
struct drm_panel_funcs {
    int (*prepare)(struct drm_panel *panel);
    int (*unprepare)(struct drm_panel *panel);
    int (*enable)(struct drm_panel *panel);
    int (*disable)(struct drm_panel *panel);
    int (*get_modes)(struct drm_panel *panel, struct drm_connector *connector);
    int (*get_timings)(struct drm_panel *panel, unsigned int num_timings,
                       struct display_timing *timings);
    int (*get_edid)(struct drm_panel *panel, struct edid *edid);
};
~~~

The driver instantiates this structure in the initialization interface:

~~~
static const struct drm_panel_funcs sprd_panel_funcs = {
	.get_modes = sprd_panel_get_modes,
	.enable = sprd_panel_enable,
	.disable = sprd_panel_disable,
	.prepare = sprd_panel_prepare,
	.unprepare = sprd_panel_unprepare,
};
~~~

get_modes provides display modes (resolution, refresh rate) supported by the panel.  
enable activates panel display (turn on backlight, signal control).  
disable deactivates panel display (turn off backlight, stop signal).  
prepare prepares panel hardware to enter working state (power supply and initialization). 
unprepare transitions panel hardware from working state to non-working state (power off and cleanup).  

dts configuration
~~~
/ {
	fragment {
		target-path = "/";
		__overlay__ {
			lcds {
				lcd_hx8279_revo_mipi: lcd_hx8279_revo_mipi {

					sprd,dsi-work-mode = <1>;
					sprd,dsi-lane-number = <4>;
					sprd,dsi-color-format = "rgb888";

					sprd,phy-bit-clock = <1105000>;

					sprd,width-mm = <107>;
					sprd,height-mm = <172>;

					sprd,esd-check-enable = <0>;
					sprd,esd-check-mode = <1>;
					sprd,esd-check-period = <2000>;

					sprd,reset-on-sequence = <1 10>, <0 20>, <1 100>;
					sprd,reset-off-sequence = <0 20>;

					sprd,initial-command = [
						.........
						13 14 00 01 29
					];
					
					sprd,sleep-in-command = [
						13 0A 00 01 28
						13 78 00 01 10
					];

					sprd,sleep-out-command = [
						13 78 00 01 11
						13 64 00 01 29
					];

					display-timings {
						native-mode = <&hx8279_revo_timing0>;
						hx8279_revo_timing0: timing0 {
						        clock-frequency = <153600000>;
						        hactive = <1200>;
						        vactive = <1920>;
						        hfront-porch = <80>;  //80
						        hback-porch = <70>;   //40
						        hsync-len = <10>;     //10
						        vfront-porch = <35>;  //26
						        vback-porch = <32>;   //31
						        vsync-len = <4>;      //2
							};
						};
					};
			};
		};
	};
};


~~~

## Backlight
Backlight driver model based on Linux driver framework
![Driver Model](figures/backlight.png)
The 7885 backlight is controlled by PWM duty cycle.

### Backlight Driver

The location of libbrightness.z.so on the product is:
/system/lib64/module/libbrightness.z.so

Device drivers mainly revolve around the following interfaces:
~~~
struct backlight_ops {
    int (*update_status)(struct backlight_device *bd);
    int (*get_brightness)(struct backlight_device *bd);
};
~~~

The driver instantiates this structure in the initialization interface:
~~~
static const struct backlight_ops sprd_backlight_ops = {
	.update_status = sprd_pwm_backlight_update,
};
~~~

update_status is used to update the user-set brightness value to hardware.

In display HDI, backlight adjustment is achieved by setting the backlight driver node:
~~~
int32_t DrmConnector::SetBrightness(uint32_t level)
{
    static int32_t brFd = 0;
    const int32_t buffer_size = 10; /* buffer size */
    char buffer[buffer_size];

    DISPLAY_LOGE("set %{public}d", level);
    if (brFd <= 0) {
        brFd = open("/sys/class/backlight/sprd_backlight/brightness", O_RDWR);
        if (brFd < 0) {
            DISPLAY_LOGE("open brightness file failed\n");
            return DISPLAY_NOT_SUPPORT;
        }
    }
    errno_t ret = memset_s((void *)buffer, sizeof(buffer), 0, sizeof(buffer));
    if (ret != EOK) {
        DISPLAY_LOGE("memset_s failed\n");
        return DISPLAY_FAILURE;
    }

    int bytes = sprintf_s(buffer, sizeof(buffer), "%d\n", level);
    if (bytes < 0) {
        DISPLAY_LOGE("change failed\n");
        return DISPLAY_FAILURE;
    }
    write(brFd, buffer, bytes);
    mBrightnessLevel = level;
    return DISPLAY_SUCCESS;
}
~~~

## TP Adaptation

![image-20241118130818423](figures/tp.png)

WUKONG100's TP driver uses Linux native drivers. When OH starts, the init process loads the touch driver .ko file through insmod and creates the input device node /dev/input/event2;

When the touchscreen generates input events, the third-party library libinput is used to detect and receive input events. At the same time, libinput interfaces with the LibinputAdapter on the multimodal input subsystem server side. The multimodal server normalizes and standardizes input events and then distributes them to the ArkUI framework through innerSDK. The ArkUI framework encapsulates events and forwards them to applications, or innerSDK directly distributes events to applications through the JsKit interface.

For more information about the multimodal subsystem, please refer to: https://gitee.com/openharmony/multimodalinput_input

TP driver adaptation includes device tree node configuration, driver program compilation, and loading configuration:

### Device Tree Configuration

Add touch node configuration in the corresponding dts file:

```patch
&i2c3 {
        #address-cells = <1>;
        #size-cells = <0>;

        status = "okay";
        touchscreen@40 {
                compatible = "gslx680,gslx680_ts";
                reg = <0x40>;
                reset-gpio = <&ap_gpio 14 0>;
                irq-gpio = <&ap_gpio 13 0>;
        };
};	
```

### TP ko Compilation and Loading

Loading the TP driver as a ko can optimize kernel startup speed and is also more flexible.

The TP driver is provided by the manufacturer.

>There are two ways to adapt TP drivers:
>1. Directly use Linux native drivers, which can refer to this document.
>2. Use the HDF framework to adapt drivers, refer to: [TP Driver Model](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/porting/porting-dayu200-on_standard-demo.md#tp)

#### ko Installation to img

~~~gn
ohos_prebuilt_executable("gslx680") {
  deps = [ ":build_modules" ]
  source = "$root_build_dir/modules/gslx680/gslx680.ko"
  module_install_dir = "modules"
  install_images = [ chipset_base_dir ]
  part_name = "product_wukong100"
  install_enable = true
}

group("modules") {
  deps = [
    ......
    ":regulatory.db.p7s",
    ":mali_kbase",
    ":gslx680",
~~~

#### ko Boot Installation

~~~
"insmod /vendor/modules/gslx680.ko"
~~~

After successful installation, if the driver is working properly, a new input node will be created:

~~~
/dev/input/eventX
~~~

### TP Driver Testing

Test using the getevent tool:

~~~
# ./getevent -l
add device 1: /dev/input/event5
  name:     "VSoC touchscreen"
add device 2: /dev/input/event4
  name:     "sprdphone-sc2730 Headset Keyboard"
add device 3: /dev/input/event3
  name:     "sprdphone-sc2730 Headset Jack"
add device 4: /dev/input/event2
  name:     "gsl680_tp"
add device 5: /dev/input/event1
  name:     "gpio-keys"
add device 6: /dev/input/event0
  name:     "sc27xx:vibrator"
/dev/input/event2: EV_SYN       SYN_MT_REPORT        00000000
/dev/input/event2: EV_SYN       SYN_REPORT           00000000
/dev/input/event2: EV_ABS       ABS_MT_TOUCH_MAJOR   0000000a
/dev/input/event2: EV_ABS       ABS_MT_POSITION_X    00000164
/dev/input/event2: EV_ABS       ABS_MT_POSITION_Y    00000506
/dev/input/event2: EV_ABS       ABS_MT_TRACKING_ID   00000001
/dev/input/event2: EV_KEY       BTN_TOUCH            DOWN
/dev/input/event2: EV_SYN       SYN_MT_REPORT        00000000
/dev/input/event2: EV_SYN       SYN_REPORT           00000000
~~~

This indicates that the touch driver is working properly.
