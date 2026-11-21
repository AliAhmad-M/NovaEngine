#include <Editor/EditorApp.h>
#include <Editor/Theme.h>
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
#include <Windows.h>
#include <shellapi.h>
#include <cfloat>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace Nova;
using namespace Nova::Editor;

static bool isPathWithin(const std::filesystem::path& root, const std::filesystem::path& candidate)
{
    std::error_code ec;
    const auto normalizedRoot = std::filesystem::weakly_canonical(root, ec);
    if (ec) return false;

    ec.clear();
    const auto normalizedCandidate = std::filesystem::weakly_canonical(candidate, ec);
    if (ec) return false;

    const auto relative = normalizedCandidate.lexically_relative(normalizedRoot);
    if (relative.empty()) return normalizedCandidate == normalizedRoot;

    const auto first = relative.begin();
    return first != relative.end() && *first != "..";
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
    m_savedLayoutAvailable = io.IniFilename && std::filesystem::exists(io.IniFilename);

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

    Theme::Apply();

    ImGui_ImplGlfw_InitForOpenGL(m_nativeWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initialize project manager and launcher
    m_projects.LoadRecent();
    Services::Register<ProjectManager>(&m_projects);
    Services::Register<ProjectSettings>(&m_settings);
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
        m_scenePanel->PollBuild();

        if (!m_projects.HasProject() || m_projectOpening)
        {
            // Captured once: openProject() can flip m_projectOpening inside Draw()
            const bool opening = m_projectOpening;

            if (opening) ImGui::BeginDisabled();
            m_launcher->Draw(m_projects, [this](const std::string& p) { openProject(p); });
            if (opening) ImGui::EndDisabled();

            if (opening)
                drawLoadingCard();

            endFrame();
            continue;
        }

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

void EditorApp::drawLoadingCard()
{
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(420.0f, 0.0f), ImVec2(420.0f, FLT_MAX));

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("##ProjectLoading", nullptr, flags))
    {
        const std::string name = m_projects.HasProject() ? m_projects.Current().name : "project";
        ImGui::Text("Opening %s", name.c_str());
        ImGui::Spacing();

        // Negative fraction = indeterminate animated bar
        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(-FLT_MIN, 4.0f), "");
        ImGui::Spacing();

        ImGui::TextDisabled("Building project...");

        const std::string line = m_scenePanel->GetLastBuildLogLine();
        if (!line.empty())
        {
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 400.0f);
            ImGui::TextDisabled("%s", line.c_str());
            ImGui::PopTextWrapPos();
        }
    }
    ImGui::End();
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

    // Build a default layout for first run or an explicit reset. A saved ini wins.
    const bool buildDefaultLayout = m_resetLayoutRequested ||
        (!m_layoutInitialized && !m_savedLayoutAvailable);
    if (!m_layoutInitialized)
        m_layoutInitialized = true;

    if (buildDefaultLayout)
    {
        m_resetLayoutRequested = false;

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

        // Assign panels. Bottom tab order: Asset Browser, Build Output, Console
        ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
        ImGui::DockBuilderDockWindow("Scene", dockCenter);
        ImGui::DockBuilderDockWindow("Inspector", dockRight);
        ImGui::DockBuilderDockWindow("Asset Browser", dockBottom);
        ImGui::DockBuilderDockWindow("Build Output", dockBottom);
        ImGui::DockBuilderDockWindow("Console", dockBottom);

        if (ImGuiDockNode* bottomNode = ImGui::DockBuilderGetNode(dockBottom))
            bottomNode->SelectedTabId = ImHashStr("Asset Browser");

        ImGui::DockBuilderFinish(dockID);
        ImGui::GetIO().WantSaveIniSettings = true;
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
                        m_scenePanel->SetActiveSceneFilePath(path);
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
        if (ImGui::MenuItem("Reset Layout"))
        {
            m_resetLayoutRequested = true;
            m_layoutInitialized = false;
        }

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
    std::string savePath = m_ctx.activeSceneRef->GetFilePath();
    if (savePath.empty())
        savePath = m_ctx.activeScenePath;
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
    if (m_projectOpening || (m_scenePanel && m_scenePanel->IsBuilding()))
        return;

    std::string err;
    m_scripts.Unload();

    if (!m_projects.Open(dir, err)) { m_launcher->SetError(err); return; }

    m_projectOpening = true;
    if (!m_scenePanel->BuildOnly([this](bool succeeded)
        {
            finishProjectOpen(succeeded);
        }))
    {
        m_projectOpening = false;
        m_projects.Close();
        m_launcher->SetError("Could not start the project build.");
    }
}

