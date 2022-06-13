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

#ifndef MEDIA_LOCAL_H
#define MEDIA_LOCAL_H

#include "i_media_service.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class MediaLocal : public IMediaService, public NoCopyable {
public:
    MediaLocal() = default;
    ~MediaLocal() = default;

    std::shared_ptr<IRecorderService> CreateRecorderService() override;
    std::shared_ptr<IPlayerService> CreatePlayerService() override;
    std::shared_ptr<IAVMetadataHelperService> CreateAVMetadataHelperService() override;
    std::shared_ptr<IAVCodecService> CreateAVCodecService() override;
    std::shared_ptr<IAVCodecListService> CreateAVCodecListService() override;
    std::shared_ptr<IRecorderProfilesService> CreateRecorderProfilesService() override;
    std::shared_ptr<IAVMuxerService> CreateAVMuxerService() override;
    int32_t DestroyRecorderService(std::shared_ptr<IRecorderService> recorder) override;
    int32_t DestroyPlayerService(std::shared_ptr<IPlayerService> player) override;
    int32_t DestroyAVMetadataHelperService(std::shared_ptr<IAVMetadataHelperService> avMetadataHelper) override;
    int32_t DestroyAVCodecService(std::shared_ptr<IAVCodecService> avCodec) override;
    int32_t DestroyAVCodecListService(std::shared_ptr<IAVCodecListService> avCodecList) override;
    int32_t DestroyMediaProfileService(std::shared_ptr<IRecorderProfilesService> recorderProfiles) override;
    int32_t DestroyAVMuxerService(std::shared_ptr<IAVMuxerService> avmuxer) override;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_LOCAL_H
