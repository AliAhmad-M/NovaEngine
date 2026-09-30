#include <Editor/Panels/ScenePanel.h>
#include <Editor/Scene/SceneSerializer.h>
#include <Core/Services.h>

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <glad/glad.h>
#include <Windows.h>
#include <Core/Components.h>

using namespace Nova;
using namespace Nova::Editor;

static std::string findMSBuild()
{
    const char* vswhere =
        R"(C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe)";

    std::string cmd =
        std::string("\"") + vswhere + "\" -latest -requires Microsoft.Component.MSBuild"
        " -find MSBuild\\**\\Bin\\MSBuild.exe";

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = nullptr, hWrite = nullptr;
    CreatePipe(&hRead, &hWrite, &sa, 0);
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    std::string cmdLine = "cmd.exe /c \"" + cmd + "\"";

    if (!CreateProcessA(nullptr, cmdLine.data(), nullptr, nullptr,
        TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        CloseHandle(hWrite);
        CloseHandle(hRead);
        return {};
    }

    CloseHandle(hWrite);
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    char buf[512]{};
    DWORD bytesRead = 0;
    ReadFile(hRead, buf, sizeof(buf) - 1, &bytesRead, nullptr);
    CloseHandle(hRead);

    std::string path(buf, bytesRead);
    // Strip trailing newline/whitespace
    while (!path.empty() && (path.back() == '\n' || path.back() == '\r' || path.back() == ' '))
        path.pop_back();

    return path;
}

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
    if (m_building) return;

    m_building = true;
    m_showBuildLog = true;

    {
        std::lock_guard<std::mutex> lock(m_buildLogMutex);
        m_buildLog.clear();
        m_buildLog += "Nova Build Started\n";
    }

    std::string slnPath = std::filesystem::absolute(
        std::filesystem::current_path() / ".." / "NovaEngine.sln"
    ).string();

    // Test project file
    std::string vcxprojPath = std::filesystem::absolute(
        std::filesystem::current_path() / ".." / "Test" / "Test.vcxproj"
    ).string();

    const std::string configuration = "Release";
    const std::string platform = "x64";

    std::thread([this, slnPath, vcxprojPath, configuration, platform]()
        {
            std::string msbuild = findMSBuild();
            if (msbuild.empty())
            {
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                m_buildLog += "[ERROR] Could not locate MSBuild via vswhere. Is Visual Studio installed?\n";
                m_buildLogScrollToBottom = true;
                m_building = false;
                return;
            }

            // Resolve the output exe path
            std::string exePath = std::filesystem::absolute(
                std::filesystem::current_path() / ".." / "Test" 
                / "bin" / "x64" / "Release" / "Output" / "Test.exe"
            ).string();

            if (exePath.empty())
            {
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                m_buildLog += "[ERROR] Could not resolve TargetPath for the Test project.\n";
                m_buildLogScrollToBottom = true;
                m_building = false;
                return;
            }

            {
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                m_buildLog += "Resolved target exe: " + exePath + "\n";
                m_buildLogScrollToBottom = true;
            }

            // Snapshot the exe's last-write-time
            std::error_code fsErr;
            bool hadPreviousExe = std::filesystem::exists(exePath, fsErr);
            std::filesystem::file_time_type previousWriteTime{};
            if (hadPreviousExe)
                previousWriteTime = std::filesystem::last_write_time(exePath, fsErr);

            // Run the actual build
            SECURITY_ATTRIBUTES sa{};
            sa.nLength = sizeof(sa);
            sa.bInheritHandle = TRUE;

            HANDLE hReadPipe = nullptr;
            HANDLE hWritePipe = nullptr;
            CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
            SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

            STARTUPINFOA si{};
            si.cb = sizeof(si);
            si.hStdOutput = hWritePipe;
            si.hStdError = hWritePipe;
            si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            PROCESS_INFORMATION pi{};

            std::string cmdLine = "\"" + msbuild + "\" \"" + slnPath + "\" /t:Test /p:Configuration="
                + configuration + ";Platform=" + platform + " /m";

            // Run with CWD
            std::string slnDir = std::filesystem::path(slnPath).parent_path().string();

            bool launched = CreateProcessA(
                nullptr,
                cmdLine.data(),
                nullptr, nullptr,
                TRUE,
                CREATE_NO_WINDOW,
                nullptr,
                slnDir.c_str(),
                &si, &pi
            );

            CloseHandle(hWritePipe);

            if (!launched)
            {
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                m_buildLog += "[ERROR] Failed to launch msbuild. Is it on PATH?\n";
                m_buildLogScrollToBottom = true;
                m_building = false;
                CloseHandle(hReadPipe);
                return;
            }

            // Read pipe output
            char buf[512];
            DWORD bytesRead = 0;
            while (ReadFile(hReadPipe, buf, sizeof(buf) - 1, &bytesRead, nullptr) && bytesRead > 0)
            {
                buf[bytesRead] = '\0';
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                m_buildLog += buf;
                m_buildLogScrollToBottom = true;
            }

            CloseHandle(hReadPipe);

			// Wait for the build process to finish and get its exit code
            WaitForSingleObject(pi.hProcess, INFINITE);

            DWORD exitCode = 0;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            {
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                if (exitCode == 0)
                    m_buildLog += "\nBuild Succeeded\n";
                else
                    m_buildLog += "\nBuild FAILED (exit code " + std::to_string(exitCode) + ")\n";
                m_buildLogScrollToBottom = true;
            }

            if (exitCode != 0)
            {
                m_building = false;
                return;
            }

            // Verify that the exe was rewritten
            {
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                m_buildLog += "Attempting to launch: " + exePath + "\n";
                m_buildLogScrollToBottom = true;
            }

            if (!std::filesystem::exists(exePath, fsErr))
            {
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                m_buildLog += "[ERROR] Exe not found at resolved TargetPath after a successful build.\n";
                m_buildLog += "        This means MSBuild reported success but didn't produce the expected output — check the log above.\n";
                m_buildLogScrollToBottom = true;
                m_building = false;
                return;
            }

            if (hadPreviousExe)
            {
                auto newWriteTime = std::filesystem::last_write_time(exePath, fsErr);
                if (!fsErr && newWriteTime == previousWriteTime)
                {
                    std::lock_guard<std::mutex> lock(m_buildLogMutex);
                    m_buildLog += "[WARNING] MSBuild reported success but the exe's timestamp did NOT change.\n";
                    m_buildLog += "          You are about to launch a STALE binary. This usually means:\n";
                    m_buildLog += "            - the /t:Test target name doesn't match the project actually containing main.cpp, or\n";
                    m_buildLog += "            - the build considered everything already up to date incorrectly.\n";
                    m_buildLog += "          Try a full rebuild (msbuild /t:Test:Rebuild) from a terminal to confirm.\n";
                    m_buildLogScrollToBottom = true;
                }
            }

            {
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                m_buildLog += "Launching Game\n";
                m_buildLogScrollToBottom = true;
            }

            STARTUPINFOA runSi{};
            runSi.cb = sizeof(runSi);
            PROCESS_INFORMATION runPi{};
            std::string runCmd = "\"" + exePath + "\"";

            bool ok = CreateProcessA(nullptr, runCmd.data(), nullptr, nullptr,
                FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &runSi, &runPi);

            if (!ok)
            {
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                m_buildLog += "[ERROR] CreateProcessA failed. Error code: " + std::to_string(GetLastError()) + "\n";
                m_buildLogScrollToBottom = true;
            }
            else
            {
                if (runPi.hProcess) CloseHandle(runPi.hProcess);
                if (runPi.hThread)  CloseHandle(runPi.hThread);
            }

            m_building = false;

        }).detach();
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
                std::filesystem::path dir = std::filesystem::absolute(
                    std::filesystem::current_path() / ".." / "Test" / "project" / "scenes");
                m_ctxRef->activeScenePath = (dir / (tab.name + ".nova")).string();
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

void ScenePanel::startPlay()
{
    if (m_playing || m_building) return;

    if (!m_ctxRef->activeSceneRef)
    {
        Debug::LogError("Cannot play: no active scene");
        return;
    }

    m_playSnapshotPath =
        (std::filesystem::temp_directory_path() / "nova_play_snapshot.nova").string();

    SceneSerializer* serializer = Services::Fetch<SceneSerializer>();
    if (!serializer || !serializer->Save(*m_ctxRef->activeSceneRef, m_playSnapshotPath))
    {
        Debug::LogError("Failed to snapshot scene before play");
        m_playSnapshotPath.clear();
        return;
    }

    m_playing = true;
    m_showGameWindow = true;
}

void ScenePanel::stopPlay()
{
    if (!m_playing) return;

    m_playing = false;

    if (!m_playSnapshotPath.empty() && m_ctxRef->activeSceneRef)
    {
        std::vector<LoadedEntity> loaded;
        m_ecsRef->DeleteAllEntities();

        if (m_sceneManagerRef->GetLoader()->Load(m_playSnapshotPath, &loaded))
        {
            m_ctxRef->activeSceneRef->GetEntities().clear();
            for (const auto& e : loaded)
                m_ctxRef->activeSceneRef->AddEntity(e.id, e.name);
        }
        else
        {
            Debug::LogError("Failed to restore scene after play");
        }

        std::error_code ec;
        std::filesystem::remove(m_playSnapshotPath, ec);
        m_playSnapshotPath.clear();
    }
}

void ScenePanel::createGameFBO(int w, int h)
{
    m_gameVpWidth = w;
    m_gameVpHeight = h;

    glGenFramebuffers(1, &m_gameFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_gameFbo);

    glGenTextures(1, &m_gameColorTex);
    glBindTexture(GL_TEXTURE_2D, m_gameColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gameColorTex, 0);

    glGenRenderbuffers(1, &m_gameDepthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_gameDepthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_gameDepthRbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ScenePanel::resizeGameFBO(int w, int h)
{
    destroyGameFBO();
    createGameFBO(w, h);
}

void ScenePanel::destroyGameFBO()
{
    if (m_gameDepthRbo) { glDeleteRenderbuffers(1, &m_gameDepthRbo); m_gameDepthRbo = 0; }
    if (m_gameColorTex) { glDeleteTextures(1, &m_gameColorTex); m_gameColorTex = 0; }
    if (m_gameFbo) { glDeleteFramebuffers(1, &m_gameFbo); m_gameFbo = 0; }
    m_gameVpWidth = m_gameVpHeight = 0;
}

void ScenePanel::drawGameWindow()
{
    if (!m_showGameWindow) return;

    bool open = m_showGameWindow;

    ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Game", &open))
    {
        ImGui::End();
        if (!open && m_playing) stopPlay();
        m_showGameWindow = open;
        return;
    }

    ImGui::TextColored(m_playing ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
        m_playing ? "Playing" : "Stopped");

    ImVec2 viewSize = ImGui::GetContentRegionAvail();
    int w = (int)viewSize.x;
    int h = (int)viewSize.y;

    if (w > 0 && h > 0)
    {
        // Resize
        if (w != m_gameVpWidth || h != m_gameVpHeight)
            resizeGameFBO(w, h);

        if (m_gameColorTex)
        {
            ImGui::Image((ImTextureID)(intptr_t)m_gameColorTex, viewSize,
                ImVec2(0, 1), ImVec2(1, 0));
        }
    }

    ImGui::End();

    if (!open && m_playing)
        stopPlay();

    m_showGameWindow = open;
}

void ScenePanel::drawSceneToolBar()
{
    ImGui::Checkbox("Grid", &m_showGrid);
    ImGui::SameLine();

    if (ImGui::SmallButton("+ New Scene"))
        newScene();
    ImGui::SameLine();

    if (m_playing)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.25f, 0.25f, 1.0f));
        if (ImGui::SmallButton("Stop"))
            stopPlay();
        ImGui::PopStyleColor();
    }
    else if (m_building)
    {
        ImGui::BeginDisabled();
        ImGui::SmallButton("> Play");
        ImGui::EndDisabled();
    }
    else
    {
        if (ImGui::SmallButton("> Play"))
            startPlay();
    }
    ImGui::SameLine();

    if (m_building || m_playing)
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
            float newZoom = max(0.1f, min(oldZoom * (1.0f + wheel * 0.1f), 10.0f));
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
    destroyGameFBO();
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
    drawGameWindow();
}

void ScenePanel::newScene()
{
    int  idx = (int)m_sceneTabs.size();
    char name[64];
    std::snprintf(name, sizeof(name), "Untitled_%d", idx);

    SceneTab tab;
    tab.name = name;
    tab.isDirty = true;
    tab.scene = std::make_unique<Scene>(name);

    m_sceneTabs.push_back(std::move(tab));
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
    float halfW = (viewportSize.x * 0.5f) / zoom;
    float halfH = (viewportSize.y * 0.5f) / zoom;

    float left = camOffset.x - halfW;
    float right = camOffset.x + halfW;
    float top = camOffset.y - halfH;
    float bottom = camOffset.y + halfH;

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