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
#include <fstream>
#include <nlohmann/json.hpp>

using namespace Nova;
using namespace Nova::Editor;

static void applyTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Rounding
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 3.0f;
    style.FrameRounding     = 3.0f;
    style.PopupRounding     = 4.0f;
    style.GrabRounding      = 3.0f;
    style.ScrollbarRounding = 6.0f;
    style.TabRounding       = 3.0f;

    // Sizing
    style.WindowPadding     = ImVec2(10.0f, 10.0f);
    style.FramePadding      = ImVec2(6.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 5.0f);
    style.IndentSpacing     = 18.0f;
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 9.0f;

    // Borders
    style.WindowBorderSize  = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.TabBorderSize     = 0.0f;

    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.WindowTitleAlign         = ImVec2(0.0f, 0.5f);

    // Anti aliasing
    style.AntiAliasedLines       = true;
    style.AntiAliasedLinesUseTex = true;
    style.AntiAliasedFill        = true;

    // Palette
    auto rgb = [](int r, int g, int b, float a = 1.0f)
        {
            return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
        };

    // Accent
    const ImVec4 accent       = rgb(0, 114, 239);
    const ImVec4 accentHover  = rgb(31, 137, 255);
    const ImVec4 accentActive = rgb(0, 95, 205);

    ImVec4* c = style.Colors;

    // Surfaces
    c[ImGuiCol_WindowBg]           = rgb(22, 23, 26);
    c[ImGuiCol_ChildBg]            = rgb(17, 18, 20);
    c[ImGuiCol_PopupBg]            = rgb(26, 27, 31, 0.98f);

    c[ImGuiCol_Border]             = rgb(38, 40, 46);
    c[ImGuiCol_BorderShadow]       = rgb(0, 0, 0, 0.0f);

    // Input
    c[ImGuiCol_FrameBg]            = rgb(14, 15, 17);
    c[ImGuiCol_FrameBgHovered]     = rgb(25, 27, 31);
    c[ImGuiCol_FrameBgActive]      = rgb(32, 34, 40);

    // Title
    c[ImGuiCol_TitleBg]            = rgb(18, 19, 21);
    c[ImGuiCol_TitleBgActive]      = rgb(22, 23, 26);
    c[ImGuiCol_TitleBgCollapsed]   = rgb(18, 19, 21);
    c[ImGuiCol_MenuBarBg]          = rgb(18, 19, 21);

    // Scrollbar
    c[ImGuiCol_ScrollbarBg]        = rgb(0, 0, 0, 0.0f);
    c[ImGuiCol_ScrollbarGrab]      = rgb(50, 52, 60);
    c[ImGuiCol_ScrollbarGrabHovered] = rgb(70, 74, 85);
    c[ImGuiCol_ScrollbarGrabActive]  = rgb(90, 95, 110);

    // Accent controls
    c[ImGuiCol_CheckMark]          = accent;
    c[ImGuiCol_SliderGrab]         = accent;
    c[ImGuiCol_SliderGrabActive]   = accentHover;

    // Buttons
    c[ImGuiCol_Button]             = rgb(36, 38, 44);
    c[ImGuiCol_ButtonHovered]      = rgb(48, 51, 60);
    c[ImGuiCol_ButtonActive]       = accentActive;

    // Headers
    c[ImGuiCol_Header]             = rgb(0, 114, 239, 0.22f);
    c[ImGuiCol_HeaderHovered]      = rgb(255, 255, 255, 0.06f);
    c[ImGuiCol_HeaderActive]       = rgb(0, 114, 239, 0.40f);

    // Separators
    c[ImGuiCol_Separator]          = rgb(36, 38, 44);
    c[ImGuiCol_SeparatorHovered]   = accent;
    c[ImGuiCol_SeparatorActive]    = accentHover;

    c[ImGuiCol_ResizeGrip]         = rgb(0, 114, 239, 0.15f);
    c[ImGuiCol_ResizeGripHovered]  = rgb(0, 114, 239, 0.55f);
    c[ImGuiCol_ResizeGripActive]   = rgb(0, 114, 239, 0.85f);

    // Tabs
    c[ImGuiCol_Tab]                = rgb(18, 19, 22);
    c[ImGuiCol_TabHovered]         = rgb(36, 38, 45);
    c[ImGuiCol_TabActive]          = rgb(28, 30, 36);
    c[ImGuiCol_TabUnfocused]       = rgb(16, 17, 20);
    c[ImGuiCol_TabUnfocusedActive] = rgb(24, 25, 30);

    // Docking
    c[ImGuiCol_DockingPreview]     = rgb(0, 114, 239, 0.35f);
    c[ImGuiCol_DockingEmptyBg]     = rgb(14, 15, 17);

    // Text
    c[ImGuiCol_Text]               = rgb(220, 224, 230);
    c[ImGuiCol_TextDisabled]       = rgb(108, 112, 122);
    c[ImGuiCol_TextSelectedBg]     = rgb(0, 114, 239, 0.35f);
    c[ImGuiCol_NavHighlight]       = accent;

    // Table
    c[ImGuiCol_TableHeaderBg]      = rgb(24, 25, 29);
    c[ImGuiCol_TableBorderStrong]  = rgb(38, 40, 46);
    c[ImGuiCol_TableBorderLight]   = rgb(28, 30, 35);
    c[ImGuiCol_TableRowBg]         = rgb(0, 0, 0, 0.0f);
    c[ImGuiCol_TableRowBgAlt]      = rgb(255, 255, 255, 0.02f);
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

    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;

    ImFont* font = io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_18pt-Regular.ttf", 19.0f, &fontConfig);
    io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_24pt-Bold.ttf", 21.0f, &fontConfig);
    if (!font)
    {
        Debug::LogError("Failed to load editor font, using default");
        io.Fonts->AddFontDefault();
    }

    applyTheme();

    ImGui_ImplGlfw_InitForOpenGL(m_nativeWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initialize project manager and launcher
    m_projects.LoadRecent();
    Services::Register<ProjectManager>(&m_projects);
    m_launcher = std::make_unique<ProjectLauncher>();

    m_scripts.onBeforeUnload = [this]()
    {
        // TO DO: iterate every entity with a BehaviorScript and drop the instance
    };

    // Load panels
    m_hierarchyPanel = std::make_unique<HierarchyPanel>(&m_ctx);
    m_inspectorPanel = std::make_unique<InspectorPanel>(&m_ctx);
    m_assetBrowserPanel = std::make_unique<AssetBrowserPanel>(&m_ctx);
    m_scenePanel = std::make_unique<ScenePanel>(&m_ctx);
    m_inputMapPanel = std::make_unique<InputMapPanel>(&m_ctx);
    m_consolePanel = std::make_unique<ConsolePanel>(&m_ctx);

    // Initialize scene serializer
    m_sceneSerializer = std::make_unique<SceneSerializer>();

    // Set scene panel reference
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

        if (!m_projects.HasProject())
        {
            m_launcher->Draw(m_projects, [this](const std::string& p) { openProject(p); });
            endFrame();
            continue;
        }

        pollScriptBuild();

        drawMenuBar();   // Draw menu bar
        buildLayout();   // Then dockspace

        ImGuiIO& io = ImGui::GetIO();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_B)) buildScripts();

        // Save scene
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
        {
            if (m_scenePanel && m_scenePanel->HasOpenScenes())
                saveActiveScene();
        }

        // New scene
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N))
            m_scenePanel->NewScene();

        // Open scene
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O))
        {
            std::string path = OpenNativeFileDialog("Open Scene", "Nova Scene (*.nvsn)", "*.nvsn;*.jso", m_scenesPath);
            if (!path.empty())
                m_scenePanel->OpenSceneTab(path);
        }


        if (m_showInputMap)
            m_inputMapPanel->Draw(m_showInputMap);

        // Open scene
        if (m_openScenePicker)
        {
            ImGui::OpenPopup("##OpenScenePicker");
            m_openScenePicker = false;
        }

        if (m_showHierarchy)    m_hierarchyPanel->Draw();
        if (m_showInspector)    m_inspectorPanel->Draw();
        if (m_showAssetBrowser) m_assetBrowserPanel->Draw();
        if (m_scenePanel)       m_scenePanel->Draw();
        if (m_showConsole)      m_consolePanel->Draw();

        DrawFilePicker("##OpenScenePicker", m_scenesPath, ".nvsn",
            [this](const std::string& path)
            {
                m_scenePanel->OpenSceneTab(path);
            });

        // Render scene into FBO
        if (m_scenePanel && m_scenePanel->HasOpenScenes())
        {
            int vpW = m_scenePanel->GetSceneViewportWidth();
            int vpH = m_scenePanel->GetSceneViewportHeight();
            if (vpW > 0 && vpH > 0)
            {
                Services::Register<Camera2D>(&m_scenePanel->GetEditorCamera());
                m_runtime.RenderEditorFrame(m_scenePanel->GetSceneFBOId(), vpW, vpH);
            }
        }

        double currentTime = glfwGetTime();
        float dt = (float)(currentTime - m_lastFrameTime);
        m_lastFrameTime = currentTime;
        if (dt > 0.1f) dt = 1.0f / 60.0f;

        endFrame();
    }
}

