#include <Graphics/Systems/CameraSystem2D.h>
#include <Core/Services.h>
#include <Core/Debug.h>
#include <Core/Components/Transform2D.h>
#include <Platform/Window.h>
#include <Graphics/Components/Camera2D.h>

using namespace Nova;

void CameraSystem2D::Update(float dt)
{
    Window* window  = Services::Fetch<Window>();
    ECS* ecs        = Services::Fetch<ECS>();

    int windowWidth, windowHeight;
    window->GetSize(&windowWidth, &windowHeight);

    EntityID mainCamEntity = INVALID_ENTITY;

    // Find the first camera flagged as main
    for (EntityID e : entities)
    {
        Camera2D* cam = ecs->GetComponent<Camera2D>(e);
        if (!cam) continue;
        if (cam->isMainCamera)
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
        Camera2D* camera = ecs->GetComponent<Camera2D>(mainCamEntity);

        // Update camera state
        camera->SetViewportSize(windowWidth, windowHeight);

        // Register this as main camera
        Services::Register<Camera2D>(camera);
    }
    else
    {
        // No main camera entity at all
        Services::Unregister<Camera2D>();
    }
}