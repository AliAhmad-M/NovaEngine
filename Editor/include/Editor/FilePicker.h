#pragma once
#include <string>
#include <functional>

namespace Nova::Editor 
{
    // Native OS file and folder dialogs (standard Windows Explorer)
    std::string OpenNativeFileDialog(
        const char* title,
        const char* filterName,
        const char* filterSpec,
        const std::string& defaultPath = ""
    );

    std::string SaveNativeFileDialog(
        const char* title,
        const char* filterName,
        const char* filterSpec,
        const std::string& defaultPath = "",
        const std::string& defaultName = ""
    );

    std::string OpenNativeFolderDialog(
        const char* title,
        const std::string& defaultPath = ""
    );

    // In-engine ImGui file picker (fallback / internal modal)
    bool DrawFilePicker(
        const char* id,
        std::string& currentPath,
        const char* filterExt = nullptr,
        std::function<void(const std::string&)> onConfirm = nullptr
    );
}