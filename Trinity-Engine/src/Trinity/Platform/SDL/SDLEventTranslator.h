#pragma once

#include "Trinity/Input/InputCodes.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>

namespace Trinity
{
    Code::KeyCode TranslateKeyCode(SDL_Keycode keyCode, SDL_Scancode scanCode);
    Code::MouseCode TranslateMouseButton(uint8_t mouseButton);
}