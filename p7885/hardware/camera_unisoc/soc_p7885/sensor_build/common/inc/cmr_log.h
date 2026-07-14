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

#ifndef _CMR_LOG_H_
#define _CMR_LOG_H_
#include <time.h>

#include <hilog/log.h>

/*
 * LEVEL_OVER_LOGE - only show ALOGE, err log is always show
 * LEVEL_OVER_LOGW - show ALOGE and ALOGW
 * LEVEL_OVER_LOGI - show ALOGE, ALOGW and ALOGI
 * LEVEL_OVER_LOGD - show ALOGE, ALOGW, ALOGI and ALOGD
 * LEVEL_OVER_LOGV - show ALOGE, ALOGW, ALOGI and ALOGD, ALOGV
 */
enum {
    LEVEL_OVER_LOGE = 1,
    LEVEL_OVER_LOGW,
    LEVEL_OVER_LOGI,
    LEVEL_OVER_LOGD,
    LEVEL_OVER_LOGV,
};

#define DEBUG_STR "%d, %s: "
#define ERROR_STR "%d, %s: hal_err "
#define DEBUG_ARGS __LINE__, __FUNCTION__
#ifndef PROPERTY_VALUE_MAX
#define PROPERTY_VALUE_MAX 93
#endif

extern long g_isp_log_level;
extern long g_oem_log_level;
extern long g_sensor_log_level;

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0xD003200  // 标识业务领域，范围0xD000000~0xD0FFFFF
#define LOG_TAG "MY_TAG"

#ifdef CONFIG_FPRINTF_LOG
#define CMRLOG(Level, fmt, args...)\
    do{\
        struct timespec ts;\
        clock_gettime(CLOCK_BOOTTIME, &ts);\
        fprintf(stderr, "[%s]" "[%5lu.%6lu]" "[%s] [%d]" fmt "\n",\
            LOG_TAG, ts.tv_sec, ts.tv_nsec/1000, __func__,__LINE__, ##args);\
            fflush(stderr);\
        } while(0)
#else

#define CMRLOG(Level, fmt, args...)\
    do{\
        HILOG_INFO(LOG_CORE, "[%s]" "[%s] [%d]" fmt "\n",\
            LOG_TAG, __func__,__LINE__, ##args);\
        } while(0)
#endif

#define IOT_PRINT_ISP(Level, format, ...)\
{\
    if (Level <= g_isp_log_level)\
    {\
        CMRLOG(Level, format, ##__VA_ARGS__);\
    }\
}

#define IOT_PRINT_OEM(Level, format, ...)\
{\
    if (Level <= g_oem_log_level)\
    {\
        CMRLOG(Level, format, ##__VA_ARGS__);\
    }\
}

#define IOT_PRINT_SENSOR(Level, format, ...)\
{\
    if (Level <= g_sensor_log_level)\
    {\
        CMRLOG(Level, format, ##__VA_ARGS__);\
    }\
}

#define ISP_LOGE(format, ...) HILOG_DEBUG(LOG_CORE, "[%s]" "[%s] [%d]" format "\n", LOG_TAG, __func__,__LINE__, ##__VA_ARGS__)
#define ISP_LOGW(format, ...) HILOG_DEBUG(LOG_CORE, "[%s]" "[%s] [%d]" format "\n", LOG_TAG, __func__,__LINE__, ##__VA_ARGS__)
#define ISP_LOGI(format, ...) HILOG_DEBUG(LOG_CORE, "[%s]" "[%s] [%d]" format "\n", LOG_TAG, __func__,__LINE__, ##__VA_ARGS__)
#define ISP_LOGD(format, ...) HILOG_DEBUG(LOG_CORE, "[%s]" "[%s] [%d]" format "\n", LOG_TAG, __func__,__LINE__, ##__VA_ARGS__)
	
/* ISP_LOGV uses ALOGD_IF */
#define ISP_LOGV(format, ...) 

#define CMR_LOGE(format, ...) HILOG_ERROR(LOG_CORE, "[%s]" "[%s] [%d]" format "\n", LOG_TAG, __func__,__LINE__, ##__VA_ARGS__)
#define CMR_LOGW(format, ...) HILOG_ERROR(LOG_CORE, "[%s]" "[%s] [%d]" format "\n", LOG_TAG, __func__,__LINE__, ##__VA_ARGS__)
#define CMR_LOGI(format, ...) HILOG_ERROR(LOG_CORE, "[%s]" "[%s] [%d]" format "\n", LOG_TAG, __func__,__LINE__, ##__VA_ARGS__)
#define CMR_LOGD(format, ...) HILOG_DEBUG(LOG_CORE, "[%s]" "[%s] [%d]" format "\n", LOG_TAG, __func__,__LINE__, ##__VA_ARGS__)
#define CMR_LOGV(format, ...) 

#define SENSOR_LOGE(format, ...) HILOG_ERROR(LOG_CORE, "[%s]" "[%s] [%d]" format "\n", LOG_TAG, __func__,__LINE__, ##__VA_ARGS__)
#define SENSOR_LOGW(format, ...) HILOG_ERROR(LOG_CORE, "[%s]" "[%s] [%d]" format "\n", LOG_TAG, __func__,__LINE__, ##__VA_ARGS__)
#define SENSOR_LOGI(format, ...) HILOG_DEBUG(LOG_CORE, "[%s]" "[%s] [%d]" format "\n", LOG_TAG, __func__,__LINE__, ##__VA_ARGS__)
#define SENSOR_LOGD(format, ...) HILOG_DEBUG(LOG_CORE, "[%s]" "[%s] [%d]" format "\n", LOG_TAG, __func__,__LINE__, ##__VA_ARGS__)
#define SENSOR_LOGV(format, ...) 


#define SENSOR_PRINT_ERR SENSOR_LOGE
#define SENSOR_PRINT_HIGH SENSOR_LOGI
#define SENSOR_PRINT SENSOR_LOGI
#define SENSOR_TRACE SENSOR_LOGI

void isp_init_log_level(void);
void oem_init_log_level(void);
void sensor_init_log_level(void);

#endif
