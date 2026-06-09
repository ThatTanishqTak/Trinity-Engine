#pragma once

#include <memory>

#include <Trinity/Audio/AudioTypes.h>

namespace Trinity
{
    class IAudioBackend;

    class AudioDevice
    {
    public:
        AudioDevice();
        ~AudioDevice();

        AudioDevice(const AudioDevice&) = delete;
        AudioDevice& operator=(const AudioDevice&) = delete;

        bool Initialize(AudioBackend backend = AudioBackend::MiniAudio);
        void Shutdown();

        bool IsValid() const { return m_Backend != nullptr; }
        IAudioBackend& GetBackend() const { return *m_Backend; }

        void SetMasterVolume(float volume);
        float GetMasterVolume() const { return m_MasterVolume; }

    private:
        std::unique_ptr<IAudioBackend> m_Backend;
        float m_MasterVolume = 1.0f;
    };
}