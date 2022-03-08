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

#ifndef I_STANDARD_MEDIA_DATA_SOURCE_H
#define I_STANDARD_MEDIA_DATA_SOURCE_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "media_data_source.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class IStandardMediaDataSource : public IRemoteBroker {
public:
    virtual ~IStandardMediaDataSource() = default;
    virtual int32_t ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) = 0;
    virtual int32_t ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) = 0;
    virtual int32_t GetSize(int64_t &size) = 0;

    enum ListenerMsg {
        READ_AT = 0,
        READ_AT_POS,
        GET_SIZE,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardMediaDataSource");
};
} // namespace Media
} // namespace OHOS
#endif // I_STANDARD__LISTENER_H