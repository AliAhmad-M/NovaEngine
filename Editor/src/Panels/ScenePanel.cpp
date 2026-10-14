#include <Editor/Panels/ScenePanel.h>
#include <Editor/Scene/SceneSerializer.h>
#include <Editor/Project/ProjectManager.h>
#include <Core/Services.h>
#include <Core/Components.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <glad/glad.h>
#include <Windows.h>

using namespace Nova;
using namespace Nova::Editor;

void ScenePanel::createFBO(int w, int h)
{
    m_vpWidth = w;
    m_vpHeight = h;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_colorTex);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);

    glGenRenderbuffers(1, &m_depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ScenePanel::resizeFBO(int w, int h)
{
    destroyFBO();
    createFBO(w, h);
}

void ScenePanel::destroyFBO()
{
    if (m_depthRbo) { glDeleteRenderbuffers(1, &m_depthRbo); m_depthRbo = 0; }
    if (m_colorTex) { glDeleteTextures(1, &m_colorTex);      m_colorTex = 0; }
    if (m_fbo) { glDeleteFramebuffers(1, &m_fbo);        m_fbo = 0; }
    m_vpWidth = m_vpHeight = 0;
}

void ScenePanel::buildAndRunGame()
{
    if (m_building)
        return;

    ProjectManager* pm =
        Services::Fetch<ProjectManager>();

    if (!pm || !pm->HasProject())
    {
        Debug::LogError(
            "[Build] No project is currently open.");
        return;
    }

    const ProjectInfo& project =
        pm->Current();

    const std::filesystem::path root =
        std::filesystem::absolute(project.root);

    const std::filesystem::path buildDir =
        std::filesystem::absolute(project.build);

    const std::filesystem::path sdkDir =
        std::filesystem::absolute(
            ProjectManager::SdkDir());

    const std::string configuration = "Release";

    const std::filesystem::path exePath =
        buildDir /
        "bin" /
        configuration /
        "Game.exe";

    std::error_code ec;
    if (std::filesystem::exists(buildDir, ec)) {
        std::filesystem::remove_all(buildDir, ec);
    }
    std::filesystem::create_directories(buildDir, ec);

    if (ec)
    {
        Debug::LogError(
            "[Build] Failed to create build directory: %s",
            ec.message().c_str());
        return;
    }

    m_building = true;
    m_showBuildLog = true;

    {
        std::lock_guard<std::mutex> lock(
            m_buildLogMutex);

        m_buildLog.clear();
        m_buildLog +=
            "Nova Build Started\n";

        m_buildLog +=
            "Project: " +
            root.string() +
            "\n";

        m_buildLog +=
            "Output: " +
            exePath.string() +
            "\n\n";
    }

    std::thread(
        [this,
        root,
        buildDir,
        sdkDir,
        exePath,
        configuration]()
        {
            auto appendLog =
                [this](const std::string& text)
                {
                    std::lock_guard<std::mutex> lock(
                        m_buildLogMutex);

                    m_buildLog += text;
                    m_buildLogScrollToBottom = true;
                };

            // Configure the project

                        std::string configureCmd =
                "cmake"
                " -S \"" + root.string() + "\""
                " -B \"" + buildDir.string() + "\""
                " -DNOVA_SDK_DIR=\"" + sdkDir.string() + "\"";

            appendLog(
                "\n[Configure]\n" +
                configureCmd +
                "\n\n");

            {
                SECURITY_ATTRIBUTES sa{};
                sa.nLength =
                    sizeof(sa);
                sa.bInheritHandle = TRUE;

                HANDLE hRead = nullptr;
                HANDLE hWrite = nullptr;

                if (!CreatePipe(
                    &hRead,
                    &hWrite,
                    &sa,
                    0))
                {
                    appendLog(
                        "[ERROR] Failed to create pipe.\n");

                    m_building = false;
                    return;
                }

                SetHandleInformation(
                    hRead,
                    HANDLE_FLAG_INHERIT,
                    0);

                STARTUPINFOA si{};
                si.cb = sizeof(si);
                si.hStdOutput = hWrite;
                si.hStdError = hWrite;
                si.dwFlags =
                    STARTF_USESTDHANDLES |
                    STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_HIDE;

                PROCESS_INFORMATION pi{};

                std::string cmdLine =
                    configureCmd;

                bool launched =
                    CreateProcessA(
                        nullptr,
                        cmdLine.data(),
                        nullptr,
                        nullptr,
                        TRUE,
                        CREATE_NO_WINDOW,
                        nullptr,
                        root.string().c_str(),
                        &si,
                        &pi);

                CloseHandle(hWrite);

                if (!launched)
                {
                    appendLog(
                        "[ERROR] Failed to run CMake configure.\n");

                    CloseHandle(hRead);
                    m_building = false;
                    return;
                }

                char buffer[512];
                DWORD bytesRead = 0;

                while (ReadFile(
                    hRead,
                    buffer,
                    sizeof(buffer) - 1,
                    &bytesRead,
                    nullptr) &&
                    bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    appendLog(buffer);
                }

                CloseHandle(hRead);

                WaitForSingleObject(
                    pi.hProcess,
                    INFINITE);

                DWORD exitCode = 0;

                GetExitCodeProcess(
                    pi.hProcess,
                    &exitCode);

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                if (exitCode != 0)
                {
                    appendLog(
                        "\n[ERROR] CMake configure failed.\n");

                    m_building = false;
                    return;
                }
            }

            // Build the actual executable
            
            const std::string buildCmd =
                "cmake"
                " --build \"" +
                buildDir.string() +
                "\""
                " --config " +
                configuration;

            appendLog(
                "\n[Build]\n" +
                buildCmd +
                "\n\n");

            {
                SECURITY_ATTRIBUTES sa{};
                sa.nLength =
                    sizeof(sa);
                sa.bInheritHandle = TRUE;

                HANDLE hRead = nullptr;
                HANDLE hWrite = nullptr;

                if (!CreatePipe(
                    &hRead,
                    &hWrite,
                    &sa,
                    0))
                {
                    appendLog(
                        "[ERROR] Failed to create pipe.\n");

                    m_building = false;
                    return;
                }

                SetHandleInformation(
                    hRead,
                    HANDLE_FLAG_INHERIT,
                    0);

                STARTUPINFOA si{};
                si.cb = sizeof(si);
                si.hStdOutput = hWrite;
                si.hStdError = hWrite;
                si.dwFlags =
                    STARTF_USESTDHANDLES |
                    STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_HIDE;

                PROCESS_INFORMATION pi{};

                std::string cmdLine =
                    buildCmd;

                bool launched =
                    CreateProcessA(
                        nullptr,
                        cmdLine.data(),
                        nullptr,
                        nullptr,
                        TRUE,
                        CREATE_NO_WINDOW,
                        nullptr,
                        root.string().c_str(),
                        &si,
                        &pi);

                CloseHandle(hWrite);

                if (!launched)
                {
                    appendLog(
                        "[ERROR] Failed to run CMake build.\n");

                    CloseHandle(hRead);
                    m_building = false;
                    return;
                }

                char buffer[512];
                DWORD bytesRead = 0;

                while (ReadFile(
                    hRead,
                    buffer,
                    sizeof(buffer) - 1,
                    &bytesRead,
                    nullptr) &&
                    bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    appendLog(buffer);
                }

                CloseHandle(hRead);

                WaitForSingleObject(
                    pi.hProcess,
                    INFINITE);

                DWORD exitCode = 0;

                GetExitCodeProcess(
                    pi.hProcess,
                    &exitCode);

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                if (exitCode != 0)
                {
                    appendLog(
                        "\nBuild FAILED.\n");

                    m_building = false;
                    return;
                }
            }

            /*
                Build succeeded. Verify the executable.
            */

            if (!std::filesystem::exists(
                exePath))
            {
                appendLog(
                    "\n[ERROR] Build succeeded but "
                    "Game.exe was not found at:\n" +
                    exePath.string() +
                    "\n");

                m_building = false;
                return;
            }

            appendLog(
                "\nBuild Succeeded.\n"
                "Launching:\n" +
                exePath.string() +
                "\n");


            // Copy vcpkg DLLs next to Game.exe
            {
                std::error_code cec;
                for (const auto& triplet :
                    std::filesystem::directory_iterator(sdkDir / "vcpkg_installed", cec))
                {
                    const auto binDir = triplet.path() / "bin";
                    if (!std::filesystem::is_directory(binDir, cec))
                        continue;

                    for (const auto& f : std::filesystem::directory_iterator(binDir, cec))
                    {
                        if (f.path().extension() == ".dll")
                            std::filesystem::copy_file(
                                f.path(),
                                exePath.parent_path() / f.path().filename(),
                                std::filesystem::copy_options::skip_existing,
                                cec);
                    }
                }
            }

            // Launch the user's actual project

            STARTUPINFOA runSi{};
            runSi.cb = sizeof(runSi);

            PROCESS_INFORMATION runPi{};

            std::string runCmd =
                "\"" +
                exePath.string() +
                "\"";

            bool launched =
                CreateProcessA(
                    nullptr,
                    runCmd.data(),
                    nullptr,
                    nullptr,
                    FALSE,
                    CREATE_NEW_CONSOLE,
                    nullptr,
                    root.string().c_str(),
                    &runSi,
                    &runPi);

            if (!launched)
            {
                appendLog(
                    "[ERROR] Failed to launch Game.exe. "
                    "Error code: " +
                    std::to_string(
                        GetLastError()) +
                    "\n");
            }
            else
            {
                CloseHandle(
                    runPi.hProcess);

                CloseHandle(
                    runPi.hThread);
            }

            m_building = false;

        })
        .detach();
}

void ScenePanel::drawBuildLog()
{
    if (!m_showBuildLog) return;

    ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Build Output", &m_showBuildLog))
    {
        ImGui::End();
        return;
    }

    if (m_building)
    {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Building...");
        ImGui::SameLine();
    }
    else
    {
        ImGui::TextDisabled("Idle");
        ImGui::SameLine();
    }

    if (ImGui::SmallButton("Clear"))
    {
        std::lock_guard<std::mutex> lock(m_buildLogMutex);
        m_buildLog.clear();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Close"))
        m_showBuildLog = false;

    ImGui::Separator();

    ImGui::BeginChild("##buildlog_scroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    {
        std::lock_guard<std::mutex> lock(m_buildLogMutex);
        ImGui::TextUnformatted(m_buildLog.c_str());
    }

    if (m_buildLogScrollToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
        m_buildLogScrollToBottom = false;
    }

    ImGui::EndChild();
    ImGui::End();
}

void ScenePanel::drawTabBar()
{
    if (m_sceneTabs.empty()) return;

    ImGuiTabBarFlags tabFlags = ImGuiTabBarFlags_Reorderable |
        ImGuiTabBarFlags_AutoSelectNewTabs |
        ImGuiTabBarFlags_FittingPolicyScroll;

    if (!ImGui::BeginTabBar("##SceneTabs", tabFlags)) return;

    for (int i = 0; i < (int)m_sceneTabs.size(); ++i)
    {
        SceneTab& tab = m_sceneTabs[i];

        char label[256];
        std::snprintf(label, sizeof(label), "%s%s###tab%d",
            tab.isDirty ? "* " : "",
            tab.name.c_str(), i);

        ImGuiTabItemFlags itemFlags = ImGuiTabItemFlags_None;
        if (tab.isDirty)
            itemFlags |= ImGuiTabItemFlags_UnsavedDocument;

        bool open = true;
        if (ImGui::BeginTabItem(label, &open, itemFlags))
        {
            if (m_activeTab != i)
            {
                m_activeTab = i;
                onTabChanged(i);
            }

            if (!tab.filePath.empty())
            {
                m_ctxRef->activeScenePath = tab.filePath;
            }
            else
            {
                m_ctxRef->activeScenePath.clear();
            }
            m_ctxRef->activeSceneRef = tab.scene.get();
            ImGui::EndTabItem();
        }

        if (!open)
        {
            closeScene(i);
            break;
        }
    }

    ImGui::EndTabBar();
}

//void ScenePanel::createGameFBO(int w, int h)
//{
//    m_gameVpWidth = w;
//    m_gameVpHeight = h;
//
//    glGenFramebuffers(1, &m_gameFbo);
//    glBindFramebuffer(GL_FRAMEBUFFER, m_gameFbo);
//
//    glGenTextures(1, &m_gameColorTex);
//    glBindTexture(GL_TEXTURE_2D, m_gameColorTex);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gameColorTex, 0);
//
//    glGenRenderbuffers(1, &m_gameDepthRbo);
//    glBindRenderbuffer(GL_RENDERBUFFER, m_gameDepthRbo);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_gameDepthRbo);
//
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//}
//
//void ScenePanel::resizeGameFBO(int w, int h)
//{
//    destroyGameFBO();
//    createGameFBO(w, h);
//}
//
//void ScenePanel::destroyGameFBO()
//{
//    if (m_gameDepthRbo) { glDeleteRenderbuffers(1, &m_gameDepthRbo); m_gameDepthRbo = 0; }
//    if (m_gameColorTex) { glDeleteTextures(1, &m_gameColorTex); m_gameColorTex = 0; }
//    if (m_gameFbo) { glDeleteFramebuffers(1, &m_gameFbo); m_gameFbo = 0; }
//    m_gameVpWidth = m_gameVpHeight = 0;
//}
//
//void ScenePanel::drawGameWindow()
//{
//    if (!m_showGameWindow) return;
//
//    bool open = m_showGameWindow;
//
//    ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
//    if (!ImGui::Begin("Game", &open))
//    {
//        ImGui::End();
//        if (!open && m_playing) stopPlay();
//        m_showGameWindow = open;
//        return;
//    }
//
//    ImGui::TextColored(m_playing ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
//        m_playing ? "Playing" : "Stopped");
//
//    ImVec2 viewSize = ImGui::GetContentRegionAvail();
//    int w = (int)viewSize.x;
//    int h = (int)viewSize.y;
//
//    if (w > 0 && h > 0)
//    {
//        // Resize
//        if (w != m_gameVpWidth || h != m_gameVpHeight)
//            resizeGameFBO(w, h);
//
//        if (m_gameColorTex)
//        {
//            ImGui::Image((ImTextureID)(intptr_t)m_gameColorTex, viewSize,
//                ImVec2(0, 1), ImVec2(1, 0));
//        }
//    }
//
//    ImGui::End();
//
//    if (!open && m_playing)
//        stopPlay();
//
//    m_showGameWindow = open;
//}

void ScenePanel::drawSceneToolBar()
{
    ImGui::Checkbox("Grid", &m_showGrid);
    ImGui::SameLine();

    if (ImGui::SmallButton("+ New Scene"))
        newScene();
    ImGui::SameLine();


    if (m_building)
    {
        ImGui::BeginDisabled();
        ImGui::SmallButton(m_building ? "Building..." : "> Build");
        ImGui::EndDisabled();
    }
    else
    {
        if (ImGui::SmallButton("> Build"))
            buildAndRunGame();
    }
    ImGui::SameLine();

    if (ImGui::SmallButton("Build Log"))
        m_showBuildLog = !m_showBuildLog;

    // Gizmo toolbar
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // Translate button
    bool isTrans = (m_gizmoOp == ImGuizmo::TRANSLATE);
    if (isTrans) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.44f, 0.70f, 1.0f, 1.0f));
    if (ImGui::SmallButton("T##trans")) m_gizmoOp = ImGuizmo::TRANSLATE;
    if (isTrans) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate (W)");

    ImGui::SameLine();

    // Rotate button
    bool isRot = (m_gizmoOp == ImGuizmo::ROTATE);
    if (isRot) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.44f, 0.70f, 1.0f, 1.0f));
    if (ImGui::SmallButton("R##rot")) m_gizmoOp = ImGuizmo::ROTATE;
    if (isRot) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate (E)");

    ImGui::SameLine();

    // Scale button
    bool isScale = (m_gizmoOp == ImGuizmo::SCALE);
    if (isScale) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.44f, 0.70f, 1.0f, 1.0f));
    if (ImGui::SmallButton("S##scale")) m_gizmoOp = ImGuizmo::SCALE;
    if (isScale) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale (R)");

    ImGui::SameLine();

    // Local/World toggle
    bool isLocal = (m_gizmoMode == ImGuizmo::LOCAL);
    if (ImGui::SmallButton(isLocal ? "Local" : "World"))
        m_gizmoMode = isLocal ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Local/World space");

    // Keyboard shortcuts (ignored while typing in a text field)
    if (!ImGui::GetIO().WantTextInput)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) m_gizmoOp = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) m_gizmoOp = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) m_gizmoOp = ImGuizmo::SCALE;
    }
}

