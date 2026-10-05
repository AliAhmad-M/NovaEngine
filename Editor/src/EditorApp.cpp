#include <Editor/EditorApp.h>
#include <Editor/Panels/HierarchyPanel.h>
#include <Editor/Panels/InspectorPanel.h>
#include <Editor/Panels/AssetBrowserPanel.h>
#include <Editor/Panels/ScenePanel.h>
#include <Editor/FilePicker.h>
#include <Runtime/Engine.h>
#include <Graphics/Systems.h>
#include <Physics/Systems.h>
#include <World/Systems.h>
#include <Core/Services.h>
#include <Core/Systems.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include <GLFW/glfw3.h>
#include <cstdio>
#include <filesystem>

using namespace Nova;
using namespace Nova::Editor;

static void applyTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Rounding
    style.WindowRounding = 0.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarRounding = 12.0f;
    style.TabRounding = 6.0f;

    // Sizing
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 10.0f;
    style.GrabMinSize = 10.0f;

    // Borders
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);

    // Palette
    auto rgb = [](int r, int g, int b, float a = 1.0f)
        {
            return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
        };

    const ImVec4 accent = rgb(66, 150, 255);
    const ImVec4 accentHover = rgb(92, 168, 255);
    const ImVec4 accentActive = rgb(46, 128, 235);

    ImVec4* c = style.Colors;

    // Surfaces
    c[ImGuiCol_WindowBg] = rgb(36, 38, 43);
    c[ImGuiCol_ChildBg] = rgb(31, 33, 37);
    c[ImGuiCol_PopupBg] = rgb(40, 42, 48);

    c[ImGuiCol_Border] = rgb(24, 25, 29);
    c[ImGuiCol_BorderShadow] = rgb(0, 0, 0, 0.0f);

    // Inputs
    c[ImGuiCol_FrameBg] = rgb(26, 27, 31);
    c[ImGuiCol_FrameBgHovered] = rgb(33, 35, 40);
    c[ImGuiCol_FrameBgActive] = rgb(40, 43, 50);

    // Title
    c[ImGuiCol_TitleBg] = rgb(26, 27, 31);
    c[ImGuiCol_TitleBgActive] = rgb(31, 33, 37);
    c[ImGuiCol_TitleBgCollapsed] = rgb(26, 27, 31);
    c[ImGuiCol_MenuBarBg] = rgb(31, 33, 37);

    // Scrollbar
    c[ImGuiCol_ScrollbarBg] = rgb(0, 0, 0, 0.0f);
    c[ImGuiCol_ScrollbarGrab] = rgb(70, 74, 84);
    c[ImGuiCol_ScrollbarGrabHovered] = rgb(90, 95, 107);
    c[ImGuiCol_ScrollbarGrabActive] = rgb(110, 116, 130);

    // Accent controls
    c[ImGuiCol_CheckMark] = accent;
    c[ImGuiCol_SliderGrab] = accent;
    c[ImGuiCol_SliderGrabActive] = accentHover;

    // Buttons
    c[ImGuiCol_Button] = rgb(58, 62, 71);
    c[ImGuiCol_ButtonHovered] = rgb(72, 77, 88);
    c[ImGuiCol_ButtonActive] = accentActive;

    // Headers
    c[ImGuiCol_Header] = rgb(66, 150, 255, 0.32f);
    c[ImGuiCol_HeaderHovered] = rgb(255, 255, 255, 0.07f);
    c[ImGuiCol_HeaderActive] = rgb(66, 150, 255, 0.50f);

    // Separators / docking splitters
    c[ImGuiCol_Separator] = rgb(24, 25, 29);
    c[ImGuiCol_SeparatorHovered] = accent;
    c[ImGuiCol_SeparatorActive] = accentHover;

    c[ImGuiCol_ResizeGrip] = rgb(66, 150, 255, 0.15f);
    c[ImGuiCol_ResizeGripHovered] = rgb(66, 150, 255, 0.60f);
    c[ImGuiCol_ResizeGripActive] = rgb(66, 150, 255, 0.90f);

    // Tabs
    c[ImGuiCol_Tab] = rgb(31, 33, 37);
    c[ImGuiCol_TabHovered] = rgb(58, 62, 71);
    c[ImGuiCol_TabActive] = rgb(46, 49, 56);
    c[ImGuiCol_TabUnfocused] = rgb(31, 33, 37);
    c[ImGuiCol_TabUnfocusedActive] = rgb(40, 43, 49);

    // Docking
    c[ImGuiCol_DockingPreview] = rgb(66, 150, 255, 0.45f);
    c[ImGuiCol_DockingEmptyBg] = rgb(22, 23, 26);

    // Text
    c[ImGuiCol_Text] = rgb(224, 227, 233);
    c[ImGuiCol_TextDisabled] = rgb(120, 126, 140);
    c[ImGuiCol_TextSelectedBg] = rgb(66, 150, 255, 0.35f);
    c[ImGuiCol_NavHighlight] = accent;
}

