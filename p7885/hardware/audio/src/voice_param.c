/*
 * Copyright 2016-2026 Unisoc (Shanghai) Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/ioctl.h>
#include "audio_common.h"
#include "alsa_snd_render.h"

#include "audio_if_lib_common.h"
#include "hdf_io_service_if.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "hdf_sbuf.h"
#include "audio_uhdf_log.h"
#include "securec.h"
#include "local.h"
#include <string.h>
#include <sys/wait.h>

#define HDF_LOG_TAG HDF_AUDIO_HAL_LIB
#define AGDSP_CTL_PIPE "/dev/audio_pipe_voice"
#define AUDIO_PIPE_MARGIC 'A'
#define AUDIO_PIPE_WAKEUP             _IOW(AUDIO_PIPE_MARGIC, 0, int)
#define PARAM_KCONTROL_LENTH            32
#define THREAD_SLEEP_TIME               200000
#define AUDIO_SREUCTURE_PROFILE_SELECT      "Audio Structure Profile Select"
#define NXP_PROFILE_SELECT                  "NXP Profile Select"
#define PARAM_ID_HIGH_SHIFT             24
#define PARAM_ID_LOW_SHIFT              16

// net_mode
enum {
    VOICE_NET_MODE_NONE     = 0x0,
    VOICE_NET_MODE_TDMA     = 0x1,
    VOICE_NET_MODE_GSM      = 0x2,
    VOICE_NET_MODE_WCDMA    = 0x4,
    VOICE_NET_MODE_VOLTE    = 0x10,
    VOICE_NET_MODE_VOWIFI   = 0x20,
    VOICE_NET_MODE_CDMA2000 = 0x40,
    VOICE_NET_MODE_LOOPBACK = 0x80,
};

// net_band
enum {
    VOICE_NET_BAND_NONE = 0,
    VOICE_NET_BAND_NB = VOICE_NET_BAND_NONE,
    VOICE_NET_BAND_WB,
    VOICE_NET_BAND_SWB,
    VOICE_NET_BAND_FB,
};

// adsp massage类型，目前暂时只用到AGDSP_CMD_NET_MSG
typedef enum {
    AGDSP_CMD_NET_MSG          = 0x0,
    AGDSP_CMD_ASSERT           = 0x25,
    AGDSP_CMD_TIMEOUT          = 0x26,
    AGDSP_CMD_ASSERT_EPIPE     = 0x27,
    AGDSP_CMD_SMARTAMP_CALI    = 0x28,
    AGDSP_CMD_SMARTAMP_FUNC    = 0x29,
    AGDSP_CMD_SMARTAMP_PT      = 0x30,
    AGDSP_CMD_AD_DA_CHECK      = 0x31,
    AGDSP_CMD_DSP_VERSION      = 0x33,
    AGDSP_CMD_AP_LOG_DEBUG     = 0x34,
    AGDSP_CMD_CP_VOICE         = 0x35,
    AGDSP_CMD_DSP_INFO_DUMP    = 0x37,
    AGDSP_CMD_STATUS_CHECK     = 0x1234,
    AGDSP_CMD_BOOT_SUCCESS     = 0xbeee,
} AgdspCmdType;

// output device的定义
enum {
    AUDIO_DEVICE_NONE                          = 0x0u,
    AUDIO_DEVICE_OUT_EARPIECE                  = 0x1u,
    AUDIO_DEVICE_OUT_SPEAKER                   = 0x2u,
    AUDIO_DEVICE_OUT_WIRED_HEADSET             = 0x4u,
    AUDIO_DEVICE_OUT_WIRED_HEADPHONE           = 0x8u,
    AUDIO_DEVICE_OUT_BLUETOOTH_SCO             = 0x10u,
    AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET     = 0x20u,
    AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT      = 0x40u,
    AUDIO_DEVICE_OUT_BLUETOOTH_A2DP            = 0x80u,
    AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES = 0x100u,
    AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER    = 0x200u,
    AUDIO_DEVICE_BIT_IN                        = 0x80000000u,
    AUDIO_DEVICE_BIT_DEFAULT                   = 0x40000000u,
};

// dsp_case定义，目前暂时只用到DAI_ID_VOICE
typedef enum {
    DAI_ID_NORMAL_OUTDSP_PLAYBACK = 0,
    DAI_ID_NORMAL_OUTDSP_CAPTURE,
    DAI_ID_NORMAL_WITHDSP,
    DAI_ID_FAST_P,
    DAI_ID_OFFLOAD,
    DAI_ID_VOICE,
    DAI_ID_VOIP,
    DAI_ID_FM,
    DAI_ID_FM_C_WITHDSP,
    DAI_ID_VOICE_CAPTURE,
    DAI_ID_LOOP,
    DAI_ID_BT_CAPTURE,
} VBC_DAI_ID_T;

// param_id，全部参数的定义
typedef enum {
    PROFILE_AUDIO_MODE_START = 0,
    PROFILE_MODE_AUDIO_HANDSET_NB1 = PROFILE_AUDIO_MODE_START,
    PROFILE_MODE_AUDIO_HANDSET_NB2,
    PROFILE_MODE_AUDIO_HANDSET_WB1,
    PROFILE_MODE_AUDIO_HANDSET_WB2,
    PROFILE_MODE_AUDIO_HANDSET_SWB1,
    PROFILE_MODE_AUDIO_HANDSET_FB1,
    PROFILE_MODE_AUDIO_HANDSET_VOIP1,

    PROFILE_MODE_AUDIO_HANDSFREE_NB1,
    PROFILE_MODE_AUDIO_HANDSFREE_NB2,
    PROFILE_MODE_AUDIO_HANDSFREE_WB1,
    PROFILE_MODE_AUDIO_HANDSFREE_WB2,
    PROFILE_MODE_AUDIO_HANDSFREE_SWB1,
    PROFILE_MODE_AUDIO_HANDSFREE_FB1,
    PROFILE_MODE_AUDIO_HANDSFREE_VOIP1,

    PROFILE_MODE_AUDIO_HEADSET4P_NB1,
    PROFILE_MODE_AUDIO_HEADSET4P_NB2,
    PROFILE_MODE_AUDIO_HEADSET4P_WB1,
    PROFILE_MODE_AUDIO_HEADSET4P_WB2,
    PROFILE_MODE_AUDIO_HEADSET4P_SWB1,
    PROFILE_MODE_AUDIO_HEADSET4P_FB1,
    PROFILE_MODE_AUDIO_HEADSET4P_VOIP1,

    PROFILE_MODE_AUDIO_HEADSET3P_NB1,
    PROFILE_MODE_AUDIO_HEADSET3P_NB2,
    PROFILE_MODE_AUDIO_HEADSET3P_WB1,
    PROFILE_MODE_AUDIO_HEADSET3P_WB2,
    PROFILE_MODE_AUDIO_HEADSET3P_SWB1,
    PROFILE_MODE_AUDIO_HEADSET3P_FB1,
    PROFILE_MODE_AUDIO_HEADSET3P_VOIP1,

    PROFILE_MODE_AUDIO_BTHS_NB1,
    PROFILE_MODE_AUDIO_BTHS_NB2,
    PROFILE_MODE_AUDIO_BTHS_WB1,
    PROFILE_MODE_AUDIO_BTHS_WB2,
    PROFILE_MODE_AUDIO_BTHS_SWB1,
    PROFILE_MODE_AUDIO_BTHS_FB1,
    PROFILE_MODE_AUDIO_BTHS_VOIP1,

    PROFILE_MODE_AUDIO_CARKIT_NB1,
    PROFILE_MODE_AUDIO_CARKIT_NB2,
    PROFILE_MODE_AUDIO_CARKIT_WB1,
    PROFILE_MODE_AUDIO_CARKIT_WB2,
    PROFILE_MODE_AUDIO_CARKIT_SWB1,
    PROFILE_MODE_AUDIO_CARKIT_FB1,
    PROFILE_MODE_AUDIO_CARKIT_VOIP1,

    PROFILE_MODE_AUDIO_BTHSNREC_NB1,
    PROFILE_MODE_AUDIO_BTHSNREC_NB2,
    PROFILE_MODE_AUDIO_BTHSNREC_WB1,
    PROFILE_MODE_AUDIO_BTHSNREC_WB2,
    PROFILE_MODE_AUDIO_BTHSNREC_SWB1,
    PROFILE_MODE_AUDIO_BTHSNREC_FB1,
    PROFILE_MODE_AUDIO_BTHSNREC_VOIP1,

    PROFILE_MODE_AUDIO_TYPEC_NB1,
    PROFILE_MODE_AUDIO_TYPEC_NB2,
    PROFILE_MODE_AUDIO_TYPEC_WB1,
    PROFILE_MODE_AUDIO_TYPEC_WB2,
    PROFILE_MODE_AUDIO_TYPEC_SWB1,
    PROFILE_MODE_AUDIO_TYPEC_FB1,
    PROFILE_MODE_AUDIO_TYPEC_VOIP1,

    PROFILE_MODE_AUDIO_HIGHVOLUME_NB1,
    PROFILE_MODE_AUDIO_HIGHVOLUME_NB2,
    PROFILE_MODE_AUDIO_HIGHVOLUME_WB1,
    PROFILE_MODE_AUDIO_HIGHVOLUME_WB2,
    PROFILE_MODE_AUDIO_HIGHVOLUME_SWB1,
    PROFILE_MODE_AUDIO_HIGHVOLUME_FB1,
    PROFILE_MODE_AUDIO_HIGHVOLUME_VOIP1,

    PROFILE_MODE_AUDIO_HAC_NB1,
    PROFILE_MODE_AUDIO_HAC_NB2,
    PROFILE_MODE_AUDIO_HAC_WB1,
    PROFILE_MODE_AUDIO_HAC_WB2,
    PROFILE_MODE_AUDIO_HAC_SWB1,
    PROFILE_MODE_AUDIO_HAC_FB1,
    PROFILE_MODE_AUDIO_HAC_VOIP1,

    PROFILE_MUSIC_MODE_START,
    PROFILE_MODE_MUSIC_HEADSET_PLAYBACK = PROFILE_MUSIC_MODE_START,
    PROFILE_MODE_MUSIC_HEADSET_RECORD,
    PROFILE_MODE_MUSIC_HEADSET_UNPROCESSRECORD,
    PROFILE_MODE_MUSIC_HEADSET_RECOGNITION,
    PROFILE_MODE_MUSIC_HEADSET_FM,

    PROFILE_MODE_MUSIC_HANDSFREE_PLAYBACK,
    PROFILE_MODE_MUSIC_HANDSFREE_RECORD,
    PROFILE_MODE_MUSIC_HANDSFREE_UNPROCESSRECORD,
    PROFILE_MODE_MUSIC_HANDSFREE_VIDEORECORD,
    PROFILE_MODE_MUSIC_HANDSFREE_RECOGNITION,
    PROFILE_MODE_MUSIC_HANDSFREE_FM,

    PROFILE_MODE_MUSIC_TYPEC_PLAYBACK,
    PROFILE_MODE_MUSIC_TYPEC_RECORD,
    PROFILE_MODE_MUSIC_TYPEC_UNPROCESSRECORD,
    PROFILE_MODE_MUSIC_TYPEC_RECOGNITION,
    PROFILE_MODE_MUSIC_TYPEC_FM,

    PROFILE_MODE_MUSIC_HEADFREE_PLAYBACK,
    PROFILE_MODE_MUSIC_HANDSET_PLAYBACK,
    PROFILE_MODE_MUSIC_BLUETOOTH_RECORD,

    PROFILE_LOOP_MODE_START,
    PROFILE_MODE_LOOP_HANDSET_LOOP1 = PROFILE_LOOP_MODE_START,
    PROFILE_MODE_LOOP_HANDSFREE_LOOP1,
    PROFILE_MODE_LOOP_HEADSET4P_LOOP1,
    PROFILE_MODE_LOOP_HEADSET3P_LOOP1,

    PROFILE_MODE_MAX,
} AUDIO_PARAM_PROFILE_MODE_E;

struct voice_data {
    int32_t netMode;
    int32_t netBand;
};

struct audio_device {
    struct voice_data voice;
    pthread_mutex_t lock;
    int32_t outDevice;
};

struct agdsp_data {
    struct audio_device* adev;

    pthread_t rxThread;
    pthread_cond_t rxCond;
    pthread_mutex_t rxLock;
    bool rxExit;
    int32_t pipeFd;
};

struct agdsp_msg_info {
    uint16_t command;
    uint16_t channel;
    uint32_t param0;
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
};

struct agdsp_data *g_agdsp = NULL;

static int32_t GetHandsetParam(int32_t net_mode, int32_t net_band)
{
    int32_t paramId = PROFILE_MODE_MAX;
    switch (net_mode) {
        case VOICE_NET_MODE_TDMA:
        case VOICE_NET_MODE_GSM:
            if (net_band == VOICE_NET_BAND_NB) {
                paramId = PROFILE_MODE_AUDIO_HANDSET_NB1;
            }
            break;
        case VOICE_NET_MODE_WCDMA:
        case VOICE_NET_MODE_VOWIFI:
            if (net_band == VOICE_NET_BAND_NB) {
                paramId = PROFILE_MODE_AUDIO_HANDSET_NB1;
            } else if (net_band == VOICE_NET_BAND_WB) {
                paramId = PROFILE_MODE_AUDIO_HANDSET_WB1;
            }
            break;
        case VOICE_NET_MODE_VOLTE:
            if (net_band == VOICE_NET_BAND_NB) {
                paramId = PROFILE_MODE_AUDIO_HANDSET_NB1;
            } else if (net_band == VOICE_NET_BAND_WB) {
                paramId = PROFILE_MODE_AUDIO_HANDSET_WB1;
            } else if (net_band == VOICE_NET_BAND_SWB) {
                paramId = PROFILE_MODE_AUDIO_HANDSET_SWB1;
            } else if (net_band == VOICE_NET_BAND_FB) {
                paramId = PROFILE_MODE_AUDIO_HANDSET_FB1;
            }
            break;
        case VOICE_NET_MODE_CDMA2000:
            if (net_band == VOICE_NET_BAND_WB) {
                paramId = PROFILE_MODE_AUDIO_HANDSET_WB1;
            }
            break;
        default:
            break;
    }
    return paramId;
}

static int32_t GetHandsfreeParam(int32_t net_mode, int32_t net_band)
{
    int32_t paramId = PROFILE_MODE_MAX;
    switch (net_mode) {
        case VOICE_NET_MODE_TDMA:
        case VOICE_NET_MODE_GSM:
            if (net_band == VOICE_NET_BAND_NB) {
                paramId = PROFILE_MODE_AUDIO_HANDSFREE_NB1;
            }
            break;
        case VOICE_NET_MODE_WCDMA:
        case VOICE_NET_MODE_VOWIFI:
            if (net_band == VOICE_NET_BAND_NB) {
                paramId = PROFILE_MODE_AUDIO_HANDSFREE_NB1;
            } else if (net_band == VOICE_NET_BAND_WB) {
                paramId = PROFILE_MODE_AUDIO_HANDSFREE_WB1;
            }
            break;
        case VOICE_NET_MODE_VOLTE:
            if (net_band == VOICE_NET_BAND_NB) {
                paramId = PROFILE_MODE_AUDIO_HANDSFREE_NB1;
            } else if (net_band == VOICE_NET_BAND_WB) {
                paramId = PROFILE_MODE_AUDIO_HANDSFREE_WB1;
            } else if (net_band == VOICE_NET_BAND_SWB) {
                paramId = PROFILE_MODE_AUDIO_HANDSFREE_SWB1;
            } else if (net_band == VOICE_NET_BAND_FB) {
                paramId = PROFILE_MODE_AUDIO_HANDSFREE_FB1;
            }
            break;
        case VOICE_NET_MODE_CDMA2000:
            if (net_band == VOICE_NET_BAND_WB) {
                paramId = PROFILE_MODE_AUDIO_HANDSFREE_WB1;
            }
            break;
        default:
            break;
    }
    return paramId;
}

static int32_t GetHeadset3pParam(int32_t net_mode, int32_t net_band)
{
    int32_t paramId = PROFILE_MODE_MAX;
    switch (net_mode) {
        case VOICE_NET_MODE_TDMA:
        case VOICE_NET_MODE_GSM:
            if (net_band == VOICE_NET_BAND_NB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET3P_NB1;
            }
            break;
        case VOICE_NET_MODE_WCDMA:
        case VOICE_NET_MODE_VOWIFI:
            if (net_band == VOICE_NET_BAND_NB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET3P_NB1;
            } else if (net_band == VOICE_NET_BAND_WB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET3P_WB1;
            }
            break;
        case VOICE_NET_MODE_VOLTE:
            if (net_band == VOICE_NET_BAND_NB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET3P_NB1;
            } else if (net_band == VOICE_NET_BAND_WB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET3P_WB1;
            } else if (net_band == VOICE_NET_BAND_SWB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET3P_SWB1;
            } else if (net_band == VOICE_NET_BAND_FB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET3P_FB1;
            }
            break;
        case VOICE_NET_MODE_CDMA2000:
            if (net_band == VOICE_NET_BAND_WB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET3P_WB1;
            }
            break;
        default:
            break;
    }
    return paramId;
}

static int32_t GetHeadset4pParam(int32_t net_mode, int32_t net_band)
{
    int32_t paramId = PROFILE_MODE_MAX;
    switch (net_mode) {
        case VOICE_NET_MODE_TDMA:
        case VOICE_NET_MODE_GSM:
            if (net_band == VOICE_NET_BAND_NB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET4P_NB1;
            }
            break;
        case VOICE_NET_MODE_WCDMA:
        case VOICE_NET_MODE_VOWIFI:
            if (net_band == VOICE_NET_BAND_NB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET4P_NB1;
            } else if (net_band == VOICE_NET_BAND_WB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET4P_WB1;
            }
            break;
        case VOICE_NET_MODE_VOLTE:
            if (net_band == VOICE_NET_BAND_NB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET4P_NB1;
            } else if (net_band == VOICE_NET_BAND_WB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET4P_WB1;
            } else if (net_band == VOICE_NET_BAND_SWB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET4P_SWB1;
            } else if (net_band == VOICE_NET_BAND_FB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET4P_FB1;
            }
            break;
        case VOICE_NET_MODE_CDMA2000:
            if (net_band == VOICE_NET_BAND_WB) {
                paramId = PROFILE_MODE_AUDIO_HEADSET4P_WB1;
            }
            break;
        default:
            break;
    }
    return paramId;
}

static int32_t GetBtParam(bool is_nrec, int32_t net_mode, int32_t net_band)
{
    int32_t base_id = PROFILE_MODE_MAX;
    switch (net_mode) {
        case VOICE_NET_MODE_TDMA:
        case VOICE_NET_MODE_GSM:
            if (net_band == VOICE_NET_BAND_NB) base_id = PROFILE_MODE_AUDIO_BTHS_NB1;
            break;
            
        case VOICE_NET_MODE_WCDMA:
        case VOICE_NET_MODE_VOWIFI:
            if (net_band == VOICE_NET_BAND_NB) base_id = PROFILE_MODE_AUDIO_BTHS_NB1;
            else if (net_band == VOICE_NET_BAND_WB) base_id = PROFILE_MODE_AUDIO_BTHS_WB1;
            break;
            
        case VOICE_NET_MODE_VOLTE:
            if (net_band == VOICE_NET_BAND_NB) base_id = PROFILE_MODE_AUDIO_BTHS_NB1;
            else if (net_band == VOICE_NET_BAND_WB) base_id = PROFILE_MODE_AUDIO_BTHS_WB1;
            else if (net_band == VOICE_NET_BAND_SWB) base_id = PROFILE_MODE_AUDIO_BTHS_SWB1;
            else if (net_band == VOICE_NET_BAND_FB) base_id = PROFILE_MODE_AUDIO_BTHS_FB1;
            break;
            
        case VOICE_NET_MODE_CDMA2000:
            if (net_band == VOICE_NET_BAND_WB) base_id = PROFILE_MODE_AUDIO_BTHS_WB1;
            break;
            
        default:
            break;
    }

    // 如果没找到匹配的基础 ID，直接返回
    if (base_id == PROFILE_MODE_MAX) {
        return PROFILE_MODE_MAX;
    }

    if (!is_nrec) {
        return base_id;
    }

    // 映射逻辑 (如果宏有规律可直接计算，否则 switch)
    switch (base_id) {
        case PROFILE_MODE_AUDIO_BTHS_NB1:  return PROFILE_MODE_AUDIO_BTHSNREC_NB1;
        case PROFILE_MODE_AUDIO_BTHS_WB1:  return PROFILE_MODE_AUDIO_BTHSNREC_WB1;
        case PROFILE_MODE_AUDIO_BTHS_SWB1: return PROFILE_MODE_AUDIO_BTHSNREC_SWB1;
        case PROFILE_MODE_AUDIO_BTHS_FB1:  return PROFILE_MODE_AUDIO_BTHSNREC_FB1;
        default: return PROFILE_MODE_MAX; // Should not happen
    }
}

static int32_t GetPlaybackParam(int32_t devices)
{
    int32_t paramId = 0;
    if (devices == AUDIO_DEVICE_OUT_EARPIECE) {
        paramId = PROFILE_MODE_MUSIC_HANDSET_PLAYBACK;
    } else if (devices == AUDIO_DEVICE_OUT_SPEAKER) {
        paramId = PROFILE_MODE_MUSIC_HANDSFREE_PLAYBACK;
    } else if (devices == AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET) {
        paramId = PROFILE_MODE_MUSIC_HEADSET_PLAYBACK;
    } else if (devices == AUDIO_DEVICE_OUT_WIRED_HEADSET) {
        paramId = PROFILE_MODE_MUSIC_HEADSET_PLAYBACK;
    } else if (devices == AUDIO_DEVICE_OUT_WIRED_HEADPHONE) {
        paramId = PROFILE_MODE_MUSIC_HEADSET_PLAYBACK;
    }
    return paramId;
}

// 根据output device、net_mode、net_band得到param_id
static int32_t GetVoiceParam(int32_t devices, bool is_nrec, int32_t net_mode, int32_t net_band)
{
    int32_t paramId = 0;
    if (devices == AUDIO_DEVICE_OUT_EARPIECE) {
        paramId = GetHandsetParam(net_mode, net_band);
    } else if (devices == AUDIO_DEVICE_OUT_SPEAKER) {
        paramId = GetHandsfreeParam(net_mode, net_band);
    } else if (devices == AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET || devices == AUDIO_DEVICE_OUT_BLUETOOTH_SCO) {
        paramId = GetBtParam(is_nrec, net_mode, net_band);
    } else if (devices == AUDIO_DEVICE_OUT_WIRED_HEADSET) {
        paramId = GetHeadset4pParam(net_mode, net_band);
    } else if (devices == AUDIO_DEVICE_OUT_WIRED_HEADPHONE) {
        paramId = GetHeadset3pParam(net_mode, net_band);
    } else {
        paramId = 0;
    }
    return paramId;
}

// make_param_kcontrol_value将offset、param_id和dsp_case组合成需要设置给kcontrol的32bit数据
// 目前offset和param_id设置成一样的都用param_id的值就可以了
static int32_t MakeParamKcontrolValue(uint8_t param_id, uint8_t dsp_case)
{
    return ((param_id<<PARAM_ID_HIGH_SHIFT)|(param_id<<PARAM_ID_LOW_SHIFT)|dsp_case);
}

/*
 * value of paramKcontrolVal
 * 16bit, 8bit, 8bit
 * (offset<<24)|(paramId<<16)|dspCase
 * offset: bit 24-31
 * paramId: bit 16-23
 * dspCase: bit 0-15
 */
