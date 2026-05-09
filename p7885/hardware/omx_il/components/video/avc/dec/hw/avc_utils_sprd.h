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
#ifndef AVC_UTILS_SPRD_H_
#define AVC_UTILS_SPRD_H_
#include <utils/omx_log.h>
namespace OHOS {
namespace OMX {
#define  SCI_TRACE_LOW OMX_LOGI
#define MAX_REF_FRAME_NUMBER    16
typedef struct Bitstream {
    unsigned int bitcnt;
    unsigned int bitsLeft;  // left bits in the word pointed by rdptr
    unsigned int *rdptr;
    unsigned int bitcntBeforeVld;
    unsigned int errorFlag;
} DEC_BS_T;
int IsInterlacedSequence(const unsigned char *bitstrmPtr, size_t bitstrmLen);
}  // namespace OMX
}  // namespace OHOS
#endif
