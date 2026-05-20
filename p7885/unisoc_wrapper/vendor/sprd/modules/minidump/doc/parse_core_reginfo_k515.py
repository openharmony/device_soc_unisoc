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

#! /usr/bin/python
# -*- coding: utf-8 -*-

import sys, re
import os
import fnmatch
import mmap
import struct
from ctypes import *

class PHEAD(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('ts_nsec', c_uint64),
        ('len',     c_uint16),
        ('text_len',c_uint16),
        ('dict_len',c_uint16),
        ('facility',c_uint8),
        ('flags',   c_uint8,5),
        ('level',   c_uint8,3),
        ('cpu',     c_uint32),
    ]
    def encode(self):
        return string_at(addressof(self), sizeof(self))
    def decode(self, data):
        memmove(addressof(self), data, sizeof(self))
        return len(data)
        
dump_dir = os.getcwd()
CORE_REGS_FILE = 'core_reginfo'
per_cpu_start = 0
per_cpu_offset = 0
cpu_id = 0
core_regs_id = 0
arch_info_version = 40

def get_python_version():
    if sys.version_info.major == 2:
        reload(sys)
        sys.setdefaultencoding('utf8')
    
def get_arch_version(self):
    global arch_info_version
    offset = 18
    file_list = fnmatch.filter(os.listdir(self), '*elfhdr')
    elf_file = file_list[0]
    if len(file_list) == 0:
        print('Don\'t exist elf file!!!')
        return
    f = open(elf_file,'rb')
    f.seek(offset, 0)
    temp = f.read(2)
    temp = struct.unpack('H',temp)
    arch_info_version = temp[0]
    print(arch_info_version)
    f.close()

def get_cpu_reginfo(self):
    global per_cpu_start
    global per_cpu_offset
    global cpu_id
    global core_regs_id
    global arch_info_version
    per_cpu_ad = "per_cpu_start"
    per_cpu_sta = "per_cpu_offset"
    cpu_i = "processor_id"
    regs_i = "core_regs_id"
    arch_flag = "kimage_voffset"
    per_cpu_ad = per_cpu_ad.encode()
    per_cpu_sta = per_cpu_sta.encode()
    cpu_i = cpu_i.encode()
    regs_i = regs_i.encode()
    arch_flag = arch_flag.encode()
    file_list = fnmatch.filter(os.listdir(self), '*vmcore_info')
    if len(file_list) == 0:
        print('Don\'t exist vmcoreinfo file!!!')
        return
    vmcoreinfo_file = file_list[0]    
    infofd = open(os.path.join(self, vmcoreinfo_file), 'rb+')
    for line in infofd.readlines():
        if per_cpu_ad in line:
            idx_a = re.split(str.encode('='), line)
            per_cpu_start = int(idx_a[1],16)
            print(per_cpu_start)
            print ("per_cpu_start: 0x%x" %per_cpu_start)
        if per_cpu_sta in line:
            idx_b = re.split(str.encode('='), line)
            per_cpu_offset = int(idx_b[1])
            print ("per_cpu_offset:%d" %per_cpu_offset)
        if cpu_i in line:
            idx_c = re.split(str.encode('='), line)
            cpu_id = int(idx_c[1])
            print ("cpu_id: %d" %cpu_id)
        if regs_i in line:
            idx_d = re.split(str.encode('='), line)
            core_regs_id = int(idx_d[1],16)
            print ("core_regs_id:0x%x" %core_regs_id)
        if arch_flag in line:
            arch_info_version = 183
    infofd.close()

def arm64_core_regs_give(self):
    temp0 = infd.read(272)
    temp0 = struct.unpack('34Q',temp0)
    cor_id = str(self)
    outfd.write("-------------------CPU"+cor_id+"------------------"+"\n")
    for reg_id in range(0,31):
        print(temp0[reg_id])
        X0 = hex(temp0[reg_id])
        X0 = X0.strip('L')
        id = str(reg_id)
        str0 = 'X'+ id + ' = '  
        print(X0)
        outfd.write(str0+X0+"\n")
    SP = hex(temp0[31])
    PC = hex(temp0[32])
    CPSR = hex(temp0[33])
    SP = SP.strip('L')
    PC = PC.strip('L')
    CPSR = CPSR.strip('L')
    outfd.write("SP = "+SP+"\n"+"PC = "+PC+"\n"+"CPSR = "+CPSR+"\n")