// get_voice_param用于获取paramId，paramId即AUDIO_PARAM_PROFILE_MODE_E枚举中定义的值
// get_voice_param需要三个入参：net_mode以及net_band，另外还需要通话的输出设备（听筒、扬声器、蓝牙、耳机等）
// dspCase表示当前的场景，这里不用改，设置成DAI_ID_VOICE就可以了

static int32_t SetAudioParam(struct audio_device *adev)
{
    uint8_t paramId = 0;
    uint8_t dspCase = 0;
    uint32_t paramKcontrolVal = 0;
    int32_t ret;
    char command[128];
    int snprintfRet;
    const char *kcontrolNames[] = { AUDIO_SREUCTURE_PROFILE_SELECT, NXP_PROFILE_SELECT };
    size_t i;

    paramId = (uint8_t)GetVoiceParam(adev->outDevice, false, adev->voice.netMode, adev->voice.netBand);
    AUDIO_FUNC_LOGI("voice_param SetAudioParam, paramId:%{public}d", paramId);
    dspCase = DAI_ID_VOICE;

    if (paramId >= PROFILE_MODE_MAX) {
        paramId = (uint8_t)GetPlaybackParam(adev->outDevice);
    }

    paramKcontrolVal = MakeParamKcontrolValue(paramId, dspCase);
    AUDIO_FUNC_LOGI("voice_param SetAudioParam, paramKcontrolVal:%{public}d", paramKcontrolVal);
    for (i = 0; i < sizeof(kcontrolNames) / sizeof(kcontrolNames[0]); i++) {
        const char *ctlName = kcontrolNames[i];

        if (ctlName == NULL || strchr(ctlName, '\'') != NULL || strchr(ctlName, ';') != NULL) {
            AUDIO_FUNC_LOGE("Invalid control name detected!");
            return HDF_FAILURE;
        }

        snprintfRet = snprintf_s(command, sizeof(command), sizeof(command) - 1, "amixer cset name='%s' %u",
                                ctlName, paramKcontrolVal);
        if (snprintfRet < 0) {
            AUDIO_FUNC_LOGE("snprintf_s failed! Constraint violation or buffer too small. Ret: %d", snprintfRet);
            return HDF_FAILURE;
        }
        if ((size_t)snprintfRet >= sizeof(command)) {
            AUDIO_FUNC_LOGE("Unexpected behavior from snprintf_s: Ret %d >= Size %zu", snprintfRet, sizeof(command));
            return HDF_FAILURE;
        }
        AUDIO_FUNC_LOGI("voice_param command:%{public}s", command);

        ret = system(command);
        if (ret == -1) {
            AUDIO_FUNC_LOGE("system() call failed to execute command: %{public}s, errno: %{public}d", command, errno);
            return HDF_FAILURE;
        }

        int exitStatus = WEXITSTATUS(ret);
        if (exitStatus != 0) {
            AUDIO_FUNC_LOGE("amixer command failed with exit status %d: %{public}s", exitStatus, command);
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

// 将消息中的net_mode和net_rate保存到adev->voice结构体
// set_audioparam用于设置参数
static int32_t VoiceSetNetInfo(struct audio_device *adev, int32_t net_mode, int32_t net_band)
{
    AUDIO_FUNC_LOGI("voice_param VoiceSetNetInfo, net_mode(%{public}#x) net_band(%{public}d)", net_mode, net_band);

    int32_t ret;
    
    adev->voice.netMode = net_mode;
    adev->voice.netBand = net_band;
    ret = SetAudioParam(adev);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("voice_param SetAudioParam fail");
        return HDF_FAILURE;
    }
    
    return HDF_SUCCESS;
}

// 处理dsp的消息
// msg->command表示消息的类型，net_mode和net_rate分别在msg->param0, msg->param1中
static int32_t AgdspMsgProcess(struct agdsp_msg_info *msg)
{
    if (g_agdsp == NULL || msg == NULL) {
        AUDIO_FUNC_LOGE("g_agdsp or msg is NULL");
        return HDF_FAILURE;
    }
    int32_t ret = 0;
    struct audio_device *adev = g_agdsp->adev;
    AUDIO_FUNC_LOGI("AgdspMsgProcess g_agdsp:%{public}p, adev:%{public}p", g_agdsp, g_agdsp->adev);
    AUDIO_FUNC_LOGI("cmd(%{public}#x) param(%{public}#x %{public}#x %{public}#x %{public}#x)",
        msg->command, msg->param0, msg->param1, msg->param2, msg->param3);
    switch (msg->command) {
        case AGDSP_CMD_NET_MSG:
            AUDIO_FUNC_LOGI("voice_param -----------cmd(%{public}#x)-------------", msg->command);
            pthread_mutex_lock(&adev->lock);
            ret = VoiceSetNetInfo(adev, msg->param0, msg->param1);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("voice_param VoiceSetNetInfo fail");
                pthread_mutex_unlock(&adev->lock);
                return HDF_FAILURE;
            }
            pthread_mutex_unlock(&adev->lock);
            break;
        case AGDSP_CMD_ASSERT:
        case AGDSP_CMD_TIMEOUT:
        case AGDSP_CMD_ASSERT_EPIPE:
            AUDIO_FUNC_LOGI("no do something");
            break;
        default:
            break;
    }
    return HDF_SUCCESS;
}

static void *AgdspPipeProcess(void)
{
    int32_t ret = 0;
    struct agdsp_msg_info msg;

    (void)memset_s(&msg, sizeof(struct agdsp_msg_info), 0, sizeof(struct agdsp_msg_info));
    AUDIO_FUNC_LOGI("voice_param AgdspRxThreadLoop loop g_agdsp->pipeFd:%{public}d", g_agdsp->pipeFd);
    pthread_mutex_lock(&g_agdsp->rxLock);

    if (g_agdsp->pipeFd > 0) {
        ret = read(g_agdsp->pipeFd, &msg, sizeof(struct agdsp_msg_info));
    }

    pthread_mutex_unlock(&g_agdsp->rxLock);
    AUDIO_FUNC_LOGI("voice_param AgdspPipeProcess read file ret:%{public}d", ret);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("voice_param AgdspRxThreadLoop: Faile to read file");
        if (errno == EINTR && (!g_agdsp->rxExit)) {
            usleep(THREAD_SLEEP_TIME);
        }
    } else {
        AUDIO_FUNC_LOGI("voice_param AgdspPipeProcess AgdspMsgProcess");
        ret = AgdspMsgProcess(&msg);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("voice_param set msg fail");
        }
    }

    AUDIO_FUNC_LOGI("voice_param AgdspPipeProcess exit");
    return NULL;
}

