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

#ifndef _CMR_PROPERTY_H_
#define _CMR_PROPERTY_H_
#ifdef __cplusplus
extern "C" {
#endif
#define CAM_YOCTO 1

#if CAM_YOCTO
#ifndef PROPERTY_VALUE_MAX
#define PROPERTY_VALUE_MAX  92
#endif
#define PROPERTY_KEY_MAX    32

int property_parser_init();
void property_parser_deinit();
int  property_get(const char * key, char * value, const char * def);
int property_set(const char * key, const char * def );
char property_get_bool(const char *key, char default_value);
	
#else	
#endif
#ifdef __cplusplus
}
#endif
#endif
