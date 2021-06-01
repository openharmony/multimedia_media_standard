/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "format.h"
#include "source.h"

#include <iostream>

using namespace std;
using namespace OHOS::Media;

namespace SourceFormatTest {
    const int FIRST_ARG_IDX = 1;
}

static void TEST(bool result, const std::string& description)
{
    std::cout << description << ": ";
    if (result) {
        std::cout << "PASS" << std::endl;
    } else {
        std::cout << "FALSE" << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if ((argv == nullptr) || (argc <= SourceFormatTest::FIRST_ARG_IDX)) {
        return EXIT_FAILURE;
    }

    // Test source API
    Source source(argv[SourceFormatTest::FIRST_ARG_IDX]);
    TEST(source.GetSourceUri() == argv[SourceFormatTest::FIRST_ARG_IDX],
        "Source constructor and get source URI");

    map<string, string> header = {{"field", "value"}};
    Source sourceHeader("http://example.com/file.mp4", header);
    map<string, string> ret = sourceHeader.GetSourceHeader();
    TEST((header["field"] == ret["field"]),
        "source header constructor and getter API");

    SourceType type = SourceType::SOURCE_TYPE_URI;
    TEST((source.GetSourceType() == type),
        "GetSourceType for SOURCE_TYPE_URI API");

    // Test format API
    int32_t intVal32 = 10;
    int32_t intRet32 = 0;
    int64_t intVal64 = 50;
    int64_t intRet64 = 0;
    float floatVal = 20.10f;
    float floatRet = 0;
    double doubleVal = 30.99;
    double doubleRet = 0;
    string stringVal = "sampleString";
    string stringRet;
    Format format;
    format.PutIntValue("int32_Val", intVal32);
    format.GetIntValue("int32_Val", intRet32);
    TEST(intRet32 == intVal32, "PutIntValue and GetIntValue");

    format.PutLongValue("int64_Val", intVal64);
    format.GetLongValue("int64_Val", intRet64);
    TEST(intRet64 == intVal64, "PutLongValue and GetLongValue");

    format.PutFloatValue("float_Val", floatVal);
    format.GetFloatValue("float_Val", floatRet);
    TEST(floatRet == floatVal, "PutFloatValue and GetFloatValue");

    format.PutDoubleValue("double_Val", doubleVal);
    format.GetDoubleValue("double_Val", doubleRet);
    TEST(doubleRet == doubleVal, "PutdoubleValue and GetDoubleValue");

    format.PutStringValue("string_Val", stringVal);
    format.GetStringValue("string_Val", stringRet);
    TEST(stringRet == stringVal, "PutStringValue and GetStringValue");

    Format formatCopy;
    int32_t intCopyRet32 = 0;
    formatCopy.CopyFrom(format);
    formatCopy.GetIntValue("int32_Val", intCopyRet32);
    TEST(intVal32 == intCopyRet32, " test");
}
