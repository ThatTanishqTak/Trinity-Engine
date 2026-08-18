#pragma once

#include <imgui.h>

namespace Trinity
{
    class FileSystem;

    struct EditorColors
    {
        static constexpr ImU32 AppToolbar = IM_COL32(0x19, 0x19, 0x19, 0xFF);
        static constexpr ImU32 AppToolbarHover = IM_COL32(0x42, 0x42, 0x42, 0xFF);
        static constexpr ImU32 AppToolbarChecked = IM_COL32(0x4C, 0x4C, 0x4C, 0xFF);
        static constexpr ImU32 WindowBackground = IM_COL32(0x38, 0x38, 0x38, 0xFF);
        static constexpr ImU32 DefaultBackground = IM_COL32(0x28, 0x28, 0x28, 0xFF);
        static constexpr ImU32 AlternatedRow = IM_COL32(0x3F, 0x3F, 0x3F, 0xFF);
        static constexpr ImU32 DefaultBorder = IM_COL32(0x23, 0x23, 0x23, 0xFF);
        static constexpr ImU32 Toolbar = IM_COL32(0x3C, 0x3C, 0x3C, 0xFF);
        static constexpr ImU32 ToolbarHover = IM_COL32(0x46, 0x46, 0x46, 0xFF);
        static constexpr ImU32 ToolbarChecked = IM_COL32(0x50, 0x50, 0x50, 0xFF);
        static constexpr ImU32 Tab = IM_COL32(0x2E, 0x2E, 0x2E, 0xFF);
        static constexpr ImU32 TabChecked = IM_COL32(0x3C, 0x3C, 0x3C, 0xFF);
        static constexpr ImU32 Highlight = IM_COL32(0x2C, 0x5D, 0x87, 0xFF);
        static constexpr ImU32 HighlightInactive = IM_COL32(0x4D, 0x4D, 0x4D, 0xFF);
        static constexpr ImU32 HighlightText = IM_COL32(0x4C, 0x7E, 0xFF, 0xFF);
        static constexpr ImU32 DefaultText = IM_COL32(0xD2, 0xD2, 0xD2, 0xFF);
        static constexpr ImU32 LabelText = IM_COL32(0xC4, 0xC4, 0xC4, 0xFF);
        static constexpr ImU32 DisabledText = IM_COL32(0x7F, 0x7F, 0x7F, 0xFF);
        static constexpr ImU32 Button = IM_COL32(0x58, 0x58, 0x58, 0xFF);
        static constexpr ImU32 ButtonHover = IM_COL32(0x67, 0x67, 0x67, 0xFF);
        static constexpr ImU32 ButtonPressed = IM_COL32(0x4A, 0x4A, 0x4A, 0xFF);
        static constexpr ImU32 InputField = IM_COL32(0x2A, 0x2A, 0x2A, 0xFF);
        static constexpr ImU32 InputFieldBorderFocus = IM_COL32(0x3A, 0x79, 0xBA, 0xFF);
        static constexpr ImU32 ScrollbarGroove = IM_COL32(0x2E, 0x2E, 0x2E, 0xFF);
        static constexpr ImU32 ScrollbarThumb = IM_COL32(0x4F, 0x4F, 0x4F, 0xFF);
        static constexpr ImU32 ScrollbarThumbHover = IM_COL32(0x5A, 0x5A, 0x5A, 0xFF);
        static constexpr ImU32 OverlayBackground = IM_COL32(0x38, 0x38, 0x38, 0xF0);
        static constexpr ImU32 WarningText = IM_COL32(0xF4, 0xBC, 0x02, 0xFF);
        static constexpr ImU32 ErrorText = IM_COL32(0xFF, 0x60, 0x60, 0xFF);

        static ImVec4 ToVec4(ImU32 color)
        {
            return ImGui::ColorConvertU32ToFloat4(color);
        }
    };

    class EditorTheme
    {
    public:
        static void Apply(FileSystem& fileSystem);

        // Heights of Unity's three chrome strips, in current-DPI pixels.
        static float MenuBarHeight();
        static float ToolBarHeight();
        static float StatusBarHeight();

        // Full-width borderless strip pinned at y; always pair with EndChromeBar().
        static bool BeginChromeBar(const char* id, float y, float height, ImU32 background, const ImVec2& padding, ImGuiWindowFlags extraFlags = 0);
        static void EndChromeBar();

        // Unity's flat toolbar button: transparent until hovered, filled when checked.
        static bool ToolBarButton(const char* label, bool checked = false, const char* tooltip = nullptr, bool enabled = true, float width = 0.0f, ImU32 checkedBackground = EditorColors::AppToolbarChecked);
        static void ToolBarSeparator();

        // The #3C3C3C strip at the top of Hierarchy / Project / Console.
        static void BeginPanelToolBar(const char* id);
        static void EndPanelToolBar();

        // Unity's search box: magnifier hint, sunken field, fixed or stretch width.
        static bool SearchField(const char* id, char* buffer, size_t bufferSize, float width);
    };
}