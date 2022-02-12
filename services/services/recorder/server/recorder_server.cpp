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

#include "recorder_server.h"
#include "media_log.h"
#include "media_errors.h"
#include "engine_factory_repo.h"
#include "param_wrapper.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderServer"};
}

namespace OHOS {
namespace Media {
const std::string START_TAG = "RecorderCreate->Start";
const std::string STOP_TAG = "RecorderStop->Destroy";
#define CHECK_STATUS_FAILED_AND_LOGE_RET(statusFailed, ret) \
    do { \
        if (statusFailed) { \
            MEDIA_LOGE("invalid status, current status is %{public}d", status_); \
            return ret; \
        }; \
    } while (false)

std::shared_ptr<IRecorderService> RecorderServer::Create()
{
    std::shared_ptr<RecorderServer> server = std::make_shared<RecorderServer>();
    int32_t ret = server->Init();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("failed to init RecorderServer");
        return nullptr;
    }
    return server;
}

RecorderServer::RecorderServer()
    : startTimeMonitor_(START_TAG),
      stopTimeMonitor_(STOP_TAG)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderServer::~RecorderServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t RecorderServer::Init()
{
    startTimeMonitor_.StartTime();

    auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(IEngineFactory::Scene::SCENE_RECORDER);
    CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, MSERR_CREATE_REC_ENGINE_FAILED, "failed to get factory");
    recorderEngine_ = engineFactory->CreateRecorderEngine();
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_CREATE_REC_ENGINE_FAILED,
        "failed to create recorder engine");
    status_ = REC_INITIALIZED;
    return MSERR_OK;
}

bool RecorderServer::CheckPermission()
{
    Security::AccessToken::AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();
    int result = Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenCaller, "ohos.permission.MICROPHONE");
    if (result == Security::AccessToken::PERMISSION_GRANTED) {
        MEDIA_LOGI("user have the right to access MICROPHONE!");
        return true;
    } else {
        MEDIA_LOGE("user do not have the right to access MICROPHONE!");
        return false;
    }
}

void RecorderServer::OnError(ErrorType errorType, int32_t errorCode)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (recorderCb_ == nullptr) {
        return;
    }
    recorderCb_->OnError(static_cast<RecorderErrorType>(errorType), errorCode);
}

void RecorderServer::OnInfo(InfoType type, int32_t extra)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (recorderCb_ != nullptr) {
        recorderCb_->OnInfo(type, extra);
    }
}

int32_t RecorderServer::SetVideoSource(VideoSourceType source, int32_t &sourceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_INITIALIZED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    return recorderEngine_->SetVideoSource(source, sourceId);
}

int32_t RecorderServer::SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    VidEnc vidEnc(encoder);
    return recorderEngine_->Configure(sourceId, vidEnc);
}

int32_t RecorderServer::SetVideoSize(int32_t sourceId, int32_t width, int32_t height)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    VidRectangle vidSize(width, height);
    return recorderEngine_->Configure(sourceId, vidSize);
}

int32_t RecorderServer::SetVideoFrameRate(int32_t sourceId, int32_t frameRate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    VidFrameRate vidFrameRate(frameRate);
    return recorderEngine_->Configure(sourceId, vidFrameRate);
}

int32_t RecorderServer::SetVideoEncodingBitRate(int32_t sourceId, int32_t rate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    VidBitRate vidBitRate(rate);
    return recorderEngine_->Configure(sourceId, vidBitRate);
}

int32_t RecorderServer::SetCaptureRate(int32_t sourceId, double fps)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    CaptureRate captureRate(fps);
    return recorderEngine_->Configure(sourceId, captureRate);
}

sptr<OHOS::Surface> RecorderServer::GetSurface(int32_t sourceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_PREPARED && status_ != REC_RECORDING && status_ != REC_PAUSED,
        nullptr);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, nullptr, "engine is nullptr");
    return recorderEngine_->GetSurface(sourceId);
}

bool RecorderServer::GetSystemParam()
{
    std::string permissionEnable;
    int32_t res = OHOS::system::GetStringParameter("sys.media.audioSource.permission", permissionEnable, "");
    if (res != 0 || permissionEnable.empty()) {
        MEDIA_LOGD("sys.media.audioSource.permission is false");
        return false;
    }
    MEDIA_LOGD("sys.media.audioSource.permission =%{public}s", permissionEnable.c_str());
    if (permissionEnable == "true") {
        return true;
    }
    return false;
}

int32_t RecorderServer::SetAudioSource(AudioSourceType source, int32_t &sourceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_INITIALIZED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");

    if (GetSystemParam()) {
        if (!CheckPermission()) {
            MEDIA_LOGE("Permission check failed!");
            return MSERR_INVALID_VAL;
        }
    }

    return recorderEngine_->SetAudioSource(source, sourceId);
}

int32_t RecorderServer::SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    AudEnc audEnc(encoder);
    MEDIA_LOGD("set audio encoder sourceId:%{public}d, encoder:%{public}d", sourceId, encoder);
    return recorderEngine_->Configure(sourceId, audEnc);
}

int32_t RecorderServer::SetAudioSampleRate(int32_t sourceId, int32_t rate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    AudSampleRate audSampleRate(rate);
    MEDIA_LOGD("set audio sampleRate sourceId:%{public}d, rate:%{public}d", sourceId, rate);
    return recorderEngine_->Configure(sourceId, audSampleRate);
}

