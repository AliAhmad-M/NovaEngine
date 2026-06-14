#pragma once
#include <Runtime/ContentLoader.h>
#include <string>

// Shared context across multiple panels
struct EditorContext
{
    int selectedEntity = -1;
    std::string activeScenePath;

    ContentLoader contentLoader;
};