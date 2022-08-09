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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "aw_common.h"


using namespace std;
using namespace OHOS;
using namespace Media;
using namespace PlayerTestParam;

namespace OHOS {
namespace Media {
namespace PlayerTestParam {
int32_t PlayerTestParam::WriteDataToFile(const string &path, const uint8_t *data, size_t size)
{
    FILE *file = nullptr;
    file = fopen(path.c_str(), "w+");
    if (file == nullptr) {
        cout << "[fuzz] open file fstab.test failed" << endl;
        return -1;
    }
    if (fwrite(data, 1, size, file) != size) {
            cout << "[fuzz] write data failed" << endl;
            (void)fclose(file);
            return -1;
    }
    (void)fclose(file);
    return 0;
}

int32_t PlayerTestParam::ProduceRandomNumberCrypt()
{
    int32_t r = 0;
    int fd = open("/dev/random", O_RDONLY);
    if (fd > 0) {
        read(fd, &r, sizeof(int32_t));
    }
    close(fd);
    return r;
}
}
}
}