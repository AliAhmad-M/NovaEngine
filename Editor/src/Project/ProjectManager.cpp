#define _CRT_SECURE_NO_WARNINGS

#include <Editor/Project/ProjectManager.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace Nova::Editor;

static const char* kCMakeTemplate = R"CMAKE(cmake_minimum_required(VERSION 3.20)

project("@NAME@" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Supplied by the Nova Editor when the project is configured
set(NOVA_SDK_DIR "" CACHE PATH "Nova SDK location")

if(NOT NOVA_SDK_DIR)
    message(FATAL_ERROR "NOVA_SDK_DIR was not provided.")
endif()

if(NOT EXISTS "${NOVA_SDK_DIR}")
    message(FATAL_ERROR
        "Nova SDK does not exist: ${NOVA_SDK_DIR}")
endif()

list(APPEND CMAKE_PREFIX_PATH "${NOVA_SDK_DIR}")

find_package(Nova CONFIG REQUIRED)

# Project source files
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Never compile generated files inside build/
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/build/.*")

if(NOT GAME_SOURCES)
    message(FATAL_ERROR "No C++ source files found.")
endif()

# Game executable
add_executable(Game
    ${GAME_SOURCES}
)

target_include_directories(Game PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(Game PRIVATE
    Nova::Engine
)

# Lets main.cpp know where the project is located
target_compile_definitions(Game PRIVATE
    PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Output
set_target_properties(Game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)
)CMAKE";

static bool writeText(const fs::path& p, const std::string& text)
{
    std::ofstream f(p, std::ios::binary);
    if (!f)
        return false;

    f << text;
    return true;
}

static bool validName(const std::string& n)
{
    if (n.empty())
        return false;

    // Rejects invalid characters
    for (unsigned char c : n)
    {
        if (std::isalnum(c) ||
            c == ' ' ||
            c == '_' ||
            c == '-')
        {
            continue;
        }

        return false;
    }

    // Prevent names consisting only of whitespace
    return std::any_of(
        n.begin(),
        n.end(),
        [](unsigned char c)
        {
            return !std::isspace(c);
        });
}

static void replaceAll(
    std::string& s,
    const std::string& from,
    const std::string& to)
{
    if (from.empty())
        return;

    for (size_t pos = 0;
        (pos = s.find(from, pos)) != std::string::npos;
        pos += to.size())
    {
        s.replace(pos, from.size(), to);
    }
}

fs::path ProjectManager::ConfigDir()
{
#ifdef _WIN32
    const char* base = std::getenv("APPDATA");

    fs::path dir =
        fs::path(base ? base : ".") /
        "NovaEditor";
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    const char* home = std::getenv("HOME");

    fs::path dir =
        xdg
        ? fs::path(xdg) / "NovaEditor"
        : fs::path(home ? home : ".") /
        ".config" /
        "NovaEditor";
#endif

    std::error_code ec;
    fs::create_directories(dir, ec);

    return dir;
}

fs::path ProjectManager::SdkDir()
{
    // The SDK is installed next to the editor's working directory
    return fs::absolute(fs::current_path() / "sdk");
}

void ProjectManager::LoadRecent()
{
    m_recent.clear();

    std::ifstream f(
        ConfigDir() / "recent_projects.txt");

    std::string line;

    while (std::getline(f, line))
    {
        std::istringstream ss(line);

        ProjectEntry e;
        std::string t;

        if (std::getline(ss, e.name, '\t') &&
            std::getline(ss, e.path, '\t') &&
            std::getline(ss, t, '\t'))
        {
            e.lastOpened =
                std::atoll(t.c_str());

            m_recent.push_back(std::move(e));
        }
    }

    std::sort(
        m_recent.begin(),
        m_recent.end(),
        [](const ProjectEntry& a,
            const ProjectEntry& b)
        {
            return a.lastOpened > b.lastOpened;
        });
}

void ProjectManager::SaveRecent() const
{
    std::ofstream f(
        ConfigDir() / "recent_projects.txt",
        std::ios::trunc);

    for (const auto& e : m_recent)
    {
        f << e.name << '\t'
            << e.path << '\t'
            << e.lastOpened << '\n';
    }
}

void ProjectManager::Forget(const std::string& path)
{
    m_recent.erase(
        std::remove_if(
            m_recent.begin(),
            m_recent.end(),
            [&](const ProjectEntry& e)
            {
                return e.path == path;
            }),
        m_recent.end());

    SaveRecent();
}

void ProjectManager::touchRecent(const ProjectInfo& p)
{
    std::string path =
        p.root.lexically_normal().string();

    m_recent.erase(
        std::remove_if(
            m_recent.begin(),
            m_recent.end(),
            [&](const ProjectEntry& e)
            {
                return e.path == path;
            }),
        m_recent.end());

    ProjectEntry e;
    e.name = p.name;
    e.path = path;

    e.lastOpened =
        static_cast<std::int64_t>(
            std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now()));

    m_recent.insert(
        m_recent.begin(),
        std::move(e));

    if (m_recent.size() > 20)
        m_recent.resize(20);

    SaveRecent();
}

bool ProjectManager::Create(
    const std::string& parentDir,
    const std::string& name,
    std::string& outRoot,
    std::string& err)
{
    if (!validName(name))
    {
        err =
            "Project name may contain letters, digits, spaces, "
            "'_' and '-'.";
        return false;
    }

    std::error_code ec;

    fs::path parent =
        fs::absolute(parentDir, ec);

    if (ec)
    {
        err =
            "Invalid parent directory: " +
            ec.message();

        return false;
    }

    fs::path root =
        parent / name;

    if (fs::exists(root, ec) &&
        !fs::is_empty(root, ec))
    {
        err =
            "That folder already exists and is not empty.";

        return false;
    }

    fs::create_directories(root / "scenes", ec);
    if (ec)
    {
        err =
            "Could not create scenes folder: " +
            ec.message();

        return false;
    }

    fs::create_directories(root / "scripts", ec);
    if (ec)
    {
        err =
            "Could not create scripts folder: " +
            ec.message();

        return false;
    }

    fs::create_directories(root / "assets", ec);
    if (ec)
    {
        err =
            "Could not create assets folder: " +
            ec.message();

        return false;
    }

    // CMake
    std::string cmake = kCMakeTemplate;

    replaceAll(
        cmake,
        "@NAME@",
        name);

    // Project descriptor
    const std::string descriptor =
        "name=" + name + "\n"
        "engine=0.1\n";

    bool ok =
        writeText(
            root / kDescriptor,
            descriptor) &&

        writeText(
            root / "CMakeLists.txt",
            cmake) &&

        writeText(
            root / ".gitignore",
            "build/\n"
            ".vs/\n");

    if (!ok)
    {
        err =
            "Could not write project files.";

        return false;
    }

    outRoot = root.string();

    return true;
}

bool ProjectManager::Open(
    const std::string& rootDir,
    std::string& err)
{
    std::error_code ec;

    fs::path root =
        fs::absolute(rootDir, ec);

    if (ec)
    {
        err =
            "Invalid project path: " +
            ec.message();

        return false;
    }

    if (!fs::is_directory(root, ec))
    {
        err =
            "Project path is not a directory.";

        return false;
    }

    std::ifstream f(
        root / kDescriptor);

    if (!f)
    {
        err =
            "Not a Nova project "
            "(missing project.nova).";

        return false;
    }

    ProjectInfo p;

    p.root = root;
    p.name = root.filename().string();

    std::string line;

    while (std::getline(f, line))
    {
        if (line.rfind("name=", 0) == 0)
        {
            p.name =
                line.substr(5);
        }
    }

    p.assets = root / "assets";
    p.scenes = root / "scenes";
    p.scripts = root / "scripts";
    p.build = root / "build";

    // Allow older projects to be opened and upgraded automatically.
    fs::create_directories(p.assets, ec);
    if (ec)
    {
        err =
            "Could not create assets folder: " +
            ec.message();

        return false;
    }

    fs::create_directories(p.scenes, ec);
    if (ec)
    {
        err =
            "Could not create scenes folder: " +
            ec.message();

        return false;
    }

    fs::create_directories(p.scripts, ec);
    if (ec)
    {
        err =
            "Could not create scripts folder: " +
            ec.message();

        return false;
    }

    fs::create_directories(p.build, ec);
    if (ec)
    {
        err =
            "Could not create build folder: " +
            ec.message();

        return false;
    }

    fs::create_directories(root / "src", ec);
    if (ec)
    {
        err = "Could not create src folder: " + ec.message();
        return false;
    }

    const char* kMainTemplate = R"CPP(
        #include <Runtime/Engine.h>
        #include <Core/Services.h>

        #include <filesystem>

        using namespace Nova;

        int main()
        {
            Engine runtime;

            runtime.Init();

            SceneManager* sceneManager =
                Services::Fetch<SceneManager>();

            const std::filesystem::path projectDir =
                PROJECT_DIR;

            const std::filesystem::path scenesDir =
                projectDir / "scenes";

            // Prefer Main.nvsn.
            const std::filesystem::path mainScene =
                scenesDir / "Main.nvsn";

            if (std::filesystem::exists(mainScene))
            {
                sceneManager->ChangeScene(
                    mainScene.string());
            }
            else if (std::filesystem::exists(scenesDir))
            {
                // Otherwise use the first scene found.
                for (const auto& entry :
                     std::filesystem::directory_iterator(scenesDir))
                {
                    if (entry.path().extension() == ".nvsn")
                    {
                        sceneManager->ChangeScene(
                            entry.path().string());
                        break;
                    }
                }
            }

            runtime.Update();
            runtime.Close();

            return 0;
        }
    )CPP";

    if (!writeText(
        root / "src" / "main.cpp", kMainTemplate))
    {
        err = "Could not create main.cpp";
        return false;
    }

    m_current = std::move(p);
    m_hasProject = true;

    touchRecent(m_current);

    return true;
}