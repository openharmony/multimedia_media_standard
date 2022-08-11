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
#include "native_avcodec_audiodecoder.h"
#include "native_avmagic.h"
#include "native_window.h"
#include "avcodec_audio_decoder.h"
#include "avsharedmemory.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeAudioDecoder"};
}

using namespace OHOS::Media;

struct AudioDecoderObject : public OH_AVCodec {
    explicit AudioDecoderObject(const std::shared_ptr<AVCodecAudioDecoder> &decoder)
        : OH_AVCodec(AVMagic::MEDIA_MAGIC_AUDIO_DECODER), audioDecoder_(decoder) {}
    ~AudioDecoderObject() = default;

    const std::shared_ptr<AVCodecAudioDecoder> audioDecoder_;
    std::list<OHOS::sptr<OH_AVMemory>> memoryObjList_;
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
    std::atomic<bool> isFlushing_ = false;
    std::atomic<bool> isStop_ = false;
    std::atomic<bool> isEOS_ = false;
    std::mutex mutex_;
};

class NativeAudioDecoderCallback : public AVCodecCallback {
public:
    NativeAudioDecoderCallback(OH_AVCodec *codec, struct OH_AVCodecAsyncCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData) {}
    virtual ~NativeAudioDecoderCallback() = default;

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
            OHOS::sptr<OH_AVFormat> object = new(std::nothrow) OH_AVFormat(format);
            // The object lifecycle is controlled by the current function stack
            callback_.onStreamChanged(codec_, reinterpret_cast<OH_AVFormat *>(object.GetRefPtr()), userData_);
        }
    }

    void OnInputBufferAvailable(uint32_t index) override
    {
        if (codec_ != nullptr) {
            struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec_);
            CHECK_AND_RETURN_LOG(audioDecObj->audioDecoder_ != nullptr, "audioDecoder_ is nullptr!");
            if (audioDecObj->isFlushing_.load() || audioDecObj->isStop_.load() || audioDecObj->isEOS_.load()) {
                MEDIA_LOGD("At flush, eos or stop, no buffer available");
                return;
            }

            OH_AVMemory *data = GetInputData(codec_, index);
            if (data != nullptr) {
                callback_.onNeedInputData(codec_, index, data, userData_);
            }
        }
    }

    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override
    {
        if (codec_ != nullptr) {
            struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec_);
            CHECK_AND_RETURN_LOG(audioDecObj->audioDecoder_ != nullptr, "audioDecoder_ is nullptr!");
            if (audioDecObj->isFlushing_.load() || audioDecObj->isStop_.load()) {
                MEDIA_LOGD("At flush or stop, ignore");
                return;
            }

            struct OH_AVCodecBufferAttr bufferAttr;
            bufferAttr.pts = info.presentationTimeUs;
            bufferAttr.size = info.size;
            bufferAttr.offset = info.offset;
            bufferAttr.flags = flag;
            // The bufferInfo lifecycle is controlled by the current function stack
            OH_AVMemory *data = GetOutputData(codec_, index);
            callback_.onNeedOutputData(codec_, index, data, &bufferAttr, userData_);
        }
    }

private:
    OH_AVMemory *GetInputData(struct OH_AVCodec *codec, uint32_t index)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, nullptr, "magic error!");

        struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, nullptr, "audioDecoder_ is nullptr!");

        std::shared_ptr<AVSharedMemory> memory = audioDecObj->audioDecoder_->GetInputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get input buffer is nullptr!");

        for (auto &memoryObj : audioDecObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<OH_AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<OH_AVMemory> object = new(std::nothrow) OH_AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVMemory");

        audioDecObj->memoryObjList_.push_back(object);
        return reinterpret_cast<OH_AVMemory *>(object.GetRefPtr());
    }

    OH_AVMemory *GetOutputData(struct OH_AVCodec *codec, uint32_t index)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, nullptr, "magic error!");

        struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, nullptr, "audioDecoder_ is nullptr!");

        std::shared_ptr<AVSharedMemory> memory = audioDecObj->audioDecoder_->GetOutputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get output buffer is nullptr!");

        for (auto &memoryObj : audioDecObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<OH_AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<OH_AVMemory> object = new(std::nothrow) OH_AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVMemory");

        audioDecObj->memoryObjList_.push_back(object);
        return reinterpret_cast<OH_AVMemory *>(object.GetRefPtr());
    }

    struct OH_AVCodec *codec_;
    struct OH_AVCodecAsyncCallback callback_;
    void *userData_;
};

struct OH_AVCodec *OH_AudioDecoder_CreateByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "input mime is nullptr!");

    std::shared_ptr<AVCodecAudioDecoder> audioDecoder = AudioDecoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(audioDecoder != nullptr, nullptr, "failed to AudioDecoderFactory::CreateByMime");

    struct AudioDecoderObject *object = new(std::nothrow) AudioDecoderObject(audioDecoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AudioDecoderObject");

    return object;
}

