/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef AVMUXER_SERVICE_STUB_H
#define AVMUXER_SERVICE_STUB_H

#include "i_standard_avmuxer_service.h"
#include "avmuxer_server.h"
#include "iremote_stub.h"

namespace OHOS {
namespace Media {
class AVMuxerServiceStub : public IRemoteStub<IStandardAVMuxerService>, public NoCopyable {
public:
    static sptr<AVMuxerServiceStub> Create();
    virtual ~AVMuxerServiceStub();
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    using AVMuxerStubFunc = int32_t(AVMuxerServiceStub::*)(MessageParcel &data, MessageParcel &reply);

    std::vector<std::string> GetAVMuxerFormatList() override;
    int32_t SetOutput(int32_t fd, const std::string &format) override;
    int32_t SetLocation(float latitude, float longitude) override;
    int32_t SetRotation(int32_t rotation) override;
    int32_t AddTrack(const MediaDescription &trackDesc, int32_t &trackId) override;
    int32_t Start() override;
    int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo) override;
    int32_t Stop() override;
    void Release() override;
    int32_t DestroyStub() override;
private:
    AVMuxerServiceStub();
    int32_t Init();
    int32_t GetAVMuxerFormatList(MessageParcel &data, MessageParcel &reply);
    int32_t SetOutput(MessageParcel &data, MessageParcel &reply);
    int32_t SetLocation(MessageParcel &data, MessageParcel &reply);
    int32_t SetRotation(MessageParcel &data, MessageParcel &reply);
    int32_t AddTrack(MessageParcel &data, MessageParcel &reply);
    int32_t Start(MessageParcel &data, MessageParcel &reply);
    int32_t WriteTrackSample(MessageParcel &data, MessageParcel &reply);
    int32_t Stop(MessageParcel &data, MessageParcel &reply);
    int32_t Release(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);

    std::mutex mutex_;
    std::shared_ptr<IAVMuxerService> avmuxerServer_ = nullptr;
    std::map<uint32_t, AVMuxerStubFunc> avmuxerFuncs_;
};
}  // namespace Media
}  // namespace OHOS
#endif