#include <Trinity/Audio/Frontend/AudioDevice.h>

#include <Trinity/Audio/Backends/AudioBackendFactory.h>
#include <Trinity/Audio/Backends/IAudioBackend.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    AudioDevice::AudioDevice() = default;

    AudioDevice::~AudioDevice()
    {
        Shutdown();
    }

    bool AudioDevice::Initialize(AudioBackend backend)
    {
        if (m_Backend != nullptr)
        {
            return true;
        }

        m_Backend = AudioBackendFactory::Create(backend);
        if (m_Backend == nullptr)
        {
            TR_CORE_ERROR("AudioDevice: failed to create audio backend");

            return false;
        }

        if (!m_Backend->Initialize())
        {
            TR_CORE_ERROR("AudioDevice: backend initialization failed");
            m_Backend.reset();

            return false;
        }

        m_Backend->SetMasterVolume(m_MasterVolume);

        return true;
    }

    void AudioDevice::Shutdown()
    {
        if (m_Backend == nullptr)
        {
            return;
        }

        m_Backend->Shutdown();
        m_Backend.reset();
    }

    void AudioDevice::SetMasterVolume(float volume)
    {
        m_MasterVolume = volume;

        if (m_Backend != nullptr)
        {
            m_Backend->SetMasterVolume(volume);
        }
    }
}