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

#ifndef PLAYBIN_SOURCE_H
#define PLAYBIN_SOURCE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <gst/gst.h>
#include "gst_appsrc_wrap.h"
#include "inner_msg_define.h"
#include "media_errors.h"
#include "uri_helper.h"

namespace OHOS {
namespace Media {
namespace PlayBin {
class SourceBase {
public:
    static std::shared_ptr<SourceBase> Create(const UriHelper &uri);
    static std::shared_ptr<SourceBase> Create(const std::shared_ptr<IMediaDataSource> &dataSrc);

    virtual ~SourceBase() = default;

    virtual std::string GetGstUrlDesc()
    {
        return "";
    }

    virtual int32_t Start()
    {
        return MSERR_OK;
    }

    virtual int32_t Stop()
    {
        return MSERR_OK;
    }

    virtual bool IsSeekable()
    {
        return true;
    }

    using ElemPtr = GstElement *;
    virtual void OnPlayBinSetup(ElemPtr playbin)
    {
        (void)playbin;
    }

    virtual void OnSourceSetup(ElemPtr source)
    {
        (void)source;
    }

    using MsgNotifier = std::function<void(const InnerMessage &msg)>;
    virtual void SetMsgNotifier(const MsgNotifier &notifier)
    {
        (void)notifier;
    }

protected:
    SourceBase() = default;
};

class AVDataSource : public SourceBase {
public:
    AVDataSource(const std::shared_ptr<IMediaDataSource> &dataSrc);
    ~AVDataSource() = default;

    std::string GetGstUrlDesc() override;
    int32_t Start() override;
    int32_t Stop() override;
    bool IsSeekable() override;

    void OnSourceSetup(ElemPtr source) override;
    void SetMsgNotifier(const MsgNotifier &notifier) override;

private:
    void OnAppsrcErrorMessageReceived(int32_t errorCode);

    std::shared_ptr<IMediaDataSource> avDataSrc_;
    std::shared_ptr<GstAppsrcWrap> appSrc_;
    MsgNotifier notifier_;
};

class NetWorkSource : public SourceBase {
public:
    NetWorkSource(const UriHelper &url) = default;
    ~NetWorkSource() = default;

    std::string GetGstUrlDesc() override;
    void OnPlayBinSetup(ElemPtr playbin) override;
    void OnSourceSetup(ElemPtr source) override;

private:
    UriHelper uriHelper_;

    static const uint64_t RING_BUFFER_MAX_SIZE;
    static const int32_t PLAYBIN_QUEUE_MAX_SIZE;
    static const uint64_t BUFFER_DURATION;
    static const int32_t BUFFER_LOW_PERCENT_DEFAULT;
    static const int32_t BUFFER_HIGH_PERCENT_DEFAULT;
    static const uint32_t HTTP_TIME_OUT_DEFAULT;
};
}
}
}

#endif