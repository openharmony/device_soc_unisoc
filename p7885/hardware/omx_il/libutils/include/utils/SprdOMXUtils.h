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
#ifndef SPRD_OMX_UTILS_H
#define SPRD_OMX_UTILS_H
#include "sprd_omx_typedef.h"
namespace OHOS {
namespace OMX {
struct SprdOMXUtils {
    struct StreamFilePathInfo {
        const char *name;
        void *pointer;
        const char *strmExtension;
        int64_t time;
    };

    static bool GetDumpValue(const char* pRoperty_str);
    static void getYuvFilePath(char* dumpFile, int max_length, const char* name, void *pointer, int64_t time);
    static void getStrmFilePath(char* dumpFile, int max_length, const StreamFilePathInfo &info);
    static void releaseMemIon(OHOS::sptr<MemIon>* pMemIon);
    static bool notifyEncodeHeaderWithFirstFrame()
    {
        return false;
    }
};
};  // namespace OMX
};  // namespace OHOS
#endif