void ScenePanel::drawGrid()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2      origin = ImGui::GetItemRectMin();
    ImVec2      size = ImGui::GetItemRectSize();

    float   zoom = m_editorCamera.GetZoom();
    Vector2 off = m_editorCamera.GetOffset();

    float step = (float)m_gridSize * zoom;
    if (step < 4.0f) return;

    float phaseX = std::fmod(off.x * zoom, step);
    float phaseY = std::fmod(off.y * zoom, step);
    if (phaseX < 0.0f) phaseX += step;
    if (phaseY < 0.0f) phaseY += step;

    ImU32 lineColor = IM_COL32(255, 255, 255, 30);

    for (float x = origin.x - phaseX; x <= origin.x + size.x; x += step)
    {
        if (x < origin.x) continue;
        float worldX = off.x + (x - origin.x) / zoom;
        bool  isYAxis = (std::abs(worldX) < 0.5f / zoom);
        ImU32 col = isYAxis ? IM_COL32(100, 180, 255, 200) : lineColor;
        float thick = isYAxis ? 2.0f : 1.0f;
        dl->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + size.y), col, thick);
    }

    for (float y = origin.y - phaseY; y <= origin.y + size.y; y += step)
    {
        if (y < origin.y) continue;
        float worldY = off.y + (y - origin.y) / zoom;
        bool  isXAxis = (std::abs(worldY) < 0.5f / zoom);
        ImU32 col = isXAxis ? IM_COL32(255, 100, 100, 200) : lineColor;
        float thick = isXAxis ? 2.0f : 1.0f;
        dl->AddLine(ImVec2(origin.x, y), ImVec2(origin.x + size.x, y), col, thick);
    }
}

