#pragma once

#include "Trinity/Platform/Input/Desktop/DesktopInputCodes.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>

namespace Trinity
{
    Code::KeyCode TranslateKeyCode(SDL_Keycode keyCode, SDL_Scancode scanCode);
    Code::MouseCode TranslateMouseButton(uint8_t mouseButton);
    Code::GamepadButton TranslateGamepadButton(SDL_GamepadButton button);
    Code::GamepadAxis TranslateGamepadAxis(SDL_GamepadAxis axis);
}