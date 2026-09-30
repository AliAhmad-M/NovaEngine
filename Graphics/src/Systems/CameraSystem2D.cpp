#include <Graphics/Systems/CameraSystem2D.h>
#include <Core/Services.h>
#include <Core/Debug.h>
#include <Core/Components/Transform2D.h>
#include <Graphics/Components/Camera2D.h>

using namespace Nova;

CameraSystem2D::CameraSystem2D() 
{
    m_windowRef = Services::Fetch<Window>();
    m_ecsRef = Services::Fetch<ECS>();
}

void CameraSystem2D::Update(float dt)
{
    int windowWidth, windowHeight;
    m_windowRef->GetSize(&windowWidth, &windowHeight);

    EntityID mainCamEntity = INVALID_ENTITY;

    // Find the first camera flagged as main
    for (EntityID e : entities)
    {
        Camera2D* c = m_ecsRef->GetComponent<Camera2D>(e);
        if (!c) continue;
        if (c->isMain)
        {
            mainCamEntity = e;
            break;
        }
    }
    if (mainCamEntity == INVALID_ENTITY && !entities.empty())
        mainCamEntity = *entities.begin();

    // Update and register / unregister
    if (mainCamEntity != INVALID_ENTITY)
    {
        Camera2D* c = m_ecsRef->GetComponent<Camera2D>(mainCamEntity);

        // Update camera state
        c->SetViewportSize(windowWidth, windowHeight);

        // Register this as main camera
        Services::Register<Camera2D>(c);
    }
    else
    {
        // No main camera entity at all
        Services::Unregister<Camera2D>();
    }
}