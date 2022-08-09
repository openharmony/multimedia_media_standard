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

#include <list>
#include "native_avcodec_base.h"
#include "native_avcodec_audioencoder.h"
#include "native_avmagic.h"
#include "native_window.h"
#include "avcodec_audio_encoder.h"
#include "avsharedmemory.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeAudioEncoder"};
}

using namespace OHOS::Media;

struct AudioEncoderObject : public AVCodec {
    explicit AudioEncoderObject(const std::shared_ptr<AVCodecAudioEncoder> &encoder)
        : AVCodec(AVMagic::MEDIA_MAGIC_AUDIO_ENCODER), audioEncoder_(encoder) {}
    ~AudioEncoderObject() = default;

    const std::shared_ptr<AVCodecAudioEncoder> audioEncoder_;
    std::list<OHOS::sptr<AVMemory>> memoryObjList_;
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
    std::atomic<bool> isFlushing_ = false;
    std::atomic<bool> isStop_ = false;
    std::atomic<bool> isEOS_ = false;
};

class NativeAudioEncoderCallback : public AVCodecCallback {
public:
    NativeAudioEncoderCallback(AVCodec *codec, struct AVCodecAsyncCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData) {}
    virtual ~NativeAudioEncoderCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override
    {
        (void)errorType;
        if (codec_ != nullptr) {
            int32_t extErr = MSErrorToExtError(static_cast<MediaServiceErrCode>(errorCode));
            callback_.onError(codec_, extErr, userData_);
        }
    }

    void OnOutputFormatChanged(const Format &format) override
    {
        if (codec_ != nullptr) {
            OHOS::sptr<AVFormat> object = new(std::nothrow) AVFormat(format);
            // The object lifecycle is controlled by the current function stack
            callback_.onStreamChanged(codec_, reinterpret_cast<AVFormat *>(object.GetRefPtr()), userData_);
        }
    }

    void OnInputBufferAvailable(uint32_t index) override
    {
        if (codec_ != nullptr) {
            struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec_);
            CHECK_AND_RETURN_LOG(audioEncObj->audioEncoder_ != nullptr, "audioEncoder_ is nullptr!");
            if (audioEncObj->isFlushing_.load() || audioEncObj->isStop_.load() || audioEncObj->isEOS_.load()) {
                MEDIA_LOGD("At flush, eos or stop, no buffer available");
                return;
            }

            AVMemory *data = GetInputData(codec_, index);
            if (data != nullptr) {
                callback_.onNeedInputData(codec_, index, data, userData_);
            }
        }
    }

    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override
    {
        if (codec_ != nullptr) {
            struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec_);
            CHECK_AND_RETURN_LOG(audioEncObj->audioEncoder_ != nullptr, "audioEncoder_ is nullptr!");
            if (audioEncObj->isFlushing_.load() || audioEncObj->isStop_.load()) {
                MEDIA_LOGD("At flush or stop, ignore");
                return;
            }
            struct AVCodecBufferAttr bufferAttr;
            bufferAttr.pts = info.presentationTimeUs;
            bufferAttr.size = info.size;
            bufferAttr.offset = info.offset;
            bufferAttr.flags = flag;
            // The bufferInfo lifecycle is controlled by the current function stack
            AVMemory *data = GetOutputData(codec_, index);
            callback_.onNeedOutputData(codec_, index, data, &bufferAttr, userData_);
        }
    }

private:
    AVMemory *GetInputData(struct AVCodec *codec, uint32_t index)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, nullptr, "magic error!");

        struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, nullptr, "audioEncoder_ is nullptr!");

        std::shared_ptr<AVSharedMemory> memory = audioEncObj->audioEncoder_->GetInputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get input buffer is nullptr!");

        for (auto &memoryObj : audioEncObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<AVMemory> object = new(std::nothrow) AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AVMemory");

        audioEncObj->memoryObjList_.push_back(object);
        return reinterpret_cast<AVMemory *>(object.GetRefPtr());
    }

    AVMemory *GetOutputData(struct AVCodec *codec, uint32_t index)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, nullptr, "magic error!");

        struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, nullptr, "audioEncoder_ is nullptr!");

        std::shared_ptr<AVSharedMemory> memory = audioEncObj->audioEncoder_->GetOutputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get output buffer is nullptr!");

        for (auto &memoryObj : audioEncObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<AVMemory> object = new(std::nothrow) AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AVMemory");

        audioEncObj->memoryObjList_.push_back(object);
        return reinterpret_cast<AVMemory *>(object.GetRefPtr());
    }

    struct AVCodec *codec_;
    struct AVCodecAsyncCallback callback_;
    void *userData_;
};

struct AVCodec *OH_AudioEncoder_CreateByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "input mime is nullptr!");

    std::shared_ptr<AVCodecAudioEncoder> audioEncoder = AudioEncoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(audioEncoder != nullptr, nullptr, "failed to AudioEncoderFactory::CreateByMime");

    struct AudioEncoderObject *object = new(std::nothrow) AudioEncoderObject(audioEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AudioEncoderObject");

    return object;
}

