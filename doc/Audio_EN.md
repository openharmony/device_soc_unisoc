# WUKONG100 UIS7885 Chip Audio Adaptation



###Audio Driver Framework Introduction

The Audio driver framework is implemented based on the HDF driver framework. The composition of the Audio driver framework is as follows:

![](figures/audio驱动框架.jpg)

The community RK3568 uses ADM adaptation. WUKONG100 previously used tinyasla, therefore we adapted the ALSA framework on OpenHarmony.

The four integration solutions selected for the audio driver are as follows:



![](figures/audio_适配方案.jpg)

The audio driver framework model serves the multimedia audio subsystem upwards, making it easier for system developers to develop applications based on scenarios. It serves specific device manufacturers downwards; chip manufacturers can develop their respective drivers and call HAL layer interfaces based on this driver architecture.

For chips, the audio driver already supports ALSA. By interfacing the ALSA library to the HDF layer, rapid development and adaptation to the OpenHarmony system can be achieved.

### ALSA适配

ALSA uses the built-in third-party library alsa-lib on OpenHarmony to interface with the underlying audio driver of WUKONG100. The third-party toolkit alsa-utils is integrated to verify the normal operation of the underlying driver device nodes.

```
# aplay -l
**** List of PLAYBACK Hardware Devices ****
card 0: sprdphonesc2730 [sprdphone-sc2730], device 0: FE_ST_NORMAL_AP01 (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: sprdphonesc2730 [sprdphone-sc2730], device 1: FE_ST_NORMAL_AP23 (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: sprdphonesc2730 [sprdphone-sc2730], device 3: FE_ST_FAST (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: sprdphonesc2730 [sprdphone-sc2730], device 5: FE_ST_VOICE (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: sprdphonesc2730 [sprdphone-sc2730], device 6: FE_ST_VOIP (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: sprdphonesc2730 [sprdphone-sc2730], device 7: FE_ST_FM (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: sprdphonesc2730 [sprdphone-sc2730], device 10: FE_ST_LOOP (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: sprdphonesc2730 [sprdphone-sc2730], device 12: FE_ST_A2DP_PCM (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: sprdphonesc2730 [sprdphone-sc2730], device 15: FE_ST_FM_DSP (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: sprdphonesc2730 [sprdphone-sc2730], device 18: FE_ST_VOICE_PCM_P (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: sprdphonesc2730 [sprdphone-sc2730], device 19: FE_ST_TEST_CODEC (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: sprdphonesc2730 [sprdphone-sc2730], device 24: DisplayPort MultiMedia (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: sprdphonesc2730 [sprdphone-sc2730], device 25: FE_ST_MM_P (*) []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
```

After configuring the audio path for playback and recording using amixer, use aplay and arecord to test the playback and recording effects of the device driver.

```
aplay -M -D hw:0,0 /etc/dynamic.wav
arecord -M -D hw:0,0 -r 8000 -c 1 -f S16_LE --period-size=640 --buffer-size=2560 /data/arecold.wav
```

### HDF Layer Adaptation

1. The community mainline uses ADM, which does not enable ALSA compilation by default. Therefore, it needs to be enabled in the config.json file in the product directory

   ```
   drivers_peripheral_audio_feature_alsa_lib = true
   ```

2.The ALSA-related configuration files in the //vendor/revoview/wukong100/hals/audio directory need to be modified.

   ```
   {
       "adapters": [
           {
               "name": "primary",
               "cardId": 0,
               "cardName": "sprdphonesc2730"
           },
           {
               "name": "hdmi",
               "cardId": 1,
               "cardName": "rockchiphdmi"
           }
       ]
   }
   
   ```

  alsa_paths.json is the configuration file for audio paths. Audio paths for Speaker, Mic, etc., in different scenarios need to be written into the corresponding configuration.

audio_policy_config.xml is the core audio policy configuration file. Audio stream types, usage scenarios, audio ports, routing rules, etc., need to be configured here.

For call routing switching and earpiece/speaker switching, set updateRouteSupport to true to update the audio route.

   ```
        <globalConfigs>
    	         <defaultOutput adapter="primary" pipe="primary_output" device="Speaker"/>
    	         <commonConfigs>
    	             <attribute name="updateRouteSupport" value="true"/>
    	             <attribute name="maxRendereres" value="128"/>
    	             <attribute name="maxCapturers" value="16"/>
    	         </commonConfigs>
   ```