struct OH_AVCodec *OH_AudioDecoder_CreateByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "input name is nullptr!");

    std::shared_ptr<AVCodecAudioDecoder> audioDecoder = AudioDecoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(audioDecoder != nullptr, nullptr, "failed to AudioDecoderFactory::CreateByMime");

    struct AudioDecoderObject *object = new(std::nothrow) AudioDecoderObject(audioDecoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AudioDecoderObject");

    return object;
}

OH_AVErrCode OH_AudioDecoder_Destroy(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);

    if (audioDecObj != nullptr && audioDecObj->audioDecoder_ != nullptr) {
        audioDecObj->memoryObjList_.clear();
        audioDecObj->isStop_.store(true);
        int32_t ret = audioDecObj->audioDecoder_->Release();
        if (ret != MSERR_OK) {
            MEDIA_LOGE("audioDecoder Release failed!");
            delete codec;
            return AV_ERR_OPERATE_NOT_PERMIT;
        }
    } else {
        MEDIA_LOGD("audioDecoder_ is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioDecoder_Configure(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "audioDecoder is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder Configure failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioDecoder_Prepare(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "audioDecoder_ is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder Prepare failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioDecoder_Start(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "audioDecoder_ is nullptr!");
    audioDecObj->isStop_.store(false);
    audioDecObj->isEOS_.store(false);
    int32_t ret = audioDecObj->audioDecoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder Start failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioDecoder_Stop(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "audioDecoder_ is nullptr!");

    audioDecObj->isStop_.store(true);
    MEDIA_LOGD("set stop status to true");

    int32_t ret = audioDecObj->audioDecoder_->Stop();
    if (ret != MSERR_OK) {
        audioDecObj->isStop_.store(false);
        MEDIA_LOGE("audioDecoder Stop failed!, set stop status to false");
        return AV_ERR_OPERATE_NOT_PERMIT;
    }
    audioDecObj->memoryObjList_.clear();

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioDecoder_Flush(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "audioDecoder_ is nullptr!");
    audioDecObj->isFlushing_.store(true);
    MEDIA_LOGD("Set flush status to true");
    int32_t ret = audioDecObj->audioDecoder_->Flush();
    if (ret != MSERR_OK) {
        audioDecObj->isFlushing_.store(false);
        MEDIA_LOGE("audioDecoder Flush failed! Set flush status to false");
        return AV_ERR_OPERATE_NOT_PERMIT;
    }

    audioDecObj->memoryObjList_.clear();
    audioDecObj->isFlushing_.store(false);
    MEDIA_LOGD("set flush status to false");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioDecoder_Reset(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "audioDecoder_ is nullptr!");
    audioDecObj->isStop_.store(true);
    MEDIA_LOGD("Set stop status to true");

    int32_t ret = audioDecObj->audioDecoder_->Reset();
    if (ret != MSERR_OK) {
        audioDecObj->isStop_.store(false);
        MEDIA_LOGE("audioDecoder Reset failed! Set stop status to false");
        return AV_ERR_OPERATE_NOT_PERMIT;
    }

    audioDecObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioDecoder_PushInputData(struct OH_AVCodec *codec, uint32_t index, OH_AVCodecBufferAttr attr)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "audioDecoder_ is nullptr!");

    struct AVCodecBufferInfo bufferInfo;
    bufferInfo.presentationTimeUs = attr.pts;
    bufferInfo.size = attr.size;
    bufferInfo.offset = attr.offset;
    enum AVCodecBufferFlag bufferFlag = static_cast<enum AVCodecBufferFlag>(attr.flags);

    int32_t ret = audioDecObj->audioDecoder_->QueueInputBuffer(index, bufferInfo, bufferFlag);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder QueueInputBuffer failed!");
    if (bufferFlag == AVCODEC_BUFFER_FLAG_EOS) {
        audioDecObj->isEOS_.store(true);
        MEDIA_LOGD("Set eos status to true");
    }

    return AV_ERR_OK;
}

OH_AVFormat *OH_AudioDecoder_GetOutputDescription(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, nullptr, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, nullptr, "audioDecoder_ is nullptr!");

    Format format;
    int32_t ret = audioDecObj->audioDecoder_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "audioDecoder GetOutputFormat failed!");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    avFormat->format_ = format;

    return avFormat;
}

OH_AVErrCode OH_AudioDecoder_FreeOutputData(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "audioDecoder_ is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->ReleaseOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder ReleaseOutputBuffer failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioDecoder_SetParameter(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "audioDecoder_ is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder SetParameter failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AudioDecoder_SetCallback(struct OH_AVCodec *codec, struct OH_AVCodecAsyncCallback callback, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "audioDecoder_ is nullptr!");

    audioDecObj->callback_ = std::make_shared<NativeAudioDecoderCallback>(codec, callback, userData);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->callback_ != nullptr, AV_ERR_INVALID_VAL, "audioDecoder_ is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->SetCallback(audioDecObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder SetCallback failed!");

    return AV_ERR_OK;
}