void ScenePanel::drawSceneViewport()
{
    ImVec2 viewSize = ImGui::GetContentRegionAvail();
    int w = (int)viewSize.x;
    int h = (int)viewSize.y;
    if (w < 1 || h < 1) return;

    m_editorCamera.SetViewportSize(w, h);

    if (w != m_vpWidth || h != m_vpHeight)
        resizeFBO(w, h);

    ImGui::Image(
        (ImTextureID)(intptr_t)m_colorTex,
        viewSize,
        ImVec2(0, 1),
        ImVec2(1, 0)
    );

    drawSelectionOutline();
    drawGizmo();

    if (ImGui::IsItemHovered())
    {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
            ImGui::IsMouseDown(ImGuiMouseButton_Middle))
        {
            panEditorCamera(-delta.x, -delta.y);
        }

        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
        {
            float oldZoom = m_editorCamera.GetZoom();
            float newZoom = std::max(0.1f, std::min(oldZoom * (1.0f + wheel * 0.1f), 10.0f));
            m_editorCamera.SetZoom(newZoom);
        }
    }

    if (m_showGrid && m_vpWidth > 0 && m_vpHeight > 0)
        drawGrid();
}

void ScenePanel::panEditorCamera(float dx, float dy)
{
    float   zoom = m_editorCamera.GetZoom();
    Vector2 current = m_editorCamera.GetOffset();
    m_editorCamera.SetOffset({ current.x + dx / zoom, current.y + dy / zoom });
}

