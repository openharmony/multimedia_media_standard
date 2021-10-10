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

#ifndef GST_UTILS_H
#define GST_UTILS_H

#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <glib/glib.h>
#include <gst/gst.h>
#include "nocopyable.h"

namespace OHOS {
namespace Media {
#define ELEM_NAME(elem) (GST_ELEMENT_NAME(elem) != nullptr) ? GST_ELEMENT_NAME(elem) : "unkonwn"

#define PAD_NAME(pad) (GST_PAD_NAME(pad) != nullptr) ? GST_PAD_NAME(pad) : "unknown"

#define PAD_PARENT_NAME(pad) (GST_PAD_PARENT(pad) != nullptr) ? \
    ((GST_ELEMENT_NAME(GST_PAD_PARENT(pad)) != nullptr) ? \
    (GST_ELEMENT_NAME(GST_PAD_PARENT(pad))) : "unknown") : "unknown"

#define STRUCTURE_NAME(struc) (gst_structure_get_name(struc) != nullptr) ? gst_structure_get_name(struc) : ""

bool MatchElementByMeta(
    const GstElement &elem, const std::string_view &metaKey, const std::vector<std::string_view> &expectedMetaFields);

template <typename T>
class ThizWrapper {
public:
    explicit ThizWrapper(std::weak_ptr<T> thiz) : thiz_(thiz) {}
    ~ThizWrapper() = default;

    DISALLOW_COPY_AND_MOVE(ThizWrapper);

    static std::shared_ptr<T> TakeStrongThiz(gpointer userdata)
    {
        if (userdata == nullptr) {
            return nullptr;
        }

        ThizWrapper<T> *wrapper = reinterpret_cast<ThizWrapper<T> *>(userdata);
        return wrapper->thiz_.lock();
    }

    static void OnDestory(gpointer wrapper)
    {
        if (wrapper == nullptr) {
            return;
        }
        delete reinterpret_cast<ThizWrapper<T> *>(wrapper);
    }

private:
    std::weak_ptr<T> thiz_;
};

class DecoderPerf {
public:
    DecoderPerf(GstElement &decoder) : decoder_(decoder) {}
    ~DecoderPerf();
    void Init();

    DISALLOW_COPY_AND_MOVE(DecoderPerf);
private:
    static GstPadProbeReturn OnOutputArrived(GstPad *pad, GstPadProbeInfo *info, gpointer thiz);
    std::vector<std::pair<GstPad *, gulong>> probes_;
    GstElement &decoder_;
};
}
}

#endif
