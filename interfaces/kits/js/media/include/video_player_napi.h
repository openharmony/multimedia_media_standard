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

#ifndef VIDEO_PLAYER_NAPI_H_
#define VIDEO_PLAYER_NAPI_H_

#include "player.h"
#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "media_data_source_callback.h"
#include "common_napi.h"

namespace OHOS {
namespace Media {
class VideoPlayerNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    /**
     * createVideoPlayer(callback: AsyncCallback<VideoPlayer>): void
     * createVideoPlayer(): Promise<VideoPlayer>
     */
    static napi_value CreateVideoPlayer(napi_env env, napi_callback_info info);
    /**
     * setDisplaySurface(surfaceId: string, callback: AsyncCallback<void>): void
     * setDisplaySurface(surfaceId: string): Promise<void>
     */
    static napi_value SetDisplaySurface(napi_env env, napi_callback_info info);
    /**
     * Informal external interface to verify functionality
     * getDisplaySurface(callback: AsyncCallback<surfaceId: string>): void
     * getDisplaySurface(): Promise<surfaceId: string>
     */
    static napi_value GetDisplaySurface(napi_env env, napi_callback_info info);
    /**
     * prepare(callback: AsyncCallback<void>): void
     * prepare(): Promise<void>
     */
    static napi_value Prepare(napi_env env, napi_callback_info info);
    /**
     * play(callback: AsyncCallback<void>): void
     * play(): Promise<void>
     */
    static napi_value Play(napi_env env, napi_callback_info info);
    /**
     * pause(callback: AsyncCallback<void>): void
     * pause(): Promise<void>
     */
    static napi_value Pause(napi_env env, napi_callback_info info);
    /**
     * stop(callback: AsyncCallback<void>): void
     * stop(): Promise<void>
     */
    static napi_value Stop(napi_env env, napi_callback_info info);
    /**
     * reset(callback: AsyncCallback<void>): void
     * reset(): Promise<void>
     */
    static napi_value Reset(napi_env env, napi_callback_info info);
    /**
     * release(callback: AsyncCallback<void>): void
     * release(): Promise<void>
     */
    static napi_value Release(napi_env env, napi_callback_info info);
    /**
     * seek(timeMs: number, callback:AsyncCallback<number>): void
     * seek(timeMs: number, mode:SeekMode, callback:AsyncCallback<number>): void
     * seek(timeMs: number): Promise<number>
     * seek(timeMs: number, mode:SeekMode): Promise<number>
     */
    static napi_value Seek(napi_env env, napi_callback_info info);
    /**
     * setSpeed(speed: number, callback:AsyncCallback<number>): void
     * setSpeed(speed: number): Promise<number>
     */
    static napi_value SetSpeed(napi_env env, napi_callback_info info);
    /**
     * setVolume(vol: number, callback:AsyncCallback<void>): void
     * setVolume(vol: number): Promise<void>
     */
    static napi_value SetVolume(napi_env env, napi_callback_info info);
    /**
     * url: string
     */
    static napi_value SetUrl(napi_env env, napi_callback_info info);
    static napi_value GetUrl(napi_env env, napi_callback_info info);
    /**
     * dataSrc?: AVDataSource
     */
    static napi_value GetDataSrc(napi_env env, napi_callback_info info);
    static napi_value SetDataSrc(napi_env env, napi_callback_info info);
    /**
     * fdSrc: AVFileDescriptor
     */
    static napi_value GetFdSrc(napi_env env, napi_callback_info info);
    static napi_value SetFdSrc(napi_env env, napi_callback_info info);
    /**
     * loop: boolenan
     */
    static napi_value SetLoop(napi_env env, napi_callback_info info);
    static napi_value GetLoop(napi_env env, napi_callback_info info);
    /**
     * readonly currentTime: number
     */
    static napi_value GetCurrentTime(napi_env env, napi_callback_info info);
    /**
     * readonly duration: number
     */
    static napi_value GetDuration(napi_env env, napi_callback_info info);
    /**
     * readonly state: VideoPlayState
     */
    static napi_value GetState(napi_env env, napi_callback_info info);
    /**
     * readonly width: number
     */
    static napi_value GetWidth(napi_env env, napi_callback_info info);
    /**
     * readonly height: number
     */
    static napi_value GetHeight(napi_env env, napi_callback_info info);
    /**
     * getTrackDescription(callback:AsyncCallback<Array<MediaDescription>>): void
     * getTrackDescription(): Promise<Array<MediaDescription>>
     */
    static napi_value GetTrackDescription(napi_env env, napi_callback_info info);
    /**
     * on(type: 'playbackCompleted', callback: Callback<void>): void
     * on(type: 'bufferingUpdate', callback: (infoType: BufferingInfoType, valut: number) => void): void
     * on(type: 'startRenderFrame', callback: Callback<void>): void
     * on(type: 'videoSizeChanged', callback: (width: number, height: number)) => void): void
     * on(type: 'error', callback: ErrorCallback): void
     */
    static napi_value On(napi_env env, napi_callback_info info);
    static void AsyncGetTrackDescription(napi_env env, void *data);
    static void AsyncSetDisplaySurface(napi_env env, void *data);
    static void AsyncGetDisplaySurface(napi_env env, void *data);
    static void CompleteAsyncWork(napi_env env, napi_status status, void *data);
    void OnErrorCallback(MediaServiceExtErrCode errCode);
    void ReleaseDataSource(std::shared_ptr<MediaDataSourceCallback> dataSourceCb);
    VideoPlayerNapi();
    ~VideoPlayerNapi();

    static napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    std::shared_ptr<Player> nativePlayer_ = nullptr;
    std::shared_ptr<PlayerCallback> jsCallback_ = nullptr;
    std::shared_ptr<MediaDataSourceCallback> dataSrcCallBack_ = nullptr;
    std::string url_ = "";
    std::vector<Format> videoTrackInfoVec_;
    AVFileDescriptor rawFd_;
};
} // namespace Media
} // namespace OHOS
#endif /* VIDEO_PLAYER_NAPI_H_ */
