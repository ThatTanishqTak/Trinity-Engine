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
        TR_CORE_TRACE("INITIALIZING AUDIO DEVICE");

        if (m_Backend != nullptr)
        {
            return true;
        }

        m_Backend = AudioBackendFactory::Create(backend);
        if (m_Backend == nullptr)
        {
            TR_CORE_CRITICAL("Failed to create audio backend");

            return false;
        }

        if (!m_Backend->Initialize())
        {
            TR_CORE_CRITICAL("Failed to initialize audio backend");

            m_Backend.reset();

            return false;
        }

        m_Backend->SetMasterVolume(m_MasterVolume);


        TR_CORE_TRACE("AUDIO DEVICE INITIALIZED");

        return true;
    }

    void AudioDevice::Shutdown()
    {
        TR_CORE_TRACE("SHUTTING DOWN AUDIO DEVICE");

        if (m_Backend == nullptr)
        {
            return;
        }

        m_Backend->Shutdown();
        m_Backend.reset();

        TR_CORE_TRACE("AUDIO DEVICE SHUTDOWN COMPLETE");
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