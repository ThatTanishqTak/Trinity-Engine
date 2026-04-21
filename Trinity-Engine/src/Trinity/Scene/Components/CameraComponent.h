#pragma once

namespace Trinity
{
    struct CameraComponent
    {
        float FOV = 60.0f;
        float NearClip = 0.1f;
        float FarClip = 1000.0f;
        bool Primary = true;
    };
}