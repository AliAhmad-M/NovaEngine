#include <Editor/Panels/ScenePanel.h>

#include <Core/Services.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <glad/glad.h>
#include <Windows.h>

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

    std::string exePath = std::filesystem::absolute(
        std::filesystem::current_path() / ".." / "Test" / "bin" / "x64" / "Release" / "Output" / "Test.exe"
    ).string();

    std::thread([this, slnPath, exePath]()
        {
            std::string msbuild = findMSBuild();
            if (msbuild.empty())
            {
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                m_buildLog += "[ERROR] Could not locate MSBuild via vswhere. Is Visual Studio installed?\n";
                m_building = false;
                return;
            }

            // Build command
            std::string buildCmd =
                "\"" + msbuild + "\" \"" + slnPath + "\" /t:Test /p:Configuration=Release;Platform=x64 /m 2>&1";

            // Set up stdout pipe
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

            std::string cmdLine = "\"" + msbuild + "\" \"" + slnPath + "\" /t:Test /p:Configuration=Release;Platform=x64 /m";

            bool launched = CreateProcessA(
                nullptr,
                cmdLine.data(),
                nullptr, nullptr,
                TRUE,
                CREATE_NO_WINDOW,
                nullptr, nullptr,
                &si, &pi
            );

            CloseHandle(hWritePipe);

            if (!launched)
            {
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                m_buildLog += "[ERROR] Failed to launch msbuild. Is it on PATH?\n";
                m_building = false;
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

            DWORD exitCode = 0;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            {
                std::lock_guard<std::mutex> lock(m_buildLogMutex);
                if (exitCode == 0)
                    m_buildLog += "\nBuild Succeeded, Launching Game\n";
                else
                    m_buildLog += "\nBuild FAILED (exit code " + std::to_string(exitCode) + ")\n";
                m_buildLogScrollToBottom = true;
            }

            if (exitCode == 0)
            {
                // Log the path being attempted
                {
                    std::lock_guard<std::mutex> lock(m_buildLogMutex);
                    m_buildLog += "Attempting to launch: " + exePath + "\n";
                    m_buildLogScrollToBottom = true;
                }

                if (!std::filesystem::exists(exePath))
                {
                    std::lock_guard<std::mutex> lock(m_buildLogMutex);
                    m_buildLog += "[ERROR] Exe not found at that path. Check your output directory.\n";
                    m_buildLogScrollToBottom = true;
                }
                else
                {
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
                }
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
                m_ctx->activeScenePath = tab.filePath;
            }
            else
            {
                std::filesystem::path dir = std::filesystem::absolute(
                    std::filesystem::current_path() / ".." / "Test" / "project" / "scenes");
                m_ctx->activeScenePath = (dir / (tab.name + ".nova")).string();
            }
            m_ctx->activeScene = tab.scene.get();
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
    ImGui::Checkbox("Grid", &m_showGrid);
    ImGui::SameLine();

    if (ImGui::SmallButton("+ New Scene"))
        newScene();
    ImGui::SameLine();

    if (m_building)
    {
        ImGui::BeginDisabled();
        ImGui::SmallButton("Building...");
        ImGui::EndDisabled();
    }
    else
    {
        if (ImGui::SmallButton("> Play"))
            buildAndRunGame();
    }
    ImGui::SameLine();

    if (ImGui::SmallButton("Build Log"))
        m_showBuildLog = !m_showBuildLog;
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

    m_ctx->selectedEntity = -1;

    SceneTab& tab = m_sceneTabs[index];

    if (!tab.filePath.empty())
    {
        std::vector<LoadedEntity> loaded;
        if (m_sceneManager->GetLoader()->Load(tab.filePath, &loaded))
        {
            tab.scene->GetEntities().clear();
            for (const auto& e : loaded)
                tab.scene->AddEntity(e.id, e.name);

            tab.isDirty = false;
        }
    }
    else
    {
        m_ecs->DeleteAllEntities();
        tab.scene->GetEntities().clear();
    }
}

ScenePanel::ScenePanel(Context* ctx)
    : m_ctx(ctx)
{
    m_editorCamera.isMainCamera = true;
    m_ecs = Services::Fetch<ECS>();
    m_sceneManager = Services::Fetch<SceneManager>();
}

ScenePanel::~ScenePanel()
{
    destroyFBO();
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