void EditorApp::finishProjectOpen(bool buildSucceeded)
{
    if (!m_projects.HasProject())
    {
        m_projectOpening = false;
        return;
    }

    const ProjectInfo p = m_projects.Current();
    openProjectSolution(p);

    m_settings.Load(p.root);
    m_inputMapPanel->Reload();

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

    if (buildSucceeded)
    {
        if (!m_scripts.Load(p))
            Debug::LogWarn("Project built, but GameScripts.dll could not be loaded");
    }
    else
    {
        Debug::LogWarn("Project opened with build errors; see Build Output");
        m_scenePanel->FocusBuildOutputOnce();
    }

    m_projectOpening = false;
}

void EditorApp::openProjectSolution(const ProjectInfo& project)
{
    const std::filesystem::path solution = project.build / (project.name + ".slnx");
    if (!std::filesystem::exists(solution))
    {
        Debug::LogWarn("Project solution was not generated: %s", solution.string().c_str());
        return;
    }

    const std::wstring solutionPath = std::filesystem::absolute(solution).wstring();
    const std::wstring workingDirectory = std::filesystem::absolute(project.root).wstring();
    const INT_PTR result = reinterpret_cast<INT_PTR>(ShellExecuteW(
        nullptr, L"open", solutionPath.c_str(), nullptr,
        workingDirectory.c_str(), SW_SHOWNORMAL));
    if (result <= 32)
        Debug::LogWarn("Could not open project solution with the system handler: %s",
            solution.string().c_str());
}

void EditorApp::closeProject()
{
    if (m_projectOpening || (m_scenePanel && m_scenePanel->IsBuilding()))
    {
        Debug::LogWarn("Wait for the current build to finish before closing the project");
        return;
    }

    if (m_projects.HasProject())
        saveEditorSession();

    if (m_scenePanel)
        m_scenePanel->CloseAllScenes();

    m_scripts.Unload();
    m_projects.Close();
    m_settings.Clear();
    m_ctx.selectedEntity = -1;
}

void EditorApp::buildScripts()
{
    Debug::LogInfo("[Scripts] Building...");
    m_scenePanel->BuildScriptsNow([this](bool succeeded)
        {
            if (!succeeded || !m_projects.HasProject())
                return;

            m_scripts.Unload();
            if (m_scripts.Load(m_projects.Current()))
                Debug::LogInfo("[Scripts] Reloaded");
            else
                Debug::LogError("[Scripts] Built, but failed to load the DLL");
        });
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

    const auto openScenePaths = m_scenePanel->GetOpenScenePaths();
    std::vector<std::string> scenePaths;
    int activeTab = -1;
    const int currentActiveTab = m_scenePanel->GetActiveTabIndex();
    for (size_t i = 0; i < openScenePaths.size(); ++i)
    {
        if (!isPathWithin(p.root, openScenePaths[i]))
            continue;

        if (static_cast<int>(i) == currentActiveTab)
            activeTab = static_cast<int>(scenePaths.size());
        scenePaths.push_back(openScenePaths[i]);
    }

    session["openScenes"] = scenePaths;
    session["activeTab"] = activeTab;

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
                if (!isPathWithin(p.root, scenePath))
                {
                    Debug::LogWarn("Ignoring scene outside the current project: %s", scenePath.c_str());
                    continue;
                }

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