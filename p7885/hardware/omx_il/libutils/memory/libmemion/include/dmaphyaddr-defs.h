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
#ifndef DMAPHYADDR_DEF_H
#define DMAPHYADDR_DEF_H
#include <linux/ioctl.h>
#include <linux/types.h>
static const char K_SYSTEM_HEAP_NAME[] = "system";
static const char K_SYSTEM_UNCACHED_HEAP_NAME[] = "system-uncached";
static const char K_WIDE_VINE_HEAP_NAME[] = "uncached_carveout_mm";
struct DmabufPhyData {
    __u32 fd;
    __u64 len;
    __u64 addr;
};
#define DMA_HEAP_IOC_MAGIC 'H'
#define DMABUF_IOC_PHY _IOWR(DMA_HEAP_IOC_MAGIC, 11, \
                             struct DmabufPhyData)
#endif // DMAPHYADDR_DEF_H
