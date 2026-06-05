#pragma once

#include <utility>

#include <Trinity/Platform/KeyCodes.h>
#include <Trinity/Platform/MouseCodes.h>

namespace Trinity
{
    class Input
    {
    public:
        virtual ~Input() = default;

        virtual bool IsKeyPressed(KeyCode key) const = 0;

        virtual bool IsMouseButtonPressed(MouseCode button) const = 0;
        virtual std::pair<float, float> GetMousePosition() const = 0;
        virtual std::pair<float, float> GetMouseDelta() const = 0;
        virtual float GetMouseX() const = 0;
        virtual float GetMouseY() const = 0;
    };
}