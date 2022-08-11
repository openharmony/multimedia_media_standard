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
#include "native_avcodec_videodecoder.h"
#include "native_avmagic.h"
#include "native_window.h"
#include "avcodec_video_decoder.h"
#include "avsharedmemory.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeVideoDecoder"};
}

using namespace OHOS::Media;

struct VideoDecoderObject : public OH_AVCodec {
    explicit VideoDecoderObject(const std::shared_ptr<AVCodecVideoDecoder> &decoder)
        : OH_AVCodec(AVMagic::MEDIA_MAGIC_VIDEO_DECODER), videoDecoder_(decoder) {}
    ~VideoDecoderObject() = default;

    const std::shared_ptr<AVCodecVideoDecoder> videoDecoder_;
    std::list<OHOS::sptr<OH_AVMemory>> memoryObjList_;
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
    std::atomic<bool> isFlushing_ = false;
    std::atomic<bool> isStop_ = false;
    std::atomic<bool> isEOS_ = false;
};

class NativeVideoDecoderCallback : public AVCodecCallback {
public:
    NativeVideoDecoderCallback(OH_AVCodec *codec, struct OH_AVCodecAsyncCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData) {}
    virtual ~NativeVideoDecoderCallback() = default;

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
            struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec_);
            CHECK_AND_RETURN_LOG(videoDecObj->videoDecoder_ != nullptr, "videoDecoder_ is nullptr!");

            if (videoDecObj->isFlushing_.load() || videoDecObj->isStop_.load() || videoDecObj->isEOS_.load()) {
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
            struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec_);
            CHECK_AND_RETURN_LOG(videoDecObj->videoDecoder_ != nullptr, "videoDecoder_ is nullptr!");

            if (videoDecObj->isFlushing_.load() || videoDecObj->isStop_.load()) {
                MEDIA_LOGD("At flush or stop, ignore");
                return;
            }
            struct OH_AVCodecBufferAttr bufferAttr;
            bufferAttr.pts = info.presentationTimeUs;
            bufferAttr.size = info.size;
            bufferAttr.offset = info.offset;
            bufferAttr.flags = flag;
            // The bufferInfo lifecycle is controlled by the current function stack
            callback_.onNeedOutputData(codec_, index, nullptr, &bufferAttr, userData_);
        }
    }

private:
    OH_AVMemory *GetInputData(struct OH_AVCodec *codec, uint32_t index)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, nullptr, "magic error!");

        struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, nullptr, "videoDecoder_ is nullptr!");

        std::shared_ptr<AVSharedMemory> memory = videoDecObj->videoDecoder_->GetInputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get input buffer is nullptr!");

        for (auto &memoryObj : videoDecObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<OH_AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<OH_AVMemory> object = new(std::nothrow) OH_AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVMemory");

        videoDecObj->memoryObjList_.push_back(object);
        return reinterpret_cast<OH_AVMemory *>(object.GetRefPtr());
    }

    OH_AVMemory *GetOutputData(struct OH_AVCodec *codec, uint32_t index)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, nullptr, "magic error!");

        struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, nullptr, "videoDecoder_ is nullptr!");

        std::shared_ptr<AVSharedMemory> memory = videoDecObj->videoDecoder_->GetOutputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get output buffer is nullptr!");

        for (auto &memoryObj : videoDecObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<OH_AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<OH_AVMemory> object = new(std::nothrow) OH_AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVMemory");

        videoDecObj->memoryObjList_.push_back(object);
        return reinterpret_cast<OH_AVMemory *>(object.GetRefPtr());
    }

    struct OH_AVCodec *codec_;
    struct OH_AVCodecAsyncCallback callback_;
    void *userData_;
};

struct OH_AVCodec *OH_VideoDecoder_CreateByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "input mime is nullptr!");

    std::shared_ptr<AVCodecVideoDecoder> videoDecoder = VideoDecoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(videoDecoder != nullptr, nullptr, "failed to VideoDecoderFactory::CreateByMime");

    struct VideoDecoderObject *object = new(std::nothrow) VideoDecoderObject(videoDecoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new VideoDecoderObject");

    return object;
}

struct OH_AVCodec *OH_VideoDecoder_CreateByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "input name is nullptr!");

    std::shared_ptr<AVCodecVideoDecoder> videoDecoder = VideoDecoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(videoDecoder != nullptr, nullptr, "failed to VideoDecoderFactory::CreateByMime");

    struct VideoDecoderObject *object = new(std::nothrow) VideoDecoderObject(videoDecoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new VideoDecoderObject");

    return object;
}

