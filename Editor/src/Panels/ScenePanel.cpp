#include <Editor/Panels/ScenePanel.h>
#include <Editor/Scene/SceneSerializer.h>
#include <Editor/Project/ProjectManager.h>
#include <Core/Services.h>
#include <Core/Components.h>
#include <Physics/Components/RigidBody2D.h>
#include <Platform/Window.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <glad/glad.h>
#if defined(APIENTRY)
#undef APIENTRY
#endif
#include <Windows.h>
#include <shellapi.h>

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

    // Save scene before building
    if (m_ctxRef->onSaveScene)
        m_ctxRef->onSaveScene();

    m_focusBuildOutputOnce = true;
    if (!BuildOnly([this](bool succeeded)
        {
            if (succeeded)
                launchBuiltGame();
        }))
    {
        appendBuildLog("[ERROR] A build is already running or no project is open.\n");
    }
}

bool ScenePanel::BuildOnly(std::function<void(bool)> onComplete)
{
    return startBuild(false, std::move(onComplete));
}

void ScenePanel::BuildScriptsNow(std::function<void(bool)> onComplete)
{
    if (m_building || m_scriptBuildPending)
    {
        m_scriptBuildPending = true;
        m_scriptBuildDue = std::chrono::steady_clock::now();
        if (onComplete)
            m_scriptBuildCompletions.push_back(std::move(onComplete));
        return;
    }

    startBuild(true, std::move(onComplete));
}

void ScenePanel::RequestScriptBuild()
{
    m_scriptBuildPending = true;
    m_scriptBuildDue = std::chrono::steady_clock::now() + std::chrono::milliseconds(400);
}

void ScenePanel::PollBuild()
{
    bool ready = false;
    bool succeeded = false;
    {
        std::lock_guard<std::mutex> lock(m_buildResultMutex);
        ready = m_buildResultReady;
        succeeded = m_buildResultSucceeded;
        m_buildResultReady = false;
    }

    if (ready)
    {
        if (m_buildThread.joinable())
            m_buildThread.join();
        m_building = false;

        auto completion = std::move(m_buildCompletion);
        if (completion)
            completion(succeeded);
    }

    if (!m_building && m_scriptBuildPending &&
        std::chrono::steady_clock::now() >= m_scriptBuildDue)
    {
        m_scriptBuildPending = false;
        auto callbacks = std::move(m_scriptBuildCompletions);
        m_scriptBuildCompletions.clear();
        startBuild(true, [callbacks = std::move(callbacks)](bool buildSucceeded)
            {
                for (const auto& callback : callbacks)
                    if (callback) callback(buildSucceeded);
            });
    }
}

void ScenePanel::appendBuildLog(const std::string& text)
{
    std::lock_guard<std::mutex> lock(m_buildLogMutex);
    m_buildLog += text;
    m_buildLogScrollToBottom = true;
}

std::string ScenePanel::GetLastBuildLogLine()
{
    std::lock_guard<std::mutex> lock(m_buildLogMutex);

    const size_t end = m_buildLog.find_last_not_of(" \r\n");
    if (end == std::string::npos)
        return {};

    size_t start = m_buildLog.find_last_of('\n', end);
    start = (start == std::string::npos) ? 0 : start + 1;
    return m_buildLog.substr(start, end - start + 1);
}

void ScenePanel::DrawBuildOutput()
{
    drawBuildLog();
}

void ScenePanel::launchBuiltGame()
{
    ProjectManager* pm = Services::Fetch<ProjectManager>();
    if (!pm || !pm->HasProject()) return;

    const ProjectInfo project = pm->Current();
    const std::filesystem::path root = std::filesystem::absolute(project.root);
    const std::filesystem::path exePath =
        std::filesystem::absolute(project.build) / "bin" / "Release" / "Game.exe";

    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::string command = "\"" + exePath.string() + "\"";

    if (!CreateProcessA(nullptr, command.data(), nullptr, nullptr, FALSE,
        CREATE_NEW_CONSOLE, nullptr, root.string().c_str(), &startup, &process))
    {
        appendBuildLog("[ERROR] Failed to launch Game.exe. Error code: " +
            std::to_string(GetLastError()) + "\n");
        return;
    }

    CloseHandle(process.hProcess);
    CloseHandle(process.hThread);
    appendBuildLog("Launching: " + exePath.string() + "\n");
}

