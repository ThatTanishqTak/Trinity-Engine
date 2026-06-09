#pragma once

#include <filesystem>
#include <memory>

#include <Trinity/Audio/Backends/IAudioBackend.h>

namespace Trinity
{
    class MiniAudioBackend : public IAudioBackend
    {
    public:
        MiniAudioBackend();
        ~MiniAudioBackend() override;

        MiniAudioBackend(const MiniAudioBackend&) = delete;
        MiniAudioBackend& operator=(const MiniAudioBackend&) = delete;

        bool Initialize() override;
        void Shutdown() override;

        AudioClipHandle LoadClip(const std::filesystem::path& path) override;
        void UnloadClip(AudioClipHandle clip) override;

        VoiceHandle Play(AudioClipHandle clip, const VoiceParameters& parameters) override;
        void Stop(VoiceHandle voice) override;
        void SetVoiceVolume(VoiceHandle voice, float volume) override;
        void SetVoicePitch(VoiceHandle voice, float pitch) override;
        void SetVoicePosition(VoiceHandle voice, const glm::vec3& position) override;
        bool IsVoiceActive(VoiceHandle voice) const override;

        void SetListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) override;
        void SetMasterVolume(float volume) override;

        void Update() override;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}