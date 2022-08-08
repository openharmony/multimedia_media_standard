/*
 * Copyright (C) 2022-2022 Huawei Device Co., Ltd.
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

#ifndef NATIVE_AVMAGIC_H
#define NATIVE_AVMAGIC_H

#include <refbase.h>
#include "format.h"
#include "avsharedmemory.h"

#define AV_MAGIC(a, b, c, d) (((a) << 24) + ((b) << 16) + ((c) << 8) + ((d) << 0))

enum class AVMagic {
    MEDIA_MAGIC_VIDEO_DECODER = AV_MAGIC('V', 'D', 'E', 'C'),
    MEDIA_MAGIC_VIDEO_ENCODER = AV_MAGIC('V', 'E', 'N', 'C'),
    MEDIA_MAGIC_AUDIO_DECODER = AV_MAGIC('A', 'D', 'E', 'C'),
    MEDIA_MAGIC_AUDIO_ENCODER = AV_MAGIC('A', 'E', 'N', 'C'),
    MEDIA_MAGIC_VIDEO_PLAYER = AV_MAGIC('V', 'P', 'L', 'Y'),
    MEDIA_MAGIC_AUDIO_PLAYER = AV_MAGIC('A', 'P', 'L', 'Y'),
    MEDIA_MAGIC_VIDEO_RECORDER = AV_MAGIC('V', 'R', 'E', 'C'),
    MEDIA_MAGIC_AUDIO_RECORDER = AV_MAGIC('A', 'R', 'E', 'C'),
    MEDIA_MAGIC_FORMAT = AV_MAGIC('F', 'R', 'M', 'T'),
    MEDIA_MAGIC_SHARED_MEMORY = AV_MAGIC('S', 'M', 'E', 'M'),
};

struct AVObjectMagic : public OHOS::RefBase {
    explicit AVObjectMagic(enum AVMagic m) : magic_(m) {}
    virtual ~AVObjectMagic() = default;
    enum AVMagic magic_;
};

struct AVFormat : public AVObjectMagic {
    AVFormat();
    explicit AVFormat(const OHOS::Media::Format &fmt);
    ~AVFormat() override;
    OHOS::Media::Format format_;
    char *outString_ = nullptr;
};

struct AVMemory : public AVObjectMagic {
    explicit AVMemory(const std::shared_ptr<OHOS::Media::AVSharedMemory> &mem);
    ~AVMemory() override;
    bool IsEqualMemory(const std::shared_ptr<OHOS::Media::AVSharedMemory> &mem);
    const std::shared_ptr<OHOS::Media::AVSharedMemory> memory_;
};

struct AVCodec : public AVObjectMagic {
    explicit AVCodec(enum AVMagic m) : AVObjectMagic(m) {}
    virtual ~AVCodec() = default;
};
#endif // NATIVE_AVMAGIC_H