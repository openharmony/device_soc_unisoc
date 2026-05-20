#
# Copyright 2016-2026 Unisoc (Shanghai) Technologies Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os,fnmatch
import struct
import mmap
import sys
import gzip
import zlib

# Reference kernel/printk/printk.c
PRINKT_LOG_STRUCT_FORMAT      = '<Q3H3BI'
PRINKT_LOG_STRUCT_SIZE = 20
TASK_COMM_LEN          = 16


MSEC = 1000000000
DUMP_FILE = 'minidump'
dump_dir = os.getcwd()

def merge_file():
    dump_files = fnmatch.filter(os.listdir(dump_dir), '*_minidump_*')
    if len(dump_files) == 0:
        print('Don\'t exist dump file!!!')
        return
    
    dump_files.sort(key = lambda x: int(x.split('_',1)[0]))
    #for item in dump_files:
    #    print item
    with open(os.path.join(dump_dir, DUMP_FILE),'wb') as outfile:
        for dump_file in dump_files:
            for line in open(os.path.join(dump_dir, dump_file), 'rb'):
                outfile.write(line)

if __name__ == '__main__':
    if len(sys.argv) == 2:
        dump_dir = sys.argv[1]
    merge_file()