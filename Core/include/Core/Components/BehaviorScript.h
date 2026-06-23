#pragma once
#include <Core/Behavior/Behavior.h>
#include <memory>
#include <string>

namespace Nova
{
    struct BehaviorScript
    {
        BehaviorScript() = default;
        BehaviorScript(BehaviorScript&&) = default;
        BehaviorScript& operator=(BehaviorScript&&) = default;

        std::string               scriptPath;
        std::string               behaviorName;
        std::unique_ptr<Behavior> instance;
        bool started = false;
    };
}