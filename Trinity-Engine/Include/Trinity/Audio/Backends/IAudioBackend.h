#pragma once

#include <filesystem>

#include <glm/vec3.hpp>

#include <Trinity/Audio/AudioTypes.h>

namespace Trinity
{
    class IAudioBackend
    {
    public:
        virtual ~IAudioBackend() = default;

        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual AudioClipHandle LoadClip(const std::filesystem::path& path) = 0;
        virtual void UnloadClip(AudioClipHandle clip) = 0;

        virtual VoiceHandle Play(AudioClipHandle clip, const VoiceParameters& parameters) = 0;
        virtual void Stop(VoiceHandle voice) = 0;
        virtual void SetVoiceVolume(VoiceHandle voice, float volume) = 0;
        virtual void SetVoicePitch(VoiceHandle voice, float pitch) = 0;
        virtual void SetVoicePosition(VoiceHandle voice, const glm::vec3& position) = 0;
        virtual bool IsVoiceActive(VoiceHandle voice) const = 0;

        virtual void SetListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) = 0;
        virtual void SetMasterVolume(float volume) = 0;

        virtual void Update() = 0;
    };
}