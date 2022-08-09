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

#ifndef AVMETADATARESOLVEMETADATA_FUZZER
#define AVMETADATARESOLVEMETADATA_FUZZER

#define FUZZ_PROJECT_NAME "avmetadataresolvemetadata_fuzzer"
#include "test_metadata.h"

namespace OHOS {
namespace Media {
class AVMetadataResolveMetadataFuzzer : public TestMetadata {
public:
    AVMetadataResolveMetadataFuzzer();
    ~AVMetadataResolveMetadataFuzzer();
    bool FuzzAVMetadataResolveMetadata(uint8_t *data, size_t size);
};
}
bool FuzzTestAVMetadataResolveMetadata(uint8_t *data, size_t size);
}
#endif
