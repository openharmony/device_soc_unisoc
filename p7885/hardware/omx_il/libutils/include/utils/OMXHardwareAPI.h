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
// 宏保护：防止头文件被重复包含（与 #pragma once 互补，兼容更多编译器）
#ifndef OHOS_OMX_HARDWARE_TYPES_H_
#define OHOS_OMX_HARDWARE_TYPES_H_
#pragma once
#include <utils/HardwareAPI.h>
typedef OHOS::OMX::GetNativeBufferUsageParams GetNativeBufferUsageParams;
typedef OHOS::OMX::EnableOMXNativeBuffersParams EnableOMXNativeBuffersParams;
typedef OHOS::OMX::PrependSPSPPSToIDRFramesParams PrependSPSPPSToIDRFramesParams;
typedef OHOS::OMX::StoreMetaDataInBuffersParams StoreMetaDataInBuffersParams;
typedef OHOS::OMX::DescribeColorFormatParams DescribeColorFormatParams;
typedef OHOS::OMX::DescribeColorAspectsParams DescribeColorAspectsParams;
typedef OHOS::OMX::HDRStaticInfo HDRStaticInfo;
typedef OHOS::OMX::DescribeHDRStaticInfoParams DescribeHDRStaticInfoParams;
typedef OHOS::OMX::DescribeHDR10PlusInfoParams DescribeHDR10PlusInfoParams;
#endif // OHOS_OMX_HARDWARE_TYPES_H_
