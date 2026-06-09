#pragma once

#include <Trinity/Core/UUID.h>
#include <Trinity/Audio/AudioTypes.h>

namespace Trinity
{
    struct AudioSourceComponent
    {
        UUID Clip = UUID(0);
        float Volume = 1.0f;
        float Pitch = 1.0f;
        bool Loop = false;
        bool PlayOnStart = false;
        bool Spatial = false;
        VoiceHandle Runtime = VoiceHandle::Invalid;
    };
}