// 在这个线程中打开并轮询"dev/audio_pipe_voice"设备节点
// 如果收到dsp的消息，调用agdsp_msg_process进行处理
static void *AgdspRxThreadLoop(void *arg)
{
    AUDIO_FUNC_LOGI("voice_param AgdspRxThreadLoop");
    struct agdsp_data *agdsp = (struct agdsp_data *)arg;
    if (agdsp == NULL) {
        AUDIO_FUNC_LOGE("voice_param AgdspRxThreadLoop: Argument is NULL");
        return NULL;
    }
    agdsp->pipeFd = open(AGDSP_CTL_PIPE, O_RDONLY);
    AUDIO_FUNC_LOGI("voice_param AgdspRxThreadLoop open agdsp->pipeFd:%{public}d", agdsp->pipeFd);
    if (agdsp->pipeFd < 0) {
        AUDIO_FUNC_LOGE("voice_param AgdspRxThreadLoop: open file failed");
        agdsp->rxExit = true;
        return NULL;
    }
    
    while (!agdsp->rxExit) {
        AgdspPipeProcess();
    }

    close(agdsp->pipeFd);
    agdsp->pipeFd = -1;

    AUDIO_FUNC_LOGI("voice_param AgdspRxThreadLoop exit");
    return NULL;
}

