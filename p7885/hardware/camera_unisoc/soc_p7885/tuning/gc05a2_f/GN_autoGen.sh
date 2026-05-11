# Copyright (c) 2021 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# set -x

# OH平台tuning文件Makefile生成工具
# by chenliu

# 清在此处手动填写：
sensorName='gc05a2_f'
install_images='chip_base_dir'
part_name = "${PART_NAME}"
subsystem_name = "${SUBSYSTEM_NAME}"








# ==============================================================================================================
# load tuning files
# ==============================================================================================================
touch BUILD.gn
# add 1st line
echo import\(\"\/\/build\/ohos\.gni\"\) > BUILD.gn

data=$(find . -name "*"|grep -Ei "\.bin|\.xml|\.c")

# get total line num (total file num)
fileNum=$(echo "$data"|grep -n ""|tail -n 1|sed 's/:.*//g')

# ==============================================================================================================
# get tuning file name
# ==============================================================================================================
source=$(                                           \
        # echo $data cannot remain line breaks, use echo "$data"
        echo "$data"                     |          \
        # add [source = "] before tuning file
        sed 's/\.\//  source = \"\.\//g' |          \
        # add ["] after tuning file
        sed 's/$/\"/g'                              \
)
# ==============================================================================================================
# get tuning file path
# ==============================================================================================================
module_install_dir=$(                                                               \
        # echo $data cannot remain line breaks, use echo "$data"
        echo "$data"                                                            |   \
        sed 's/cfg\.xml/\/cfg\.xml/g'                                           |   \
        grep -o "\.\/.*\/"                                                      |   \
        sed 's/\.\//  module_install_dir = \"firmware\/'"${sensorName}"'\//g'   |   \
        sed 's/\/$/\"/g'                                                        |   \
        sed 's/\/\///g'                                                             \
)

# add new line
echo>> ./BUILD.gn

# ==============================================================================================================
# add tuning files into BUILD.gn
# ==============================================================================================================
var=1
while [ $var -le $fileNum ];
do
    echo $(($fileNum - $var))" remains"
    echo ohos_prebuilt_etc\(\""$sensorName"_tuning"$var"\"\) \{     >>    ./BUILD.gn
    # how to use varibles in sed:                 '"${var}"'
    # how to get specific line of a varible:      sed 'start,endp'
    echo "$source"|sed -n ''"${var}"','"${var}"'p'                  >>    ./BUILD.gn
    echo "  install_images = [ $install_images ]"                   >>    ./BUILD.gn
    echo "$module_install_dir"|sed -n ''"${var}"','"${var}"'p'      >>    ./BUILD.gn
    echo "  part_name = \"$part_name\""                             >>    ./BUILD.gn
    echo "  subsystem_name = \"$subsystem_name\""                   >>    ./BUILD.gn
    echo "  install_enable = true"                                  >>    ./BUILD.gn
    echo \}                                                         >>    ./BUILD.gn
    # add new line
    echo                                                            >>    ./BUILD.gn
    var=$(($var+1));
done



# add new line
echo                                                            >>    ./BUILD.gn
echo                                                            >>    ./BUILD.gn
echo                                                            >>    ./BUILD.gn
echo                                                            >>    ./BUILD.gn
echo group\(\"tuning_"$sensorName"\"\) \{                       >>    ./BUILD.gn
echo "  deps = ["                                               >>    ./BUILD.gn

# ==============================================================================================================
# add file deps in to BUILD.gn
# ==============================================================================================================
var=1
while [ $var -le $fileNum ];
do
    echo $(($fileNum - $var))" remains"
    echo "    \":"$sensorName"_tuning"$var"\","                        >>    ./BUILD.gn
    var=$(($var+1));
done

# ==============================================================================================================
# add brackets
# ==============================================================================================================
echo "  ]"                                                      >>    ./BUILD.gn
echo "}"                                                        >>    ./BUILD.gn

# add new line
echo                                                            >>    ./BUILD.gn
