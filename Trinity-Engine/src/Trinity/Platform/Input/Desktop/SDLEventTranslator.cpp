#include "Trinity/Platform/Input/Desktop/SDLEventTranslator.h"

namespace Trinity
{
    Code::KeyCode TranslateKeyCode(SDL_Keycode keyCode, SDL_Scancode scanCode)
    {
        switch (scanCode)
        {
        case SDL_SCANCODE_A: return Code::KeyCode::TR_KEY_A;
        case SDL_SCANCODE_B: return Code::KeyCode::TR_KEY_B;
        case SDL_SCANCODE_C: return Code::KeyCode::TR_KEY_C;
        case SDL_SCANCODE_D: return Code::KeyCode::TR_KEY_D;
        case SDL_SCANCODE_E: return Code::KeyCode::TR_KEY_E;
        case SDL_SCANCODE_F: return Code::KeyCode::TR_KEY_F;
        case SDL_SCANCODE_G: return Code::KeyCode::TR_KEY_G;
        case SDL_SCANCODE_H: return Code::KeyCode::TR_KEY_H;
        case SDL_SCANCODE_I: return Code::KeyCode::TR_KEY_I;
        case SDL_SCANCODE_J: return Code::KeyCode::TR_KEY_J;
        case SDL_SCANCODE_K: return Code::KeyCode::TR_KEY_K;
        case SDL_SCANCODE_L: return Code::KeyCode::TR_KEY_L;
        case SDL_SCANCODE_M: return Code::KeyCode::TR_KEY_M;
        case SDL_SCANCODE_N: return Code::KeyCode::TR_KEY_N;
        case SDL_SCANCODE_O: return Code::KeyCode::TR_KEY_O;
        case SDL_SCANCODE_P: return Code::KeyCode::TR_KEY_P;
        case SDL_SCANCODE_Q: return Code::KeyCode::TR_KEY_Q;
        case SDL_SCANCODE_R: return Code::KeyCode::TR_KEY_R;
        case SDL_SCANCODE_S: return Code::KeyCode::TR_KEY_S;
        case SDL_SCANCODE_T: return Code::KeyCode::TR_KEY_T;
        case SDL_SCANCODE_U: return Code::KeyCode::TR_KEY_U;
        case SDL_SCANCODE_V: return Code::KeyCode::TR_KEY_V;
        case SDL_SCANCODE_W: return Code::KeyCode::TR_KEY_W;
        case SDL_SCANCODE_X: return Code::KeyCode::TR_KEY_X;
        case SDL_SCANCODE_Y: return Code::KeyCode::TR_KEY_Y;
        case SDL_SCANCODE_Z: return Code::KeyCode::TR_KEY_Z;
        case SDL_SCANCODE_0: return Code::KeyCode::TR_KEY_D0;
        case SDL_SCANCODE_1: return Code::KeyCode::TR_KEY_D1;
        case SDL_SCANCODE_2: return Code::KeyCode::TR_KEY_D2;
        case SDL_SCANCODE_3: return Code::KeyCode::TR_KEY_D3;
        case SDL_SCANCODE_4: return Code::KeyCode::TR_KEY_D4;
        case SDL_SCANCODE_5: return Code::KeyCode::TR_KEY_D5;
        case SDL_SCANCODE_6: return Code::KeyCode::TR_KEY_D6;
        case SDL_SCANCODE_7: return Code::KeyCode::TR_KEY_D7;
        case SDL_SCANCODE_8: return Code::KeyCode::TR_KEY_D8;
        case SDL_SCANCODE_9: return Code::KeyCode::TR_KEY_D9;
        case SDL_SCANCODE_F1: return Code::KeyCode::TR_KEY_F1;
        case SDL_SCANCODE_F2: return Code::KeyCode::TR_KEY_F2;
        case SDL_SCANCODE_F3: return Code::KeyCode::TR_KEY_F3;
        case SDL_SCANCODE_F4: return Code::KeyCode::TR_KEY_F4;
        case SDL_SCANCODE_F5: return Code::KeyCode::TR_KEY_F5;
        case SDL_SCANCODE_F6: return Code::KeyCode::TR_KEY_F6;
        case SDL_SCANCODE_F7: return Code::KeyCode::TR_KEY_F7;
        case SDL_SCANCODE_F8: return Code::KeyCode::TR_KEY_F8;
        case SDL_SCANCODE_F9: return Code::KeyCode::TR_KEY_F9;
        case SDL_SCANCODE_F10: return Code::KeyCode::TR_KEY_F10;
        case SDL_SCANCODE_F11: return Code::KeyCode::TR_KEY_F11;
        case SDL_SCANCODE_F12: return Code::KeyCode::TR_KEY_F12;
        case SDL_SCANCODE_F13: return Code::KeyCode::TR_KEY_F13;
        case SDL_SCANCODE_F14: return Code::KeyCode::TR_KEY_F14;
        case SDL_SCANCODE_F15: return Code::KeyCode::TR_KEY_F15;
        case SDL_SCANCODE_F16: return Code::KeyCode::TR_KEY_F16;
        case SDL_SCANCODE_F17: return Code::KeyCode::TR_KEY_F17;
        case SDL_SCANCODE_F18: return Code::KeyCode::TR_KEY_F18;
        case SDL_SCANCODE_F19: return Code::KeyCode::TR_KEY_F19;
        case SDL_SCANCODE_F20: return Code::KeyCode::TR_KEY_F20;
        case SDL_SCANCODE_F21: return Code::KeyCode::TR_KEY_F21;
        case SDL_SCANCODE_F22: return Code::KeyCode::TR_KEY_F22;
        case SDL_SCANCODE_F23: return Code::KeyCode::TR_KEY_F23;
        case SDL_SCANCODE_F24: return Code::KeyCode::TR_KEY_F24;
        case SDL_SCANCODE_ESCAPE: return Code::KeyCode::TR_KEY_ESCAPE;
        case SDL_SCANCODE_RETURN: return Code::KeyCode::TR_KEY_ENTER;
        case SDL_SCANCODE_TAB: return Code::KeyCode::TR_KEY_TAB;
        case SDL_SCANCODE_BACKSPACE: return Code::KeyCode::TR_KEY_BACKSPACE;
        case SDL_SCANCODE_INSERT: return Code::KeyCode::TR_KEY_INSERT;
        case SDL_SCANCODE_DELETE: return Code::KeyCode::TR_KEY_DELETE;
        case SDL_SCANCODE_RIGHT: return Code::KeyCode::TR_KEY_RIGHT;
        case SDL_SCANCODE_LEFT: return Code::KeyCode::TR_KEY_LEFT;
        case SDL_SCANCODE_DOWN: return Code::KeyCode::TR_KEY_DOWN;
        case SDL_SCANCODE_UP: return Code::KeyCode::TR_KEY_UP;
        case SDL_SCANCODE_PAGEUP: return Code::KeyCode::TR_KEY_PAGE_UP;
        case SDL_SCANCODE_PAGEDOWN: return Code::KeyCode::TR_KEY_PAGE_DOWN;
        case SDL_SCANCODE_HOME: return Code::KeyCode::TR_KEY_HOME;
        case SDL_SCANCODE_END: return Code::KeyCode::TR_KEY_END;
        case SDL_SCANCODE_CAPSLOCK: return Code::KeyCode::TR_KEY_CAPSLOCK;
        case SDL_SCANCODE_SCROLLLOCK: return Code::KeyCode::TR_KEY_SCROLLLOCK;
        case SDL_SCANCODE_NUMLOCKCLEAR: return Code::KeyCode::TR_KEY_NUMLOCK;
        case SDL_SCANCODE_PRINTSCREEN: return Code::KeyCode::TR_KEY_PRINTSCREEN;
        case SDL_SCANCODE_PAUSE: return Code::KeyCode::TR_KEY_PAUSE;
        case SDL_SCANCODE_KP_0: return Code::KeyCode::TR_KEY_KP0;
        case SDL_SCANCODE_KP_1: return Code::KeyCode::TR_KEY_KP1;
        case SDL_SCANCODE_KP_2: return Code::KeyCode::TR_KEY_KP2;
        case SDL_SCANCODE_KP_3: return Code::KeyCode::TR_KEY_KP3;
        case SDL_SCANCODE_KP_4: return Code::KeyCode::TR_KEY_KP4;
        case SDL_SCANCODE_KP_5: return Code::KeyCode::TR_KEY_KP5;
        case SDL_SCANCODE_KP_6: return Code::KeyCode::TR_KEY_KP6;
        case SDL_SCANCODE_KP_7: return Code::KeyCode::TR_KEY_KP7;
        case SDL_SCANCODE_KP_8: return Code::KeyCode::TR_KEY_KP8;
        case SDL_SCANCODE_KP_9: return Code::KeyCode::TR_KEY_KP9;
        case SDL_SCANCODE_KP_DECIMAL: return Code::KeyCode::TR_KEY_KEYPAD_DECIMAL;
        case SDL_SCANCODE_KP_DIVIDE: return Code::KeyCode::TR_KEY_KEYPAD_DIVIDE;
        case SDL_SCANCODE_KP_MULTIPLY: return Code::KeyCode::TR_KEY_KEYPAD_MULTIPLY;
        case SDL_SCANCODE_KP_MINUS: return Code::KeyCode::TR_KEY_KEYPAD_SUBTRACT;
        case SDL_SCANCODE_KP_PLUS: return Code::KeyCode::TR_KEY_KEYPAD_ADD;
        case SDL_SCANCODE_KP_ENTER: return Code::KeyCode::TR_KEY_KEYPAD_ENTER;
        case SDL_SCANCODE_KP_EQUALS: return Code::KeyCode::TR_KEY_KEYPAD_EQUAL;
        case SDL_SCANCODE_LSHIFT: return Code::KeyCode::TR_KEY_LEFT_SHIFT;
        case SDL_SCANCODE_LCTRL: return Code::KeyCode::TR_KEY_LEFT_CONTROL;
        case SDL_SCANCODE_LALT: return Code::KeyCode::TR_KEY_LEFT_ALT;
        case SDL_SCANCODE_LGUI: return Code::KeyCode::TR_KEY_LEFT_SUPER;
        case SDL_SCANCODE_RSHIFT: return Code::KeyCode::TR_KEY_RIGHT_SHIFT;
        case SDL_SCANCODE_RCTRL: return Code::KeyCode::TR_KEY_RIGHT_CONTROL;
        case SDL_SCANCODE_RALT: return Code::KeyCode::TR_KEY_RIGHT_ALT;
        case SDL_SCANCODE_RGUI: return Code::KeyCode::TR_KEY_RIGHT_SUPER;
        case SDL_SCANCODE_MENU: return Code::KeyCode::TR_KEY_MENU;
        default: break;
        }

        switch (keyCode)
        {
        case SDLK_SPACE: return Code::KeyCode::TR_KEY_SPACE;
        case SDLK_APOSTROPHE: return Code::KeyCode::TR_KEY_APOSTROPHE;
        case SDLK_COMMA: return Code::KeyCode::TR_KEY_COMMA;
        case SDLK_MINUS: return Code::KeyCode::TR_KEY_MINUS;
        case SDLK_PERIOD: return Code::KeyCode::TR_KEY_PERIOD;
        case SDLK_SLASH: return Code::KeyCode::TR_KEY_SLASH;
        case SDLK_SEMICOLON: return Code::KeyCode::TR_KEY_SEMICOLON;
        case SDLK_EQUALS: return Code::KeyCode::TR_KEY_EQUAL;
        case SDLK_LEFTBRACKET: return Code::KeyCode::TR_KEY_LEFT_BRACKET;
        case SDLK_BACKSLASH: return Code::KeyCode::TR_KEY_BACK_SLASH;
        case SDLK_RIGHTBRACKET: return Code::KeyCode::TR_KEY_RIGHT_BRACKET;
        case SDLK_GRAVE: return Code::KeyCode::TR_KEY_GRAVE_ACCENT;
        default: return Code::KeyCode::UNKNOWN;
        }
    }