bool ScenePanel::startBuild(bool scriptsOnly, std::function<void(bool)> onComplete)
{
    if (m_building)
        return false;

    ProjectManager* pm = Services::Fetch<ProjectManager>();
    if (!pm || !pm->HasProject())
        return false;

    if (m_buildThread.joinable())
        m_buildThread.join();

    const ProjectInfo project = pm->Current();
    const std::filesystem::path root = std::filesystem::absolute(project.root);
    const std::filesystem::path buildDir = std::filesystem::absolute(project.build);
    const std::filesystem::path sdkDir = std::filesystem::absolute(ProjectManager::SdkDir());
    const std::filesystem::path exePath = buildDir / "bin" / "Release" / "Game.exe";
    const std::string configuration = "Release";

    m_building = true;
    m_buildCompletion = std::move(onComplete);
    {
        std::lock_guard<std::mutex> lock(m_buildLogMutex);
        m_buildLog = scriptsOnly ? "Nova GameScripts Build Started\n" : "Nova Game Build Started\n";
        m_buildLog += "Project: " + root.string() + "\n";
        m_buildLog += "Output: " + buildDir.string() + "\n\n";
        m_buildLogScrollToBottom = true;
    }
    {
        std::lock_guard<std::mutex> lock(m_buildResultMutex);
        m_buildResultReady = false;
        m_buildResultSucceeded = false;
    }

    m_buildThread = std::thread(
        [this,
        scriptsOnly,
        root,
        buildDir,
        sdkDir,
        exePath,
        configuration]()
        {
            auto finishBuild = [this](bool succeeded)
                {
                    {
                        std::lock_guard<std::mutex> lock(m_buildResultMutex);
                        m_buildResultSucceeded = succeeded;
                        m_buildResultReady = true;
                    }
                };

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

                    finishBuild(false);
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
                    finishBuild(false);
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

                    finishBuild(false);
                    return;
                }
            }

            // Build the requested target. CMake keeps this incremental.

            std::string buildCmd =
                "cmake"
                " --build \"" +
                buildDir.string() +
                "\""
                " --config " +
                configuration;
            if (scriptsOnly)
                buildCmd += " --target GameScripts";

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

                    finishBuild(false);
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
                    finishBuild(false);
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

                    finishBuild(false);
                    return;
                }
            }

            if (!scriptsOnly && !std::filesystem::exists(exePath))
            {
                appendLog(
                    "\n[ERROR] Build succeeded but "
                    "Game.exe was not found at:\n" +
                    exePath.string() +
                    "\n");

                finishBuild(false);
                return;
            }

            if (!scriptsOnly)
            {
                appendLog("\nBuild Succeeded.\n");

                // Copy vcpkg DLLs next to Game.exe
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
            else
                appendLog("\nGameScripts Build Succeeded.\n");

            finishBuild(true);
        });
    return true;
}