3. There was an issue with parsing the alsa_paths.json file in the community mainline. After modification, it has been merged into the mainline.

   ```
    #include "cJSON.h"
    	 #include "osal_mem.h"
    	 #include "securec.h"
    	 #include "audio_common.h"
    	 
    	 #ifdef IDL_MODE
    	 #define HDF_LOG_TAG AUDIO_HDI_IMPL
   @@ -27,9 +28,9 @@	 
    	 
    	 #define SPEAKER                   "Speaker"
    	 #define HEADPHONES                "Headphones"
    	 #define MIC                       "Mic"
    	 #define HS_MIC                    "MicHs"
    	 #define EARPIECE                  "Earpiece"
    	 #define BLUETOOTH_SCO             "Bluetooth"
    	 #define BLUETOOTH_SCO_HEADSET     "Bluetooth_SCO_Headset"
    	 #define JSON_UNPRINT 1
   @@ -42,7 +43,8 @@	 
    	 #define AUDIO_DEV_ON  1
    	 #define AUDIO_DEV_OFF 0
    	 
    	 #define HDF_PATH_NUM_MAX (32 * 4)
    	 #define ADM_VALUE_SIZE 4
   ```

4. Added modifications for device switching and routing information switching, which have been merged into the mainline.

   ```
    int32_t AudioAdapterUpdateAudioRoute(
    	     struct IAudioAdapter *adapter, const struct AudioRoute *route, int32_t *routeHandle)
    	 {
    	     AUDIO_FUNC_LOGI("Enter.");
    	     struct AudioHwAdapter *hwAdapter = (struct AudioHwAdapter *)adapter;
    	     if (hwAdapter == NULL) {
    	         AUDIO_FUNC_LOGE("AudioAdapterUpdateAudioRoute Invalid input param!");
    	         return AUDIO_ERR_INVALID_PARAM;
    	     }
    	     if (renderId_ <= -1 || renderId_ >= MAX_AUDIO_STREAM_NUM) {
    	         AUDIO_FUNC_LOGE("render is Invalid");
    	         return AUDIO_ERR_INVALID_PARAM;
    	     }
    	     AUDIO_FUNC_LOGI("AudioAdapterUpdateAudioRoute renderId_: %{public}d", renderId_);
    	     struct IAudioRender *render = (struct IAudioRender *)hwAdapter->infos.renderServicePtr[renderId_];
    	     struct AudioHwRender *hwRender = (struct AudioHwRender *)render;
    	     if (hwRender == NULL) {
    	         AUDIO_FUNC_LOGE("hwRender is NULL!");
    	         return AUDIO_ERR_INTERNAL;
    	     }
    	 
    	     if (hwRender->devCtlHandle == NULL) {
    	         AUDIO_FUNC_LOGE("RenderSetVoiceVolume Bind Fail!");
    	         return AUDIO_ERR_INTERNAL;
    	     }
    	 
    	     InterfaceLibModeRenderPassthrough *pInterfaceLibModeRender = AudioPassthroughGetInterfaceLibModeRender();
    	     if (pInterfaceLibModeRender == NULL || *pInterfaceLibModeRender == NULL) {
    	         AUDIO_FUNC_LOGE("InterfaceLibModeRender not exist");
    	         return HDF_FAILURE;
    	     }
    	 
    	     int32_t ret =
    	         (*pInterfaceLibModeRender)(hwRender->devCtlHandle, &hwRender->renderParam, AUDIODRV_CTL_IOCTL_UPDATE_ROUTER);
    	     if (ret < 0) {
    	         AUDIO_FUNC_LOGE("Audio RENDER_CLOSE FAIL");
    	     }
    	 
    	     return AUDIO_SUCCESS;
    	 }
   ```

