#pragma once
#include <Core/Behavior/Behavior.h>
#include <Core/Services.h>
#include <Core/Components/Transform2D.h>
#include <Core/Debug.h>
#include <Platform/InputMap.h>
#include <Graphics/Components/Camera2D.h>

class MainShipMovement : public Nova::Behavior
{
public:
    const char* GetName() const override { return "Main Ship"; }

    void OnStart()          override;
    void OnUpdate(float dt) override;
    void OnDestroy()        override {}

    void ReflectFields(Nova::Inspectables& visitor) override
    {
        visitor.Float("Max Speed", &maxSpeed);
        visitor.Float("Acceleration", &acceleration);
        visitor.Float("Deceleration", &deceleration);
    }

private:
    Nova::InputMap* m_InputMapRef = nullptr;
    Nova::Transform2D* m_transform = nullptr;

    // Current velocity
    Nova::Vector2 m_velocity{ 0.0f, 0.0f };

    float maxSpeed = 140.0f;
    float acceleration = 575.0f;
    float deceleration = 890.0f;

    static constexpr float kMinX = 0.0f;
    static constexpr float kMaxX = 592.0f;
    static constexpr float kMinY = 0.0f;
    static constexpr float kMaxY = 312.0f;
};