#include <Trinity/Audio/Backends/MiniAudio/MiniAudioBackend.h>

#include <string>
#include <unordered_map>

#include <Trinity/Core/Log.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace Trinity
{
    struct MiniAudioBackend::Implementation
    {
        ma_engine Engine{};
        bool Initialized = false;
        uint64_t NextClip = 1;
        uint64_t NextVoice = 1;
        std::unordered_map<uint64_t, std::string> Clips;
        std::unordered_map<uint64_t, ma_sound*> Voices;

        ma_sound* FindVoice(VoiceHandle voice) const
        {
            std::unordered_map<uint64_t, ma_sound*>::const_iterator it_Voice = Voices.find(static_cast<uint64_t>(voice));

            return it_Voice != Voices.end() ? it_Voice->second : nullptr;
        }
    };

    MiniAudioBackend::MiniAudioBackend() : m_Implementation(std::make_unique<Implementation>())
    {

    }

    MiniAudioBackend::~MiniAudioBackend()
    {
        Shutdown();
    }

    bool MiniAudioBackend::Initialize()
    {
        ("Initializing miniaudio backend");

        if (m_Implementation->Initialized)
        {
            return true;
        }

        ma_result l_Result = ma_engine_init(nullptr, &m_Implementation->Engine);
        if (l_Result != MA_SUCCESS)
        {
            ("ma_engine_init failed ({})", static_cast<int>(l_Result));

            return false;
        }

        m_Implementation->Initialized = true;

        ("Miniaudio backend initialized");

        return true;
    }

    void MiniAudioBackend::Shutdown()
    {
        ("Shutting down miniaudio backend");

        if (!m_Implementation->Initialized)
        {
            return;
        }

        for (std::pair<const uint64_t, ma_sound*>& it_Voice : m_Implementation->Voices)
        {
            ma_sound_uninit(it_Voice.second);
            delete it_Voice.second;
        }

        m_Implementation->Voices.clear();
        m_Implementation->Clips.clear();

        ma_engine_uninit(&m_Implementation->Engine);
        m_Implementation->Initialized = false;

        ("Miniaudio backend shutdown complete");
    }

    AudioClipHandle MiniAudioBackend::LoadClip(const std::filesystem::path& path)
    {
        if (!m_Implementation->Initialized)
        {
            return AudioClipHandle::Invalid;
        }

        uint64_t l_Id = m_Implementation->NextClip++;
        m_Implementation->Clips[l_Id] = path.string();

        return static_cast<AudioClipHandle>(l_Id);
    }

    void MiniAudioBackend::UnloadClip(AudioClipHandle clip)
    {
        m_Implementation->Clips.erase(static_cast<uint64_t>(clip));
    }

    VoiceHandle MiniAudioBackend::Play(AudioClipHandle clip, const VoiceParameters& parameters)
    {
        if (!m_Implementation->Initialized)
        {
            return VoiceHandle::Invalid;
        }

        std::unordered_map<uint64_t, std::string>::iterator it_Clip = m_Implementation->Clips.find(static_cast<uint64_t>(clip));
        if (it_Clip == m_Implementation->Clips.end())
        {
            return VoiceHandle::Invalid;
        }

        ma_uint32 l_Flags = parameters.Spatial ? 0u : static_cast<ma_uint32>(MA_SOUND_FLAG_NO_SPATIALIZATION);

        ma_sound* l_Sound = new ma_sound{};
        ma_result l_Result = ma_sound_init_from_file(&m_Implementation->Engine, it_Clip->second.c_str(), l_Flags, nullptr, nullptr, l_Sound);
        if (l_Result != MA_SUCCESS)
        {
            ("Failed to load '{}' ({})", it_Clip->second, static_cast<int>(l_Result));
            delete l_Sound;

            return VoiceHandle::Invalid;
        }

        ma_sound_set_volume(l_Sound, parameters.Volume);
        ma_sound_set_pitch(l_Sound, parameters.Pitch);
        ma_sound_set_looping(l_Sound, parameters.Loop ? MA_TRUE : MA_FALSE);

        if (parameters.Spatial)
        {
            ma_sound_set_position(l_Sound, parameters.Position.x, parameters.Position.y, parameters.Position.z);
        }

        ma_sound_start(l_Sound);

        uint64_t l_Id = m_Implementation->NextVoice++;
        m_Implementation->Voices[l_Id] = l_Sound;

        return static_cast<VoiceHandle>(l_Id);
    }

    void MiniAudioBackend::Stop(VoiceHandle voice)
    {
        std::unordered_map<uint64_t, ma_sound*>::iterator it_Voice = m_Implementation->Voices.find(static_cast<uint64_t>(voice));
        if (it_Voice == m_Implementation->Voices.end())
        {
            return;
        }

        ma_sound_uninit(it_Voice->second);
        delete it_Voice->second;
        m_Implementation->Voices.erase(it_Voice);
    }

    void MiniAudioBackend::SetVoiceVolume(VoiceHandle voice, float volume)
    {
        if (ma_sound* l_Sound = m_Implementation->FindVoice(voice))
        {
            ma_sound_set_volume(l_Sound, volume);
        }
    }

    void MiniAudioBackend::SetVoicePitch(VoiceHandle voice, float pitch)
    {
        if (ma_sound* l_Sound = m_Implementation->FindVoice(voice))
        {
            ma_sound_set_pitch(l_Sound, pitch);
        }
    }

    void MiniAudioBackend::SetVoicePosition(VoiceHandle voice, const glm::vec3& position)
    {
        if (ma_sound* l_Sound = m_Implementation->FindVoice(voice))
        {
            ma_sound_set_position(l_Sound, position.x, position.y, position.z);
        }
    }

    bool MiniAudioBackend::IsVoiceActive(VoiceHandle voice) const
    {
        ma_sound* l_Sound = m_Implementation->FindVoice(voice);

        return l_Sound != nullptr && ma_sound_is_playing(l_Sound) == MA_TRUE;
    }

    void MiniAudioBackend::SetListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
    {
        if (!m_Implementation->Initialized)
        {
            return;
        }

        ma_engine_listener_set_position(&m_Implementation->Engine, 0, position.x, position.y, position.z);
        ma_engine_listener_set_direction(&m_Implementation->Engine, 0, forward.x, forward.y, forward.z);
        ma_engine_listener_set_world_up(&m_Implementation->Engine, 0, up.x, up.y, up.z);
    }

    void MiniAudioBackend::SetMasterVolume(float volume)
    {
        if (m_Implementation->Initialized)
        {
            ma_engine_set_volume(&m_Implementation->Engine, volume);
        }
    }

    void MiniAudioBackend::Update()
    {
        if (!m_Implementation->Initialized)
        {
            return;
        }

        for (std::unordered_map<uint64_t, ma_sound*>::iterator it_Voice = m_Implementation->Voices.begin(); it_Voice != m_Implementation->Voices.end();)
        {
            if (ma_sound_is_looping(it_Voice->second) == MA_FALSE && ma_sound_at_end(it_Voice->second) == MA_TRUE)
            {
                ma_sound_uninit(it_Voice->second);
                delete it_Voice->second;
                it_Voice = m_Implementation->Voices.erase(it_Voice);
            }
            else
            {
                ++it_Voice;
            }
        }
    }
}