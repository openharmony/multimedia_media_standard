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
#include "native_avcodec_videoencoder.h"
#include "native_avmagic.h"
#include "native_window.h"
#include "avcodec_video_encoder.h"
#include "avsharedmemory.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeVideoEncoder"};
}

using namespace OHOS::Media;

struct VideoEncoderObject : public OH_AVCodec {
    explicit VideoEncoderObject(const std::shared_ptr<AVCodecVideoEncoder> &encoder)
        : OH_AVCodec(AVMagic::MEDIA_MAGIC_VIDEO_ENCODER), videoEncoder_(encoder) {}
    ~VideoEncoderObject() = default;

    const std::shared_ptr<AVCodecVideoEncoder> videoEncoder_;
    std::list<OHOS::sptr<OH_AVMemory>> memoryObjList_;
    OHOS::sptr<OH_AVFormat> outputFormat_ = nullptr;
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
    std::atomic<bool> isFlushing_ = false;
    std::atomic<bool> isStop_ = false;
    std::atomic<bool> isEOS_ = false;
};

class NativeVideoEncoderCallback : public AVCodecCallback {
public:
    NativeVideoEncoderCallback(OH_AVCodec *codec, struct OH_AVCodecAsyncCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData) {}
    virtual ~NativeVideoEncoderCallback() = default;

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
            struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec_);
            CHECK_AND_RETURN_LOG(videoEncObj->videoEncoder_ != nullptr, "context videoDecoder is nullptr!");

            if (videoEncObj->isFlushing_.load() || videoEncObj->isStop_.load() || videoEncObj->isEOS_.load()) {
                MEDIA_LOGD("At flush, eos or stop, no buffer available");
                return;
            }
        }

        callback_.onNeedInputData(codec_, index, nullptr, userData_);
    }

    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override
    {
        if (codec_ != nullptr) {
            struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec_);
            CHECK_AND_RETURN_LOG(videoEncObj->videoEncoder_ != nullptr, "context videoDecoder is nullptr!");

            if (videoEncObj->isFlushing_.load() || videoEncObj->isStop_.load()) {
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
    OH_AVMemory *GetOutputData(struct OH_AVCodec *codec, uint32_t index)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, nullptr, "magic error!");

        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "videoEncoder_ is nullptr!");

        std::shared_ptr<AVSharedMemory> memory = videoEncObj->videoEncoder_->GetOutputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get output buffer is nullptr!");

        for (auto &memoryObj : videoEncObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<OH_AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<OH_AVMemory> object = new(std::nothrow) OH_AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new OH_AVMemory");

        videoEncObj->memoryObjList_.push_back(object);
        return reinterpret_cast<OH_AVMemory *>(object.GetRefPtr());
    }

    struct OH_AVCodec *codec_;
    struct OH_AVCodecAsyncCallback callback_;
    void *userData_;
};

struct OH_AVCodec *OH_VideoEncoder_CreateByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "input mime is nullptr!");

    std::shared_ptr<AVCodecVideoEncoder> videoEncoder = VideoEncoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, nullptr, "failed to VideoEncoderFactory::CreateByMime");

    struct VideoEncoderObject *object = new(std::nothrow) VideoEncoderObject(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new VideoEncoderObject");

    return object;
}

struct OH_AVCodec *OH_VideoEncoder_CreateByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "input name is nullptr!");

    std::shared_ptr<AVCodecVideoEncoder> videoEncoder = VideoEncoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, nullptr, "failed to VideoEncoderFactory::CreateByMime");

    struct VideoEncoderObject *object = new(std::nothrow) VideoEncoderObject(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new VideoEncoderObject");

    return object;
}

