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
    static constexpr const char* k_IconFontFile = "materialdesignicons-webfont.ttf";
    static constexpr float k_BaseFontSize = 16.0f;
    static constexpr float k_UserScale = 0.6f;

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

        const ImVec4 l_Accent = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
        const ImVec4 l_AccentHovered = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        const ImVec4 l_AccentActive = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);

        ImVec4* l_Colors = l_Style.Colors;
        l_Colors[ImGuiCol_Text] = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
        l_Colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        l_Colors[ImGuiCol_WindowBg] = ImVec4(0.070f, 0.070f, 0.070f, 1.00f);
        l_Colors[ImGuiCol_ChildBg] = ImVec4(0.055f, 0.055f, 0.055f, 1.00f);
        l_Colors[ImGuiCol_PopupBg] = ImVec4(0.100f, 0.100f, 0.100f, 1.00f);
        l_Colors[ImGuiCol_Border] = ImVec4(0.180f, 0.180f, 0.180f, 1.00f);
        l_Colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.00f);
        l_Colors[ImGuiCol_FrameBg] = ImVec4(0.135f, 0.135f, 0.135f, 1.00f);
        l_Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.175f, 0.175f, 0.175f, 1.00f);
        l_Colors[ImGuiCol_FrameBgActive] = ImVec4(0.210f, 0.210f, 0.210f, 1.00f);
        l_Colors[ImGuiCol_TitleBg] = ImVec4(0.050f, 0.050f, 0.050f, 1.00f);
        l_Colors[ImGuiCol_TitleBgActive] = ImVec4(0.100f, 0.100f, 0.100f, 1.00f);
        l_Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.050f, 0.050f, 0.050f, 1.00f);
        l_Colors[ImGuiCol_MenuBarBg] = ImVec4(0.110f, 0.110f, 0.110f, 1.00f);
        l_Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.050f, 0.050f, 0.050f, 1.00f);
        l_Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.220f, 0.220f, 0.220f, 1.00f);
        l_Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.00f);
        l_Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.380f, 0.380f, 0.380f, 1.00f);
        l_Colors[ImGuiCol_CheckMark] = l_Accent;
        l_Colors[ImGuiCol_SliderGrab] = l_Accent;
        l_Colors[ImGuiCol_SliderGrabActive] = l_AccentActive;
        l_Colors[ImGuiCol_Button] = ImVec4(0.150f, 0.150f, 0.150f, 1.00f);
        l_Colors[ImGuiCol_ButtonHovered] = ImVec4(0.210f, 0.210f, 0.210f, 1.00f);
        l_Colors[ImGuiCol_ButtonActive] = ImVec4(0.260f, 0.260f, 0.260f, 1.00f);
        l_Colors[ImGuiCol_Header] = ImVec4(0.240f, 0.240f, 0.240f, 1.00f);
        l_Colors[ImGuiCol_HeaderHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.00f);
        l_Colors[ImGuiCol_HeaderActive] = ImVec4(0.340f, 0.340f, 0.340f, 1.00f);
        l_Colors[ImGuiCol_Separator] = ImVec4(0.180f, 0.180f, 0.180f, 1.00f);
        l_Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.350f, 0.350f, 0.350f, 1.00f);
        l_Colors[ImGuiCol_SeparatorActive] = ImVec4(0.450f, 0.450f, 0.450f, 1.00f);
        l_Colors[ImGuiCol_ResizeGrip] = ImVec4(0.220f, 0.220f, 0.220f, 0.60f);
        l_Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.350f, 0.350f, 0.350f, 1.00f);
        l_Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.450f, 0.450f, 0.450f, 1.00f);
        l_Colors[ImGuiCol_Tab] = ImVec4(0.100f, 0.100f, 0.100f, 1.00f);
        l_Colors[ImGuiCol_TabHovered] = ImVec4(0.240f, 0.240f, 0.240f, 1.00f);
        l_Colors[ImGuiCol_TabActive] = ImVec4(0.170f, 0.170f, 0.170f, 1.00f);
        l_Colors[ImGuiCol_TabUnfocused] = ImVec4(0.080f, 0.080f, 0.080f, 1.00f);
        l_Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.130f, 0.130f, 0.130f, 1.00f);
        l_Colors[ImGuiCol_DockingPreview] = ImVec4(0.450f, 0.450f, 0.450f, 0.55f);
        l_Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.050f, 0.050f, 0.050f, 1.00f);
        l_Colors[ImGuiCol_PlotLines] = l_Accent;
        l_Colors[ImGuiCol_PlotLinesHovered] = l_AccentHovered;
        l_Colors[ImGuiCol_PlotHistogram] = l_Accent;
        l_Colors[ImGuiCol_PlotHistogramHovered] = l_AccentHovered;
        l_Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.120f, 0.120f, 0.120f, 1.00f);
        l_Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.180f, 0.180f, 0.180f, 1.00f);
        l_Colors[ImGuiCol_TableBorderLight] = ImVec4(0.130f, 0.130f, 0.130f, 1.00f);
        l_Colors[ImGuiCol_TableRowBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.00f);
        l_Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.000f, 1.000f, 1.000f, 0.020f);
        l_Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.450f, 0.450f, 0.450f, 0.35f);
        l_Colors[ImGuiCol_DragDropTarget] = l_AccentHovered;
        l_Colors[ImGuiCol_NavHighlight] = l_Accent;
        l_Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 1.000f, 1.000f, 0.70f);
        l_Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.200f, 0.200f, 0.200f, 0.20f);
        l_Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.55f);
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