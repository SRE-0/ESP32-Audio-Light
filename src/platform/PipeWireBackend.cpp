#include "als/audio/BackendFactory.h"

#ifndef _WIN32

#include "als/core/Constants.h"

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

#include <cstdint>
#include <memory>
#include <utility>

namespace als {
namespace {

class PipeWireBackend final : public IAudioBackend {
public:
    ~PipeWireBackend() override {
        shutdown();
    }

    bool initialize() override {
        pw_init(nullptr, nullptr);
        initialized_ = true;
        loop_ = pw_main_loop_new(nullptr);
        if (!loop_) {
            return false;
        }
        context_ = pw_context_new(pw_main_loop_get_loop(loop_), nullptr, 0);
        if (!context_) {
            return false;
        }
        core_ = pw_context_connect(context_, nullptr, 0);
        return core_ != nullptr;
    }

    void startCapture(AudioCallback callback) override {
        callback_ = std::move(callback);
        pw_properties* properties = pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Monitor",
            PW_KEY_APP_ID, "audio-light-sync",
            PW_KEY_STREAM_CAPTURE_SINK, "true",
            nullptr);
        stream_ = pw_stream_new(core_, "audio-capture", properties);
        if (!stream_) {
            return;
        }

        static const pw_stream_events events = {
            PW_VERSION_STREAM_EVENTS,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            &PipeWireBackend::onProcess,
        };
        pw_stream_add_listener(stream_, &streamListener_, &events, this);

        spa_audio_info_raw audioInfo{};
        audioInfo.format = SPA_AUDIO_FORMAT_F32;
        audioInfo.rate = kSampleRate;
        audioInfo.channels = kChannels;
        std::uint8_t storage[1024];
        spa_pod_builder builder = SPA_POD_BUILDER_INIT(storage, sizeof(storage));
        const spa_pod* parameters = spa_format_audio_raw_build(
            &builder, SPA_PARAM_EnumFormat, &audioInfo);

        pw_stream_connect(
            stream_, PW_DIRECTION_INPUT, PW_ID_ANY,
            static_cast<pw_stream_flags>(
                PW_STREAM_FLAG_AUTOCONNECT |
                PW_STREAM_FLAG_MAP_BUFFERS |
                PW_STREAM_FLAG_RT_PROCESS),
            &parameters, 1);
        pw_main_loop_run(loop_);
    }

    void stop() override {
        if (loop_) {
            pw_main_loop_quit(loop_);
        }
    }

    void shutdown() override {
        stop();
        if (stream_) {
            pw_stream_destroy(stream_);
            stream_ = nullptr;
        }
        if (core_) {
            pw_core_disconnect(core_);
            core_ = nullptr;
        }
        if (context_) {
            pw_context_destroy(context_);
            context_ = nullptr;
        }
        if (loop_) {
            pw_main_loop_destroy(loop_);
            loop_ = nullptr;
        }
        if (initialized_) {
            pw_deinit();
            initialized_ = false;
        }
    }

    std::uint32_t actualSampleRate() const override {
        return kSampleRate;
    }

    std::uint32_t actualChannels() const override {
        return kChannels;
    }

private:
    static void onProcess(void* userData) {
        static_cast<PipeWireBackend*>(userData)->process();
    }

    void process() {
        pw_buffer* buffer = pw_stream_dequeue_buffer(stream_);
        if (!buffer) {
            return;
        }
        spa_data& source = buffer->buffer->datas[0];
        const std::uint32_t offset = source.chunk ? source.chunk->offset : 0;
        const auto* rawBytes = static_cast<const std::uint8_t*>(source.data);
        const auto* data = rawBytes
            ? reinterpret_cast<const float*>(rawBytes + offset)
            : nullptr;
        const std::uint32_t byteCount =
            source.chunk ? source.chunk->size : source.maxsize;
        if (data && callback_) {
            callback_(data, byteCount / sizeof(float));
        }
        pw_stream_queue_buffer(stream_, buffer);
    }

    pw_main_loop* loop_ = nullptr;
    pw_context* context_ = nullptr;
    pw_core* core_ = nullptr;
    pw_stream* stream_ = nullptr;
    spa_hook streamListener_{};
    bool initialized_ = false;
    AudioCallback callback_;
};

}  // namespace

std::unique_ptr<IAudioBackend> createAudioBackend() {
    return std::make_unique<PipeWireBackend>();
}

}  // namespace als

#endif