void ScenePanel::onTabChanged(int index)
{
    if (index < 0 || index >= (int)m_sceneTabs.size()) return;

    m_ctxRef->selectedEntity = -1;

    SceneTab& tab = m_sceneTabs[index];

    if (!tab.filePath.empty())
    {
        std::vector<LoadedEntity> loaded;
        if (m_sceneManagerRef->GetLoader()->Load(tab.filePath, &loaded))
        {
            tab.scene->GetEntities().clear();
            for (const auto& e : loaded)
                tab.scene->AddEntity(e.id, e.name, e.parentId);

            tab.isDirty = false;
        }
    }
    else
    {
        m_ecsRef->DeleteAllEntities();
        tab.scene->GetEntities().clear();
    }
}

ScenePanel::ScenePanel(Context* ctx)
    : m_ctxRef(ctx)
{
    m_editorCamera.isMain = true;
    m_ecsRef = Services::Fetch<ECS>();
    m_sceneManagerRef = Services::Fetch<SceneManager>();
}

ScenePanel::~ScenePanel()
{
    destroyFBO();
    // destroyGameFBO();
}

void ScenePanel::Draw()
{
    ImGui::Begin("Scene");
    drawSceneToolBar();
    ImGui::Separator();
    drawTabBar();
    ImGui::Separator();
    drawSceneViewport();
    ImGui::End();

    drawBuildLog();
    // drawGameWindow();
}

