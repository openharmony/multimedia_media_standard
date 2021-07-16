#ifndef RECORDER_PARAM_H
#define RECORDER_PARAM_H

#include <cstdint>
#include <string>
#include "recorder.h"

namespace OHOS {
namespace Media {
/*
 * Declare the type enum value valid range for different kind of RecorderParam.
 * The "Public" means externally visable.
 * The "Private" means internally visable.
 */
enum RecorderParamSectionStart {
    PUBLIC_PARAM_SECTION_START = 0,
    PRIVATE_PARAM_SECTION_START = 0x10000,
};

enum RecorderPublicParamType : uint32_t {
    PUBLIC_PARAM_TYPE_BEGIN = PUBLIC_PARAM_SECTION_START,
    // video begin
    VID_PUBLIC_PARAM_BEGIN,
    VID_ENC_FMT,
    VID_RECTANGLE,
    VID_BITRATE,
    VID_FRAMERATE,
    VID_CAPTURERATE,
    VID_PUBLIC_PARAM_END,
    // audio begin
    AUD_PUBLIC_PARAM_BEGIN,
    AUD_ENC_FMT,
    AUD_SAMPLERATE,
    AUD_CHANNEL,
    AUD_BITRATE,
    AUD_PUBIC_PARAM_END,
    // output begin,
    MAX_DURATION,
    MAX_SIZE,
    OUT_PATH,
    OUT_FD,
    NEXT_OUT_FD, // reserved.

    PUBLIC_PARAM_TYPE_END,
};

/*
 * Recorder parameter base structure, inherite to it to extend the new parameter.
 */
struct RecorderParam {
    explicit RecorderParam(uint32_t t) : type(t)  {}
    RecorderParam() = delete;
    virtual ~RecorderParam() = default;
    bool IsVideoParam() const
    {
        return (type > VID_PUBLIC_PARAM_BEGIN) && (type < VID_PUBLIC_PARAM_END);
    }

    bool IsAudioParam() const
    {
        return (type > AUD_PUBLIC_PARAM_BEGIN) && (type < AUD_PUBIC_PARAM_END);
    }

    uint32_t type;
};

struct VidEnc : public RecorderParam {
    explicit VidEnc(VideoCodecFormat fmt) : RecorderParam(RecorderPublicParamType::VID_ENC_FMT), encFmt(fmt) {}
    VideoCodecFormat encFmt;
};

struct VidRectangle : public RecorderParam {
    VidRectangle(int32_t w, int32_t h) : RecorderParam(RecorderPublicParamType::VID_RECTANGLE), width(w), height(h) {}
    int32_t width;
    int32_t height;
};

struct VidBitRate : public RecorderParam {
    explicit VidBitRate(int32_t br) : RecorderParam(RecorderPublicParamType::VID_BITRATE), bitRate(br) {}
    int32_t bitRate;
};

struct VidFrameRate : public RecorderParam {
    explicit VidFrameRate(int32_t r) : RecorderParam(RecorderPublicParamType::VID_FRAMERATE), frameRate(r) {}
    int32_t frameRate;
};

struct CaptureRate : public RecorderParam {
    explicit CaptureRate(double cr) : RecorderParam(RecorderPublicParamType::VID_CAPTURERATE), capRate(cr) {}
    double capRate;
};

struct AudEnc : public RecorderParam {
    explicit AudEnc(AudioCodecFormat fmt) : RecorderParam(RecorderPublicParamType::AUD_ENC_FMT), encFmt(fmt) {}
    AudioCodecFormat encFmt;
};

struct AudSampleRate : public RecorderParam {
    AudSampleRate(int32_t sr) : RecorderParam(RecorderPublicParamType::AUD_SAMPLERATE), sampleRate(sr) {}
    int32_t sampleRate;
};

struct AudChannel : public RecorderParam {
    explicit AudChannel(int32_t num) : RecorderParam(RecorderPublicParamType::AUD_CHANNEL), channel(num) {}
    int32_t channel;
};

struct AudBitRate : public RecorderParam {
    explicit AudBitRate(int32_t br) : RecorderParam(RecorderPublicParamType::AUD_BITRATE), bitRate(br) {}
    int32_t bitRate;
};

struct MaxDuration : public RecorderParam {
    explicit MaxDuration(int32_t maxDur) : RecorderParam(RecorderPublicParamType::MAX_DURATION), duration(maxDur) {}
    int32_t duration;
};

struct MaxFileSize : public RecorderParam {
    explicit MaxFileSize(int64_t maxSize) : RecorderParam(RecorderPublicParamType::MAX_SIZE), size(maxSize) {}
    int64_t size;
};

struct OutFilePath : public RecorderParam {
    explicit OutFilePath(const std::string &filePath)
        : RecorderParam(RecorderPublicParamType::OUT_PATH), path(filePath) {}
    std::string path;
};

struct OutFd : public RecorderParam {
    explicit OutFd(int32_t outFd) : RecorderParam(RecorderPublicParamType::OUT_FD), fd(outFd) {}
    int32_t fd;
};

struct NextOutFd : public RecorderParam {
    explicit NextOutFd(int32_t nextOutFd) : RecorderParam(RecorderPublicParamType::NEXT_OUT_FD), fd(nextOutFd) {}
    int32_t fd;
};
}
}

#endif