#include <Graphics/Systems/CameraSystem2D.h>
#include <Core/Components/Transform2D.h>
#include <Core/EngineDebug.h>
#include <Platform/Window.h>
#include <Graphics/Components/Camera2D.h>

CameraSystem2D::CameraSystem2D(EngineContext* ctx) 
    : m_ctx(ctx) 
{
    if (!m_ctx) 
    {
        EngineDebug::LogErrorAndExit("CameraSystem2D -> EngineContext is NULL!");
    }
 
    if (!m_ctx->Get<Window>())
    {
        EngineDebug::LogErrorAndExit("CameraSystem2D -> Window is NULL!");
    }

    if (!m_ctx->Get<ECS>())
    {
        EngineDebug::LogErrorAndExit("CameraSystem2D -> ECS is NULL!");
    }
}

void CameraSystem2D::Update(float dt)
{
    Window* window = m_ctx->Get<Window>();
    ECS* ecs = m_ctx->Get<ECS>();

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

        // Register this camera component as the active context camera
        m_ctx->Register<Camera2D>(camera);
    }
    else
    {
        // No camera entity at all
        m_ctx->Unregister<Camera2D>();
    }
}