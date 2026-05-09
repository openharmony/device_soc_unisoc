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
#ifndef SPRD_OMX_CORE_IMPL_H
#define SPRD_OMX_CORE_IMPL_H
#include "SprdOMXComponent.h"
namespace OHOS {
namespace OMX {
typedef OHOS::sptr<SprdOMXComponent> PSprdOMXComponent;
struct OMXCore;
void IncOmxCore(PSprdOMXComponent pComponent, OMXCore* pCore)
{
    pComponent->IncStrongRef(pCore);
}
void decOMXCore(SprdOMXComponent* pComponent, OMXCore* pCore)
{
    pComponent->DecStrongRef(pCore);
}
int getOMXCoreCount(SprdOMXComponent* pComponent)
{
    return pComponent->GetSptrRefCount();
}
};  // namespace OMX
};  // namespace OHOS
#endif //SPRD_OMX_CORE_IMPL_H
