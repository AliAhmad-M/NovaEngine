#pragma once
#include <imgui.h>

namespace Nova::Editor::Theme
{
    inline ImVec4 Rgb(int r, int g, int b, float a = 1.0f)
    {
        return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
    }

    inline ImU32 U32(const ImVec4& c, float alphaMul = 1.0f)
    {
        ImVec4 v = c;
        v.w *= alphaMul;
        return ImGui::ColorConvertFloat4ToU32(v);
    }

    // Surfaces
    inline ImVec4 Sunken() { return Rgb(15, 16, 19); }      // inputs, wells, panels
    inline ImVec4 Base() { return Rgb(21, 22, 27); }        // window background
    inline ImVec4 Raised() { return Rgb(30, 32, 39); }      // cards, popups
    inline ImVec4 RaisedHover() { return Rgb(38, 41, 50); }

    // Lines
    inline ImVec4 Border() { return Rgb(44, 47, 56); }
    inline ImVec4 BorderStrong() { return Rgb(66, 71, 85); }

    // Accent

    inline ImVec4 Accent() { return Rgb(79, 140, 255); }
    inline ImVec4 AccentHover() { return Rgb(112, 163, 255); }
    inline ImVec4 AccentActive() { return Rgb(52, 112, 224); }
    inline ImVec4 Danger() { return Rgb(239, 98, 98); }
    inline ImVec4 DangerHover() { return Rgb(246, 128, 128); }
    inline ImVec4 DangerActive() { return Rgb(204, 78, 78); }

    // Text

    inline ImVec4 Text() { return Rgb(226, 230, 238); }
    inline ImVec4 TextDim() { return Rgb(116, 121, 134); }

    inline void Apply()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // Rounding
        
        style.WindowRounding = 0.0f;
        style.ChildRounding = 4.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 6.0f;
        style.GrabRounding = 3.0f;
        style.ScrollbarRounding = 9.0f;
        style.TabRounding = 4.0f;

        // Sizing
        
        style.WindowPadding = ImVec2(10.0f, 10.0f);
        style.FramePadding = ImVec2(8.0f, 5.0f);
        style.ItemSpacing = ImVec2(8.0f, 6.0f);
        style.IndentSpacing = 18.0f;
        style.ScrollbarSize = 12.0f;
        style.GrabMinSize = 10.0f;

        // Borders
        
        style.WindowBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.TabBorderSize = 0.0f;

        style.WindowMenuButtonPosition = ImGuiDir_None;
        style.WindowTitleAlign = ImVec2(0.0f, 0.5f);

        style.AntiAliasedLines = true;
        style.AntiAliasedLinesUseTex = true;
        style.AntiAliasedFill = true;

        const ImVec4 accent = Accent();
        const ImVec4 accentHover = AccentHover();
        const ImVec4 accentActive = AccentActive();

        ImVec4* c = style.Colors;

        // Surfaces
        
        c[ImGuiCol_WindowBg] = Base();
        c[ImGuiCol_ChildBg] = Sunken();
        c[ImGuiCol_PopupBg] = Rgb(30, 32, 39, 0.98f);

        c[ImGuiCol_Border] = Border();
        c[ImGuiCol_BorderShadow] = Rgb(0, 0, 0, 0.0f);

        // Inputs
        
        c[ImGuiCol_FrameBg] = Sunken();
        c[ImGuiCol_FrameBgHovered] = Rgb(24, 26, 32);
        c[ImGuiCol_FrameBgActive] = Rgb(30, 33, 41);

        // Title / menu
        
        c[ImGuiCol_TitleBg] = Rgb(17, 18, 22);
        c[ImGuiCol_TitleBgActive] = Rgb(21, 22, 27);
        c[ImGuiCol_TitleBgCollapsed] = Rgb(17, 18, 22);
        c[ImGuiCol_MenuBarBg] = Rgb(17, 18, 22);

        // Scrollbar
        
        c[ImGuiCol_ScrollbarBg] = Rgb(0, 0, 0, 0.0f);
        c[ImGuiCol_ScrollbarGrab] = Rgb(54, 57, 67);
        c[ImGuiCol_ScrollbarGrabHovered] = Rgb(76, 80, 93);
        c[ImGuiCol_ScrollbarGrabActive] = Rgb(96, 101, 116);

        // Accent controls
        
        c[ImGuiCol_CheckMark] = accent;
        c[ImGuiCol_SliderGrab] = accent;
        c[ImGuiCol_SliderGrabActive] = accentHover;

        // Buttons
        
        c[ImGuiCol_Button] = Rgb(38, 41, 50);
        c[ImGuiCol_ButtonHovered] = Rgb(50, 54, 65);
        c[ImGuiCol_ButtonActive] = accentActive;

        // Headers / selectables / menu items
        
        c[ImGuiCol_Header] = Rgb(79, 140, 255, 0.20f);
        c[ImGuiCol_HeaderHovered] = Rgb(255, 255, 255, 0.06f);
        c[ImGuiCol_HeaderActive] = Rgb(79, 140, 255, 0.38f);

        // Separators / grips
        
        c[ImGuiCol_Separator] = Rgb(38, 41, 49);
        c[ImGuiCol_SeparatorHovered] = accent;
        c[ImGuiCol_SeparatorActive] = accentHover;

        c[ImGuiCol_ResizeGrip] = Rgb(79, 140, 255, 0.15f);
        c[ImGuiCol_ResizeGripHovered] = Rgb(79, 140, 255, 0.55f);
        c[ImGuiCol_ResizeGripActive] = Rgb(79, 140, 255, 0.85f);

        // Tabs
        
        c[ImGuiCol_Tab] = Rgb(18, 19, 23);
        c[ImGuiCol_TabHovered] = Rgb(40, 43, 52);
        c[ImGuiCol_TabActive] = Rgb(30, 32, 39);
        c[ImGuiCol_TabUnfocused] = Rgb(17, 18, 21);
        c[ImGuiCol_TabUnfocusedActive] = Rgb(25, 27, 33);

        // Docking
        
        c[ImGuiCol_DockingPreview] = Rgb(79, 140, 255, 0.35f);
        c[ImGuiCol_DockingEmptyBg] = Rgb(14, 15, 18);

        // Text
        
        c[ImGuiCol_Text] = Text();
        c[ImGuiCol_TextDisabled] = TextDim();
        c[ImGuiCol_TextSelectedBg] = Rgb(79, 140, 255, 0.35f);
        c[ImGuiCol_NavHighlight] = accent;

        c[ImGuiCol_ModalWindowDimBg] = Rgb(0, 0, 0, 0.55f);

        // Tables

        c[ImGuiCol_TableHeaderBg] = Rgb(25, 27, 32);
        c[ImGuiCol_TableBorderStrong] = Border();
        c[ImGuiCol_TableBorderLight] = Rgb(30, 32, 38);
        c[ImGuiCol_TableRowBg] = Rgb(0, 0, 0, 0.0f);
        c[ImGuiCol_TableRowBgAlt] = Rgb(255, 255, 255, 0.02f);
    }
}