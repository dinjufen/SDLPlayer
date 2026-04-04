#include "player/Player.h"

#include <SDL.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace player {

namespace {

constexpr int kAudioDeviceSampleRate = 48000;
constexpr int kAudioDeviceChannels = 2;
constexpr int kAudioBytesPerSample = 2;
constexpr int kMaxQueuedAudioMilliseconds = 3000;
constexpr std::size_t kMaxQueuedVideoFrames = 24;
constexpr double kSeekClampMarginSeconds = 0.1;
constexpr double kVideoDisplayLeewaySeconds = 0.04;

const std::vector<double> kSupportedSpeeds = {0.5, 1.0, 1.25, 1.5, 2.0};

std::string AvErrorToString(const int errorCode) {
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(errorCode, buffer, sizeof(buffer));
    return buffer;
}

double TimestampToSeconds(const int64_t pts, const AVRational timeBase) {
    if (pts == AV_NOPTS_VALUE) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    return static_cast<double>(pts) * av_q2d(timeBase);
}

int64_t SecondsToGlobalTimestamp(const double seconds) {
    return static_cast<int64_t>(seconds * static_cast<double>(AV_TIME_BASE));
}

std::string SafeCString(const char* value) {
    return value != nullptr ? std::string(value) : std::string();
}

}  // namespace

struct Player::Impl {
    struct AudioChunk {
        std::vector<std::uint8_t> bytes;
        std::size_t offset = 0;
        double mediaPtsSeconds = 0.0;
        double mediaDurationSeconds = 0.0;
    };

    struct VideoFrame {
        std::vector<std::uint8_t> pixels;
        int width = 0;
        int height = 0;
        int pitch = 0;
        double ptsSeconds = 0.0;
    };

    MediaInfo mediaInfo;
    std::string statusText = "Open a media file with O.";

    AVFormatContext* formatContext = nullptr;
    AVCodecContext* audioCodecContext = nullptr;
    AVCodecContext* videoCodecContext = nullptr;
    AVStream* audioStream = nullptr;
    AVStream* videoStream = nullptr;
    SwrContext* swrContext = nullptr;
    SwsContext* swsContext = nullptr;
    int audioStreamIndex = -1;
    int videoStreamIndex = -1;

    SDL_AudioDeviceID audioDevice = 0;
    SDL_Texture* videoTexture = nullptr;
    SDL_Renderer* textureRenderer = nullptr;
    int textureWidth = 0;
    int textureHeight = 0;

    std::deque<AudioChunk> audioQueue;
    std::deque<VideoFrame> videoQueue;
    VideoFrame displayedFrame;
    bool hasDisplayedFrame = false;

    std::mutex audioMutex;
    std::mutex videoMutex;
    mutable std::mutex clockMutex;
    mutable std::mutex stateMutex;

    std::thread workerThread;
    std::atomic<bool> stopRequested = false;
    std::atomic<bool> hasMedia = false;
    std::atomic<bool> paused = false;
    std::atomic<bool> seekRequested = false;
    std::atomic<double> requestedSeekSeconds = 0.0;
    std::atomic<double> playbackSpeed = 1.0;
    std::atomic<double> lastKnownClockSeconds = 0.0;

    std::chrono::steady_clock::time_point noAudioAnchorWallClock = std::chrono::steady_clock::now();
    double noAudioAnchorPositionSeconds = 0.0;
    double preparedResampleSpeed = 1.0;
    int preparedResampleRate = kAudioDeviceSampleRate;
    int preparedInputSampleRate = 0;
    AVSampleFormat preparedInputSampleFormat = AV_SAMPLE_FMT_NONE;
    AVChannelLayout preparedInputChannelLayout = {};
    bool hasPreparedInputChannelLayout = false;
    double nextAudioPtsSeconds = 0.0;
    bool flushedAtEof = false;
    std::atomic<bool> reachedEof = false;
    int preparedVideoWidth = 0;
    int preparedVideoHeight = 0;
    AVPixelFormat preparedVideoFormat = AV_PIX_FMT_NONE;

    ~Impl() { Close(); }