EditorApp::EditorApp() = default;
EditorApp::~EditorApp() = default;

bool EditorApp::Init()
{
    // Init runtime
    m_runtime.Init();
    m_nativeWindow = Services::Fetch<Window>()->GetNativeWindow();

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigDragClickToInputText = true;

    ImFont* font = io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_18pt-Regular.ttf", 20.0f);
    if (!font)
    {
        Debug::LogError("Failed to load editor font, using default");
        io.Fonts->AddFontDefault();
    }

    ImGui_ImplGlfw_InitForOpenGL(m_nativeWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    applyTheme();

    io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_18pt-Regular.ttf", 18.0f);

    // Load panels
    m_hierarchyPanel = std::make_unique<HierarchyPanel>(&m_ctx);
    m_inspectorPanel = std::make_unique<InspectorPanel>(&m_ctx);
    m_assetBrowserPanel = std::make_unique<AssetBrowserPanel>(&m_ctx);
    m_scenePanel = std::make_unique<ScenePanel>(&m_ctx);
    m_inputMapPanel = std::make_unique<InputMapPanel>(&m_ctx);

    // Initialize scene serializer
    m_sceneSerializer = std::make_unique<SceneSerializer>();

    // Set scene panel reference in context for dirty marking
    m_ctx.scenePanelRef = m_scenePanel.get();

    m_ctx.onSaveScene = [this]() { saveActiveScene(); };

    return true;
}

void EditorApp::Run()
{
    while (m_running && !glfwWindowShouldClose(m_nativeWindow))
    {
        glfwPollEvents();

        beginFrame();
        drawMenuBar();   // draw menu bar
        buildLayout();   // then dockspace

        ImGuiIO& io = ImGui::GetIO();

        // Save scene
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
            saveActiveScene();

        // New scene
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N))
            m_scenePanel->newScene();

        // Open scene shortcut (menu item sets the same flag)
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O))
            m_openScenePicker = true;

        // Draw panels
        m_hierarchyPanel->Draw();
        m_inspectorPanel->Draw();
        m_assetBrowserPanel->Draw();
        m_scenePanel->Draw();

        if (m_showInputMap)
            m_inputMapPanel->Draw(m_showInputMap);

        // Open scene picker: open once, draw once per frame
        if (m_openScenePicker)
        {
            ImGui::OpenPopup("##OpenScenePicker");
            m_openScenePicker = false;
        }

        DrawFilePicker("##OpenScenePicker", m_scenesPath, ".nova",
            [this](const std::string& path)
            {
                m_scenePanel->OpenSceneTab(path);
            });

        // Render scene into FBO
        int vpW = m_scenePanel->GetSceneViewportWidth();
        int vpH = m_scenePanel->GetSceneViewportHeight();
        if (vpW > 0 && vpH > 0)
        {
            Services::Register<Camera2D>(&m_scenePanel->GetEditorCamera());
            m_runtime.RenderEditorFrame(m_scenePanel->GetSceneFBOId(), vpW, vpH);
        }

        double currentTime = glfwGetTime();
        float dt = (float)(currentTime - m_lastFrameTime);
        m_lastFrameTime = currentTime;
        if (dt > 0.1f) dt = 1.0f / 60.0f;

        if (m_scenePanel->IsPlaying())
        {
            m_runtime.UpdateEditorGame(dt);

            int gvpW = m_scenePanel->GetGameViewportWidth();
            int gvpH = m_scenePanel->GetGameViewportHeight();
            if (gvpW > 0 && gvpH > 0)
            {
                m_runtime.RenderEditorFrame(m_scenePanel->GetGameFBOId(), gvpW, gvpH);
            }
        }

        endFrame();
    }
}

