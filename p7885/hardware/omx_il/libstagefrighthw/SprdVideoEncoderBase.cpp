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
#include "SprdVideoEncoderBase.h"
namespace OHOS {
namespace OMX {
SprdVideoEncoderBase::SprdVideoEncoderBase(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdVideoEncoderOMXComponent(name, callbacks, appData, component)
{
}
void SprdVideoEncoderBase::setOutputPortformat(OMX_PARAM_PORTDEFINITIONTYPE *def, int32_t mFrameWidth,
                                               int32_t mFrameHeight)
{
    if (def->nPortIndex == K_OUTPUT_PORT_INDEX) {
        def->format.video.nFrameWidth = mFrameWidth;
        def->format.video.nFrameHeight = mFrameHeight;
    }
}
}
}