OH_AVErrCode OH_VideoDecoder_Destroy(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);

    if (videoDecObj != nullptr && videoDecObj->videoDecoder_ != nullptr) {
        videoDecObj->memoryObjList_.clear();
        videoDecObj->isStop_.store(false);
        int32_t ret = videoDecObj->videoDecoder_->Release();
        if (ret != MSERR_OK) {
            MEDIA_LOGE("videoDecoder Release failed!");
            delete codec;
            return AV_ERR_OPERATE_NOT_PERMIT;
        }
    } else {
        MEDIA_LOGD("videoDecoder_ is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_Configure(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder Configure failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_Prepare(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder_ is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder Prepare failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_Start(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder_ is nullptr!");
    videoDecObj->isStop_.store(false);
    videoDecObj->isEOS_.store(false);
    int32_t ret = videoDecObj->videoDecoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder Start failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_Stop(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder_ is nullptr!");

    videoDecObj->isStop_.store(true);
    MEDIA_LOGD("Set stop status to true");

    int32_t ret = videoDecObj->videoDecoder_->Stop();
    if (ret != MSERR_OK) {
        videoDecObj->isStop_.store(false);
        MEDIA_LOGE("videoDecoder Stop failed! Set stop status to false");
        return AV_ERR_OPERATE_NOT_PERMIT;
    }
    videoDecObj->memoryObjList_.clear();

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_Flush(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder_ is nullptr!");

    videoDecObj->isFlushing_.store(true);
    MEDIA_LOGD("Set flush status to true");

    int32_t ret = videoDecObj->videoDecoder_->Flush();
    if (ret != MSERR_OK) {
        videoDecObj->isFlushing_.store(false);
        MEDIA_LOGD("videoDecoder Flush failed! Set flush status to false");
        return AV_ERR_OPERATE_NOT_PERMIT;
    }

    videoDecObj->memoryObjList_.clear();
    videoDecObj->isFlushing_.store(false);
    MEDIA_LOGD("Set flush status to false");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_Reset(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder_ is nullptr!");
    videoDecObj->isStop_.store(false);
    MEDIA_LOGD("Set stop status to true");
    int32_t ret = videoDecObj->videoDecoder_->Reset();
    if (ret != MSERR_OK) {
        videoDecObj->isStop_.store(false);
        MEDIA_LOGE("videoDecoder Reset failed! Set stop status to false");
        return AV_ERR_OPERATE_NOT_PERMIT;
    }

    videoDecObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_SetSurface(struct OH_AVCodec *codec, struct NativeWindow *window)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(window != nullptr, AV_ERR_INVALID_VAL, "input window is nullptr!");
    CHECK_AND_RETURN_RET_LOG(window->surface != nullptr, AV_ERR_INVALID_VAL, "input surface is nullptr!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder_ is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->SetOutputSurface(window->surface);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder SetOutputSurface failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_PushInputData(struct OH_AVCodec *codec, uint32_t index, OH_AVCodecBufferAttr attr)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder_ is nullptr!");

    struct AVCodecBufferInfo bufferInfo;
    bufferInfo.presentationTimeUs = attr.pts;
    bufferInfo.size = attr.size;
    bufferInfo.offset = attr.offset;
    enum AVCodecBufferFlag bufferFlag = static_cast<enum AVCodecBufferFlag>(attr.flags);

    int32_t ret = videoDecObj->videoDecoder_->QueueInputBuffer(index, bufferInfo, bufferFlag);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder QueueInputBuffer failed!");
    if (bufferFlag == AVCODEC_BUFFER_FLAG_EOS) {
        videoDecObj->isEOS_.store(true);
    }

    return AV_ERR_OK;
}

OH_AVFormat *OH_VideoDecoder_GetOutputDescription(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, nullptr, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, nullptr, "videoDecoder_ is nullptr!");

    Format format;
    int32_t ret = videoDecObj->videoDecoder_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "videoDecoder GetOutputFormat failed!");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    avFormat->format_ = format;

    return avFormat;
}

OH_AVErrCode OH_VideoDecoder_RenderOutputData(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder_ is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->ReleaseOutputBuffer(index, true);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder ReleaseOutputBuffer failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_FreeOutputData(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder_ is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->ReleaseOutputBuffer(index, false);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder ReleaseOutputBuffer failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_SetParameter(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder_ is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder SetParameter failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoDecoder_SetCallback(struct OH_AVCodec *codec, struct OH_AVCodecAsyncCallback callback, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecoderObject *videoDecObj = reinterpret_cast<VideoDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder_ is nullptr!");

    videoDecObj->callback_ = std::make_shared<NativeVideoDecoderCallback>(codec, callback, userData);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->callback_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder_ is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->SetCallback(videoDecObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder SetCallback failed!");

    return AV_ERR_OK;
}
