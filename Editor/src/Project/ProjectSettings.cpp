#include <Editor/Project/ProjectSettings.h>
#include <fstream>

namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace Nova::Editor;

void ProjectSettings::Clear()
{
    m_path.clear();
    m_extra = json::object();
    m_input.clear();
}

void ProjectSettings::Load(const fs::path& projectRoot)
{
    Clear();
    m_path = projectRoot / kFileName;

    std::ifstream f(m_path);
    if (!f)
        return;   // no file yet, first Save() creates it

    json root = json::parse(f, nullptr, false);
    if (root.is_discarded() || !root.is_object())
        return;

    if (auto it = root.find("input"); it != root.end() && it->is_object())
    {
        if (auto b = it->find("bindings"); b != it->end() && b->is_array())
        {
            for (const auto& e : *b)
            {
                if (e.is_object() &&
                    e.contains("action") && e["action"].is_string() &&
                    e.contains("key") && e["key"].is_number_integer())
                {
                    m_input.push_back({ e["action"].get<std::string>(),
                                        e["key"].get<int>() });
                }
            }
        }
        root.erase("input");
    }

    m_extra = std::move(root);
}

bool ProjectSettings::Save() const
{
    if (m_path.empty())
        return false;

    json root = m_extra;
    root["version"] = 1;

    json arr = json::array();
    for (const auto& b : m_input)
        arr.push_back({ {"action", b.action}, {"key", b.key} });
    root["input"]["bindings"] = std::move(arr);

    const fs::path tmp = m_path.string() + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out)
            return false;
        out << root.dump(4);
    }

    std::error_code ec;
    fs::rename(tmp, m_path, ec);
    return !ec;
}