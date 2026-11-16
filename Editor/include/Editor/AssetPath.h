#pragma once
#include <filesystem>
#include <string>

namespace Nova::Editor
{
    inline constexpr const char* kAssetPayload = "NOVA_ASSET";

    inline std::string ToProjectRelative(const std::filesystem::path& p, const std::filesystem::path& root)
    {
        if (p.empty()) return {};
        if (root.empty()) return p.generic_string();
        auto rel = p.lexically_normal().lexically_relative(root.lexically_normal());
        if (rel.empty() || *rel.begin() == "..") return p.generic_string();
        return rel.generic_string();
    }

    inline std::filesystem::path ResolveProjectPath(const std::string& s, const std::filesystem::path& root)
    {
        std::filesystem::path p(s);
        return p.is_absolute() ? p : root / p;
    }
}