    bool Open(const std::string& path, std::string& errorMessage) {
        Close();

        SetStatusText("Opening: " + path);

        avformat_network_init();

        int error = avformat_open_input(&formatContext, path.c_str(), nullptr, nullptr);
        if (error < 0) {
            errorMessage = "Failed to open media: " + AvErrorToString(error);
            SetStatusText(errorMessage);
            return false;
        }

        error = avformat_find_stream_info(formatContext, nullptr);
        if (error < 0) {
            errorMessage = "Failed to read stream info: " + AvErrorToString(error);
            SetStatusText(errorMessage);
            Close();
            return false;
        }

        audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

        if (audioStreamIndex < 0 && videoStreamIndex < 0) {
            errorMessage = "No playable audio/video stream was found.";
            SetStatusText(errorMessage);
            Close();
            return false;
        }

        if (audioStreamIndex >= 0) {
            audioStream = formatContext->streams[audioStreamIndex];
            error = OpenCodecContext(audioStream, audioCodecContext);
            if (error < 0) {
                errorMessage = "Failed to open audio decoder: " + AvErrorToString(error);
                SetStatusText(errorMessage);
                Close();
                return false;
            }
        }

        if (videoStreamIndex >= 0) {
            videoStream = formatContext->streams[videoStreamIndex];
            error = OpenCodecContext(videoStream, videoCodecContext);
            if (error < 0) {
                errorMessage = "Failed to open video decoder: " + AvErrorToString(error);
                SetStatusText(errorMessage);
                Close();
                return false;
            }
        }

        if (audioStream != nullptr) {
            if (!OpenAudioDevice(errorMessage)) {
                SetStatusText(errorMessage);
                Close();
                return false;
            }
        }

        PopulateMediaInfo();
        mediaInfo.path = path;

        flushedAtEof = false;
        reachedEof.store(false);
        nextAudioPtsSeconds = 0.0;
        hasDisplayedFrame = false;
        playbackSpeed.store(1.0);
        paused.store(false);
        stopRequested.store(false);
        seekRequested.store(false);
        lastKnownClockSeconds.store(0.0);
        noAudioAnchorPositionSeconds = 0.0;
        noAudioAnchorWallClock = std::chrono::steady_clock::now();
        hasMedia.store(true);
        SetStatusText("Playing");

        workerThread = std::thread([this]() { DecodeLoop(); });

        if (audioDevice != 0) {
            SDL_PauseAudioDevice(audioDevice, 0);
        }

        return true;
    }

    void Close() {
        hasMedia.store(false);
        stopRequested.store(true);
        seekRequested.store(false);

        if (workerThread.joinable()) {
            workerThread.join();
        }

        if (audioDevice != 0) {
            SDL_PauseAudioDevice(audioDevice, 1);
        }

        {
            std::lock_guard<std::mutex> audioLock(audioMutex);
            audioQueue.clear();
        }
        {
            std::lock_guard<std::mutex> videoLock(videoMutex);
            videoQueue.clear();
            displayedFrame = {};
            hasDisplayedFrame = false;
        }

        if (audioDevice != 0) {
            SDL_CloseAudioDevice(audioDevice);
            audioDevice = 0;
        }

        if (videoTexture != nullptr) {
            SDL_DestroyTexture(videoTexture);
            videoTexture = nullptr;
            textureRenderer = nullptr;
            textureWidth = 0;
            textureHeight = 0;
        }

        if (swrContext != nullptr) {
            swr_free(&swrContext);
        }

        if (hasPreparedInputChannelLayout) {
            av_channel_layout_uninit(&preparedInputChannelLayout);
            hasPreparedInputChannelLayout = false;
        }

        if (swsContext != nullptr) {
            sws_freeContext(swsContext);
            swsContext = nullptr;
        }

        if (audioCodecContext != nullptr) {
            avcodec_free_context(&audioCodecContext);
        }

        if (videoCodecContext != nullptr) {
            avcodec_free_context(&videoCodecContext);
        }

        if (formatContext != nullptr) {
            avformat_close_input(&formatContext);
        }

        mediaInfo = {};
        SetStatusText("Open a media file with O.");
        audioStream = nullptr;
        videoStream = nullptr;
        audioStreamIndex = -1;
        videoStreamIndex = -1;
        preparedResampleSpeed = 1.0;
        preparedResampleRate = kAudioDeviceSampleRate;
        preparedInputSampleRate = 0;
        preparedInputSampleFormat = AV_SAMPLE_FMT_NONE;
        nextAudioPtsSeconds = 0.0;
        stopRequested.store(false);
        reachedEof.store(false);
        preparedVideoWidth = 0;
        preparedVideoHeight = 0;
        preparedVideoFormat = AV_PIX_FMT_NONE;
    }