static void VoiceInit(struct audio_device *adev)
{
    AUDIO_FUNC_LOGI("voice_param VoiceInit");
    adev->voice.netMode = VOICE_NET_MODE_NONE;
    adev->voice.netBand = VOICE_NET_BAND_NONE;
    pthread_mutex_init(&adev->lock, NULL);
}

static int32_t AgdspInit(void *adev)
{
    AUDIO_FUNC_LOGI("voice_param AgdspInit");
    int32_t ret = 0;

    g_agdsp = (struct agdsp_data*)OsalMemCalloc(sizeof(struct agdsp_data));
    CHECK_NULL_PTR_RETURN_DEFAULT(g_agdsp);

    (void)memset_s(g_agdsp, sizeof(struct agdsp_data), 0, sizeof(struct agdsp_data));
    g_agdsp->adev = (struct audio_device*)adev;
    
    ret = pthread_create(&g_agdsp->rxThread, NULL, AgdspRxThreadLoop,  (void *)g_agdsp);
    if (ret) {
        AUDIO_FUNC_LOGE("AgdspInit, Failed to create rx thread");
        g_agdsp->rxExit = true;
        pthread_cond_signal(&g_agdsp->rxCond);
        pthread_join(g_agdsp->rxThread, NULL);
        pthread_cond_destroy(&g_agdsp->rxCond);
        pthread_mutex_destroy(&g_agdsp->rxLock);
        if (g_agdsp != NULL) {
            if (g_agdsp->adev != NULL) {
                free(g_agdsp->adev);
                g_agdsp->adev = NULL;
            }
            free(g_agdsp);
            g_agdsp = NULL;
        }
        return ret;
    } else {
        g_agdsp->rxExit = false;
        pthread_cond_init(&g_agdsp->rxCond, NULL);
        pthread_mutex_init(&g_agdsp->rxLock, NULL);
    }

    AUDIO_FUNC_LOGI("voice_param AgdspInit end");
    return HDF_SUCCESS;
}

