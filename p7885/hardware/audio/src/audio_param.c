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

#include <sys/stat.h>
#include "securec.h"
#include "local.h"
#include "audio_uhdf_log.h"
#include "hdf_io_service_if.h"

#define HDF_LOG_TAG HDF_AUDIO_HAL_LIB

#define SND_AUDIO_PARAM_PROFILE_MAX 4

static const char *g_audioPrfileName[SND_AUDIO_PARAM_PROFILE_MAX] = {
    "vendor/etc/audio/audio_structure",
    "vendor/etc/audio/dsp_vbc",
    "vendor/etc/audio/cvs",
    "vendor/etc/audio/dsp_smartamp"
};

static const char *g_audioTurning[SND_AUDIO_PARAM_PROFILE_MAX] = {
    "/dev/vbc_turning0",
    "/dev/vbc_turning1",
    "/dev/vbc_turning2",
    "/dev/vbc_turning3"
};

struct aud_param_info {
    const char *dataPtr;
    size_t    dataSize;
};

off_t GetFileSize(const char *filename)
{
    if (filename == NULL) {
        AUDIO_FUNC_LOGE("filename is NULL");
        return HDF_FAILURE;
    }
    if (strlen(filename) == 0) {
        AUDIO_FUNC_LOGE("filename len is 0");
        return HDF_FAILURE;
    }
    struct stat fileStat;
    off_t size = 0;

    (void)memset_s(&fileStat, sizeof(struct stat), 0, sizeof(struct stat));
    if (stat(filename, &fileStat) == 0) {
        size = fileStat.st_size;
        AUDIO_FUNC_LOGI("%{public}s, %{public}s file size is %{public}ld", __func__, filename, size);
        return size;
    }

    AUDIO_FUNC_LOGE("%{public}s, ERR: get %{public}s file size failed", __func__, filename);
    return HDF_FAILURE;
}

static int32_t AudioLoadFirmwareParams(const char *filename, struct aud_param_info *fp_info)
{
    off_t readLen;
    off_t size;
    off_t paramsSize = -1;
    char *buf = NULL;
    char *audioImageBuffer;
    int32_t fd;
    if (!filename || !fp_info)
        return HDF_FAILURE;

    paramsSize = GetFileSize(filename);
    if (paramsSize <= 0) {
        AUDIO_FUNC_LOGE("%{public}s Err:file %{public}s size error.", __func__, filename);
        return HDF_FAILURE;
    }
    AUDIO_FUNC_LOGI("%{public}s get audio params size=%{public}ld", __func__, paramsSize);
    audioImageBuffer = (char *)malloc(paramsSize);
    if (audioImageBuffer == NULL) {
        AUDIO_FUNC_LOGE("%{public}s Err:no memory\n", __func__);
        return HDF_FAILURE;
    }
    (void)memset_s(audioImageBuffer, paramsSize, 0, paramsSize);

    fd = open(filename, O_RDONLY, 0);
    if (fd < 0) {
        AUDIO_FUNC_LOGE("%{public}s, Err:open file %{public}s error, fd =%{public}d", __func__, filename, fd);
        free((void *)audioImageBuffer);
        return HDF_FAILURE;
    }

    /* read file to buffer */
    size = paramsSize;
    buf = audioImageBuffer;
    while (size > 0) {
        readLen = read(fd, buf, size);
        if (readLen > 0) {
            size -= readLen;
            buf += readLen;
        } else {
            AUDIO_FUNC_LOGE("%{public}s, Err: read data from file failed", __func__);
            break;
        }
    }
    close(fd);
    fp_info->dataSize = (size_t)(paramsSize - size);
    if (fp_info->dataSize == 0) {
        free((void *)audioImageBuffer);
        fp_info->dataPtr = NULL;
    } else {
        fp_info->dataPtr = audioImageBuffer;
    }
    return HDF_SUCCESS;
}

static int32_t SendFirmwareParamsToKernel(int32_t profile, struct aud_param_info * fp_info)
{
    int32_t ef_fd = -1;
    const char *firmwarePath = NULL;
    AUDIO_FUNC_LOGI("SendFirmwareParamsToKernel enter @@@@@@");

    if (fp_info == NULL || fp_info->dataPtr == NULL || fp_info->dataSize == 0) {
        return HDF_FAILURE;
    }

    firmwarePath = g_audioTurning[profile];
    if (firmwarePath == NULL) {
        AUDIO_FUNC_LOGE("%{public}s, Err: firmware profile is null.", __func__);
        return HDF_FAILURE;
    }

    AUDIO_FUNC_LOGI("%{public}s, open file(%{public}s) device to send effect data.", __func__, firmwarePath);
    ef_fd = open(firmwarePath, O_WRONLY);
    if (ef_fd < 0) {
        AUDIO_FUNC_LOGE("%{public}s open failed %{public}d", firmwarePath, ef_fd);
        return HDF_FAILURE;
    }

    AUDIO_FUNC_LOGI("%{public}s, send %{public}zu bytes effect data at %{public}p to kernel.",
        __func__, fp_info->dataSize, fp_info->dataPtr);
    if (write(ef_fd, fp_info->dataPtr, fp_info->dataSize) < 0) {
        AUDIO_FUNC_LOGE("%{public}s, Err:send data to driver failed.", __func__);
        close(ef_fd);
        return HDF_FAILURE;
    }

    AUDIO_FUNC_LOGI("%{public}s, send effect data to file(%{public}s) device success.", __func__, firmwarePath);
    close(ef_fd);
    return HDF_SUCCESS;
}

static void AudioReleaseFirmwareParams(struct aud_param_info *param)
{
    if (param && param->dataPtr) {
        free((void *)param->dataPtr);
        param->dataPtr = NULL;
    }
}

static int32_t UploadAudioProfileParamFromTurning(int32_t profile)
{
    AUDIO_FUNC_LOGI("UploadAudioProfileParamFromTurning enter @@@@@@");
    struct aud_param_info audioParamInfo = {NULL, 0};
    int32_t ret = 0;
    if (profile >= SND_AUDIO_PARAM_PROFILE_MAX || profile < 0) {
        return HDF_FAILURE;
    }
    if (AudioLoadFirmwareParams(g_audioPrfileName[profile], &audioParamInfo) < 0) {
        AUDIO_FUNC_LOGE("%{public}s, Err:load firmware params failed", __func__);
        return HDF_FAILURE;
    }

    if (audioParamInfo.dataSize == 0) {
        AUDIO_FUNC_LOGE("%{public}s, No valid parameters, param data size == 0", __func__);
        return -1;
    }

    if (SendFirmwareParamsToKernel(profile, &audioParamInfo) != 0) {
        AUDIO_FUNC_LOGE("%{public}s, Err:send firmware params to kernel failed.", __func__);
        AudioReleaseFirmwareParams(&audioParamInfo);
        return -1;
    }
    AudioReleaseFirmwareParams(&audioParamInfo);
    return ret;
}

int32_t UploadAudioProfileParamFromTurnings(void)
{
    int32_t ret = 0;
    AUDIO_FUNC_LOGI("upload_audio_profile_param_from_turnings enter @@@@@@");
    for (int32_t i = 0; i < SND_AUDIO_PARAM_PROFILE_MAX; i++) {
        ret = UploadAudioProfileParamFromTurning(i);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("UploadAudioProfileParamFromTurning fail i: %{public}d", i);
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}
