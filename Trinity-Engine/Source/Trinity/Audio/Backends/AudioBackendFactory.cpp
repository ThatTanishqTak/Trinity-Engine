#include <Trinity/Audio/Backends/AudioBackendFactory.h>

#include <Trinity/Audio/Backends/IAudioBackend.h>
#include <Trinity/Audio/Backends/MiniAudio/MiniAudioBackend.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    std::unique_ptr<IAudioBackend> AudioBackendFactory::Create(AudioBackend backend)
    {
        switch (backend)
        {
            case AudioBackend::MiniAudio:
                ("Selected Audio Backend: MiniAudio");
                return std::make_unique<MiniAudioBackend>();

            default:
                ("Unsupported audio backend {}", static_cast<uint32_t>(backend));

                return nullptr;
        }
    }
}