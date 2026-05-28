#pragma once

#include <Trinity/Platform/Input.h>

namespace Trinity
{
    class SDLInput : public Input
    {
    public:
        bool IsKeyPressed(KeyCode key) const override;

        bool IsMouseButtonPressed(MouseCode button) const override;
        std::pair<float, float> GetMousePosition() const override;
        float GetMouseX() const override;
        float GetMouseY() const override;
    };
}