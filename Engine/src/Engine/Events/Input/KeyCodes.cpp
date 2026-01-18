#include "Engine/Events/Input/KeyCodes.h"

#include <GLFW/glfw3.h>

namespace Engine
{
    namespace Input
    {
        KeyCode FromGLFWKey(int k)
        {
            switch (k)
            {
            case GLFW_KEY_SPACE: return TR_KEY_SPACE;
            case GLFW_KEY_APOSTROPHE: return TR_KEY_APOSTROPHE;
            case GLFW_KEY_COMMA: return TR_KEY_COMMA;
            case GLFW_KEY_MINUS: return TR_KEY_MINUS;
            case GLFW_KEY_PERIOD: return TR_KEY_PERIOD;
            case GLFW_KEY_SLASH: return TR_KEY_SLASH;

            case GLFW_KEY_0: return TR_KEY_0;
            case GLFW_KEY_1: return TR_KEY_1;
            case GLFW_KEY_2: return TR_KEY_2;
            case GLFW_KEY_3: return TR_KEY_3;
            case GLFW_KEY_4: return TR_KEY_4;
            case GLFW_KEY_5: return TR_KEY_5;
            case GLFW_KEY_6: return TR_KEY_6;
            case GLFW_KEY_7: return TR_KEY_7;
            case GLFW_KEY_8: return TR_KEY_8;
            case GLFW_KEY_9: return TR_KEY_9;

            case GLFW_KEY_SEMICOLON: return TR_KEY_SEMICOLON;
            case GLFW_KEY_EQUAL: return TR_KEY_EQUAL;

            case GLFW_KEY_A: return TR_KEY_A;
            case GLFW_KEY_B: return TR_KEY_B;
            case GLFW_KEY_C: return TR_KEY_C;
            case GLFW_KEY_D: return TR_KEY_D;
            case GLFW_KEY_E: return TR_KEY_E;
            case GLFW_KEY_F: return TR_KEY_F;
            case GLFW_KEY_G: return TR_KEY_G;
            case GLFW_KEY_H: return TR_KEY_H;
            case GLFW_KEY_I: return TR_KEY_I;
            case GLFW_KEY_J: return TR_KEY_J;
            case GLFW_KEY_K: return TR_KEY_K;
            case GLFW_KEY_L: return TR_KEY_L;
            case GLFW_KEY_M: return TR_KEY_M;
            case GLFW_KEY_N: return TR_KEY_N;
            case GLFW_KEY_O: return TR_KEY_O;
            case GLFW_KEY_P: return TR_KEY_P;
            case GLFW_KEY_Q: return TR_KEY_Q;
            case GLFW_KEY_R: return TR_KEY_R;
            case GLFW_KEY_S: return TR_KEY_S;
            case GLFW_KEY_T: return TR_KEY_T;
            case GLFW_KEY_U: return TR_KEY_U;
            case GLFW_KEY_V: return TR_KEY_V;
            case GLFW_KEY_W: return TR_KEY_W;
            case GLFW_KEY_X: return TR_KEY_X;
            case GLFW_KEY_Y: return TR_KEY_Y;
            case GLFW_KEY_Z: return TR_KEY_Z;

            case GLFW_KEY_LEFT_BRACKET: return TR_KEY_LEFT_BRACKET;
            case GLFW_KEY_BACKSLASH: return TR_KEY_BACKSLASH;
            case GLFW_KEY_RIGHT_BRACKET: return TR_KEY_RIGHT_BRACKET;
            case GLFW_KEY_GRAVE_ACCENT: return TR_KEY_GRAVE_ACCENT;

            case GLFW_KEY_ESCAPE: return TR_KEY_ESCAPE;
            case GLFW_KEY_ENTER: return TR_KEY_ENTER;
            case GLFW_KEY_TAB: return TR_KEY_TAB;
            case GLFW_KEY_BACKSPACE: return TR_KEY_BACKSPACE;
            case GLFW_KEY_INSERT: return TR_KEY_INSERT;
            case GLFW_KEY_DELETE: return TR_KEY_DELETE;

            case GLFW_KEY_RIGHT: return TR_KEY_RIGHT;
            case GLFW_KEY_LEFT: return TR_KEY_LEFT;
            case GLFW_KEY_DOWN: return TR_KEY_DOWN;
            case GLFW_KEY_UP: return TR_KEY_UP;

            case GLFW_KEY_PAGE_UP: return TR_KEY_PAGE_UP;
            case GLFW_KEY_PAGE_DOWN: return TR_KEY_PAGE_DOWN;
            case GLFW_KEY_HOME: return TR_KEY_HOME;
            case GLFW_KEY_END: return TR_KEY_END;

            case GLFW_KEY_CAPS_LOCK: return TR_KEY_CAPS_LOCK;
            case GLFW_KEY_SCROLL_LOCK: return TR_KEY_SCROLL_LOCK;
            case GLFW_KEY_NUM_LOCK: return TR_KEY_NUM_LOCK;
            case GLFW_KEY_PRINT_SCREEN: return TR_KEY_PRINT_SCREEN;
            case GLFW_KEY_PAUSE: return TR_KEY_PAUSE;

            case GLFW_KEY_F1: return TR_KEY_F1;
            case GLFW_KEY_F2: return TR_KEY_F2;
            case GLFW_KEY_F3: return TR_KEY_F3;
            case GLFW_KEY_F4: return TR_KEY_F4;
            case GLFW_KEY_F5: return TR_KEY_F5;
            case GLFW_KEY_F6: return TR_KEY_F6;
            case GLFW_KEY_F7: return TR_KEY_F7;
            case GLFW_KEY_F8: return TR_KEY_F8;
            case GLFW_KEY_F9: return TR_KEY_F9;
            case GLFW_KEY_F10: return TR_KEY_F10;
            case GLFW_KEY_F11: return TR_KEY_F11;
            case GLFW_KEY_F12: return TR_KEY_F12;

            case GLFW_KEY_KP_0: return TR_KEY_KP_0;
            case GLFW_KEY_KP_1: return TR_KEY_KP_1;
            case GLFW_KEY_KP_2: return TR_KEY_KP_2;
            case GLFW_KEY_KP_3: return TR_KEY_KP_3;
            case GLFW_KEY_KP_4: return TR_KEY_KP_4;
            case GLFW_KEY_KP_5: return TR_KEY_KP_5;
            case GLFW_KEY_KP_6: return TR_KEY_KP_6;
            case GLFW_KEY_KP_7: return TR_KEY_KP_7;
            case GLFW_KEY_KP_8: return TR_KEY_KP_8;
            case GLFW_KEY_KP_9: return TR_KEY_KP_9;

            case GLFW_KEY_KP_DECIMAL: return TR_KEY_KP_DECIMAL;
            case GLFW_KEY_KP_DIVIDE: return TR_KEY_KP_DIVIDE;
            case GLFW_KEY_KP_MULTIPLY: return TR_KEY_KP_MULTIPLY;
            case GLFW_KEY_KP_SUBTRACT: return TR_KEY_KP_SUBTRACT;
            case GLFW_KEY_KP_ADD: return TR_KEY_KP_ADD;
            case GLFW_KEY_KP_ENTER: return TR_KEY_KP_ENTER;
            case GLFW_KEY_KP_EQUAL: return TR_KEY_KP_EQUAL;

            case GLFW_KEY_LEFT_SHIFT: return TR_KEY_LEFT_SHIFT;
            case GLFW_KEY_LEFT_CONTROL: return TR_KEY_LEFT_CONTROL;
            case GLFW_KEY_LEFT_ALT: return TR_KEY_LEFT_ALT;
            case GLFW_KEY_LEFT_SUPER: return TR_KEY_LEFT_SUPER;

            case GLFW_KEY_RIGHT_SHIFT: return TR_KEY_RIGHT_SHIFT;
            case GLFW_KEY_RIGHT_CONTROL: return TR_KEY_RIGHT_CONTROL;
            case GLFW_KEY_RIGHT_ALT: return TR_KEY_RIGHT_ALT;
            case GLFW_KEY_RIGHT_SUPER: return TR_KEY_RIGHT_SUPER;

            case GLFW_KEY_MENU: return TR_KEY_MENU;

            default: return TR_KEY_UNKNOWN;
            }
        }
    }
}