void ScenePanel::drawBuildLog()
{
    ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiCond_FirstUseEver);
    if (m_focusBuildOutputOnce)
    {
        ImGui::SetNextWindowFocus();
        m_focusBuildOutputOnce = false;
    }

    if (!ImGui::Begin("Build Output"))
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

    ImGui::Separator();

    ImGui::BeginChild("##buildlog_scroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    bool scrollToBottom = false;
    {
        std::lock_guard<std::mutex> lock(m_buildLogMutex);
        ImGui::TextUnformatted(m_buildLog.c_str());
        scrollToBottom = m_buildLogScrollToBottom;
        m_buildLogScrollToBottom = false;
    }

    if (scrollToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
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
        if (m_tabToSelect == i)
        {
            itemFlags |= ImGuiTabItemFlags_SetSelected;
            m_tabToSelect = -1;
        }

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

void ScenePanel::drawSceneToolBar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 3.0f));

    // Gizmo operation helper button
    auto gizmoBtn = [](const char* label, bool active, const char* tooltip)
        {
            if (active)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.52f, 0.90f, 0.90f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.22f, 0.50f));
            }
            bool clicked = ImGui::Button(label, ImVec2(26.0f, 0.0f));
            if (active)
                ImGui::PopStyleColor(2);
            else
                ImGui::PopStyleColor();
            if (tooltip && ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", tooltip);
            return clicked;
        };

    if (gizmoBtn("T", m_gizmoOp == ImGuizmo::TRANSLATE, "Translate (W)"))
        m_gizmoOp = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (gizmoBtn("R", m_gizmoOp == ImGuizmo::ROTATE, "Rotate (E)"))
        m_gizmoOp = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (gizmoBtn("S", m_gizmoOp == ImGuizmo::SCALE, "Scale (R)"))
        m_gizmoOp = ImGuizmo::SCALE;
    ImGui::SameLine();

    bool isLocal = (m_gizmoMode == ImGuizmo::LOCAL);
    if (ImGui::Button(isLocal ? "Local" : "World", ImVec2(48.0f, 0.0f)))
        m_gizmoMode = isLocal ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle Local/World space coordinate system");

    ImGui::SameLine(0, 12);
    ImGui::TextDisabled("|");
    ImGui::SameLine(0, 12);

    ImGui::Checkbox("Grid", &m_showGrid);

    ImGui::SameLine(0, 8);
    ImGui::Checkbox("Physics", &m_showPhysicsDebug);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Show rigid body colliders as debug boxes/circles");

    ImGui::SameLine(0, 12);
    ImGui::TextDisabled("|");
    ImGui::SameLine(0, 12);

    if (ImGui::Button("+ New Scene"))
        NewScene();

    ImGui::SameLine(0, 12);

    if (m_building)
    {
        ImGui::BeginDisabled();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.5f, 0.1f, 0.6f));
        ImGui::Button("Building...");
        ImGui::PopStyleColor();
        ImGui::EndDisabled();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.28f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.68f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.45f, 0.22f, 1.0f));
        if (ImGui::Button("> Build & Run"))
            buildAndRunGame();
        ImGui::PopStyleColor(3);
    }

    ImGui::PopStyleVar();

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

    const ImVec2 imgMin = ImGui::GetItemRectMin();
    const ImVec2 imgSize = ImGui::GetItemRectSize();

    if (m_showPhysicsDebug)
        DrawRigidBodyDebug(imgMin, imgSize);

    DrawGizmo();

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

    DrawViewOutlines(imgMin, imgSize);
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

    // Only attempt to load from disk if the file exists on disk
    std::error_code ec;
    if (!tab.filePath.empty() && std::filesystem::exists(tab.filePath, ec))
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

ScenePanel::ScenePanel(EditorContext* ctx)
    : m_ctxRef(ctx)
{
    m_editorCamera.isMain = true;
    m_ecsRef = Services::Fetch<ECS>();
    m_sceneManagerRef = Services::Fetch<SceneManager>();
}

ScenePanel::~ScenePanel()
{
    if (m_buildThread.joinable())
        m_buildThread.join();
    destroyFBO();
}

void ScenePanel::Draw()
{
    ImGui::Begin("Scene");
    drawSceneToolBar();
    ImGui::Separator();

    if (m_sceneTabs.empty())
    {
        drawNoScenePlaceholder();
    }
    else
    {
        drawTabBar();
        ImGui::Separator();
        drawSceneViewport();
    }

    ImGui::End();

    drawBuildLog();
}

void ScenePanel::drawNoScenePlaceholder()
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float textHeight = ImGui::GetTextLineHeight() * 3.0f;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (avail.y - textHeight) * 0.4f);

    const char* msg = "No scene is open";
    float textW = ImGui::CalcTextSize(msg).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - textW) * 0.5f);
    ImGui::TextDisabled("%s", msg);

    const char* hint = "Use File > Open Scene or Ctrl+O to open a scene";
    float hintW = ImGui::CalcTextSize(hint).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - hintW) * 0.5f);
    ImGui::TextDisabled("%s", hint);
}

void ScenePanel::NewScene()
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
    m_tabToSelect = (int)m_sceneTabs.size() - 1;
}

