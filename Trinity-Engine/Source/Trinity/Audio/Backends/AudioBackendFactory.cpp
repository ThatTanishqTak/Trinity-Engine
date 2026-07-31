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
                TR_CORE_TRACE("Audio backend selected: MiniAudio");

                return std::make_unique<MiniAudioBackend>();

            default:
                TR_CORE_TRACE("No audio backend selected");

                return nullptr;
        }
    }
}