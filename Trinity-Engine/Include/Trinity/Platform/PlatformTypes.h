#pragma once

#include <cstdint>

namespace Trinity
{
    enum class PlatformType
    {
        Unknown = 0,
        Windows,
        Linux,
        MacOS,
        PlayStation,
        Xbox,
        NintendoSwitch
    };

    enum class NativeHandleType
    {
        None = 0,
        Win32,
        Xlib,
        Wayland,
        Cocoa
    };

    struct NativeWindowHandle
    {
        NativeHandleType Type = NativeHandleType::None;
        void* Handle = nullptr;
        void* Display = nullptr;

        bool IsValid() const
        {
            return Type != NativeHandleType::None && Handle != nullptr;
        }
    };
}