void ScenePanel::closeScene(int index)
{
    if (index < 0 || index >= (int)m_sceneTabs.size()) return;
    m_sceneTabs.erase(m_sceneTabs.begin() + index);
    if (m_activeTab >= (int)m_sceneTabs.size())
        m_activeTab = (int)m_sceneTabs.size() - 1;

    // If all scenes are closed, clear context to prevent null dereferences
    if (m_sceneTabs.empty())
    {
        m_activeTab = -1;
        m_ctxRef->activeSceneRef = nullptr;
        m_ctxRef->activeScenePath.clear();
        m_ctxRef->selectedEntity = -1;

        // Clear the ECS since no scene is active
        m_ecsRef->DeleteAllEntities();
    }
}

void ScenePanel::CloseAllScenes()
{
    m_sceneTabs.clear();
    m_activeTab = -1;
    m_lastActiveTab = -1;
    m_tabToSelect = -1;
    m_ctxRef->activeSceneRef = nullptr;
    m_ctxRef->activeScenePath.clear();
    m_ctxRef->selectedEntity = -1;
    m_ecsRef->DeleteAllEntities();
}

void ScenePanel::SetActiveSceneFilePath(const std::string& filePath)
{
    if (m_activeTab < 0 || m_activeTab >= static_cast<int>(m_sceneTabs.size()))
        return;

    SceneTab& tab = m_sceneTabs[m_activeTab];
    tab.filePath = filePath;
    tab.name = std::filesystem::path(filePath).stem().string();
    tab.scene->SetFilePath(filePath);
    m_ctxRef->activeScenePath = filePath;
}