void ScenePanel::newScene()
{
    int idx = static_cast<int>(m_sceneTabs.size());

    char name[64];
    std::snprintf(
        name,
        sizeof(name),
        "Untitled_%d",
        idx);

    SceneTab tab;
    tab.name = name;
    tab.isDirty = true;
    tab.scene = std::make_unique<Scene>(name);

    ProjectManager* pm =
        Services::Fetch<ProjectManager>();

    if (pm && pm->HasProject())
    {
        const ProjectInfo& project =
            pm->Current();

        tab.filePath =
            (project.scenes /
                (tab.name + ".nvsn"))
            .lexically_normal()
            .string();

        tab.scene->SetFilePath(
            tab.filePath);
    }

    m_sceneTabs.push_back(
        std::move(tab));
}

void ScenePanel::closeScene(int index)
{
    if (index < 0 || index >= (int)m_sceneTabs.size()) return;
    m_sceneTabs.erase(m_sceneTabs.begin() + index);
    if (m_activeTab >= (int)m_sceneTabs.size())
        m_activeTab = (int)m_sceneTabs.size() - 1;
}

void ScenePanel::OpenSceneTab(const std::string& filePath)
{
    for (int i = 0; i < (int)m_sceneTabs.size(); i++)
    {
        if (m_sceneTabs[i].filePath == filePath)
        {
            m_lastActiveTab = -1;
            m_activeTab = i - 1;
            return;
        }
    }

    std::string name = std::filesystem::path(filePath).stem().string();

    SceneTab tab;
    tab.name = name;
    tab.filePath = filePath;
    tab.isDirty = false;
    tab.scene = std::make_unique<Scene>(name);
    tab.scene->SetFilePath(filePath);

    m_sceneTabs.push_back(std::move(tab));
}

