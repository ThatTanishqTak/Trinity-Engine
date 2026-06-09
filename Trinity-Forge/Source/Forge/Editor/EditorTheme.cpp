#include <Forge/Editor/EditorTheme.h>

#include <filesystem>

#include <imgui.h>

#include <Forge/Editor/EditorIcons.h>
#include <Trinity/Core/Application.h>
#include <Trinity/Platform/Window.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    static constexpr const char* k_UIFontFile = "JetBrainsMonoNerdFontMono-Regular.ttf";
    static constexpr const char* k_IconFontFile = "fa-solid-900.ttf";
    static constexpr float k_BaseFontSize = 15.5f;
    static constexpr float k_UserScale = 0.7f;

    static void ApplyStyle()
    {
        ImGuiStyle& l_Style = ImGui::GetStyle();

        l_Style.WindowRounding = 0.0f;
        l_Style.ChildRounding = 0.0f;
        l_Style.FrameRounding = 3.0f;
        l_Style.PopupRounding = 3.0f;
        l_Style.ScrollbarRounding = 3.0f;
        l_Style.GrabRounding = 2.0f;
        l_Style.TabRounding = 3.0f;

        l_Style.WindowBorderSize = 1.0f;
        l_Style.FrameBorderSize = 0.0f;
        l_Style.PopupBorderSize = 1.0f;
        l_Style.TabBorderSize = 0.0f;

        l_Style.WindowPadding = ImVec2(8.0f, 6.0f);
        l_Style.FramePadding = ImVec2(8.0f, 5.0f);
        l_Style.ItemSpacing = ImVec2(8.0f, 6.0f);
        l_Style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
        l_Style.CellPadding = ImVec2(6.0f, 4.0f);
        l_Style.IndentSpacing = 18.0f;
        l_Style.ScrollbarSize = 13.0f;
        l_Style.GrabMinSize = 9.0f;

        l_Style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
        l_Style.WindowMenuButtonPosition = ImGuiDir_None;

        const ImVec4 l_Accent = ImVec4(0.16f, 0.52f, 0.92f, 1.00f);
        const ImVec4 l_AccentHovered = ImVec4(0.26f, 0.62f, 1.00f, 1.00f);
        const ImVec4 l_AccentActive = ImVec4(0.12f, 0.42f, 0.78f, 1.00f);

        ImVec4* l_Colors = l_Style.Colors;
        l_Colors[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.88f, 1.00f);
        l_Colors[ImGuiCol_TextDisabled] = ImVec4(0.48f, 0.49f, 0.53f, 1.00f);
        l_Colors[ImGuiCol_WindowBg] = ImVec4(0.085f, 0.088f, 0.098f, 1.00f);
        l_Colors[ImGuiCol_ChildBg] = ImVec4(0.070f, 0.073f, 0.082f, 1.00f);
        l_Colors[ImGuiCol_PopupBg] = ImVec4(0.105f, 0.110f, 0.122f, 1.00f);
        l_Colors[ImGuiCol_Border] = ImVec4(0.180f, 0.190f, 0.215f, 1.00f);
        l_Colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.00f);
        l_Colors[ImGuiCol_FrameBg] = ImVec4(0.140f, 0.147f, 0.165f, 1.00f);
        l_Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.180f, 0.190f, 0.215f, 1.00f);
        l_Colors[ImGuiCol_FrameBgActive] = ImVec4(0.220f, 0.235f, 0.265f, 1.00f);
        l_Colors[ImGuiCol_TitleBg] = ImVec4(0.060f, 0.063f, 0.072f, 1.00f);
        l_Colors[ImGuiCol_TitleBgActive] = ImVec4(0.090f, 0.094f, 0.105f, 1.00f);
        l_Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.060f, 0.063f, 0.072f, 1.00f);
        l_Colors[ImGuiCol_MenuBarBg] = ImVec4(0.100f, 0.104f, 0.116f, 1.00f);
        l_Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.060f, 0.063f, 0.072f, 1.00f);
        l_Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.200f, 0.210f, 0.240f, 1.00f);
        l_Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.260f, 0.275f, 0.310f, 1.00f);
        l_Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.320f, 0.340f, 0.380f, 1.00f);
        l_Colors[ImGuiCol_CheckMark] = l_Accent;
        l_Colors[ImGuiCol_SliderGrab] = l_Accent;
        l_Colors[ImGuiCol_SliderGrabActive] = l_AccentActive;
        l_Colors[ImGuiCol_Button] = ImVec4(0.160f, 0.168f, 0.190f, 1.00f);
        l_Colors[ImGuiCol_ButtonHovered] = ImVec4(0.210f, 0.225f, 0.255f, 1.00f);
        l_Colors[ImGuiCol_ButtonActive] = ImVec4(0.250f, 0.270f, 0.310f, 1.00f);
        l_Colors[ImGuiCol_Header] = ImVec4(0.120f, 0.330f, 0.580f, 1.00f);
        l_Colors[ImGuiCol_HeaderHovered] = ImVec4(0.160f, 0.420f, 0.720f, 1.00f);
        l_Colors[ImGuiCol_HeaderActive] = ImVec4(0.180f, 0.460f, 0.800f, 1.00f);
        l_Colors[ImGuiCol_Separator] = ImVec4(0.180f, 0.190f, 0.215f, 1.00f);
        l_Colors[ImGuiCol_SeparatorHovered] = l_AccentHovered;
        l_Colors[ImGuiCol_SeparatorActive] = l_AccentActive;
        l_Colors[ImGuiCol_ResizeGrip] = ImVec4(0.200f, 0.210f, 0.240f, 0.60f);
        l_Colors[ImGuiCol_ResizeGripHovered] = l_AccentHovered;
        l_Colors[ImGuiCol_ResizeGripActive] = l_AccentActive;
        l_Colors[ImGuiCol_Tab] = ImVec4(0.090f, 0.094f, 0.105f, 1.00f);
        l_Colors[ImGuiCol_TabHovered] = ImVec4(0.180f, 0.300f, 0.460f, 1.00f);
        l_Colors[ImGuiCol_TabActive] = ImVec4(0.140f, 0.165f, 0.205f, 1.00f);
        l_Colors[ImGuiCol_TabUnfocused] = ImVec4(0.080f, 0.083f, 0.092f, 1.00f);
        l_Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.110f, 0.120f, 0.140f, 1.00f);
        l_Colors[ImGuiCol_DockingPreview] = ImVec4(l_Accent.x, l_Accent.y, l_Accent.z, 0.55f);
        l_Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.060f, 0.063f, 0.072f, 1.00f);
        l_Colors[ImGuiCol_PlotLines] = l_Accent;
        l_Colors[ImGuiCol_PlotLinesHovered] = l_AccentHovered;
        l_Colors[ImGuiCol_PlotHistogram] = l_Accent;
        l_Colors[ImGuiCol_PlotHistogramHovered] = l_AccentHovered;
        l_Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.120f, 0.125f, 0.140f, 1.00f);
        l_Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.180f, 0.190f, 0.215f, 1.00f);
        l_Colors[ImGuiCol_TableBorderLight] = ImVec4(0.140f, 0.147f, 0.165f, 1.00f);
        l_Colors[ImGuiCol_TableRowBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.00f);
        l_Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.000f, 1.000f, 1.000f, 0.020f);
        l_Colors[ImGuiCol_TextSelectedBg] = ImVec4(l_Accent.x, l_Accent.y, l_Accent.z, 0.35f);
        l_Colors[ImGuiCol_DragDropTarget] = l_AccentHovered;
        l_Colors[ImGuiCol_NavHighlight] = l_Accent;
        l_Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 1.000f, 1.000f, 0.70f);
        l_Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.200f, 0.200f, 0.200f, 0.20f);
        l_Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.040f, 0.040f, 0.050f, 0.55f);
    }

    static void ApplyFonts(FileSystem& fileSystem, float scale)
    {
        ImGuiIO& l_IO = ImGui::GetIO();
        float l_FontSize = k_BaseFontSize * scale;
        std::filesystem::path l_FontsDir = fileSystem.Resolve(BaseDirectory::Executable, "Assets/Fonts");

        std::filesystem::path l_UIPath = l_FontsDir / k_UIFontFile;
        if (std::filesystem::exists(l_UIPath))
        {
            ImFontConfig l_UIConfig;
            l_UIConfig.OversampleH = 2;
            l_UIConfig.OversampleV = 2;
            l_IO.Fonts->AddFontFromFileTTF(l_UIPath.string().c_str(), l_FontSize, &l_UIConfig);
        }
        else
        {
            TR_WARN("EditorTheme: UI font '{}' not found, using default font", l_UIPath.string());
            l_IO.Fonts->AddFontDefault();
        }

        std::filesystem::path l_IconPath = l_FontsDir / k_IconFontFile;
        if (std::filesystem::exists(l_IconPath))
        {
            static const ImWchar l_IconRanges[] = { k_IconRangeMin, k_IconRangeMax, 0 };
            ImFontConfig l_IconConfig;
            l_IconConfig.MergeMode = true;
            l_IconConfig.PixelSnapH = true;
            l_IconConfig.GlyphMinAdvanceX = l_FontSize;
            l_IO.Fonts->AddFontFromFileTTF(l_IconPath.string().c_str(), l_FontSize, &l_IconConfig, l_IconRanges);
        }
        else
        {
            TR_WARN("EditorTheme: icon font '{}' not found, icons disabled", l_IconPath.string());
        }
    }

    void EditorTheme::Apply(FileSystem& fileSystem)
    {
        float l_Scale = k_UserScale;
        if (Application::Get().HasWindow())
        {
            l_Scale *= Application::Get().GetWindow().GetContentScale();
        }

        if (l_Scale < 1.0f)
        {
            l_Scale = 1.0f;
        }

        ApplyStyle();
        ImGui::GetStyle().ScaleAllSizes(l_Scale);
        ApplyFonts(fileSystem, l_Scale);

        TR_INFO("EditorTheme: applied (scale {:.2f})", l_Scale);
    }
}