void EditorApp::Shutdown()
{
    m_runtime.Close();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
}

void EditorApp::beginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void EditorApp::endFrame()
{
    ImGui::Render();

    int w, h;
    glfwGetFramebufferSize(m_nativeWindow, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_nativeWindow);
}

void EditorApp::buildLayout()
{
    // Fullscreen host window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("##DockSpaceHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockID = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockID, ImVec2(0, 0), ImGuiDockNodeFlags_None);

    // Build default layout only once
    if (!m_layoutInitialized)
    {
        m_layoutInitialized = true;

        ImGui::DockBuilderRemoveNode(dockID);
        ImGui::DockBuilderAddNode(dockID, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockID, viewport->WorkSize);

        ImGuiID dockLeft = 0;
        ImGuiID dockRight = 0;
        ImGuiID dockBottom = 0;
        ImGuiID dockCenter = 0;

        // Step 1 - split right (Inspector, full height)
        ImGui::DockBuilderSplitNode(dockID, ImGuiDir_Right, kInspectorWidthRatio, &dockRight, &dockCenter);

        // Step 2 - split bottom (Asset Browser, spans Hierarchy + Scene)
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Down, kAssetBrowserHeightRatio, &dockBottom, &dockCenter);

        // Step 3 - split left (Hierarchy) from remaining top area
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Left, kHierarchyWidthRatio, &dockLeft, &dockCenter);

        // Assign panels - names must match ImGui::Begin() strings exactly
        ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
        ImGui::DockBuilderDockWindow("Scene", dockCenter);
        ImGui::DockBuilderDockWindow("Inspector", dockRight);
        ImGui::DockBuilderDockWindow("Asset Browser", dockBottom);

        ImGui::DockBuilderFinish(dockID);
    }

    ImGui::End();
}

void EditorApp::drawMenuBar()
{
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Scene", "Ctrl+N"))
            m_scenePanel->newScene();

        if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
            m_openScenePicker = true;

        if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
            saveActiveScene();

        ImGui::Separator();

        if (ImGui::MenuItem("Exit"))
            m_running = false;

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Input Map"))
            m_showInputMap = true;

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        if (ImGui::MenuItem("Hierarchy", nullptr, true)) {}
        if (ImGui::MenuItem("Inspector", nullptr, true)) {}
        if (ImGui::MenuItem("Asset Browser", nullptr, true)) {}
        if (ImGui::MenuItem("Console", nullptr, true)) {}
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void EditorApp::saveActiveScene()
{
    if (!m_ctx.activeSceneRef || !m_sceneSerializer)
    {
        Debug::LogError("Cannot save: no active scene or serializer");
        return;
    }

    // Determine save path
    std::string savePath = m_ctx.activeScenePath;
    if (savePath.empty())
    {
        Debug::LogError("No valid save path");
        return;
    }

    // Save the scene
    if (m_sceneSerializer->Save(*m_ctx.activeSceneRef, savePath))
    {
        m_scenePanel->MarkSceneAsSaved();
        Debug::LogInfo("Scene saved to: %s", savePath.c_str());
    }
    else
    {
        Debug::LogError("Failed to save scene to: %s", savePath.c_str());
    }
}

void EditorApp::markSceneDirty()
{
    if (m_ctx.scenePanelRef)
    {
        m_ctx.scenePanelRef->MarkSceneAsDirty();
    }
}