void ScenePanel::MarkSceneAsSaved()
{
    if (m_activeTab >= 0 && m_activeTab < (int)m_sceneTabs.size())
        m_sceneTabs[m_activeTab].isDirty = false;
}

void ScenePanel::MarkSceneAsDirty()
{
    if (m_activeTab >= 0 && m_activeTab < (int)m_sceneTabs.size())
        m_sceneTabs[m_activeTab].isDirty = true;
}

void ScenePanel::drawGizmo()
{
    if (m_ctxRef->selectedEntity == -1) return;
    if (!m_ctxRef->activeSceneRef) return;

    ECS* ecs = Services::Fetch<ECS>();
    EntityID entity = static_cast<EntityID>(m_ctxRef->selectedEntity);

    Transform2D* transform = ecs->GetComponent<Transform2D>(entity);
    if (!transform) return;

    // Set ImGuizmo rect to match viewport
    ImVec2 viewportMin = ImGui::GetItemRectMin();
    ImVec2 viewportSize = ImGui::GetItemRectSize();

    ImGuizmo::SetOrthographic(true);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(
        viewportMin.x, viewportMin.y,
        viewportSize.x, viewportSize.y
    );

    float zoom = m_editorCamera.GetZoom();
    Vector2 camOffset = m_editorCamera.GetOffset();

    // Build orthographic projection matrix

    float left = camOffset.x;
    float right = camOffset.x + viewportSize.x / zoom;
    float top = camOffset.y;
    float bottom = camOffset.y + viewportSize.y / zoom;

    float proj[16] = {
        2.0f / (right - left),  0,                  0,  0,
        0,                  2.0f / (top - bottom),   0,  0,
        0,                  0,                  -1,  0,
        -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0, 1
    };

    // View matrix - identity for 2D
    float view[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };

    // Build entity transform matrix
    float translation[3] = { transform->position.x, transform->position.y, 0.0f };
    float rotation[3] = { 0.0f, 0.0f, transform->rotation };
    float scale[3] = { transform->scale.x, transform->scale.y, 1.0f };

    float matrix[16];
    ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, matrix);

    // Draw gizmo
    ImGuizmo::Manipulate(view, proj, m_gizmoOp, m_gizmoMode, matrix);

    // If gizmo was used, update component
    if (ImGuizmo::IsUsing())
    {
        float newT[3], newR[3], newS[3];
        ImGuizmo::DecomposeMatrixToComponents(matrix, newT, newR, newS);

        if (m_gizmoOp == ImGuizmo::TRANSLATE)
        {
            transform->position.x = newT[0];
            transform->position.y = newT[1];
        }
        else if (m_gizmoOp == ImGuizmo::ROTATE)
        {
            transform->rotation = newR[2];
        }
        else if (m_gizmoOp == ImGuizmo::SCALE)
        {
            transform->scale.x = newS[0];
            transform->scale.y = newS[1];
        }

        if (m_ctxRef->scenePanelRef)
            m_ctxRef->scenePanelRef->MarkSceneAsDirty();
    }
}

void ScenePanel::drawSelectionOutline()
{
    // TODO: implement selection outline
}