    Code::MouseCode TranslateMouseButton(uint8_t mouseButton)
    {
        switch (mouseButton)
        {
        case SDL_BUTTON_LEFT: return Code::MouseCode::TR_BUTTON_LEFT;
        case SDL_BUTTON_RIGHT: return Code::MouseCode::TR_BUTTON_RIGHT;
        case SDL_BUTTON_MIDDLE: return Code::MouseCode::TR_BUTTON_MIDDLE;
        case SDL_BUTTON_X1: return Code::MouseCode::TR_BUTTON_3;
        case SDL_BUTTON_X2: return Code::MouseCode::TR_BUTTON_4;
        default:
        {
            const int l_ButtonIndex = static_cast<int>(mouseButton) - 1;
            if (l_ButtonIndex >= static_cast<int>(Code::MouseCode::TR_BUTTON_0) && l_ButtonIndex <= static_cast<int>(Code::MouseCode::TR_BUTTON_LAST))
            {
                return static_cast<Code::MouseCode>(l_ButtonIndex);
            }

            return Code::MouseCode::TR_BUTTON_0;
        }
        }
    }

    Code::GamepadButton TranslateGamepadButton(SDL_GamepadButton button)
    {
        switch (button)
        {
        case SDL_GAMEPAD_BUTTON_SOUTH:          return Code::GamepadButton::TR_BUTTON_A;
        case SDL_GAMEPAD_BUTTON_EAST:           return Code::GamepadButton::TR_BUTTON_B;
        case SDL_GAMEPAD_BUTTON_WEST:           return Code::GamepadButton::TR_BUTTON_X;
        case SDL_GAMEPAD_BUTTON_NORTH:          return Code::GamepadButton::TR_BUTTON_Y;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:  return Code::GamepadButton::TR_BUTTON_LEFT_BUMPER;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return Code::GamepadButton::TR_BUTTON_RIGHT_BUMPER;
        case SDL_GAMEPAD_BUTTON_BACK:           return Code::GamepadButton::TR_BUTTON_BACK;
        case SDL_GAMEPAD_BUTTON_START:          return Code::GamepadButton::TR_BUTTON_START;
        case SDL_GAMEPAD_BUTTON_GUIDE:          return Code::GamepadButton::TR_BUTTON_GUIDE;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:     return Code::GamepadButton::TR_BUTTON_LEFT_THUMB;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:    return Code::GamepadButton::TR_BUTTON_RIGHT_THUMB;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:        return Code::GamepadButton::TR_BUTTON_DPAD_UP;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:     return Code::GamepadButton::TR_BUTTON_DPAD_RIGHT;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:      return Code::GamepadButton::TR_BUTTON_DPAD_DOWN;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:      return Code::GamepadButton::TR_BUTTON_DPAD_LEFT;
        default:                                return Code::GamepadButton::TR_BUTTON_A;
        }
    }

    Code::GamepadAxis TranslateGamepadAxis(SDL_GamepadAxis axis)
    {
        switch (axis)
        {
        case SDL_GAMEPAD_AXIS_LEFTX:         return Code::GamepadAxis::TR_AXIS_LEFT_X;
        case SDL_GAMEPAD_AXIS_LEFTY:         return Code::GamepadAxis::TR_AXIS_LEFT_Y;
        case SDL_GAMEPAD_AXIS_RIGHTX:        return Code::GamepadAxis::TR_AXIS_RIGHT_X;
        case SDL_GAMEPAD_AXIS_RIGHTY:        return Code::GamepadAxis::TR_AXIS_RIGHT_Y;
        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:  return Code::GamepadAxis::TR_AXIS_LEFT_TRIGGER;
        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: return Code::GamepadAxis::TR_AXIS_RIGHT_TRIGGER;
        default:                             return Code::GamepadAxis::TR_AXIS_LEFT_X;
        }
    }
}