def arm32_core_regs_give(self):
    temp0 = infd.read(160)
    temp0 = struct.unpack('40I',temp0)
    cor_id = str(self)
    outfd.write("\n"+"\n"+"**********************CPU"+cor_id+"**********************"+"\n")
    for reg_id in range(0,13):
        #print(temp0[reg_id])
        X0 = hex(temp0[reg_id])
        id = str(reg_id)
        str0 = 'R'+ id + ' = '  
        #print(X0)
        outfd.write(str0+X0+"\n")
    R13_SVC = hex(temp0[13])
    R14_SVC = hex(temp0[14])
    SPSR_SVC = hex(temp0[15])
    PC = hex(temp0[16])
    CPSR = hex(temp0[17])
    outfd.write("R13_SVC = "+R13_SVC+"\n"+"R14_SVC = "+R14_SVC+"\n"+"SPSR_SVC = "+SPSR_SVC+"\n"+"PC = "+PC+"\n"+"CPSR = "+CPSR+"\n")
    outfd.write("-------------------USER------------------"+"\n")
    for reg_id in range (18,20):
        R_user = hex(temp0[reg_id])
        id = str(reg_id-5)
        str_user = 'R'+id+'_user = '
        outfd.write(str_user+R_user+"\n")
    outfd.write("-------------------FIQ------------------"+"\n")
    for reg_id in range(20,27):
        R_fiq = hex(temp0[reg_id])
        id = str(reg_id-12)
        str_fiq = 'R'+id+'_fiq = '
        outfd.write(str_fiq+R_fiq+"\n")
    SPSR_fiq = hex(temp0[27])
    outfd.write("SPSR_fiq = "+SPSR_fiq+"\n")
    outfd.write("-------------------IRQ------------------"+"\n")
    for reg_id in range(28,30):
        R_irq = hex(temp0[reg_id])
        id = str(reg_id-15)
        str_irq = 'R'+id+'_irq = '
        outfd.write(str_irq+R_irq+"\n")
    SPSR_irq = hex(temp0[30])
    outfd.write("SPSR_irq = "+SPSR_irq+"\n")
    outfd.write("-------------------MON------------------"+"\n")
    for reg_id in range(31,33):
        R_mon = hex(temp0[reg_id])
        id = str(reg_id-18)
        str_mon = 'R'+id+'_mon = '
        outfd.write(str_mon+R_mon+"\n")
    SPSR_mon = hex(temp0[33])
    outfd.write("SPSR_mon = "+SPSR_mon+"\n")
    outfd.write("-------------------ABT------------------"+"\n")
    for reg_id in range(34,36):
        R_abt = hex(temp0[reg_id])
        id = str(reg_id-21)
        str_abt = 'R'+id+'_abt = '
        outfd.write(str_abt+R_abt+"\n")
    SPSR_abt = hex(temp0[36])
    outfd.write("SPSR_abt = "+SPSR_abt+"\n")
    outfd.write("-------------------UND------------------"+"\n")
    for reg_id in range(37,39):
        R_und = hex(temp0[reg_id])
        id = str(reg_id-24)
        str_und = 'R'+id+'_und = '
        outfd.write(str_und+R_und+"\n")
    SPSR_und = hex(temp0[39])
    outfd.write("SPSR_und = "+SPSR_und+"\n")
    
        
     
if __name__ == "__main__":
    
    get_python_version()
    if len(sys.argv) == 1:
        path = dump_dir
    else:
        path = os.path.join(dump_dir, sys.argv[1])
    get_cpu_reginfo(path)
    #get_arch_version(path)
    file_list = fnmatch.filter(os.listdir(path), '*per_cpu')
    if len(file_list) == 0:
        print('Don\'t exist per_cpu file!!!')
    per_cpu_file = file_list[0]
    fd = open(os.path.join(path, per_cpu_file), "rb")
    offset = core_regs_id - per_cpu_start-(cpu_id*per_cpu_offset)
    print("offset: %d" %offset)
    print(arch_info_version)    
    with open(os.path.join(path, CORE_REGS_FILE), 'w') as outfd:
        with open(os.path.join(path, per_cpu_file), 'rb+') as infd:
            if arch_info_version == 183:
                for core_id in range(0,8):
                    infd.seek(offset+core_id*per_cpu_offset, 0)
                    arm64_core_regs_give(core_id)
            if arch_info_version == 40:
                for core_id in range(0,8):
                    infd.seek(offset+core_id*per_cpu_offset, 0)
                    arm32_core_regs_give(core_id)
    fd.close()
                