    void Update() {
        if (!hasMedia.load()) {
            return;
        }

        if (videoStream == nullptr) {
            return;
        }

        const double clockSeconds = GetPositionSeconds();

        std::lock_guard<std::mutex> lock(videoMutex);

        if (!paused.load()) {
            while (videoQueue.size() > 1 && videoQueue[1].ptsSeconds <= clockSeconds + 0.01) {
                videoQueue.pop_front();
            }
        }

        if (!videoQueue.empty()) {
            const VideoFrame& candidate = videoQueue.front();
            if (paused.load() || !mediaInfo.hasAudio || candidate.ptsSeconds <= clockSeconds + kVideoDisplayLeewaySeconds) {
                if (!hasDisplayedFrame || displayedFrame.ptsSeconds != candidate.ptsSeconds) {
                    displayedFrame = candidate;
                    hasDisplayedFrame = true;
                }
            }

            if (reachedEof.load() && !paused.load() && videoQueue.size() == 1 &&
                candidate.ptsSeconds <= clockSeconds + GetVideoFrameHoldSeconds()) {
                videoQueue.pop_front();
            }
        }
    }

    bool RenderVideo(SDL_Renderer* renderer, const SDL_Rect& targetRect) {
        std::lock_guard<std::mutex> lock(videoMutex);

        if (!hasDisplayedFrame || renderer == nullptr) {
            return false;
        }

        if (videoTexture == nullptr || textureRenderer != renderer || textureWidth != displayedFrame.width ||
            textureHeight != displayedFrame.height) {
            if (videoTexture != nullptr) {
                SDL_DestroyTexture(videoTexture);
            }

            videoTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING,
                                             displayedFrame.width, displayedFrame.height);

            if (videoTexture == nullptr) {
                SetStatusText("Failed to create SDL texture.");
                return false;
            }

            textureRenderer = renderer;
            textureWidth = displayedFrame.width;
            textureHeight = displayedFrame.height;
        }

        if (SDL_UpdateTexture(videoTexture, nullptr, displayedFrame.pixels.data(), displayedFrame.pitch) != 0) {
            SetStatusText("Failed to upload video frame to texture.");
            return false;
        }

        const float srcAspect = static_cast<float>(displayedFrame.width) / static_cast<float>(displayedFrame.height);
        const float dstAspect = static_cast<float>(targetRect.w) / static_cast<float>(targetRect.h);

        SDL_Rect destination = targetRect;
        if (srcAspect > dstAspect) {
            destination.h = static_cast<int>(static_cast<float>(targetRect.w) / srcAspect);
            destination.y = targetRect.y + (targetRect.h - destination.h) / 2;
        } else {
            destination.w = static_cast<int>(static_cast<float>(targetRect.h) * srcAspect);
            destination.x = targetRect.x + (targetRect.w - destination.w) / 2;
        }