struct AVCodec *OH_AudioEncoder_CreateByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "input name is nullptr!");

    std::shared_ptr<AVCodecAudioEncoder> audioEncoder = AudioEncoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(audioEncoder != nullptr, nullptr, "failed to AudioEncoderFactory::CreateByMime");

    struct AudioEncoderObject *object = new(std::nothrow) AudioEncoderObject(audioEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AudioEncoderObject");

    return object;
}

AVErrCode OH_AudioEncoder_Destroy(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);

    if (audioEncObj != nullptr && audioEncObj->audioEncoder_ != nullptr) {
        audioEncObj->memoryObjList_.clear();
        int32_t ret = audioEncObj->audioEncoder_->Release();
        if (ret != MSERR_OK) {
            MEDIA_LOGE("audioEncoder Release failed!");
            delete codec;
            return AV_ERR_OPERATE_NOT_PERMIT;
        }
    } else {
        MEDIA_LOGD("audioEncoder_ is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

AVErrCode OH_AudioEncoder_Configure(struct AVCodec *codec, struct AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder Configure failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AudioEncoder_Prepare(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder Prepare failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AudioEncoder_Start(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");
    audioEncObj->isStop_.store(false);
    audioEncObj->isEOS_.store(false);
    MEDIA_LOGD("Set stop and eos status to false");
    int32_t ret = audioEncObj->audioEncoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder Start failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AudioEncoder_Stop(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");
    audioEncObj->isStop_.store(true);
    MEDIA_LOGD("Set stop status to true");

    int32_t ret = audioEncObj->audioEncoder_->Stop();
    if (ret != MSERR_OK) {
        audioEncObj->isStop_.store(false);
        MEDIA_LOGE("audioEncoder Stop failed! Set stop status to false");
        return AV_ERR_OPERATE_NOT_PERMIT;
    }
    audioEncObj->memoryObjList_.clear();

    return AV_ERR_OK;
}

AVErrCode OH_AudioEncoder_Flush(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");

    audioEncObj->isFlushing_.store(true);
    MEDIA_LOGD("Set flush status to true");

    int32_t ret = audioEncObj->audioEncoder_->Flush();
    if (ret != MSERR_OK) {
        audioEncObj->isFlushing_.store(false);
        MEDIA_LOGE("audioEncObj Flush failed! Set flush status to false");
        return AV_ERR_OPERATE_NOT_PERMIT;
    }
    audioEncObj->memoryObjList_.clear();
    audioEncObj->isFlushing_.store(false);
    MEDIA_LOGD("Set flush status to false");
    return AV_ERR_OK;
}

AVErrCode OH_AudioEncoder_Reset(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");
    audioEncObj->isStop_.store(true);
    int32_t ret = audioEncObj->audioEncoder_->Reset();
    if (ret != MSERR_OK) {
        audioEncObj->isStop_.store(false);
        MEDIA_LOGE("audioEncoder Reset failed! Set stop status to false");
        return AV_ERR_OPERATE_NOT_PERMIT;
    }
    audioEncObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

AVErrCode OH_AudioEncoder_PushInputData(struct AVCodec *codec, uint32_t index, AVCodecBufferAttr attr)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");

    struct AVCodecBufferInfo bufferInfo;
    bufferInfo.presentationTimeUs = attr.pts;
    bufferInfo.size = attr.size;
    bufferInfo.offset = attr.offset;
    enum AVCodecBufferFlag bufferFlag = static_cast<enum AVCodecBufferFlag>(attr.flags);

    int32_t ret = audioEncObj->audioEncoder_->QueueInputBuffer(index, bufferInfo, bufferFlag);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder QueueInputBuffer failed!");
    if (bufferFlag == AVCODEC_BUFFER_FLAG_EOS) {
        audioEncObj->isEOS_.store(true);
        MEDIA_LOGD("Set eos status to true");
    }

    return AV_ERR_OK;
}

AVFormat *OH_AudioEncoder_GetOutputDescription(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, nullptr, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, nullptr, "audioEncoder_ is nullptr!");

    Format format;
    int32_t ret = audioEncObj->audioEncoder_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "audioEncoder GetOutputFormat failed!");

    AVFormat *avFormat = OH_AVFormat_Create();
    avFormat->format_ = format;

    return avFormat;
}

AVErrCode OH_AudioEncoder_FreeOutputData(struct AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->ReleaseOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder ReleaseOutputBuffer failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AudioEncoder_SetParameter(struct AVCodec *codec, struct AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder SetParameter failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AudioEncoder_SetCallback(struct AVCodec *codec, struct AVCodecAsyncCallback callback, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder_ is nullptr!");

    audioEncObj->callback_ = std::make_shared<NativeAudioEncoderCallback>(codec, callback, userData);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->callback_ != nullptr, AV_ERR_INVALID_VAL, "callback_ is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->SetCallback(audioEncObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder SetCallback failed!");

    return AV_ERR_OK;
}
