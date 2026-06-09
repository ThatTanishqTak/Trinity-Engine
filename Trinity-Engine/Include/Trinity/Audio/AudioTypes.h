#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace Trinity
{
    enum class AudioBackend : uint32_t
    {
        MiniAudio = 0
    };

    enum class AudioClipHandle : uint64_t
    {
        Invalid = 0
    };

    enum class VoiceHandle : uint64_t
    {
        Invalid = 0
    };

    struct VoiceParameters
    {
        float Volume = 1.0f;
        float Pitch = 1.0f;
        bool Loop = false;
        bool Spatial = false;
        glm::vec3 Position = glm::vec3(0.0f);
    };
}