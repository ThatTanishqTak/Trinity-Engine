#pragma once

#include <Trinity/Renderer/Frontend/Camera.h>

namespace Trinity
{
    struct CameraComponent
    {
        Camera Camera;
        bool Primary = true;
    };
}