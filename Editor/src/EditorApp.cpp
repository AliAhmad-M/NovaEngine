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
    style.ChildRounding = 0.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.PopupRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.TabRounding = 2.0f;

    // Sizing
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.IndentSpacing = 16.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 8.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    // Colors
    ImVec4* c = style.Colors;

    // Backgrounds
    c[ImGuiCol_WindowBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    c[ImGuiCol_ChildBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    c[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);

    // Borders
    c[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    c[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Frame (inputs, checkboxes)
    c[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);

    // Title bars
    c[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);

    // Menu bar
    c[ImGuiCol_MenuBarBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);

    // Scrollbar
    c[ImGuiCol_ScrollbarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);

    // Checkmark and slider
    c[ImGuiCol_CheckMark] = ImVec4(0.44f, 0.70f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.44f, 0.70f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.78f, 1.00f, 1.00f);

    // Buttons
    c[ImGuiCol_Button] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.44f, 0.70f, 1.00f, 1.00f);

    // Headers (TreeNode, CollapsingHeader, Selectable)
    c[ImGuiCol_Header] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.44f, 0.70f, 1.00f, 1.00f);

    // Separator
    c[ImGuiCol_Separator] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    c[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.70f, 1.00f, 1.00f);
    c[ImGuiCol_SeparatorActive] = ImVec4(0.44f, 0.70f, 1.00f, 1.00f);

    // Resize grip
    c[ImGuiCol_ResizeGrip] = ImVec4(0.44f, 0.70f, 1.00f, 0.20f);
    c[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.70f, 1.00f, 0.67f);
    c[ImGuiCol_ResizeGripActive] = ImVec4(0.44f, 0.70f, 1.00f, 0.95f);

    // Tabs
    c[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    c[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    c[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    c[ImGuiCol_TabUnfocused] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);

    // Docking
    c[ImGuiCol_DockingPreview] = ImVec4(0.44f, 0.70f, 1.00f, 0.70f);
    c[ImGuiCol_DockingEmptyBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);

    // Text
    c[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    // Highlight
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.44f, 0.70f, 1.00f, 0.35f);
    c[ImGuiCol_NavHighlight] = ImVec4(0.44f, 0.70f, 1.00f, 1.00f);
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

    ImGui_ImplGlfw_InitForOpenGL(m_nativeWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

	applyTheme();

	io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_18pt-Regular.ttf", 18.0f);

	// Load panels
	m_hierarchyPanel = std::make_unique<HierarchyPanel>(&m_ctx);
	m_inspectorPanel = std::make_unique<InspectorPanel>(&m_ctx);
	m_assetBrowserPanel = std::make_unique<AssetBrowserPanel>(&m_ctx);
	m_scenePanel = std::make_unique<ScenePanel>(&m_ctx);

	// Initialize scene serializer
	m_sceneSerializer = std::make_unique<SceneSerializer>();
    Services::Register(m_sceneSerializer.get());

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
        {
            saveActiveScene();
        }

        // Draw panels
        m_hierarchyPanel->Draw();
        m_inspectorPanel->Draw();
        m_assetBrowserPanel->Draw();
        m_scenePanel->Draw();

        // Open scene
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O)) 
        {
            m_openSceneRequested = true;
        }

        if (m_openSceneRequested)
        {
            ImGui::OpenPopup("##OpenScenePicker");
            m_openSceneRequested = false;
        }

        std::string scenesPath = "../Test/project/scenes";
        DrawFilePicker("##OpenScenePicker", scenesPath, ".nova",
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

        ImGui::DockBuilderSplitNode(dockID, ImGuiDir_Right, kInspectorWidthRatio, &dockRight, &dockCenter);
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Down, kAssetBrowserHeightRatio, &dockBottom, &dockCenter);
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Left, kHierarchyWidthRatio, &dockLeft, &dockCenter);

        // Assign panels
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
    if (ImGui::BeginMainMenuBar())
    {
        // File menu
        if (ImGui::BeginMenu("File"))
        {
            bool playing = m_scenePanel->IsPlaying();

            if (playing) ImGui::BeginDisabled();

            if (ImGui::MenuItem("New Scene", "Ctrl+N")) { m_ctx.activeScenePath.clear(); }
            if (ImGui::MenuItem("Open Scene", "Ctrl+O")) { m_openSceneRequested = true; }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) { saveActiveScene(); }

            if (playing) ImGui::EndDisabled();

            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) m_running = false;
            ImGui::EndMenu();
        }

        // Edit menu
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) { /* TODO */ }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) { /* TODO */ }
            ImGui::EndMenu();
        }

        // View menu – toggle panels
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Hierarchy")) { /* TODO */ }
            if (ImGui::MenuItem("Inspector")) { /* TODO */ }
            if (ImGui::MenuItem("Asset Browser")) { /* TODO */ }
            if (ImGui::MenuItem("Scene")) { /* TODO */ }
            ImGui::EndMenu();
        }

        // Display active scene path on the right side
        if (!m_ctx.activeScenePath.empty())
        {
            float textWidth = ImGui::CalcTextSize(m_ctx.activeScenePath.c_str()).x;
            float menuBarWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + menuBarWidth - textWidth - 8.0f);
            ImGui::TextDisabled("%s", m_ctx.activeScenePath.c_str());
        }

        ImGui::EndMainMenuBar();
    }
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
