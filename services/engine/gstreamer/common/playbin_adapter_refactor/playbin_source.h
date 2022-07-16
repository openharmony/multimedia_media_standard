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
#include <gst/gst.h>
#include "gst_appsrc_wrap.h"
#include "media_errors.h"
#include "uri_helper.h"

namespace OHOS {
namespace Media {
class PlayBinSource {
public:
    static std::unique_ptr<PlayBinSource> Create(const UriHelper &uri);
    static std::unique_ptr<PlayBinSource> Create(const std::shared_ptr<IMediaDataSource> &dataSrc);

    virtual ~PlayBinSource() = default;

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
    virtual void OnSourceSetup(ElemPtr source)
    {
        (void)source;
    }

    using MsgNotifier = std::function<void(int32_t)>;
    virtual void SetMsgNotifier(const MsgNotifier &notifier)
    {
        (void)notifier;
    }

protected:
    PlayBinSource() = default;
};

class AVDataSource : public PlayBinSource {
public:
    AVDataSource(const std::shared_ptr<IMediaDataSource> &dataSrc);
    ~AVDataSource() = default;

    int32_t Start();
    int32_t Stop();
    bool IsSeekable();

    void OnSourceSetup(ElemPtr source);
    void SetMsgNotifier(const MsgNotifier &notifier);

private:
    void OnAppsrcErrorMessageReceived(int32_t errorCode);

    std::shared_ptr<IMediaDataSource> avDataSrc_;
    std::shared_ptr<GstAppsrcWrap> appSrc_;
    MsgNotifier notifier_;
};

class HttpsSource : public PlayBinSource {
public:
    HttpsSource() = default;
    ~HttpsSource() = default;

    void OnSourceSetup(ElemPtr source);
};
}
}

#endif