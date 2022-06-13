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

#include "media_local.h"
#include "media_errors.h"
#include "media_log.h"
#include "avcodec_server.h"
#include "avmetadatahelper_server.h"
#include "player_server.h"
#include "recorder_server.h"
#include "avcodeclist_server.h"
#include "recorder_profiles_server.h"
#include "avmuxer_server.h"

namespace OHOS {
namespace Media {
IMediaService &MediaServiceFactory::GetInstance()
{
    static MediaLocal instance;
    return instance;
}

std::shared_ptr<IRecorderService> MediaLocal::CreateRecorderService()
{
    return RecorderServer::Create();
}

std::shared_ptr<IPlayerService> MediaLocal::CreatePlayerService()
{
    return PlayerServer::Create();
}

std::shared_ptr<IAVMetadataHelperService> MediaLocal::CreateAVMetadataHelperService()
{
    return AVMetadataHelperServer::Create();
}

std::shared_ptr<IAVCodecService> MediaLocal::CreateAVCodecService()
{
    return AVCodecServer::Create();
}

std::shared_ptr<IAVCodecListService> MediaLocal::CreateAVCodecListService()
{
    return AVCodecListServer::Create();
}

std::shared_ptr<IRecorderProfilesService> MediaLocal::CreateRecorderProfilesService()
{
    return RecorderProfilesServer::Create();
}

std::shared_ptr<IAVMuxerService> MediaLocal::CreateAVMuxerService()
{
    return AVMuxerServer::Create();
}

int32_t MediaLocal::DestroyRecorderService(std::shared_ptr<IRecorderService> recorder)
{
    (void)recorder;
    return MSERR_OK;
}

int32_t MediaLocal::DestroyPlayerService(std::shared_ptr<IPlayerService> player)
{
    (void)player;
    return MSERR_OK;
}

int32_t MediaLocal::DestroyAVMetadataHelperService(std::shared_ptr<IAVMetadataHelperService> avMetadataHelper)
{
    (void)avMetadataHelper;
    return MSERR_OK;
}

int32_t MediaLocal::DestroyAVCodecService(std::shared_ptr<IAVCodecService> avCodec)
{
    (void)avCodec;
    return MSERR_OK;
}

int32_t MediaLocal::DestroyAVCodecListService(std::shared_ptr<IAVCodecListService> avCodecList)
{
    (void)avCodecList;
    return MSERR_OK;
}

int32_t MediaLocal::DestroyMediaProfileService(std::shared_ptr<IRecorderProfilesService> recorderProfiles)
{
    (void)recorderProfiles;
    return MSERR_OK;
}

int32_t MediaLocal::DestroyAVMuxerService(std::shared_ptr<IAVMuxerService> avmuxer)
{
    (void)avmuxer;
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
