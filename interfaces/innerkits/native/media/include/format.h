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

#ifndef FORMAT_H
#define FORMAT_H

#include <map>
#include <string>

namespace OHOS {
namespace Media {
enum FormatDataType : uint32_t {
    /* None */
    FORMAT_TYPE_NONE,
    /* Int32 */
    FORMAT_TYPE_INT32,
    /* Int64 */
    FORMAT_TYPE_INT64,
    /* Float */
    FORMAT_TYPE_FLOAT,
    /* Double */
    FORMAT_TYPE_DOUBLE,
    /* String */
    FORMAT_TYPE_STRING,
    /* Addr */
    FORMAT_TYPE_ADDR,
};

struct FormatData {
    FormatDataType type = FORMAT_TYPE_NONE;
    union Val {
        int32_t int32Val;
        int64_t int64Val;
        float floatVal;
        double doubleVal;
    } val = {0};
    std::string stringVal = "";
    intptr_t addr = 0;
    int32_t size = 0;
};

class __attribute__((visibility("default"))) Format {
public:
    Format() = default;
    ~Format() = default;
    /**
     * @brief Sets metadata of the integer type.
     *
     * @param key Indicates the metadata key.
     * @param value Indicates the metadata value, which is a 32-bit integer.
     * @return Returns <b>true</b> if the setting is successful; returns <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    bool PutIntValue(const std::string &key, int32_t value);

    /**
     * @brief Sets metadata of the long integer type.
     *
     * @param key Indicates the metadata key.
     * @param value Indicates the metadata value, which is a 64-bit integer.
     * @return Returns <b>true</b> if the setting is successful; returns <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    bool PutLongValue(const std::string &key, int64_t value);

    /**
     * @brief Sets metadata of the single-precision floating-point type.
     *
     * @param key Indicates the metadata key.
     * @param value Indicates the metadata value, which is a single-precision floating-point number.
     * @return Returns <b>true</b> if the metadata is successfully set; returns <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    bool PutFloatValue(const std::string &key, float value);

    /**
     * @brief Sets metadata of the double-precision floating-point type.
     *
     * @param key Indicates the metadata key.
     * @param value Indicates the metadata value, which is a double-precision floating-point number.
     * @return Returns <b>true</b> if the setting is successful; returns <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    bool PutDoubleValue(const std::string &key, double value);

    /**
     * @brief Sets metadata of the string type.
     *
     * @param key Indicates the metadata key.
     * @param value Indicates the metadata value, which is a string.
     * @return Returns <b>true</b> if the metadata is successfully set; returns <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    bool PutStringValue(const std::string &key, const std::string &value);

    /**
     * @brief Sets metadata of the string type.
     *
     * @param key Indicates the metadata key.
     * @param addr Indicates the metadata addr, which is a intptr_t.
     * @param size Indicates the metadata addr size, which is a int32_t.
     * @return Returns <b>true</b> if the metadata is successfully set; returns <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    bool PutBuffer(const std::string &key, const intptr_t addr, int32_t size);

    /**
     * @brief Obtains the metadata value of the integer type.
     *
     * @param key Indicates the metadata key.
     * @param value Indicates the metadata value to obtain, which is a 32-bit integer.
     * @return Returns <b>true</b> if the integer is successfully obtained; returns <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    bool GetIntValue(const std::string &key, int32_t &value) const;

    /**
     * @brief Obtains the metadata value of the long integer type.
     *
     * @param key Indicates the metadata key.
     * @param value Indicates the metadata value to obtain, which is a 64-bit long integer.
     * @return Returns <b>true</b> if the integer is successfully obtained; returns <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    bool GetLongValue(const std::string &key, int64_t &value) const;

    /**
     * @brief Obtains the metadata value of the single-precision floating-point type.
     *
     * @param key Indicates the metadata key.
     * @param value Indicates the metadata value to obtain, which is a single-precision floating-point number.
     * @return Returns <b>true</b> if the single-precision number is successfully obtained; returns
     * <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    bool GetFloatValue(const std::string &key, float &value) const;

    /**
     * @brief Obtains the metadata value of the double-precision floating-point type.
     *
     * @param key Indicates the metadata key.
     * @param value Indicates the metadata value to obtain, which is a double-precision floating-point number.
     * @return Returns <b>true</b> if the double-precision number is successfully obtained; returns
     * <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    bool GetDoubleValue(const std::string &key, double &value) const;

    /**
     * @brief Obtains the metadata value of the string type.
     *
     * @param key Indicates the metadata key.
     * @param value Indicates the metadata value to obtain, which is a string.
     * @return Returns <b>true</b> if the string is successfully obtained; returns <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    bool GetStringValue(const std::string &key, std::string &value) const;

    /**
     * @brief Obtains the metadata value of the string type.
     *
     * @param key Indicates the metadata key.
     * @param addr Indicates the metadata addr to obtain, which is a intptr_t.
     * @param size Indicates the metadata addr size to obtain, which is a int32_t.
     * @return Returns <b>true</b> if the string is successfully obtained; returns <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    bool GetBuffer(const std::string &key, intptr_t &addr, int32_t &size) const;

    /**
     * @brief Obtains the metadata map.
     *
     * @return Returns the map object.
     * @since 1.0
     * @version 1.0
     */
    const std::map<std::string, FormatData> &GetFormatMap() const;

private:
    std::map<std::string, FormatData> formatMap_;
};
} // namespace Media
} // namespace OHOS
#endif // FORMAT_H
