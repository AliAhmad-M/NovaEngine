#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace Nova::Editor
{
    struct InputBindingSetting
    {
        std::string action;
        int key = -1;
    };

    class ProjectSettings
    {
    public:
        static constexpr const char* kFileName = "settings.json";

        void Load(const std::filesystem::path& projectRoot);
        bool Save() const;
        void Clear();

        bool IsLoaded() const { return !m_path.empty(); }
        std::vector<InputBindingSetting>& Input() { return m_input; }

    private:
        std::filesystem::path m_path;
        nlohmann::json m_extra = nlohmann::json::object();
        std::vector<InputBindingSetting> m_input;
    };
}