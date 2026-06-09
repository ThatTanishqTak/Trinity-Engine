#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include <glm/vec3.hpp>

#include <Trinity/Audio/AudioTypes.h>
#include <Trinity/Audio/Frontend/AudioDevice.h>

namespace Trinity
{
    class Scene;
    class AssetDatabase;
    struct AudioSourceComponent;

    class AudioEngine
    {
    public:
        AudioEngine() = default;
        ~AudioEngine();

        AudioEngine(const AudioEngine&) = delete;
        AudioEngine& operator=(const AudioEngine&) = delete;

        bool Initialize();
        void Shutdown();

        AudioClipHandle LoadClip(const std::filesystem::path& path);

        VoiceHandle Play(AudioClipHandle clip, const VoiceParameters& parameters);
        void Stop(VoiceHandle voice);
        void SetVoiceVolume(VoiceHandle voice, float volume);
        void SetVoicePitch(VoiceHandle voice, float pitch);
        void SetVoicePosition(VoiceHandle voice, const glm::vec3& position);
        bool IsVoiceActive(VoiceHandle voice) const;

        void SetListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

        void SetMasterVolume(float volume) { m_Device.SetMasterVolume(volume); }
        float GetMasterVolume() const { return m_Device.GetMasterVolume(); }

        VoiceHandle PlaySource(AudioSourceComponent& source, AssetDatabase& assetDatabase, const glm::vec3& worldPosition);
        void StopSource(AudioSourceComponent& source);

        void Update();
        void Update(Scene& scene, AssetDatabase& assetDatabase);
        void StartScene(Scene& scene, AssetDatabase& assetDatabase);
        void StopScene(Scene& scene);

        bool IsValid() const { return m_Device.IsValid(); }

    private:
        AudioDevice m_Device;
        std::unordered_map<std::string, AudioClipHandle> m_ClipCache;
    };
}