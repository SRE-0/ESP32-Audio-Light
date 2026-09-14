#include "als/audio/BackendFactory.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "als/core/Constants.h"
#include "als/audio/SampleConverter.h"

#include <audioclient.h>
#include <combaseapi.h>
#include <mmdeviceapi.h>
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace als {
namespace {

class WasapiBackend final : public IAudioBackend {
public:
    ~WasapiBackend() override {
        shutdown();
    }

    bool initialize() override {
        HRESULT result = CoInitializeEx(
            nullptr, COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);
        if (FAILED(result) && result != RPC_E_CHANGED_MODE) {
            logError("Failed to initialize COM", result);
            return false;
        }
        comInitialized_ = result != RPC_E_CHANGED_MODE;

        result = CoCreateInstance(
            __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            reinterpret_cast<void**>(&deviceEnumerator_));
        if (FAILED(result)) {
            logError("Failed to create device enumerator", result);
            return false;
        }

        result = deviceEnumerator_->GetDefaultAudioEndpoint(
            eRender, eConsole, &device_);
        if (FAILED(result)) {
            logError("Failed to get default audio endpoint", result);
            return false;
        }

        result = device_->Activate(
            __uuidof(IAudioClient), CLSCTX_ALL, nullptr,
            reinterpret_cast<void**>(&audioClient_));
        if (FAILED(result)) {
            logError("Failed to activate audio client", result);
            return false;
        }

        WAVEFORMATEX* mixFormat = nullptr;
        result = audioClient_->GetMixFormat(&mixFormat);
        if (FAILED(result)) {
            logError("Failed to get mix format", result);
            return false;
        }

        sampleRate_ = mixFormat->nSamplesPerSec;
        channels_ = mixFormat->nChannels;
        sampleFormat_ = detectSampleFormat(mixFormat);

        constexpr REFERENCE_TIME bufferDuration = 20 * 10000;
        result = audioClient_->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            bufferDuration, 0, mixFormat, nullptr);
        CoTaskMemFree(mixFormat);
        if (FAILED(result)) {
            logError("Failed to initialize audio client", result);
            return false;
        }

        audioEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!audioEvent_) {
            std::cerr << "[WASAPI] Failed to create audio event\n";
            return false;
        }
        result = audioClient_->SetEventHandle(audioEvent_);
        if (FAILED(result)) {
            logError("Failed to set event handle", result);
            return false;
        }

        result = audioClient_->GetService(
            __uuidof(IAudioCaptureClient),
            reinterpret_cast<void**>(&captureClient_));
        if (FAILED(result)) {
            logError("Failed to get capture client", result);
            return false;
        }

        UINT32 bufferFrames = 0;
        result = audioClient_->GetBufferSize(&bufferFrames);
        if (FAILED(result)) {
            logError("Failed to get buffer size", result);
            return false;
        }
        conversionBuffer_.resize(bufferFrames);
        return true;
    }

    void startCapture(AudioCallback callback) override {
        if (!audioClient_ || !captureClient_) {
            return;
        }
        callback_ = std::move(callback);
        running_ = true;

        const HRESULT result = audioClient_->Start();
        if (FAILED(result)) {
            logError("Failed to start audio client", result);
            running_ = false;
            return;
        }

        while (running_) {
            const DWORD waitResult = WaitForSingleObject(audioEvent_, 2000);
            if (waitResult == WAIT_TIMEOUT) {
                continue;
            }
            if (waitResult != WAIT_OBJECT_0) {
                break;
            }
            processPackets();
        }
        audioClient_->Stop();
    }

    void stop() override {
        running_ = false;
        if (audioEvent_) {
            SetEvent(audioEvent_);
        }
    }

    void shutdown() override {
        stop();
        release(captureClient_);
        release(audioClient_);
        release(device_);
        release(deviceEnumerator_);
        if (audioEvent_) {
            CloseHandle(audioEvent_);
            audioEvent_ = nullptr;
        }
        if (comInitialized_) {
            CoUninitialize();
            comInitialized_ = false;
        }
    }

    std::uint32_t actualSampleRate() const override {
        return sampleRate_;
    }

    std::uint32_t actualChannels() const override {
        return channels_;
    }

private:
    template <typename Interface>
    static void release(Interface*& value) {
        if (value) {
            value->Release();
            value = nullptr;
        }
    }

    static void logError(const char* message, HRESULT result) {
        std::cerr << "[WASAPI] " << message << ": 0x"
                  << std::hex << result << std::dec << '\n';
    }

    static AudioSampleFormat detectSampleFormat(const WAVEFORMATEX* format) {
        bool isFloat = format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT;
        if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
            format->cbSize >=
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
            const auto* extensible =
                reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(format);
            static constexpr GUID ieeeFloatSubtype = {
                0x00000003, 0x0000, 0x0010,
                {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}
            };
            isFloat = IsEqualGUID(extensible->SubFormat, ieeeFloatSubtype);
        }
        if (isFloat && format->wBitsPerSample == 32) {
            return AudioSampleFormat::Float32;
        }
        if (format->wBitsPerSample == 16) {
            return AudioSampleFormat::Int16;
        }
        if (format->wBitsPerSample == 24) {
            return AudioSampleFormat::Int24;
        }
        if (format->wBitsPerSample == 32) {
            return AudioSampleFormat::Int32;
        }
        return AudioSampleFormat::Unsupported;
    }

    void processPackets() {
        UINT32 packetLength = 0;
        HRESULT result = captureClient_->GetNextPacketSize(&packetLength);
        if (FAILED(result)) {
            logError("Failed to get packet size", result);
            return;
        }

        while (packetLength != 0 && running_) {
            BYTE* data = nullptr;
            UINT32 frameCount = 0;
            DWORD flags = 0;
            result = captureClient_->GetBuffer(
                &data, &frameCount, &flags, nullptr, nullptr);
            if (FAILED(result)) {
                logError("Failed to get capture buffer", result);
                return;
            }

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                std::fill_n(conversionBuffer_.begin(), frameCount, 0.0F);
            } else {
                SampleConverter::toMono(
                    data, frameCount, channels_, sampleFormat_, conversionBuffer_);
            }
            callback_(conversionBuffer_.data(), frameCount);
            captureClient_->ReleaseBuffer(frameCount);

            result = captureClient_->GetNextPacketSize(&packetLength);
            if (FAILED(result)) {
                logError("Failed to get next packet size", result);
                return;
            }
        }
    }

    IMMDeviceEnumerator* deviceEnumerator_ = nullptr;
    IMMDevice* device_ = nullptr;
    IAudioClient* audioClient_ = nullptr;
    IAudioCaptureClient* captureClient_ = nullptr;
    HANDLE audioEvent_ = nullptr;
    bool comInitialized_ = false;
    std::atomic<bool> running_{false};
    std::uint32_t sampleRate_ = kSampleRate;
    std::uint32_t channels_ = 2;
    AudioSampleFormat sampleFormat_ = AudioSampleFormat::Float32;
    AudioCallback callback_;
    std::vector<float> conversionBuffer_;
};

}  // namespace

std::unique_ptr<IAudioBackend> createAudioBackend() {
    return std::make_unique<WasapiBackend>();
}

}  // namespace als

#endif