int32_t UpdateDevice(enum AudioPortPin pin)
{
    AUDIO_FUNC_LOGI("voice_param UpdateDevice enter");
    if (g_agdsp == NULL || g_agdsp->adev == NULL) {
        AUDIO_FUNC_LOGE("voice_param UpdateDevice g_agdsp is NULL");
        return HDF_FAILURE;
    }
    if (pin < PIN_OUT_SPEAKER || pin > PIN_IN_BLUETOOTH_SCO_HEADSET) {
        return HDF_FAILURE;
    }
    AUDIO_FUNC_LOGI("voice_param UpdateDevice pin:%{public}d", pin);
    struct audio_device *adev = g_agdsp->adev;
    CHECK_NULL_PTR_RETURN_DEFAULT(adev);

    switch (pin) {
        case PIN_OUT_SPEAKER:
            adev->outDevice = AUDIO_DEVICE_OUT_SPEAKER;
            break;
        case PIN_OUT_HEADSET:
            adev->outDevice = AUDIO_DEVICE_OUT_WIRED_HEADSET;
            break;
        case PIN_OUT_EARPIECE:
            adev->outDevice = AUDIO_DEVICE_OUT_EARPIECE;
            break;
        case PIN_OUT_BLUETOOTH_SCO:
            adev->outDevice = AUDIO_DEVICE_OUT_BLUETOOTH_SCO;
            break;
        case PIN_OUT_DAUDIO_DEFAULT:
            adev->outDevice = AUDIO_DEVICE_NONE;
            break;
        case PIN_OUT_HEADPHONE:
            adev->outDevice = AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
            break;
        case PIN_IN_BLUETOOTH_SCO_HEADSET:
            adev->outDevice = AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET;
            break;
        default:
            adev->outDevice = AUDIO_DEVICE_NONE;
            AUDIO_FUNC_LOGE("UseCase not support!");
            break;
    }
    AUDIO_FUNC_LOGI("voice_param UpdateDevice adev->outDevice:%{public}d", adev->outDevice);
    int32_t ret = SetAudioParam(adev);
    return ret;
}

