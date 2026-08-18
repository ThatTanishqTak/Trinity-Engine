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
    // Unity ships the editor in Inter; the Nerd Font stays last so the editor still boots on a bare checkout.
    static constexpr const char* k_UIFontCandidates[] = { "Inter-Regular.ttf", "Inter-Medium.ttf", "Roboto-Regular.ttf", "JetBrainsMonoNerdFontMono-Regular.ttf" };
    static constexpr const char* k_IconFontFile = "materialdesignicons-webfont.ttf";
    // Unity's editor text is 12 px; 13 keeps it legible under ImGui's lighter hinting.
    static constexpr float k_BaseFontSize = 13.0f;
    static constexpr float k_UserScale = 1.0f;

    static void ApplyStyle()
    {
        TR_TRACE("Applying style");

        ImGuiStyle& l_Style = ImGui::GetStyle();

        // Unity is square and dense: only controls carry a radius, panels and docks are hard-edged.
        l_Style.WindowRounding = 0.0f;
        l_Style.ChildRounding = 0.0f;
        l_Style.FrameRounding = 3.0f;
        l_Style.PopupRounding = 3.0f;
        l_Style.ScrollbarRounding = 6.0f;
        l_Style.GrabRounding = 3.0f;
        l_Style.TabRounding = 4.0f;

        // Every Unity control is outlined with a near-black hairline instead of being separated by spacing.
        l_Style.WindowBorderSize = 0.0f;
        l_Style.ChildBorderSize = 1.0f;
        l_Style.FrameBorderSize = 1.0f;
        l_Style.PopupBorderSize = 1.0f;
        l_Style.TabBorderSize = 0.0f;
        l_Style.DockingSeparatorSize = 2.0f;

        l_Style.WindowPadding = ImVec2(4.0f, 4.0f);
        l_Style.FramePadding = ImVec2(6.0f, 3.0f);
        l_Style.ItemSpacing = ImVec2(4.0f, 3.0f);
        l_Style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        l_Style.CellPadding = ImVec2(4.0f, 2.0f);
        l_Style.IndentSpacing = 14.0f;
        l_Style.ScrollbarSize = 13.0f;
        l_Style.GrabMinSize = 10.0f;

        l_Style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
        l_Style.WindowMenuButtonPosition = ImGuiDir_None;
        l_Style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        l_Style.SelectableTextAlign = ImVec2(0.0f, 0.5f);
        l_Style.SeparatorTextBorderSize = 1.0f;

        auto l_Color = [](ImU32 packed) -> ImVec4
        {
            return EditorColors::ToVec4(packed);
        };

        ImVec4* l_Colors = l_Style.Colors;
        l_Colors[ImGuiCol_Text] = l_Color(EditorColors::DefaultText);
        l_Colors[ImGuiCol_TextDisabled] = l_Color(EditorColors::DisabledText);
        l_Colors[ImGuiCol_WindowBg] = l_Color(EditorColors::WindowBackground);
        l_Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        l_Colors[ImGuiCol_PopupBg] = l_Color(EditorColors::WindowBackground);
        l_Colors[ImGuiCol_Border] = l_Color(EditorColors::DefaultBorder);
        l_Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        // Unity sinks input fields below the panel and lifts buttons above it; both read against #383838.
        l_Colors[ImGuiCol_FrameBg] = l_Color(EditorColors::InputField);
        l_Colors[ImGuiCol_FrameBgHovered] = l_Color(EditorColors::InputField);
        l_Colors[ImGuiCol_FrameBgActive] = l_Color(EditorColors::InputField);
        l_Colors[ImGuiCol_TitleBg] = l_Color(EditorColors::DefaultBackground);
        l_Colors[ImGuiCol_TitleBgActive] = l_Color(EditorColors::DefaultBackground);
        l_Colors[ImGuiCol_TitleBgCollapsed] = l_Color(EditorColors::DefaultBackground);
        l_Colors[ImGuiCol_MenuBarBg] = l_Color(EditorColors::AppToolbar);
        l_Colors[ImGuiCol_ScrollbarBg] = l_Color(EditorColors::ScrollbarGroove);
        l_Colors[ImGuiCol_ScrollbarGrab] = l_Color(EditorColors::ScrollbarThumb);
        l_Colors[ImGuiCol_ScrollbarGrabHovered] = l_Color(EditorColors::ScrollbarThumbHover);
        l_Colors[ImGuiCol_ScrollbarGrabActive] = l_Color(EditorColors::ScrollbarThumbHover);
        l_Colors[ImGuiCol_CheckMark] = l_Color(EditorColors::DefaultText);
        l_Colors[ImGuiCol_SliderGrab] = l_Color(EditorColors::Button);
        l_Colors[ImGuiCol_SliderGrabActive] = l_Color(EditorColors::ButtonHover);
        l_Colors[ImGuiCol_Button] = l_Color(EditorColors::Button);
        l_Colors[ImGuiCol_ButtonHovered] = l_Color(EditorColors::ButtonHover);
        l_Colors[ImGuiCol_ButtonActive] = l_Color(EditorColors::ButtonPressed);
        // Tree rows, foldouts and menu items all share Unity's single selection blue.
        l_Colors[ImGuiCol_Header] = l_Color(EditorColors::Highlight);
        l_Colors[ImGuiCol_HeaderHovered] = l_Color(EditorColors::Highlight);
        l_Colors[ImGuiCol_HeaderActive] = l_Color(EditorColors::Highlight);
        l_Colors[ImGuiCol_Separator] = l_Color(EditorColors::DefaultBorder);
        l_Colors[ImGuiCol_SeparatorHovered] = l_Color(EditorColors::InputFieldBorderFocus);
        l_Colors[ImGuiCol_SeparatorActive] = l_Color(EditorColors::InputFieldBorderFocus);
        l_Colors[ImGuiCol_ResizeGrip] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        l_Colors[ImGuiCol_ResizeGripHovered] = l_Color(EditorColors::HighlightInactive);
        l_Colors[ImGuiCol_ResizeGripActive] = l_Color(EditorColors::Highlight);
        l_Colors[ImGuiCol_Tab] = l_Color(EditorColors::Tab);
        l_Colors[ImGuiCol_TabHovered] = l_Color(EditorColors::ToolbarHover);
        l_Colors[ImGuiCol_TabActive] = l_Color(EditorColors::TabChecked);
        l_Colors[ImGuiCol_TabUnfocused] = l_Color(EditorColors::Tab);
        l_Colors[ImGuiCol_TabUnfocusedActive] = l_Color(EditorColors::TabChecked);
        l_Colors[ImGuiCol_DockingPreview] = ImVec4(0.17f, 0.36f, 0.53f, 0.70f);
        l_Colors[ImGuiCol_DockingEmptyBg] = l_Color(EditorColors::DefaultBackground);
        l_Colors[ImGuiCol_PlotLines] = l_Color(EditorColors::HighlightText);
        l_Colors[ImGuiCol_PlotLinesHovered] = l_Color(EditorColors::HighlightText);
        l_Colors[ImGuiCol_PlotHistogram] = l_Color(EditorColors::HighlightText);
        l_Colors[ImGuiCol_PlotHistogramHovered] = l_Color(EditorColors::HighlightText);
        l_Colors[ImGuiCol_TableHeaderBg] = l_Color(EditorColors::Toolbar);
        l_Colors[ImGuiCol_TableBorderStrong] = l_Color(EditorColors::DefaultBorder);
        l_Colors[ImGuiCol_TableBorderLight] = l_Color(EditorColors::DefaultBorder);
        l_Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        l_Colors[ImGuiCol_TableRowBgAlt] = l_Color(EditorColors::AlternatedRow);
        l_Colors[ImGuiCol_TextSelectedBg] = l_Color(EditorColors::Highlight);
        l_Colors[ImGuiCol_DragDropTarget] = l_Color(EditorColors::HighlightText);
        l_Colors[ImGuiCol_NavHighlight] = l_Color(EditorColors::InputFieldBorderFocus);
        l_Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.70f);
        l_Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.35f);
        l_Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.55f);

        TR_TRACE("Style applied");
    }

    static void ApplyFonts(FileSystem& fileSystem, float scale)
    {
        TR_TRACE("Applying fonts");

        ImGuiIO& l_IO = ImGui::GetIO();
        float l_FontSize = k_BaseFontSize * scale;
        std::filesystem::path l_FontsDirectory = fileSystem.Resolve(BaseDirectory::Executable, "Assets/Fonts");

        bool l_Loaded = false;
        for (const char* it_Candidate : k_UIFontCandidates)
        {
            std::filesystem::path l_UIPath = l_FontsDirectory / it_Candidate;
            if (!std::filesystem::exists(l_UIPath))
            {
                continue;
            }

            ImFontConfig l_UIConfig;
            l_UIConfig.OversampleH = 2;
            l_UIConfig.OversampleV = 2;
            l_IO.Fonts->AddFontFromFileTTF(l_UIPath.string().c_str(), l_FontSize, &l_UIConfig);

            TR_TRACE("UI font loaded: {}", it_Candidate);
            l_Loaded = true;

            break;
        }

        if (!l_Loaded)
        {
            TR_WARN("No UI font found in {}, reverting to default", l_FontsDirectory.string());

            l_IO.Fonts->AddFontDefault();
        }

        std::filesystem::path l_IconPath = l_FontsDirectory / k_IconFontFile;
        if (std::filesystem::exists(l_IconPath))
        {
            static const ImWchar l_IconRanges[] = { k_IconRangeMin, k_IconRangeMax, 0 };
            ImFontConfig l_IconConfig;
            l_IconConfig.MergeMode = true;
            l_IconConfig.PixelSnapH = true;
            l_IconConfig.GlyphMinAdvanceX = l_FontSize;
            // Drop the glyphs a hair so they sit on the text baseline like Unity's icon set.
            l_IconConfig.GlyphOffset = ImVec2(0.0f, 1.0f);
            l_IO.Fonts->AddFontFromFileTTF(l_IconPath.string().c_str(), l_FontSize, &l_IconConfig, l_IconRanges);
        }
        else
        {
            TR_WARN("Failed to load icon pack {} at {}", k_IconFontFile, l_IconPath.string());
        }

        TR_TRACE("Fonts applied: {}", k_IconFontFile);
    }

    void EditorTheme::Apply(FileSystem& fileSystem)
    {
        TR_INFO("APPLYING EDITOR THEME");

        float l_Scale = k_UserScale;
        if (Application::Get().HasWindow())
        {
            l_Scale *= Application::Get().GetWindow().GetContentScale();
        }

        if (l_Scale < 0.5f)
        {
            l_Scale = 0.5f;
        }

        ApplyStyle();
        ImGui::GetStyle().ScaleAllSizes(l_Scale);
        ApplyFonts(fileSystem, l_Scale);

        TR_INFO("EDITOR THEME APPLIED");
    }

    float EditorTheme::MenuBarHeight()
    {
        return ImGui::GetFrameHeight() + 2.0f;
    }

    float EditorTheme::ToolBarHeight()
    {
        return ImGui::GetFrameHeight() + 9.0f;
    }

    float EditorTheme::StatusBarHeight()
    {
        return ImGui::GetFrameHeight() + 2.0f;
    }

    bool EditorTheme::BeginChromeBar(const char* id, float y, float height, ImU32 background, const ImVec2& padding, ImGuiWindowFlags extraFlags)
    {
        ImGuiViewport* l_Viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(ImVec2(l_Viewport->Pos.x, l_Viewport->Pos.y + y));
        ImGui::SetNextWindowSize(ImVec2(l_Viewport->Size.x, height));
        ImGui::SetNextWindowViewport(l_Viewport->ID);

        ImGuiWindowFlags l_Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings | extraFlags;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, EditorColors::ToVec4(background));

        bool l_Open = ImGui::Begin(id, nullptr, l_Flags);

        // Unity separates every chrome strip from the next with a single near-black hairline.
        ImVec2 l_Position = ImGui::GetWindowPos();
        ImVec2 l_Size = ImGui::GetWindowSize();
        ImGui::GetWindowDrawList()->AddLine(ImVec2(l_Position.x, l_Position.y + l_Size.y - 1.0f), ImVec2(l_Position.x + l_Size.x, l_Position.y + l_Size.y - 1.0f), EditorColors::DefaultBorder);

        return l_Open;
    }

    void EditorTheme::EndChromeBar()
    {
        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(5);
    }

    bool EditorTheme::ToolBarButton(const char* label, bool checked, const char* tooltip, bool enabled, float width, ImU32 checkedBackground)
    {
        float l_Height = ImGui::GetFrameHeight();
        ImVec2 l_Size = ImVec2(width > 0.0f ? width : l_Height + 6.0f, l_Height);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, checked ? EditorColors::ToVec4(checkedBackground) : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, checked ? EditorColors::ToVec4(checkedBackground) : EditorColors::ToVec4(EditorColors::AppToolbarHover));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, EditorColors::ToVec4(EditorColors::AppToolbarChecked));

        ImGui::BeginDisabled(!enabled);
        bool l_Pressed = ImGui::Button(label, l_Size);
        ImGui::EndDisabled();

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        if (tooltip != nullptr && enabled && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", tooltip);
        }

        return l_Pressed;
    }

    void EditorTheme::ToolBarSeparator()
    {
        float l_Height = ImGui::GetFrameHeight();

        ImGui::SameLine(0.0f, 6.0f);
        ImVec2 l_Position = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(ImVec2(l_Position.x, l_Position.y + 2.0f), ImVec2(l_Position.x, l_Position.y + l_Height - 2.0f), EditorColors::DefaultBorder);
        ImGui::SameLine(0.0f, 6.0f);
    }

    void EditorTheme::BeginPanelToolBar(const char* id)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, EditorColors::ToVec4(EditorColors::Toolbar));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 2.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));

        ImGui::BeginChild(id, ImVec2(0.0f, ImGui::GetFrameHeight() + 5.0f), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    }

    void EditorTheme::EndPanelToolBar()
    {
        ImVec2 l_Position = ImGui::GetWindowPos();
        ImVec2 l_Size = ImGui::GetWindowSize();
        ImGui::GetWindowDrawList()->AddLine(ImVec2(l_Position.x, l_Position.y + l_Size.y - 1.0f), ImVec2(l_Position.x + l_Size.x, l_Position.y + l_Size.y - 1.0f), EditorColors::DefaultBorder);

        ImGui::EndChild();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    bool EditorTheme::SearchField(const char* id, char* buffer, size_t bufferSize, float width)
    {
        ImGui::SetNextItemWidth(width);

        return ImGui::InputTextWithHint(id, ICON_TR_SEARCH "  Search", buffer, bufferSize);
    }
}