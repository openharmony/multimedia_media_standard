/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "test_metadata.h"
#include <iostream>
#include "directory_ex.h"

using namespace std;
using namespace OHOS;
using namespace Media;

TestMetadata::TestMetadata()
{
}

TestMetadata::~TestMetadata()
{
}

int32_t TestMetadata::MetaDataSetSource(const string &path)
{
    int32_t fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        cout << "Open file failed" << endl;
        (void)close(fd);
        return -1;
    }
    int64_t offset = 0;
    struct stat64 buffer;
    if (fstat64(fd, &buffer) != 0) {
        cout << "Get file state failed" << endl;
        (void)close(fd);
        return -1;
    }
    int64_t size = static_cast<int64_t>(buffer.st_size);
    int32_t usage = AVMetadataUsage::AV_META_USAGE_PIXEL_MAP;
    cout << "fd : " << fd << "; offset : " << offset << "; size : " << size << ": usage : " << usage << endl;
    int32_t ret = avmetadata -> SetSource(fd, offset, size, usage);
    if (ret != 0) {
        cout << "SetSource fail!" << endl;
        (void)close(fd);
        return -1;
    }
    return 0;
}