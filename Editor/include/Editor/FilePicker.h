#pragma once
#include <string>
#include <functional>

namespace Nova::Editor 
{
    bool DrawFilePicker(
        const char* id,
        std::string& currentPath,
        const char* filterExt,
        std::function<void(const std::string&)> onConfirm
    );
}