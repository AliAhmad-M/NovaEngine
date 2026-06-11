#pragma once
#include <string>

// Shared context across multiple panels
struct EditorContext
{
    int selectedEntity = -1;
    std::string activeScenePath;
};