5. Added call volume adjustment, which has been merged into the mainline.

   ```
   int32_t AudioAdapterSetVoiceVolume(struct IAudioAdapter *adapter, float volume)
    	 {
    	     AUDIO_FUNC_LOGD("AudioAdapterSetVoiceVolume Enter.");
    	     int32_t ret = 0;
    	     struct AudioHwAdapter *hwAdapter = (struct AudioHwAdapter *)adapter;
    	     if (hwAdapter == NULL) {
    	         AUDIO_FUNC_LOGE("AudioAdapterSetVoiceVolume Invalid input param!");
    	         return AUDIO_ERR_INVALID_PARAM;
    	     }
    	     if (renderId_ <= -1 || renderId_ >= MAX_AUDIO_STREAM_NUM) {
    	         AUDIO_FUNC_LOGE("render is Invalid");
    	         return AUDIO_ERR_INVALID_PARAM;
    	     }
    	     AUDIO_FUNC_LOGI("AudioAdapterSetVoiceVolume renderId_: %{public}d", renderId_);
    	     struct IAudioRender *render = (struct IAudioRender *)hwAdapter->infos.renderServicePtr[renderId_];
    	     struct AudioHwRender *hwRender = (struct AudioHwRender *)render;
    	     if (hwRender == NULL) {
    	         AUDIO_FUNC_LOGE("hwRender is NULL!");
    	         return AUDIO_ERR_INTERNAL;
    	     }
    	 
    	     if (volume < 0 || volume > 1) {
    	         AUDIO_FUNC_LOGE("AudioAdapterSetVoiceVolume volume param Is error!");
    	         return AUDIO_ERR_INVALID_PARAM;
    	     }
    	     if (hwRender->devCtlHandle == NULL) {
    	         AUDIO_FUNC_LOGE("RenderSetVoiceVolume Bind Fail!");
    	         return AUDIO_ERR_INTERNAL;
    	     }
    	 
    	     InterfaceLibModeRenderPassthrough *pInterfaceLibModeRender = AudioPassthroughGetInterfaceLibModeRender();
    	     if (pInterfaceLibModeRender == NULL || *pInterfaceLibModeRender == NULL) {
    	         AUDIO_FUNC_LOGE("InterfaceLibModeRender not exist");
    	         return HDF_FAILURE;
    	     }
    	 
    	     hwRender->renderParam.renderMode.ctlParam.voiceVolume = volume;
    	 
    	     ret = (*pInterfaceLibModeRender)(hwRender->devCtlHandle, &hwRender->renderParam,
    	                                      AUDIODRV_CTL_IOCTL_VOICE_VOLUME_WRITTE);
    	     if (ret < 0) {
    	         AUDIO_FUNC_LOGE("Audio RENDER_CLOSE FAIL");
    	     }
    	     return AUDIO_SUCCESS;
    	 }
    	 
   ```

6. The community mainline RK3568 audio code does not have call functionality. WUKONG100 implemented audio calls based on the ALSA solution and re-adapted in the //device/board/revoview/wukong100/audio_alsa/ directory. This mainly involved adding handling of render and capture for call scenarios, switching audio paths when changing scenarios, and simultaneously processing render and capture devices during calls.

   ```
   int32_t RenderGetSceneDev(enum AudioCategory scene)
   {
       if (scene < AUDIO_IN_MEDIA || scene > AUDIO_MMAP_NOIRQ) {
           scene = AUDIO_IN_MEDIA;
       }
       if (scene == AUDIO_IN_CALL) {
           return SND_CALL_PCM_DEV;
       } else {
           return SND_DEFAULT_PCM_DEV;
       }
   }
   
   static bool CheckSceneIsChange(enum AudioCategory scene)
   {
       if (scene != AUDIO_IN_CALL) {
           if (g_currentScene == AUDIO_IN_CALL || g_currentScene == AUDIO_MMAP_NOIRQ) {
               return true;
           } else {
               return false;
           }
       } else {
           if (g_currentScene != AUDIO_IN_CALL) {
               return true;
           } else {
               return false;
           }
       }
   }
   
   static int32_t UpdateAudioRenderRoute(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData)
   {
       CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
       struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;
       CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
       int32_t ret;
       int32_t devCount = handleData->renderMode.hwInfo.pathSelect.deviceInfo.deviceNum;
   
       AUDIO_FUNC_LOGI("UpdateAudioRenderRoute devCount:%{public}d!", devCount);
       if (devCount < 0 || devCount > PATHPLAN_COUNT) {
           AUDIO_FUNC_LOGE("devCount is error!");
           return HDF_FAILURE;
       }
   
       struct AlsaMixerCtlElement elems[devCount];
       for (int i = 0; i < devCount; i++) {
           SndElementItemInit(&elems[i]);
   
           elems[i].numid = 0;
           elems[i].name = handleData->renderMode.hwInfo.pathSelect.deviceInfo.deviceSwitchs[i].deviceSwitch;
           elems[i].value = handleData->renderMode.hwInfo.pathSelect.deviceInfo.deviceSwitchs[i].value;
       }
   
       ret = SndElementGroupWrite(cardIns, elems, devCount);
       if (ret < 0) {
           AUDIO_FUNC_LOGE("render SndElementGroupWrite fail");
           return HDF_FAILURE;
       }
       return HDF_SUCCESS;
   }
   
   ```

### audio-framework Adaptation

As general code, WUKONG100 does not involve modifications to the audio-framework repository.

### Related Repositories

[**device_board_revoview**](https://gitcode.com/openharmony-sig/device_board_revoview)

[**vendor_revoview**](https://gitcode.com/openharmony-sig/vendor_revoview)

[**drivers_peripheral**](https://gitcode.com/openharmony/drivers_peripheral)
