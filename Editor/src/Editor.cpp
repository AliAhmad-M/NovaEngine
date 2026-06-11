#include <Editor/Editor.h>
#include <Editor/Panels/HierarchyPanel.h>
#include <Editor/Panels/InspectorPanel.h>
#include <Editor/Panels/AssetBrowserPanel.h>
#include <Editor/Panels/ScenePanel.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#include <cstdio>

Editor::Editor() = default;
Editor::~Editor() = default;

bool Editor::Init()
{
    // Initialize OpenGL
    if (!glfwInit())
    {
        fprintf(stderr, "[Editor] glfwInit failed\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(1600, 900, "Star Platinum Editor", nullptr, nullptr);
    if (!m_window)
    {
        fprintf(stderr, "[Editor] Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "[Editor] Failed to initialize GLAD\n");
        return false;
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Load panels
    m_hierarchyPanel = std::make_unique<HierarchyPanel>(&m_ctx);
    m_inspectorPanel = std::make_unique<InspectorPanel>(&m_ctx);
    m_assetBrowserPanel = std::make_unique<AssetBrowserPanel>(&m_ctx);
    m_scenePanel = std::make_unique<ScenePanel>(&m_ctx);

    return true;
}

void Editor::Run()
{
    while (m_running && !glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        beginFrame();
        buildLayout();
        drawMenuBar();

        m_hierarchyPanel->Draw();
        m_inspectorPanel->Draw();
        m_assetBrowserPanel->Draw();
        m_scenePanel->Draw();

        endFrame();
    }
}

void Editor::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Editor::beginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Editor::endFrame()
{
    ImGui::Render();

    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
}

void Editor::buildLayout()
{
    // -----------------------------------------------------------------------
    // Fixed grid layout — all panels are locked in position/size every frame.
    // Panels cannot be moved or resized by the user (NoMove + NoResize).
    //
    //  +----------------+----------------------------+---------------+
    //  |                |                            |               |
    //  |   Hierarchy    |          Scene             |   Inspector   |
    //  |                |                            |               |
    //  +----------------+                            +---------------+
    //  |                |                            |               |
    //  |  (lower-left   |                            |  (lower-right |
    //  |   reserved)    +----------------------------+   reserved)   |
    //  |                |       Asset Browser        |               |
    //  +----------------+----------------------------+---------------+
    //
    // -----------------------------------------------------------------------

    int fbW, fbH;
    glfwGetFramebufferSize(m_window, &fbW, &fbH);

    const float W = (float)fbW;
    const float H = (float)fbH;

    // Reserve space for the ImGui main menu bar (drawn separately, ~19 px)
    const float menuH = ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.y * 2.0f;
    const float topY = menuH;
    const float workH = H - topY;   // total height below the menu bar

    const float hierW = W * kHierarchyWidthRatio;
    const float inspW = W * kInspectorWidthRatio;
    const float sceneW = W - hierW - inspW;

    const float assetH = workH * kAssetBrowserHeightRatio;
    const float upperH = workH - assetH;

    // Common flags: prevent user resizing / moving so the grid stays fixed
    const ImGuiWindowFlags kFixed =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoSavedSettings;

    // --- Hierarchy (left column, full height) ---
    ImGui::SetNextWindowPos(ImVec2(0, topY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(hierW, workH), ImGuiCond_Always);
    m_hierarchyPanel->SetWindowFlags(kFixed);

    // --- Scene (centre, upper) ---
    ImGui::SetNextWindowPos(ImVec2(hierW, topY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(sceneW, upperH), ImGuiCond_Always);
    m_scenePanel->SetWindowFlags(kFixed);

    // --- Asset Browser (centre, lower) ---
    ImGui::SetNextWindowPos(ImVec2(hierW, topY + upperH), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(sceneW, assetH), ImGuiCond_Always);
    m_assetBrowserPanel->SetWindowFlags(kFixed);

    // --- Inspector (right column, full height) ---
    ImGui::SetNextWindowPos(ImVec2(hierW + sceneW, topY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(inspW, workH), ImGuiCond_Always);
    m_inspectorPanel->SetWindowFlags(kFixed);
}

void Editor::drawMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        // File menu
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) { m_ctx.activeScenePath.clear(); }
            if (ImGui::MenuItem("Open Scene", "Ctrl+O")) { /* TODO: open file dialog */ }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) { /* TODO: save JSON */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))    m_running = false;
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
            if (ImGui::MenuItem("Hierarchy")) { /* TODO: toggle */ }
            if (ImGui::MenuItem("Inspector")) { /* TODO: toggle */ }
            if (ImGui::MenuItem("Asset Browser")) { /* TODO: toggle */ }
            if (ImGui::MenuItem("Scene")) { /* TODO: toggle */ }
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