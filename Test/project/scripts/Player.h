#pragma once
#include <Core/Behavior/Behavior.h>
#include <Core/Debug.h>

class Player : public Nova::Behavior
{
public:
    const char* GetName() const override { return "Player"; }

    void OnStart()          override {}
    void OnUpdate(float dt) override {}
    void OnDestroy()        override {}
};
