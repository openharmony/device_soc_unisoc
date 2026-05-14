#!/bin/bash

# Copyright (C) 2023 HiHope Open Source Organization.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#$1 - *.tar.gz.gpg name
#$2 - output dir

set -e
set -x

echo "991789154" | gpg --batch --yes --passphrase-fd 0 -d "$1" | tar -xzf - --overwrite -C "$2"
