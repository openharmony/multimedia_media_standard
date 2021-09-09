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

#ifndef MEDIA_DATA_SOURCE_H_
#define MEDIA_DATA_SOURCE_H_

#include "avsharedmemory.h"

namespace OHOS {
namespace Media {
/**
 * @brief Use with IMediaDataSource::ReadAt.
 */
enum MediaDataSourceError : int32_t {
    /**
     * use with ReadAt.the resource is cut off and player will end.
     * And the player will complete buffers and return an error.
     */
    SOURCE_ERROR_IO = -2,
    /* use with ReadAt.the resource is eos and player will complete. */
    SOURCE_ERROR_EOF = -1,
};

/**
 * @brief the mediaDataSource instance need set to player.
 *
 */
class IMediaDataSource {
public:
    virtual ~IMediaDataSource() = default;

    /**
     * @brief One-to-one use with ReadAt.
     * After ReadAt return a size > 0, player use GetMem to get a AVSharedMemory.
     * Return a new AVSharedMemory instance containing a chunk of shared memory.
     * @return The AVSharedMemory instance containing a chunk of shared memory.
     */
    virtual std::shared_ptr<AVSharedMemory> GetMem() = 0;

    /**
     * @brief One-to-one use with getMem.
     * If the size of the datasource is not -1, provide the implementation of this interface.
     * Player use ReadAt to tell the position and length of mem want get.(length is number of Bytes)
     * @param pos The stream pos player want get start.
     * @param length Stream length player want to get.
     * @return The actual length of stream length player will return, if failed or no mem return MediaDataSourceError;
     */
    virtual int32_t ReadAt(int64_t pos, uint32_t length) = 0;

    /**
     * @brief One-to-one use with getMem.
     * If the size of the datasource is -1, provide the implementation of this interface.
     * Player use ReadAt to tell the length of mem want get.(length is number of Bytes)
     * @param length Stream length player want to get.
     * @return The actual length of stream length player will return, if failed or no mem return MediaDataSourceError;
     */
    virtual int32_t ReadAt(uint32_t length) = 0;

    /**
     * @brief Get the total size of the stream.
     * If the stream does not have the length, return -1. With -1, player will use the datasource not seekable.
     * @param size Total size of the stream. If no size return -1.
     * @return MSERR_OK if ok; others if failed. see media_errors.h
     */
    virtual int32_t GetSize(int64_t &size) = 0;
};
} // namespace Media
} // namespace OHOS
#endif /* MEDIA_DATA_SOURCE_H_ */