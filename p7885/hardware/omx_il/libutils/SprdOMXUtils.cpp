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
#include "SprdOMXUtils.h"
#include <utils/omx_log.h>
#include "refbase.h"
#include <securec.h>
#include <cinttypes>
namespace OHOS {
namespace OMX {
void SprdOMXUtils::getYuvFilePath(char *dumpFile, int max_length, const char *name, void *pointer, int64_t time)
{
    int ret = snprintf_s(dumpFile, max_length, max_length - 1,
                         "/data/local/tmp/video_%s_%p_%lld.yuv",
                         name, pointer, (long long)time);
    if (ret < 0) {
        dumpFile[0] = '\0';
    }
}
void SprdOMXUtils::getStrmFilePath(
    char *dumpFile, int max_length, const StreamFilePathInfo &info)
{
    int ret = snprintf_s(dumpFile, max_length, max_length - 1,
                         "/data/local/tmp/video_%s_%p_%lld.%s",
                         info.name, info.pointer, (long long)info.time, info.strmExtension);
    if (ret < 0) {
        dumpFile[0] = '\0';
    }
}
bool SprdOMXUtils::GetDumpValue(const char *property_str)
{
    bool dumpValue = false;
    (void)property_str;
    OMX_LOGI("---OHOS  SprdOMXUtils::GetDumpValue ALWAYS return false");
    return dumpValue;
}
void SprdOMXUtils::releaseMemIon(OHOS::sptr<MemIon>* pMemIon)
{
    if (pMemIon == nullptr) {
        OMX_LOGE("releaseMemIon: nullptr pointer!");
        return;
    }
    (*pMemIon).clear();
    OMX_LOGD("releaseMemIon exit");
}
};    // namespace OMX
};    // namespace OHOS
