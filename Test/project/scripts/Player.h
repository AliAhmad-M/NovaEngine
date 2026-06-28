#pragma once
#include <Core/Behavior/Behavior.h>
#include <Core/Services.h>
#include <Core/Debug.h>
#include <Graphics/Components/Camera2D.h>

class Player : public Nova::Behavior
{
public:
    const char* GetName() const override { return "Player"; }

    void OnStart()          override {}
    void OnUpdate(float dt) override
    {
        Nova::Camera2D* camera = Nova::Services::Fetch<Nova::Camera2D>();
    }
    void OnDestroy()        override {}
};
