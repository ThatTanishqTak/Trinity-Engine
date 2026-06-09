#pragma once

#include <memory>

#include <Trinity/Audio/AudioTypes.h>

namespace Trinity
{
    class IAudioBackend;

    class AudioBackendFactory
    {
    public:
        static std::unique_ptr<IAudioBackend> Create(AudioBackend backend);
    };
}