int32_t RecorderServer::SetAudioChannels(int32_t sourceId, int32_t num)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    AudChannel audChannel(num);
    return recorderEngine_->Configure(sourceId, audChannel);
}

int32_t RecorderServer::SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    AudBitRate audBitRate(bitRate);
    return recorderEngine_->Configure(sourceId, audBitRate);
}

int32_t RecorderServer::SetDataSource(DataSourceType dataType, int32_t &sourceId)
{
    (void)dataType;
    (void)sourceId;
    return MSERR_INVALID_OPERATION;
}

int32_t RecorderServer::SetMaxDuration(int32_t duration)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    MaxDuration maxDuration(duration);
    return recorderEngine_->Configure(DUMMY_SOURCE_ID, maxDuration);
}

int32_t RecorderServer::SetOutputFormat(OutputFormatType format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_INITIALIZED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = recorderEngine_->SetOutputFormat(format);
    status_ = (ret == MSERR_OK ? REC_CONFIGURED : REC_INITIALIZED);
    return ret;
}

int32_t RecorderServer::SetOutputPath(const std::string &path)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    OutFilePath outFilePath(path);
    return recorderEngine_->Configure(DUMMY_SOURCE_ID, outFilePath);
}

int32_t RecorderServer::SetOutputFile(int32_t fd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    OutFd outFileFd(fd);
    return recorderEngine_->Configure(DUMMY_SOURCE_ID, outFileFd);
}

int32_t RecorderServer::SetNextOutputFile(int32_t fd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    NextOutFd nextFileFd(fd);
    return recorderEngine_->Configure(DUMMY_SOURCE_ID, nextFileFd);
}

int32_t RecorderServer::SetMaxFileSize(int64_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    MaxFileSize maxFileSize(size);
    return recorderEngine_->Configure(DUMMY_SOURCE_ID, maxFileSize);
}

void RecorderServer::SetLocation(float latitude, float longitude)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ != REC_CONFIGURED) {
        return;
    }
    CHECK_AND_RETURN_LOG(recorderEngine_ != nullptr, "engine is nullptr");
    GeoLocation geoLocation(latitude, longitude);
    recorderEngine_->Configure(DUMMY_SOURCE_ID, geoLocation);
    return;
}

void RecorderServer::SetOrientationHint(int32_t rotation)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ != REC_CONFIGURED) {
        return;
    }
    CHECK_AND_RETURN_LOG(recorderEngine_ != nullptr, "engine is nullptr");
    RotationAngle rotationAngle(rotation);
    recorderEngine_->Configure(DUMMY_SOURCE_ID, rotationAngle);
    return;
}

int32_t RecorderServer::SetRecorderCallback(const std::shared_ptr<RecorderCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_INITIALIZED && status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);

    {
        std::lock_guard<std::mutex> cbLock(cbMutex_);
        recorderCb_ = callback;
    }

    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    std::shared_ptr<IRecorderEngineObs> obs = shared_from_this();
    (void)recorderEngine_->SetObs(obs);
    return MSERR_OK;
}

int32_t RecorderServer::Prepare()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ == REC_PREPARED) {
        return MSERR_OK;
    }
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = recorderEngine_->Prepare();
    status_ = (ret == MSERR_OK ? REC_PREPARED : REC_ERROR);
    return ret;
}

int32_t RecorderServer::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ == REC_RECORDING) {
        return MSERR_OK;
    }
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_PREPARED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");

    int32_t ret = recorderEngine_->Start();
    status_ = (ret == MSERR_OK ? REC_RECORDING : REC_ERROR);

    startTimeMonitor_.FinishTime();
    return ret;
}

int32_t RecorderServer::Pause()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ == REC_PAUSED) {
        return MSERR_OK;
    }
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_RECORDING, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = recorderEngine_->Pause();
    status_ = (ret == MSERR_OK ? REC_PAUSED : REC_ERROR);
    return ret;
}

int32_t RecorderServer::Resume()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ == REC_RECORDING) {
        return MSERR_OK;
    }
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_RECORDING && status_ != REC_PAUSED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = recorderEngine_->Resume();
    status_ = (ret == MSERR_OK ? REC_RECORDING : REC_ERROR);
    return ret;
}

int32_t RecorderServer::Stop(bool block)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_RECORDING && status_ != REC_PAUSED, MSERR_INVALID_OPERATION);
    stopTimeMonitor_.StartTime();

    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = recorderEngine_->Stop(block);
    status_ = (ret == MSERR_OK ? REC_INITIALIZED : REC_ERROR);
    return ret;
}

int32_t RecorderServer::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = recorderEngine_->Reset();
    status_ = (ret == MSERR_OK ? REC_INITIALIZED : REC_ERROR);

    stopTimeMonitor_.FinishTime();
    return ret;
}

int32_t RecorderServer::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    recorderEngine_ = nullptr;
    return MSERR_OK;
}

int32_t RecorderServer::SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_RECORDING && status_ != REC_PAUSED, MSERR_INVALID_OPERATION);

    (void)type;
    (void)timestamp;
    (void)duration;
    return MSERR_OK;
}

int32_t RecorderServer::SetParameter(int32_t sourceId, const Format &format)
{
    (void)sourceId;
    (void)format;
    return MSERR_OK;
}
}
}
