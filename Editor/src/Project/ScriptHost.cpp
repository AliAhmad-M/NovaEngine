#include <Editor/Project/ScriptHost.h>
#include <Core/Behavior/BehaviorRegistry.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <future>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void* LoadLib(const std::string& p)
{
    return (void*)LoadLibraryA(p.c_str());
}

static void FreeLib(void* h)
{
    FreeLibrary((HMODULE)h);
}

static const char* kPrefix = "";
static const char* kExt = ".dll";

#elif defined(__linux__)

#include <dlfcn.h>

static void* LoadLib(const std::string& p)
{
    return dlopen(
        p.c_str(),
        RTLD_NOW | RTLD_LOCAL);
}

static void FreeLib(void* h)
{
    dlclose(h);
}

static const char* kPrefix = "lib";
static const char* kExt = ".so";

#else

#error "Unsupported platform"

#endif

namespace fs = std::filesystem;
using namespace Nova::Editor;

#ifdef NDEBUG
static const char* kConfig = "Release";
#else
static const char* kConfig = "Debug";
#endif

static std::string quotePath(const fs::path& p)
{
    return "\"" + p.generic_string() + "\"";
}

void ScriptHost::BuildAsync(const ProjectInfo& p)
{
    if (IsBuilding())
        return;

    // Consume a completed previous future before reusing it
    if (m_build.valid())
        m_build.get();

    std::error_code ec;

    fs::create_directories(
        p.build,
        ec);

    if (ec)
        return;

    m_logPath =
        (p.build / "build.log").string();

    const fs::path root =
        fs::absolute(p.root);

    const fs::path build =
        fs::absolute(p.build);

    const fs::path sdk =
        fs::absolute(
            ProjectManager::SdkDir());

    const std::string log =
        quotePath(
            fs::absolute(p.build / "build.log"));

    const std::string configure =
        "cmake"
        " -S " + quotePath(root) +
        " -B " + quotePath(build) +
        " -DNOVA_SDK_DIR=" + quotePath(sdk) +
        " -DCMAKE_BUILD_TYPE=" + kConfig +
        " > " + log +
        " 2>&1";

    const std::string compile =
        "cmake"
        " --build " + quotePath(build) +
        " --config " + kConfig +
        " --target GameScripts"
        " >> " + log +
        " 2>&1";

    const std::string cmd =
        configure +
        " && " +
        compile;

    m_build =
        std::async(
            std::launch::async,
            [cmd]()
            {
                return std::system(
                    cmd.c_str());
            });
}

bool ScriptHost::PollBuild(
    std::string& log,
    bool& ok)
{
    if (!m_build.valid())
        return false;

    if (m_build.wait_for(
        std::chrono::milliseconds(0)) !=
        std::future_status::ready)
    {
        return false;
    }

    const int result =
        m_build.get();

    ok = (result == 0);

    std::ifstream f(
        m_logPath);

    if (f)
    {
        std::stringstream ss;
        ss << f.rdbuf();
        log = ss.str();
    }
    else
    {
        log.clear();
    }

    return true;
}

bool ScriptHost::Load(
    const ProjectInfo& p)
{
    const fs::path src =
        p.build /
        "bin" /
        kConfig /
        (std::string(kPrefix) +
            "GameScripts" +
            kExt);

    if (!fs::exists(src))
        return false;

    // Shadow-copy the DLL
    const fs::path loadedDir =
        p.build / "loaded";

    std::error_code ec;

    fs::create_directories(
        loadedDir,
        ec);

    if (ec)
        return false;

    const fs::path dst =
        loadedDir /
        (std::string(kPrefix) +
            "GameScripts_" +
            std::to_string(++m_loadCounter) +
            kExt);

    fs::copy_file(
        src,
        dst,
        fs::copy_options::overwrite_existing,
        ec);

    if (ec)
        return false;

    // Snapshot registrations before loading
    const std::vector<std::string> before =
        BehaviorRegistry::Get().GetNames();

    m_handle =
        LoadLib(dst.string());

    if (!m_handle)
        return false;

    // Find registrations added by this DLL
    for (const auto& n :
        BehaviorRegistry::Get().GetNames())
    {
        if (std::find(
            before.begin(),
            before.end(),
            n) == before.end())
        {
            m_registered.push_back(n);
        }
    }

    return true;
}

void ScriptHost::Unload()
{
    if (!m_handle)
        return;

    // Destroy live behavior instances
    if (onBeforeUnload)
        onBeforeUnload();

    for (const auto& n :
        m_registered)
    {
        // BehaviorRegistry::Get().Unregister(n);
    }

    m_registered.clear();

    FreeLib(m_handle);
    m_handle = nullptr;
}