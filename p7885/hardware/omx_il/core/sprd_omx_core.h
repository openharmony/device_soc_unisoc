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
#ifndef SPRD_OMX_CORE_H_
#define SPRD_OMX_CORE_H_
namespace OHOS {
namespace OMX {
struct OMXCore {
    int mOmxCoreRefCount;
    OMXCore()
    {
        mOmxCoreRefCount = 0;
    }
    ~OMXCore() { }
};
const struct {
    const char *mName;
    const char *mLibNameSuffix;
    const char *mRole;
} K_COMPONENTS[] = {
    /*Video decoder*/
   { "OMX.sprd.h264.decoder", "sprd_h264dec", "video_decoder.avc" },
    /*Video encoder*/
   { "OMX.sprd.h264.encoder", "sprd_h264enc", "video_encoder.avc" },
   { "OMX.sprd.hevc.decoder", "sprd_h265dec", "video_decoder.hevc" },
   { "OMX.sprd.h265.encoder", "sprd_h265enc", "video_encoder.hevc" },
};
};    // namespace OMX
};    // namespace OHOS
#endif  // SPRD_OMX_CORE_H_