void ScenePanel::OpenSceneTab(const std::string& filePath)
{
    for (int i = 0; i < (int)m_sceneTabs.size(); i++)
    {
        if (m_sceneTabs[i].filePath == filePath)
        {
            m_tabToSelect = i;
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
    m_tabToSelect = (int)m_sceneTabs.size() - 1;
}

void ScenePanel::SetActiveTab(int index)
{
    if (index >= 0 && index < (int)m_sceneTabs.size())
        m_tabToSelect = index;
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

std::vector<std::string> ScenePanel::GetOpenScenePaths() const
{
    std::vector<std::string> paths;
    for (const auto& tab : m_sceneTabs)
    {
        if (!tab.filePath.empty())
            paths.push_back(tab.filePath);
    }
    return paths;
}

void ScenePanel::DrawGizmo()
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

    float left = camOffset.x;
    float right = camOffset.x + viewportSize.x / zoom;
    float top = camOffset.y;
    float bottom = camOffset.y + viewportSize.y / zoom;

    // Orthographic projection matrix
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

void ScenePanel::DrawRigidBodyDebug(ImVec2 imgMin, ImVec2 imgSize)
{
    if (!m_ctxRef->activeSceneRef || !m_ecsRef) return;

    const float zoom = m_editorCamera.GetZoom();
    if (zoom <= 0.0f || !std::isfinite(zoom)) return;

    const Vector2 offset = m_editorCamera.GetOffset();

    auto toScreen = [&](float wx, float wy)
        {
            return ImVec2(
                imgMin.x + (wx - offset.x) * zoom,
                imgMin.y + (wy - offset.y) * zoom
            );
        };

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(
        imgMin,
        ImVec2(imgMin.x + imgSize.x, imgMin.y + imgSize.y),
        true
    );

    // Dynamic = green, Static = blue, Sensor = yellow
    const ImU32 dynamicFill = IM_COL32(80, 220, 120, 55);
    const ImU32 dynamicLine = IM_COL32(80, 220, 120, 220);
    const ImU32 staticFill = IM_COL32(120, 200, 255, 45);
    const ImU32 staticLine = IM_COL32(120, 200, 255, 200);
    const ImU32 sensorFill = IM_COL32(255, 200, 80, 45);
    const ImU32 sensorLine = IM_COL32(255, 200, 80, 210);

    for (const auto& entity : m_ctxRef->activeSceneRef->GetEntities())
    {
        const EntityID id = static_cast<EntityID>(entity.id);

        const RigidBody2D* rb = m_ecsRef->GetComponent<RigidBody2D>(id);
        if (!rb) continue;

        const Transform2D* transform = m_ecsRef->GetComponent<Transform2D>(id);
        if (!transform) continue;

        ImU32 fill, line;
        if (rb->IsSensor()) { fill = sensorFill;  line = sensorLine; }
        else if (rb->IsStatic()) { fill = staticFill;  line = staticLine; }
        else { fill = dynamicFill; line = dynamicLine; }

        const float cosR = std::cos(transform->rotation);
        const float sinR = std::sin(transform->rotation);

        // World-space center of the collider: body position + rotated (scaled) offset
        const float ox = rb->centerOffset.x * transform->scale.x;
        const float oy = rb->centerOffset.y * transform->scale.y;
        const float cx = transform->position.x + ox * cosR - oy * sinR;
        const float cy = transform->position.y + ox * sinR + oy * cosR;

        if (rb->GetShape() == Shape2D::CIRCLE)
        {
            const float r = rb->GetRadius()
                * std::max(transform->scale.x, transform->scale.y)
                * zoom;
            if (r <= 0.0f) continue;

            const ImVec2 c = toScreen(cx, cy);
            dl->AddCircleFilled(c, r, fill, 0);
            dl->AddCircle(c, r, line, 0, 1.5f);
        }
        else if (rb->GetShape() == Shape2D::BOX)
        {
            const float hw = rb->GetFullWidth() * transform->scale.x * 0.5f;
            const float hh = rb->GetFullHeight() * transform->scale.y * 0.5f;
            if (hw <= 0.0f || hh <= 0.0f) continue;

            auto corner = [&](float lx, float ly)
                {
                    const float wx = cx + lx * cosR - ly * sinR;
                    const float wy = cy + lx * sinR + ly * cosR;
                    return toScreen(wx, wy);
                };

            const ImVec2 c0 = corner(-hw, -hh);
            const ImVec2 c1 = corner(hw, -hh);
            const ImVec2 c2 = corner(hw, hh);
            const ImVec2 c3 = corner(-hw, hh);

            dl->AddQuadFilled(c0, c1, c2, c3, fill);
            dl->AddQuad(c0, c1, c2, c3, line, 1.5f);
        }
    }

    dl->PopClipRect();
}

void ScenePanel::DrawViewOutlines(ImVec2 imgMin, ImVec2 imgSize)
{
    // Fixed runtime game resolution
    constexpr float gameWidth = 1280.0f;
    constexpr float gameHeight = 720.0f;

    // Editor camera
    const float editorZoom = m_editorCamera.GetZoom();
    const Vector2 editorOffset = m_editorCamera.GetOffset();

    if (editorZoom <= 0.0f || !std::isfinite(editorZoom))
        return;

    auto toScreen = [&](float worldX, float worldY)
        {
            return ImVec2(
                imgMin.x + (worldX - editorOffset.x) * editorZoom,
                imgMin.y + (worldY - editorOffset.y) * editorZoom
            );
        };

    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->PushClipRect(
        imgMin,
        ImVec2(imgMin.x + imgSize.x, imgMin.y + imgSize.y),
        true
    );

    bool drewMainCamera = false;

    // Find the scene's main camera
    if (m_ctxRef->activeSceneRef && m_ecsRef)
    {
        for (const auto& entity : m_ctxRef->activeSceneRef->GetEntities())
        {
            auto* camera = m_ecsRef->GetComponent<decltype(m_editorCamera)>(
                static_cast<EntityID>(entity.id)
            );

            if (!camera || !camera->isMain)
                continue;

            const float zoom = camera->GetZoom();

            if (zoom <= 0.0f || !std::isfinite(zoom))
                continue;

            const Vector2 offset = camera->GetOffset();

            // Camera's visible world dimensions
            const float width = gameWidth / zoom;
            const float height = gameHeight / zoom;

            dl->AddRect(
                toScreen(offset.x, offset.y),
                toScreen(offset.x + width, offset.y + height),
                IM_COL32(100, 220, 255, 210),
                0.0f, 0, 2.0f
            );

            drewMainCamera = true;
            break;
        }
    }

    // Default game boundary
    if (!drewMainCamera)
    {
        dl->AddRect(
            toScreen(0.0f, 0.0f),
            toScreen(gameWidth, gameHeight),
            IM_COL32(255, 255, 100, 160),
            0.0f, 0, 2.0f
        );
    }

    dl->PopClipRect();
}