void EditorApp::Shutdown()
{
    // Save editor session before shutting down
    if (m_projects.HasProject())
        saveEditorSession();

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
    glClearColor(0.06f, 0.06f, 0.08f, 1.0f);
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

        // Split right (Inspector, full height)
        ImGui::DockBuilderSplitNode(dockID, ImGuiDir_Right, kInspectorWidthRatio, &dockRight, &dockCenter);

        // Split bottom (Asset Browser, spans Hierarchy + Scene)
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Down, kAssetBrowserHeightRatio, &dockBottom, &dockCenter);

        // Split left (Hierarchy, Build Output) from remaining top area
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Left, kHierarchyWidthRatio, &dockLeft, &dockCenter);

        // Assign panels
        ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
        ImGui::DockBuilderDockWindow("Scene", dockCenter);
        ImGui::DockBuilderDockWindow("Inspector", dockRight);
        ImGui::DockBuilderDockWindow("Asset Browser", dockBottom);
        ImGui::DockBuilderDockWindow("Console", dockBottom);
        ImGui::DockBuilderDockWindow("Build Output", dockBottom);

        ImGui::DockBuilderFinish(dockID);
    }

    ImGui::End();
}

void EditorApp::drawMenuBar()
{
    if (!ImGui::BeginMainMenuBar()) return;

    // File menu
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Project"))
        {
            // TODO: new project
        }
        
        if (ImGui::MenuItem("Open Project"))
        {
            // Close current project to return to launcher
            closeProject();
        }

        if (ImGui::MenuItem("Close Project"))
            closeProject();

        ImGui::Separator();

        if (ImGui::MenuItem("New Scene", "Ctrl+N"))
            m_scenePanel->NewScene();

        // Guard save operations
        bool hasScene = m_scenePanel && m_scenePanel->HasOpenScenes();

        if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, hasScene))
            saveActiveScene();

        if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
        {
            std::string path = OpenNativeFileDialog("Open Scene", "Nova Scene (*.nvsn)", "*.nvsn;*.jso", m_scenesPath);
            if (!path.empty())
                m_scenePanel->OpenSceneTab(path);
        }

        if (ImGui::MenuItem("Save Scene As", "Ctrl+Shift+S", false, hasScene))
        {
            std::string defName = m_ctx.activeSceneRef ? m_ctx.activeSceneRef->GetName() : "NewScene";
            std::string path = SaveNativeFileDialog("Save Scene As", "Nova Scene (*.nvsn)", "*.nvsn", m_scenesPath, defName);
            if (!path.empty())
            {
                if (m_sceneSerializer && m_ctx.activeSceneRef)
                {
                    if (m_sceneSerializer->Save(*m_ctx.activeSceneRef, path))
                    {
                        m_ctx.activeScenePath = path;
                        m_scenePanel->MarkSceneAsSaved();
                        Debug::LogInfo("Scene saved as: %s", path.c_str());
                    }
                }
            }
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Exit"))
            m_running = false;

        ImGui::EndMenu();
    }

    // Edit menu
    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Undo", "Ctrl+Z"))
        {
            // TODO: undo
        }
        if (ImGui::MenuItem("Redo", "Ctrl+Y"))
        {
            // TODO: redo
        }

        ImGui::EndMenu();
    }

    // Project menu
    if (ImGui::BeginMenu("Project"))
    {
        if (ImGui::MenuItem("Input"))
            m_showInputMap = true;
        
        if (ImGui::MenuItem("Physics"))
        {
            // TODO: physics settings
        }
        
        if (ImGui::MenuItem("Resolution"))
        {
            // TODO: resolution settings
        }

        ImGui::EndMenu();
    }

    // View menu
    if (ImGui::BeginMenu("View"))
    {
        if (ImGui::MenuItem("Hierarchy", nullptr, m_showHierarchy))
            m_showHierarchy = !m_showHierarchy;
        if (ImGui::MenuItem("Inspector", nullptr, m_showInspector))
            m_showInspector = !m_showInspector;
        if (ImGui::MenuItem("Console", nullptr, m_showConsole))
            m_showConsole = !m_showConsole;
        if (ImGui::MenuItem("Asset Browser", nullptr, m_showAssetBrowser))
            m_showAssetBrowser = !m_showAssetBrowser;

        ImGui::EndMenu();
    }

    // Help menu
    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("Docs"))
        {
            // TODO: open docs site
        }
        if (ImGui::MenuItem("About"))
            m_showAbout = true;

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();

    // About dialog
    if (m_showAbout)
    {
        ImGui::SetNextWindowSize(ImVec2(440, 240), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(500, 250), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("About##dialog", &m_showAbout))
        {
            ImGui::Text("Nova Engine");
            ImGui::Spacing();
            ImGui::TextDisabled("An ECS based, lightweight game engine.");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Version: 0.1.0");
            ImGui::Text("Built with ImGui, GLFW, GLAD, Box2D, JoltPhysics");
            ImGui::Spacing();
            if (ImGui::Button("Close", ImVec2(100, 0)))
                m_showAbout = false;
        }
        ImGui::End();
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

void EditorApp::openProject(const std::string& dir)
{
    std::string err;
    m_scripts.Unload();

    if (!m_projects.Open(dir, err)) { m_launcher->SetError(err); return; }

    const ProjectInfo& p = m_projects.Current();
    createDefaultMainScene(p);

    m_scenesPath = p.scenes.string();
    m_ctx.selectedEntity = -1;

    m_assetBrowserPanel->SetRoot(p.root);

    // Try to restore the previous editor session
    loadEditorSession();

    // If no scenes were restored, open the default main scene
    if (!m_scenePanel->HasOpenScenes())
    {
        std::string sceneName = p.mainScene.empty() ? "Main" : p.mainScene;
        std::filesystem::path mainScene = p.scenes / (sceneName + ".nvsn");
        if (std::filesystem::exists(mainScene))
        {
            m_scenePanel->OpenSceneTab(mainScene.string());
        }
        else
        {
            // Fallback: open first .nvsn found
            std::error_code ec;
            if (std::filesystem::exists(p.scenes, ec))
            {
                for (const auto& entry : std::filesystem::directory_iterator(p.scenes, ec))
                {
                    if (entry.path().extension() == ".nvsn")
                    {
                        m_scenePanel->OpenSceneTab(entry.path().string());
                        break;
                    }
                }
            }

            // If still nothing, create a new scene
            if (!m_scenePanel->HasOpenScenes())
                m_scenePanel->NewScene();
        }
    }

    m_scripts.Load(p);
}

void EditorApp::closeProject()
{
    if (m_projects.HasProject())
        saveEditorSession();

    m_scripts.Unload();
    m_projects.Close();
    m_ctx.selectedEntity = -1;
}

void EditorApp::buildScripts()
{
    //if (m_scenePanel->IsPlaying()) { Debug::LogWarn("Stop play mode before rebuilding scripts"); return; }
    if (m_scripts.IsBuilding())    return;

    Debug::LogInfo("[Scripts] Building...");
    m_scripts.BuildAsync(m_projects.Current());
}

void EditorApp::pollScriptBuild()
{
    //if (m_scenePanel->IsPlaying()) return;

    std::string log;
    bool ok = false;
    if (!m_scripts.PollBuild(log, ok)) return;

    if (!ok) { Debug::LogError("[Scripts] Build failed:\n%s", log.c_str()); return; }

    m_scripts.Unload();
    if (m_scripts.Load(m_projects.Current()))
        Debug::LogInfo("[Scripts] Reloaded");
    else
        Debug::LogError("[Scripts] Built, but failed to load the DLL");
}

void EditorApp::createDefaultMainScene(
    const ProjectInfo& project)
{
    std::string sceneName = project.mainScene.empty() ? "Main" : project.mainScene;
    const std::filesystem::path mainPath =
        project.scenes / (sceneName + ".nvsn");

    if (std::filesystem::exists(mainPath))
        return;

    Scene mainScene(sceneName);
    mainScene.SetFilePath(mainPath.string());

    if (!m_sceneSerializer->Save(
        mainScene,
        mainPath.string()))
    {
        Debug::LogError(
            "Failed to create default %s.nvsn", sceneName.c_str());
    }
}

void EditorApp::saveEditorSession()
{
    if (!m_projects.HasProject() || !m_scenePanel)
        return;

    const ProjectInfo& p = m_projects.Current();
    std::filesystem::path sessionPath = p.root / ".nova_session.json";

    nlohmann::json session;
    session["version"] = 1;

    auto scenePaths = m_scenePanel->GetOpenScenePaths();
    session["openScenes"] = scenePaths;
    session["activeTab"] = m_scenePanel->GetActiveTabIndex();

    std::ofstream out(sessionPath);
    if (out.is_open())
    {
        out << session.dump(2);
    }
}

void EditorApp::loadEditorSession()
{
    if (!m_projects.HasProject() || !m_scenePanel)
        return;

    const ProjectInfo& p = m_projects.Current();
    std::filesystem::path sessionPath = p.root / ".nova_session.json";

    if (!std::filesystem::exists(sessionPath))
        return;

    try
    {
        std::ifstream in(sessionPath);
        if (!in.is_open())
            return;

        nlohmann::json session = nlohmann::json::parse(in);

        if (session.contains("openScenes") && session["openScenes"].is_array())
        {
            for (const auto& path : session["openScenes"])
            {
                std::string scenePath = path.get<std::string>();
                if (std::filesystem::exists(scenePath))
                {
                    m_scenePanel->OpenSceneTab(scenePath);
                }
            }
        }

        if (session.contains("activeTab") && session["activeTab"].is_number_integer())
        {
            int activeIdx = session["activeTab"].get<int>();
            m_scenePanel->SetActiveTab(activeIdx);
        }
    }
    catch (const std::exception& e)
    {
        Debug::LogWarn("Failed to restore editor session: %s", e.what());
    }
}