OH_AVErrCode OH_VideoEncoder_Destroy(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);

    if (videoEncObj != nullptr && videoEncObj->videoEncoder_ != nullptr) {
        videoEncObj->memoryObjList_.clear();
        videoEncObj->isStop_.store(true);
        int32_t ret = videoEncObj->videoEncoder_->Release();
        if (ret != MSERR_OK) {
            MEDIA_LOGE("videoEncoder Release failed!");
            delete codec;
            return AV_ERR_OPERATE_NOT_PERMIT;
        }
    } else {
        MEDIA_LOGD("videoEncoder_ is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Configure(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder Configure failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Prepare(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder Prepare failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Start(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");
    videoEncObj->isStop_.store(false);
    videoEncObj->isEOS_.store(false);
    int32_t ret = videoEncObj->videoEncoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder Start failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Stop(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");
    videoEncObj->isStop_.store(true);
    MEDIA_LOGD("set stop status to true");

    int32_t ret = videoEncObj->videoEncoder_->Stop();
    if (ret != MSERR_OK) {
        videoEncObj->isStop_.store(false);
        MEDIA_LOGE("videoEncoder Stop failed! Set stop status to false");
        return AV_ERR_OPERATE_NOT_PERMIT;
    }
    videoEncObj->memoryObjList_.clear();

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Flush(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");

    videoEncObj->isFlushing_.store(true);
    MEDIA_LOGD("Set flush status to true");
    int32_t ret = videoEncObj->videoEncoder_->Flush();
    if (ret != MSERR_OK) {
        videoEncObj->isFlushing_.store(false);
        MEDIA_LOGD("videoEncoder Flush failed! Set flush status to false");
        return AV_ERR_OPERATE_NOT_PERMIT;
    }
    videoEncObj->memoryObjList_.clear();
    videoEncObj->isFlushing_.store(false);
    MEDIA_LOGD("set flush status to false");
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_Reset(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");
    videoEncObj->isStop_.store(true);
    MEDIA_LOGD("Set stop status to true");

    int32_t ret = videoEncObj->videoEncoder_->Reset();
    if (ret != MSERR_OK) {
        videoEncObj->isStop_.store(false);
        MEDIA_LOGE("videoEncoder Reset failed! Set stop status to false");
        return AV_ERR_OPERATE_NOT_PERMIT;
    }

    videoEncObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_GetSurface(OH_AVCodec *codec, OHNativeWindow **window)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr && window != nullptr, AV_ERR_INVALID_VAL, "input is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");

    OHOS::sptr<OHOS::Surface> surface = videoEncObj->videoEncoder_->CreateInputSurface();
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, AV_ERR_OPERATE_NOT_PERMIT, "venc createInputSurface failed!");

    *window = CreateNativeWindowFromSurface(&surface);

    CHECK_AND_RETURN_RET_LOG(*window != nullptr, AV_ERR_INVALID_VAL, "CreateNativeWindowFromSurface failed!");

    return AV_ERR_OK;
}

OH_AVFormat *OH_VideoEncoder_GetOutputDescription(struct OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, nullptr, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "videoEncoder_ is nullptr!");

    Format format;
    int32_t ret = videoEncObj->videoEncoder_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "videoEncoder GetOutputFormat failed!");

    videoEncObj->outputFormat_ = new(std::nothrow) OH_AVFormat(format);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->outputFormat_ != nullptr, nullptr, "failed to new OH_AVFormat");

    return reinterpret_cast<OH_AVFormat *>(videoEncObj->outputFormat_.GetRefPtr());
}

OH_AVErrCode OH_VideoEncoder_FreeOutputData(struct OH_AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->ReleaseOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder ReleaseOutputBuffer failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_NotifyEndOfStream(OH_AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->NotifyEos();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder NotifyEos failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_SetParameter(struct OH_AVCodec *codec, struct OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder SetParameter failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoEncoder_SetCallback(
    struct OH_AVCodec *codec, struct OH_AVCodecAsyncCallback callback, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");

    videoEncObj->callback_ = std::make_shared<NativeVideoEncoderCallback>(codec, callback, userData);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->callback_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->SetCallback(videoEncObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder SetCallback failed!");

    return AV_ERR_OK;
}
