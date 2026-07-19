#include <Trinity/Audio/Frontend/AudioEngine.h>

#include <glm/glm.hpp>

#include <Trinity/Audio/Backends/IAudioBackend.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/AudioSourceComponent.h>
#include <Trinity/Scene/Components/AudioListenerComponent.h>
#include <Trinity/Assets/AssetDatabase.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    AudioEngine::~AudioEngine()
    {
        Shutdown();
    }

    bool AudioEngine::Initialize()
    {
        ("INITIALIZING AUDIO ENGINE");

        if (m_Device.IsValid())
        {
            return true;
        }

        if (!m_Device.Initialize())
        {
            ("device initialization failed");

            return false;
        }

        ("AUDIO ENGINE INITIALIZED");

        return true;
    }

    void AudioEngine::Shutdown()
    {
        ("SHUTTING DOWN AUDIO ENGINE");

        m_ClipCache.clear();
        m_Device.Shutdown();

        ("AUDIO ENGINE SHUTDOWN COMPLETE");
    }

    AudioClipHandle AudioEngine::LoadClip(const std::filesystem::path& path)
    {
        if (!m_Device.IsValid())
        {
            return AudioClipHandle::Invalid;
        }

        std::string l_Key = path.string();

        std::unordered_map<std::string, AudioClipHandle>::iterator it_Clip = m_ClipCache.find(l_Key);
        if (it_Clip != m_ClipCache.end())
        {
            return it_Clip->second;
        }

        AudioClipHandle l_Clip = m_Device.GetBackend().LoadClip(path);
        if (l_Clip != AudioClipHandle::Invalid)
        {
            m_ClipCache[l_Key] = l_Clip;
        }

        return l_Clip;
    }

    VoiceHandle AudioEngine::Play(AudioClipHandle clip, const VoiceParameters& parameters)
    {
        if (!m_Device.IsValid() || clip == AudioClipHandle::Invalid)
        {
            return VoiceHandle::Invalid;
        }

        return m_Device.GetBackend().Play(clip, parameters);
    }

    void AudioEngine::Stop(VoiceHandle voice)
    {
        if (m_Device.IsValid())
        {
            m_Device.GetBackend().Stop(voice);
        }
    }

    void AudioEngine::SetVoiceVolume(VoiceHandle voice, float volume)
    {
        if (m_Device.IsValid())
        {
            m_Device.GetBackend().SetVoiceVolume(voice, volume);
        }
    }

    void AudioEngine::SetVoicePitch(VoiceHandle voice, float pitch)
    {
        if (m_Device.IsValid())
        {
            m_Device.GetBackend().SetVoicePitch(voice, pitch);
        }
    }

    void AudioEngine::SetVoicePosition(VoiceHandle voice, const glm::vec3& position)
    {
        if (m_Device.IsValid())
        {
            m_Device.GetBackend().SetVoicePosition(voice, position);
        }
    }

    bool AudioEngine::IsVoiceActive(VoiceHandle voice) const
    {
        return m_Device.IsValid() && m_Device.GetBackend().IsVoiceActive(voice);
    }

    void AudioEngine::SetListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
    {
        if (m_Device.IsValid())
        {
            m_Device.GetBackend().SetListener(position, forward, up);
        }
    }

    VoiceHandle AudioEngine::PlaySource(AudioSourceComponent& source, AssetDatabase& assetDatabase, const glm::vec3& worldPosition)
    {
        StopSource(source);

        AudioClipHandle l_Clip = assetDatabase.ResolveAudioClip(source.Clip);
        if (l_Clip == AudioClipHandle::Invalid)
        {
            return VoiceHandle::Invalid;
        }

        VoiceParameters l_Parameters;
        l_Parameters.Volume = source.Volume;
        l_Parameters.Pitch = source.Pitch;
        l_Parameters.Loop = source.Loop;
        l_Parameters.Spatial = source.Spatial;
        l_Parameters.Position = worldPosition;

        source.Runtime = Play(l_Clip, l_Parameters);

        return source.Runtime;
    }

    void AudioEngine::StopSource(AudioSourceComponent& source)
    {
        if (source.Runtime != VoiceHandle::Invalid)
        {
            Stop(source.Runtime);
            source.Runtime = VoiceHandle::Invalid;
        }
    }

    void AudioEngine::Update()
    {
        if (m_Device.IsValid())
        {
            m_Device.GetBackend().Update();
        }
    }

    void AudioEngine::Update(Scene& scene, AssetDatabase& assetDatabase)
    {
        if (!m_Device.IsValid())
        {
            return;
        }

        (void)assetDatabase;

        entt::registry& l_Registry = scene.GetRegistry();

        auto l_Listeners = l_Registry.view<AudioListenerComponent>();
        for (entt::entity l_Entity : l_Listeners)
        {
            const AudioListenerComponent& l_Listener = l_Listeners.get<AudioListenerComponent>(l_Entity);
            if (!l_Listener.Active)
            {
                continue;
            }

            glm::mat4 l_World = scene.GetWorldMatrix(l_Entity);
            glm::vec3 l_Position = glm::vec3(l_World[3]);
            glm::vec3 l_Forward = -glm::normalize(glm::vec3(l_World[2]));
            glm::vec3 l_Up = glm::normalize(glm::vec3(l_World[1]));
            SetListener(l_Position, l_Forward, l_Up);

            break;
        }

        auto l_Sources = l_Registry.view<AudioSourceComponent>();
        for (entt::entity l_Entity : l_Sources)
        {
            AudioSourceComponent& l_Source = l_Sources.get<AudioSourceComponent>(l_Entity);
            if (l_Source.Runtime == VoiceHandle::Invalid)
            {
                continue;
            }

            if (!IsVoiceActive(l_Source.Runtime))
            {
                l_Source.Runtime = VoiceHandle::Invalid;

                continue;
            }

            SetVoiceVolume(l_Source.Runtime, l_Source.Volume);
            SetVoicePitch(l_Source.Runtime, l_Source.Pitch);

            if (l_Source.Spatial)
            {
                SetVoicePosition(l_Source.Runtime, glm::vec3(scene.GetWorldMatrix(l_Entity)[3]));
            }
        }

        m_Device.GetBackend().Update();
    }

    void AudioEngine::StartScene(Scene& scene, AssetDatabase& assetDatabase)
    {
        if (!m_Device.IsValid())
        {
            return;
        }

        auto l_Sources = scene.GetRegistry().view<AudioSourceComponent>();
        for (entt::entity l_Entity : l_Sources)
        {
            AudioSourceComponent& l_Source = l_Sources.get<AudioSourceComponent>(l_Entity);
            if (l_Source.PlayOnStart)
            {
                PlaySource(l_Source, assetDatabase, glm::vec3(scene.GetWorldMatrix(l_Entity)[3]));
            }
        }
    }

    void AudioEngine::StopScene(Scene& scene)
    {
        auto l_Sources = scene.GetRegistry().view<AudioSourceComponent>();
        for (entt::entity l_Entity : l_Sources)
        {
            StopSource(l_Sources.get<AudioSourceComponent>(l_Entity));
        }
    }
}