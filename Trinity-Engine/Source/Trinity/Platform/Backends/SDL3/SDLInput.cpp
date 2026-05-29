#include <Trinity/Platform/Backends/SDL3/SDLInput.h>

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>

namespace Trinity
{
    static SDL_Scancode ToScancode(KeyCode key)
    {
        switch (key)
        {
            case Key::Space: return SDL_SCANCODE_SPACE;
            case Key::Apostrophe: return SDL_SCANCODE_APOSTROPHE;
            case Key::Comma: return SDL_SCANCODE_COMMA;
            case Key::Minus: return SDL_SCANCODE_MINUS;
            case Key::Period: return SDL_SCANCODE_PERIOD;
            case Key::Slash: return SDL_SCANCODE_SLASH;

            case Key::D0: return SDL_SCANCODE_0;
            case Key::D1: return SDL_SCANCODE_1;
            case Key::D2: return SDL_SCANCODE_2;
            case Key::D3: return SDL_SCANCODE_3;
            case Key::D4: return SDL_SCANCODE_4;
            case Key::D5: return SDL_SCANCODE_5;
            case Key::D6: return SDL_SCANCODE_6;
            case Key::D7: return SDL_SCANCODE_7;
            case Key::D8: return SDL_SCANCODE_8;
            case Key::D9: return SDL_SCANCODE_9;

            case Key::Semicolon: return SDL_SCANCODE_SEMICOLON;
            case Key::Equal: return SDL_SCANCODE_EQUALS;

            case Key::A: return SDL_SCANCODE_A;
            case Key::B: return SDL_SCANCODE_B;
            case Key::C: return SDL_SCANCODE_C;
            case Key::D: return SDL_SCANCODE_D;
            case Key::E: return SDL_SCANCODE_E;
            case Key::F: return SDL_SCANCODE_F;
            case Key::G: return SDL_SCANCODE_G;
            case Key::H: return SDL_SCANCODE_H;
            case Key::I: return SDL_SCANCODE_I;
            case Key::J: return SDL_SCANCODE_J;
            case Key::K: return SDL_SCANCODE_K;
            case Key::L: return SDL_SCANCODE_L;
            case Key::M: return SDL_SCANCODE_M;
            case Key::N: return SDL_SCANCODE_N;
            case Key::O: return SDL_SCANCODE_O;
            case Key::P: return SDL_SCANCODE_P;
            case Key::Q: return SDL_SCANCODE_Q;
            case Key::R: return SDL_SCANCODE_R;
            case Key::S: return SDL_SCANCODE_S;
            case Key::T: return SDL_SCANCODE_T;
            case Key::U: return SDL_SCANCODE_U;
            case Key::V: return SDL_SCANCODE_V;
            case Key::W: return SDL_SCANCODE_W;
            case Key::X: return SDL_SCANCODE_X;
            case Key::Y: return SDL_SCANCODE_Y;
            case Key::Z: return SDL_SCANCODE_Z;

            case Key::LeftBracket: return SDL_SCANCODE_LEFTBRACKET;
            case Key::Backslash: return SDL_SCANCODE_BACKSLASH;
            case Key::RightBracket: return SDL_SCANCODE_RIGHTBRACKET;
            case Key::GraveAccent: return SDL_SCANCODE_GRAVE;

            case Key::Escape: return SDL_SCANCODE_ESCAPE;
            case Key::Enter: return SDL_SCANCODE_RETURN;
            case Key::Tab: return SDL_SCANCODE_TAB;
            case Key::Backspace: return SDL_SCANCODE_BACKSPACE;
            case Key::Insert: return SDL_SCANCODE_INSERT;
            case Key::Delete: return SDL_SCANCODE_DELETE;
            case Key::Right: return SDL_SCANCODE_RIGHT;
            case Key::Left: return SDL_SCANCODE_LEFT;
            case Key::Down: return SDL_SCANCODE_DOWN;
            case Key::Up: return SDL_SCANCODE_UP;
            case Key::PageUp: return SDL_SCANCODE_PAGEUP;
            case Key::PageDown: return SDL_SCANCODE_PAGEDOWN;
            case Key::Home: return SDL_SCANCODE_HOME;
            case Key::End: return SDL_SCANCODE_END;
            case Key::CapsLock: return SDL_SCANCODE_CAPSLOCK;
            case Key::ScrollLock: return SDL_SCANCODE_SCROLLLOCK;
            case Key::NumLock: return SDL_SCANCODE_NUMLOCKCLEAR;
            case Key::PrintScreen: return SDL_SCANCODE_PRINTSCREEN;
            case Key::Pause: return SDL_SCANCODE_PAUSE;

            case Key::F1: return SDL_SCANCODE_F1;
            case Key::F2: return SDL_SCANCODE_F2;
            case Key::F3: return SDL_SCANCODE_F3;
            case Key::F4: return SDL_SCANCODE_F4;
            case Key::F5: return SDL_SCANCODE_F5;
            case Key::F6: return SDL_SCANCODE_F6;
            case Key::F7: return SDL_SCANCODE_F7;
            case Key::F8: return SDL_SCANCODE_F8;
            case Key::F9: return SDL_SCANCODE_F9;
            case Key::F10: return SDL_SCANCODE_F10;
            case Key::F11: return SDL_SCANCODE_F11;
            case Key::F12: return SDL_SCANCODE_F12;

            case Key::KP0: return SDL_SCANCODE_KP_0;
            case Key::KP1: return SDL_SCANCODE_KP_1;
            case Key::KP2: return SDL_SCANCODE_KP_2;
            case Key::KP3: return SDL_SCANCODE_KP_3;
            case Key::KP4: return SDL_SCANCODE_KP_4;
            case Key::KP5: return SDL_SCANCODE_KP_5;
            case Key::KP6: return SDL_SCANCODE_KP_6;
            case Key::KP7: return SDL_SCANCODE_KP_7;
            case Key::KP8: return SDL_SCANCODE_KP_8;
            case Key::KP9: return SDL_SCANCODE_KP_9;
            case Key::KPDecimal: return SDL_SCANCODE_KP_PERIOD;
            case Key::KPDivide: return SDL_SCANCODE_KP_DIVIDE;
            case Key::KPMultiply: return SDL_SCANCODE_KP_MULTIPLY;
            case Key::KPSubtract: return SDL_SCANCODE_KP_MINUS;
            case Key::KPAdd: return SDL_SCANCODE_KP_PLUS;
            case Key::KPEnter: return SDL_SCANCODE_KP_ENTER;
            case Key::KPEqual: return SDL_SCANCODE_KP_EQUALS;

            case Key::LeftShift: return SDL_SCANCODE_LSHIFT;
            case Key::LeftControl: return SDL_SCANCODE_LCTRL;
            case Key::LeftAlt: return SDL_SCANCODE_LALT;
            case Key::LeftSuper: return SDL_SCANCODE_LGUI;
            case Key::RightShift: return SDL_SCANCODE_RSHIFT;
            case Key::RightControl: return SDL_SCANCODE_RCTRL;
            case Key::RightAlt: return SDL_SCANCODE_RALT;
            case Key::RightSuper: return SDL_SCANCODE_RGUI;
            case Key::Menu: return SDL_SCANCODE_MENU;

            default: return SDL_SCANCODE_UNKNOWN;
        }
    }

    bool SDLInput::IsKeyPressed(KeyCode key) const
    {
        SDL_Scancode l_Scancode = ToScancode(key);
        if (l_Scancode == SDL_SCANCODE_UNKNOWN)
        {
            return false;
        }

        const bool* l_State = SDL_GetKeyboardState(nullptr);

        return l_State[l_Scancode];
    }

    bool SDLInput::IsMouseButtonPressed(MouseCode button) const
    {
        SDL_MouseButtonFlags l_Flags = SDL_GetMouseState(nullptr, nullptr);

        return (l_Flags & SDL_BUTTON_MASK(button + 1)) != 0;
    }

    std::pair<float, float> SDLInput::GetMousePosition() const
    {
        float l_X = 0.0f;
        float l_Y = 0.0f;
        SDL_GetMouseState(&l_X, &l_Y);

        return { l_X, l_Y };
    }

    float SDLInput::GetMouseX() const
    {
        return GetMousePosition().first;
    }

    float SDLInput::GetMouseY() const
    {
        return GetMousePosition().second;
    }
}