        SDL_RenderCopy(renderer, videoTexture, nullptr, &destination);
        return true;
    }

    void TogglePause() { SetPaused(!paused.load()); }

    void SetPaused(const bool shouldPause) {
        if (!hasMedia.load()) {
            return;
        }

        const bool current = paused.load();
        if (current == shouldPause) {
            return;
        }

        if (audioStream == nullptr) {
            SetNoAudioClockAnchor(GetPositionSeconds(), std::chrono::steady_clock::now());
        }

        paused.store(shouldPause);
        SetStatusText(shouldPause ? "Paused" : "Playing");
    }

    void RequestSeek(const double seconds) {
        if (!hasMedia.load()) {
            return;
        }

        const double duration = GetDurationSeconds();
        const double clamped = duration > 0.0 ? std::clamp(seconds, 0.0, std::max(0.0, duration - kSeekClampMarginSeconds))
                                              : std::max(0.0, seconds);

        requestedSeekSeconds.store(clamped);
        seekRequested.store(true);
    }

    void StepRelativeSeek(const double deltaSeconds) { RequestSeek(GetPositionSeconds() + deltaSeconds); }

    void SetSpeed(const double speed) {
        const auto nearest = std::min_element(
            kSupportedSpeeds.begin(), kSupportedSpeeds.end(),
            [speed](const double left, const double right) { return std::abs(left - speed) < std::abs(right - speed); });

        const double newSpeed = *nearest;
        const double currentPosition = GetPositionSeconds();

        playbackSpeed.store(newSpeed);

        if (audioStream == nullptr) {
            SetNoAudioClockAnchor(currentPosition, std::chrono::steady_clock::now());
        } else if (hasMedia.load()) {
            RequestSeek(currentPosition);
        }
    }

    double GetSpeed() const { return playbackSpeed.load(); }

    bool HasMedia() const { return hasMedia.load(); }

    bool IsPaused() const { return paused.load(); }

    double GetPositionSeconds() const {
        if (!hasMedia.load()) {
            return 0.0;
        }

        if (audioStream != nullptr) {
            return ClampPosition(lastKnownClockSeconds.load());
        }

        double anchor = 0.0;
        std::chrono::steady_clock::time_point wallClock;
        {
            std::lock_guard<std::mutex> lock(clockMutex);
            anchor = noAudioAnchorPositionSeconds;
            wallClock = noAudioAnchorWallClock;
        }

        if (paused.load()) {
            return ClampPosition(anchor);
        }

        const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - wallClock).count();
        return ClampPosition(anchor + elapsed * playbackSpeed.load());
    }

    double GetDurationSeconds() const { return mediaInfo.durationSeconds; }

    MediaInfo GetMediaInfo() const { return mediaInfo; }

    int OpenCodecContext(AVStream* stream, AVCodecContext*& codecContext) {
        const AVCodecParameters* params = stream->codecpar;
        const AVCodec* codec = avcodec_find_decoder(params->codec_id);
        if (codec == nullptr) {
            return AVERROR_DECODER_NOT_FOUND;
        }

        codecContext = avcodec_alloc_context3(codec);
        if (codecContext == nullptr) {
            return AVERROR(ENOMEM);
        }

        int error = avcodec_parameters_to_context(codecContext, params);
        if (error < 0) {
            return error;
        }

        return avcodec_open2(codecContext, codec, nullptr);
    }

    bool OpenAudioDevice(std::string& errorMessage) {
        SDL_AudioSpec desired = {};
        desired.freq = kAudioDeviceSampleRate;
        desired.channels = static_cast<Uint8>(kAudioDeviceChannels);
        desired.format = AUDIO_S16SYS;
        desired.samples = 2048;
        desired.callback = &Impl::AudioCallback;
        desired.userdata = this;

        SDL_AudioSpec obtained = {};
        audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
        if (audioDevice == 0) {
            errorMessage = "Failed to open SDL audio device: " + std::string(SDL_GetError());
            return false;
        }

        return true;
    }

    void PopulateMediaInfo() {
        mediaInfo.hasAudio = audioStream != nullptr;
        mediaInfo.hasVideo = videoStream != nullptr;
        mediaInfo.formatName = formatContext != nullptr ? SafeCString(formatContext->iformat->long_name) : "";
        mediaInfo.durationSeconds = formatContext != nullptr && formatContext->duration > 0
                                        ? static_cast<double>(formatContext->duration) / static_cast<double>(AV_TIME_BASE)
                                        : 0.0;

        if (videoStream != nullptr) {
            mediaInfo.width = videoCodecContext->width;
            mediaInfo.height = videoCodecContext->height;
            mediaInfo.videoCodec = SafeCString(avcodec_get_name(videoCodecContext->codec_id));
            const AVRational frameRate = av_guess_frame_rate(formatContext, videoStream, nullptr);
            mediaInfo.frameRate = frameRate.num > 0 && frameRate.den > 0 ? av_q2d(frameRate) : 0.0;
        }

        if (audioStream != nullptr) {
            mediaInfo.sampleRate = audioCodecContext->sample_rate;
            mediaInfo.channels = audioCodecContext->ch_layout.nb_channels;
            mediaInfo.audioCodec = SafeCString(avcodec_get_name(audioCodecContext->codec_id));
        }
    }

    void DecodeLoop() {
        AVPacket* packet = av_packet_alloc();
        if (packet == nullptr) {
            SetStatusText("Failed to allocate packet.");
            return;
        }

        while (!stopRequested.load()) {
            if (seekRequested.exchange(false)) {
                PerformSeek(requestedSeekSeconds.load());
            }

            if (ShouldThrottle()) {
                SDL_Delay(8);
                continue;
            }

            const int readResult = av_read_frame(formatContext, packet);
            if (readResult == AVERROR_EOF) {
                if (!flushedAtEof) {
                    DecodeFrames(audioCodecContext, nullptr, AVMEDIA_TYPE_AUDIO);
                    DecodeFrames(videoCodecContext, nullptr, AVMEDIA_TYPE_VIDEO);
                    flushedAtEof = true;
                    reachedEof.store(true);
                }

                if (QueuesDrained()) {
                    SetPaused(true);
                    SetStatusText("Playback finished");
                }

                SDL_Delay(15);
                continue;
            }

            if (readResult < 0) {
                SetStatusText("Read packet failed: " + AvErrorToString(readResult));
                SDL_Delay(20);
                continue;
            }

            flushedAtEof = false;
            reachedEof.store(false);

            if (packet->stream_index == audioStreamIndex) {
                DecodeFrames(audioCodecContext, packet, AVMEDIA_TYPE_AUDIO);
            } else if (packet->stream_index == videoStreamIndex) {
                DecodeFrames(videoCodecContext, packet, AVMEDIA_TYPE_VIDEO);
            }

            av_packet_unref(packet);
        }

        av_packet_free(&packet);
    }

    bool ShouldThrottle() {
        const std::size_t queuedAudioBytes = GetQueuedAudioBytes();
        const std::size_t maxQueuedAudioBytes =
            static_cast<std::size_t>(kAudioDeviceSampleRate) * kAudioDeviceChannels * kAudioBytesPerSample *
            kMaxQueuedAudioMilliseconds / 1000;

        std::lock_guard<std::mutex> videoLock(videoMutex);
        return queuedAudioBytes >= maxQueuedAudioBytes || videoQueue.size() >= kMaxQueuedVideoFrames;
    }

    std::size_t GetQueuedAudioBytes() {
        std::lock_guard<std::mutex> lock(audioMutex);

        std::size_t total = 0;
        for (const AudioChunk& chunk : audioQueue) {
            total += chunk.bytes.size() - chunk.offset;
        }

        return total;
    }

    bool QueuesDrained() {
        std::lock_guard<std::mutex> audioLock(audioMutex);
        std::lock_guard<std::mutex> videoLock(videoMutex);
        return audioQueue.empty() && videoQueue.empty();
    }

    void PerformSeek(const double targetSeconds) {
        if (formatContext == nullptr) {
            return;
        }

        const int error = av_seek_frame(formatContext, -1, SecondsToGlobalTimestamp(targetSeconds), AVSEEK_FLAG_BACKWARD);
        if (error < 0) {
            SetStatusText("Seek failed: " + AvErrorToString(error));
            return;
        }

        if (audioCodecContext != nullptr) {
            avcodec_flush_buffers(audioCodecContext);
        }

        if (videoCodecContext != nullptr) {
            avcodec_flush_buffers(videoCodecContext);
        }

        {
            std::lock_guard<std::mutex> audioLock(audioMutex);
            audioQueue.clear();
            lastKnownClockSeconds.store(targetSeconds);
            nextAudioPtsSeconds = targetSeconds;
        }

        {
            std::lock_guard<std::mutex> videoLock(videoMutex);
            videoQueue.clear();
            displayedFrame = {};
            hasDisplayedFrame = false;
        }

        if (audioStream == nullptr) {
            SetNoAudioClockAnchor(targetSeconds, std::chrono::steady_clock::now());
        }

        if (swrContext != nullptr) {
            swr_free(&swrContext);
        }

        preparedResampleSpeed = playbackSpeed.load();
        flushedAtEof = false;
        reachedEof.store(false);
        SetStatusText("Seeked");
    }

    void DecodeFrames(AVCodecContext* codecContext, AVPacket* packet, const AVMediaType type) {
        if (codecContext == nullptr) {
            return;
        }

        // Passing nullptr as the packet is FFmpeg's convention for flushing
        // remaining frames out of the decoder after EOF or seek.
        const bool flushing = (packet == nullptr);
        int error = avcodec_send_packet(codecContext, packet);
        if (error < 0 && error != AVERROR(EAGAIN) && !(flushing && error == AVERROR_EOF)) {
            SetStatusText(flushing ? "Decoder flush failed: " : "Decoder send packet failed: ");
            return;
        }

        AVFrame* frame = av_frame_alloc();
        if (frame == nullptr) {
            if (!flushing) {
                SetStatusText("Failed to allocate frame.");
            }
            return;
        }

        while (!stopRequested.load()) {
            error = avcodec_receive_frame(codecContext, frame);
            if (error == AVERROR(EAGAIN) || error == AVERROR_EOF) {
                break;
            }

            if (error < 0) {
                SetStatusText("Decoder receive frame failed: " + AvErrorToString(error));
                break;
            }

            if (type == AVMEDIA_TYPE_AUDIO) {
                ProcessAudioFrame(frame);
            } else if (type == AVMEDIA_TYPE_VIDEO) {
                ProcessVideoFrame(frame);
            }

            av_frame_unref(frame);
        }

        av_frame_free(&frame);
    }

    void ProcessAudioFrame(AVFrame* frame) {
        if (audioStream == nullptr || frame == nullptr) {
            return;
        }

        const double speed = playbackSpeed.load();
        const int outputRate =
            std::clamp(static_cast<int>(std::lround(static_cast<double>(kAudioDeviceSampleRate) / speed)), 8000, 192000);

        const bool inputLayoutChanged =
            !hasPreparedInputChannelLayout || av_channel_layout_compare(&preparedInputChannelLayout, &frame->ch_layout) != 0;
        if (swrContext == nullptr || std::abs(preparedResampleSpeed - speed) > 0.001 || preparedResampleRate != outputRate ||
            preparedInputSampleRate != frame->sample_rate ||
            preparedInputSampleFormat != static_cast<AVSampleFormat>(frame->format) || inputLayoutChanged) {
            RecreateSwrContext(frame, outputRate, speed);
        }

        if (swrContext == nullptr) {
            return;
        }

        const int64_t delay = swr_get_delay(swrContext, frame->sample_rate);
        const int maxOutputSamples =
            av_rescale_rnd(delay + frame->nb_samples, outputRate, frame->sample_rate, AV_ROUND_UP);
        const int outputBufferBytes = maxOutputSamples * kAudioDeviceChannels * kAudioBytesPerSample;

        AudioChunk chunk;
        chunk.bytes.resize(static_cast<std::size_t>(outputBufferBytes));

        std::uint8_t* outputData[1] = {chunk.bytes.data()};
        const int convertedSamples = swr_convert(swrContext, outputData, maxOutputSamples,
                                                 const_cast<const std::uint8_t**>(frame->extended_data), frame->nb_samples);

        if (convertedSamples < 0) {
            SetStatusText("Audio resample failed: " + AvErrorToString(convertedSamples));
            return;
        }

        chunk.bytes.resize(static_cast<std::size_t>(convertedSamples) * kAudioDeviceChannels * kAudioBytesPerSample);

        const double ptsSeconds = std::isnan(TimestampToSeconds(frame->best_effort_timestamp, audioStream->time_base))
                                      ? nextAudioPtsSeconds
                                      : TimestampToSeconds(frame->best_effort_timestamp, audioStream->time_base);

        chunk.mediaPtsSeconds = ptsSeconds;
        chunk.mediaDurationSeconds = frame->sample_rate > 0 ? static_cast<double>(frame->nb_samples) / frame->sample_rate : 0.0;
        nextAudioPtsSeconds = ptsSeconds + chunk.mediaDurationSeconds;

        std::lock_guard<std::mutex> lock(audioMutex);
        audioQueue.push_back(std::move(chunk));
    }

    void RecreateSwrContext(AVFrame* frame, const int outputRate, const double speed) {
        if (swrContext != nullptr) {
            swr_free(&swrContext);
        }

        if (hasPreparedInputChannelLayout) {
            av_channel_layout_uninit(&preparedInputChannelLayout);
            hasPreparedInputChannelLayout = false;
        }

        AVChannelLayout outputLayout = {};
        av_channel_layout_default(&outputLayout, kAudioDeviceChannels);

        SwrContext* newContext = nullptr;
        const int error = swr_alloc_set_opts2(
            &newContext, &outputLayout, AV_SAMPLE_FMT_S16, outputRate, &frame->ch_layout,
            static_cast<AVSampleFormat>(frame->format), frame->sample_rate, 0, nullptr);

        av_channel_layout_uninit(&outputLayout);

        if (error < 0 || newContext == nullptr) {
            SetStatusText("Failed to configure audio resampler.");
            swr_free(&newContext);
            return;
        }

        if (swr_init(newContext) < 0) {
            SetStatusText("Failed to initialize audio resampler.");
            swr_free(&newContext);
            return;
        }

        swrContext = newContext;
        preparedResampleRate = outputRate;
        preparedResampleSpeed = speed;
        preparedInputSampleRate = frame->sample_rate;
        preparedInputSampleFormat = static_cast<AVSampleFormat>(frame->format);
        if (av_channel_layout_copy(&preparedInputChannelLayout, &frame->ch_layout) == 0) {
            hasPreparedInputChannelLayout = true;
        }
    }

    void ProcessVideoFrame(AVFrame* frame) {
        if (videoStream == nullptr || frame == nullptr) {
            return;
        }

        if (swsContext == nullptr || preparedVideoWidth != frame->width || preparedVideoHeight != frame->height ||
            preparedVideoFormat != static_cast<AVPixelFormat>(frame->format)) {
            if (swsContext != nullptr) {
                sws_freeContext(swsContext);
                swsContext = nullptr;
            }

            swsContext = sws_getContext(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format), frame->width,
                                        frame->height, AV_PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, nullptr);
        }

        if (swsContext == nullptr) {
            SetStatusText("Failed to create video scaler.");
            return;
        }

        preparedVideoWidth = frame->width;
        preparedVideoHeight = frame->height;
        preparedVideoFormat = static_cast<AVPixelFormat>(frame->format);

        VideoFrame videoFrame;
        videoFrame.width = frame->width;
        videoFrame.height = frame->height;
        videoFrame.pitch = frame->width * 4;
        videoFrame.pixels.resize(static_cast<std::size_t>(videoFrame.pitch) * frame->height);

        std::uint8_t* destinationData[4] = {videoFrame.pixels.data(), nullptr, nullptr, nullptr};
        int destinationLinesize[4] = {videoFrame.pitch, 0, 0, 0};

        sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, destinationData, destinationLinesize);

        const double ptsSeconds = std::isnan(TimestampToSeconds(frame->best_effort_timestamp, videoStream->time_base))
                                      ? GetPositionSeconds()
                                      : TimestampToSeconds(frame->best_effort_timestamp, videoStream->time_base);
        videoFrame.ptsSeconds = ptsSeconds;

        std::lock_guard<std::mutex> lock(videoMutex);
        videoQueue.push_back(std::move(videoFrame));
    }

    static void AudioCallback(void* userdata, Uint8* stream, int length) {
        auto* self = static_cast<Impl*>(userdata);
        self->FillAudioBuffer(stream, length);
    }

    void FillAudioBuffer(Uint8* stream, const int length) {
        SDL_memset(stream, 0, length);

        if (paused.load() || audioStream == nullptr) {
            return;
        }

        int remaining = length;
        while (remaining > 0) {
            std::lock_guard<std::mutex> lock(audioMutex);
            if (audioQueue.empty()) {
                break;
            }

            AudioChunk& chunk = audioQueue.front();
            const std::size_t available = chunk.bytes.size() - chunk.offset;
            const int copyBytes = static_cast<int>(std::min<std::size_t>(available, static_cast<std::size_t>(remaining)));

            std::memcpy(stream + (length - remaining), chunk.bytes.data() + chunk.offset, static_cast<std::size_t>(copyBytes));
            chunk.offset += static_cast<std::size_t>(copyBytes);
            remaining -= copyBytes;

            const double ratio = chunk.bytes.empty()
                                     ? 1.0
                                     : static_cast<double>(chunk.offset) / static_cast<double>(chunk.bytes.size());
            lastKnownClockSeconds.store(chunk.mediaPtsSeconds + chunk.mediaDurationSeconds * ratio);

            if (chunk.offset >= chunk.bytes.size()) {
                audioQueue.pop_front();
            }
        }
    }

    double ClampPosition(const double value) const {
        if (mediaInfo.durationSeconds <= 0.0) {
            return std::max(0.0, value);
        }

        return std::clamp(value, 0.0, mediaInfo.durationSeconds);
    }

    double GetVideoFrameHoldSeconds() const {
        if (mediaInfo.frameRate > 0.0) {
            return std::max(1.0 / mediaInfo.frameRate, kVideoDisplayLeewaySeconds);
        }

        return 0.1;
    }

    void SetStatusText(std::string text) {
        std::lock_guard<std::mutex> lock(stateMutex);
        statusText = std::move(text);
    }

    std::string GetStatusTextCopy() const {
        std::lock_guard<std::mutex> lock(stateMutex);
        return statusText;
    }

    void SetNoAudioClockAnchor(const double positionSeconds, const std::chrono::steady_clock::time_point wallClock) {
        std::lock_guard<std::mutex> lock(clockMutex);
        noAudioAnchorPositionSeconds = positionSeconds;
        noAudioAnchorWallClock = wallClock;
    }
};

