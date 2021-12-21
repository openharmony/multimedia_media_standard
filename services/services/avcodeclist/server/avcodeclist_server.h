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

#ifndef AVCODECLIST_SERVER_H
#define AVCODECLIST_SERVER_H

#include "i_avcodeclist_service.h"
#include "i_avcodeclist_engine.h"
#include "time_monitor.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class AVCodecListServer : public IAVCodecListService {
public:
    static std::shared_ptr<IAVCodecListService> Create();
    AVCodecListServer();
    virtual ~AVCodecListServer();
    DISALLOW_COPY_AND_MOVE(AVCodecListServer);

    enum AVCodecListStatus : int32_t {
        AVCODECLIST_UNINITIALIZED = 0,
        AVCODECLIST_INITIALIZED,
        AVCODECLIST_END_OF_STREAM,
        AVCODECLIST_ERROR,
    };

    std::string FindVideoDecoder(const Format &format) override;
    std::string FindVideoEncoder(const Format &format) override;
    std::string FindAudioDecoder(const Format &format) override;
    std::string FindAudioEncoder(const Format &format) override;
    std::vector<CapabilityData>  GetCodecCapabilityInfos() override;

private:
    int32_t Init();
    AVCodecListStatus status_ = AVCODECLIST_UNINITIALIZED;
    std::unique_ptr<IAVCodecListEngine> codecListEngine_;
    std::mutex mutex_;
    std::mutex cbMutex_;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODECLIST_SERVER_H