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
                return std::make_unique<MiniAudioBackend>();
            default:
                TR_CORE_ERROR("AudioBackendFactory: unsupported audio backend {}", static_cast<uint32_t>(backend));

                return nullptr;
        }
    }
}