Player::Player() : impl_(std::make_unique<Impl>()) {}

Player::~Player() = default;

bool Player::Open(const std::string& path, std::string& errorMessage) { return impl_->Open(path, errorMessage); }

void Player::Close() { impl_->Close(); }

void Player::Update() { impl_->Update(); }

bool Player::RenderVideo(SDL_Renderer* renderer, const SDL_Rect& targetRect) {
    return impl_->RenderVideo(renderer, targetRect);
}

void Player::TogglePause() { impl_->TogglePause(); }

void Player::SetPaused(const bool paused) { impl_->SetPaused(paused); }

bool Player::IsPaused() const { return impl_->IsPaused(); }

void Player::RequestSeek(const double seconds) { impl_->RequestSeek(seconds); }

void Player::StepRelativeSeek(const double deltaSeconds) { impl_->StepRelativeSeek(deltaSeconds); }

void Player::CycleSpeed() {
    const double current = GetSpeed();
    const auto iterator = std::find(kSupportedSpeeds.begin(), kSupportedSpeeds.end(), current);
    if (iterator == kSupportedSpeeds.end() || std::next(iterator) == kSupportedSpeeds.end()) {
        impl_->SetSpeed(kSupportedSpeeds.front());
        return;
    }

    impl_->SetSpeed(*std::next(iterator));
}

void Player::IncreaseSpeed() {
    const double current = GetSpeed();
    for (const double speed : kSupportedSpeeds) {
        if (speed > current + 0.001) {
            impl_->SetSpeed(speed);
            return;
        }
    }
}

void Player::DecreaseSpeed() {
    const double current = GetSpeed();
    for (auto iterator = kSupportedSpeeds.rbegin(); iterator != kSupportedSpeeds.rend(); ++iterator) {
        if (*iterator < current - 0.001) {
            impl_->SetSpeed(*iterator);
            return;
        }
    }
}

void Player::SetSpeed(const double speed) { impl_->SetSpeed(speed); }

double Player::GetSpeed() const { return impl_->GetSpeed(); }

bool Player::HasMedia() const { return impl_->HasMedia(); }

double Player::GetPositionSeconds() const { return impl_->GetPositionSeconds(); }

double Player::GetDurationSeconds() const { return impl_->GetDurationSeconds(); }

MediaInfo Player::GetMediaInfo() const { return impl_->GetMediaInfo(); }

std::string Player::GetStatusText() const { return impl_->GetStatusTextCopy(); }

}  // namespace player