int32_t DeviceInit(const char *cardName)
{
    AUDIO_FUNC_LOGI("voice_param DeviceInit enter cardName: %{public}s", cardName);
    // audio_device结构体的初始化
    struct audio_device *adev = NULL;
    int32_t ret = 0;
    adev = (struct audio_device*)OsalMemCalloc(sizeof(struct audio_device));
    CHECK_NULL_PTR_RETURN_DEFAULT(adev);

    (void)memset_s(adev, sizeof(struct audio_device), 0, sizeof(struct audio_device));
    adev->outDevice = AUDIO_DEVICE_NONE;
    VoiceInit(adev);
    ret = AgdspInit(adev);
    return ret;
}

void DeviceClose(void)
{
    AUDIO_FUNC_LOGI("voice_param DeviceClose enter");
    if (g_agdsp == NULL) {
        return;
    }

    g_agdsp->rxExit = true;
    if (g_agdsp->pipeFd  < 0) {
        AUDIO_FUNC_LOGE("voice_param DeviceClose open pipe failed");
    } else {
        AUDIO_FUNC_LOGE("voice_param DeviceClose g_agdsp->pipeFd:%{public}d", g_agdsp->pipeFd);
        g_agdsp->rxExit = true;
        int32_t ret = ioctl(g_agdsp->pipeFd, AUDIO_PIPE_WAKEUP, 1);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("voice_param DeviceClose ioctl faild");
        }
        pthread_cond_signal(&g_agdsp->rxCond);
        pthread_join(g_agdsp->rxThread, NULL);
        pthread_cond_destroy(&g_agdsp->rxCond);
        close(g_agdsp->pipeFd);
        g_agdsp->pipeFd = -1;
    }
    pthread_mutex_destroy(&g_agdsp->rxLock);
    if (g_agdsp->adev != NULL) {
        free(g_agdsp->adev);
        g_agdsp->adev = NULL;
    }
    if (g_agdsp != NULL) {
        free(g_agdsp);
        g_agdsp = NULL;
    }
    AUDIO_FUNC_LOGI("voice_param DeviceClose exit");
}
