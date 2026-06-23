#include <Runtime/Engine.h>
#include <Core/Services.h>
#include <Core/Components.h>
#include <Core/Systems.h>
#include <Graphics/Components.h>
#include <Graphics/Systems.h>
#include <Physics/Components.h>
#include <Physics/Systems.h>
#include <World/Components.h>
#include <World/Systems.h>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>

#include <thread>

using namespace Nova;

bool Engine::Init()
{
    // Register services
    Services::Init();
    Services::Register(&m_ecs);
    Services::Register(&m_window);
    Services::Register(&m_renderer);
    Services::Register(&m_contentLoader);
    Services::Register(&m_inputMap);
    Services::Register(&m_sceneManager);

    // Create window
    if (!m_window.Init(1280, 720, "Nova Engine")) 
    {
        return false;
    }

    // Init renderer
    m_renderer.Init();

    // Setup ECS
    m_ecs.Init();
    registerComponents();
    initializeSystems();

    // Initialize Jolt physics
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    return true;
}

void Engine::renderApplication() 
{
    Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(
        0, (float)m_window.GetWidth(), (float)m_window.GetHeight(), 0, -100.0f, 100.0f);

    m_renderer.Begin(projection);
        m_ecs.GetSystem<SpriteSystem2D>()->Update();
    m_renderer.Flush();
}

void Engine::Update()
{
    const double targetFrameTime = 1.0 / 60.0;
    double lastTime = glfwGetTime();

    while (m_running) 
    {
        double currentTime = glfwGetTime();
        double frameStartTime = currentTime;
        m_deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        if (m_deltaTime > 0.1) m_deltaTime = (float)targetFrameTime;

        m_window.PollEvents();
        if (m_window.ShouldClose()) m_running = false;

        m_inputMap.Update(m_window.GetNativeWindow());

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        m_ecs.GetSystem<ScriptSystem>()->Update(m_deltaTime);
		m_ecs.GetSystem<CameraSystem2D>()->Update(m_deltaTime);
		m_ecs.GetSystem<RigidBodySystem2D>()->Update(m_deltaTime);
		m_ecs.GetSystem<TilemapSystem2D>()->Update(); 

        renderApplication();

        m_window.SwapBuffers();

        while (glfwGetTime() < frameStartTime + targetFrameTime)
            std::this_thread::yield();
    }
}

void Engine::RenderEditorFrame(unsigned int targetFBO, int vpWidth, int vpHeight, Camera2D* editorCamera)
{
    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
    glViewport(0, 0, vpWidth, vpHeight);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Vector2 offset = editorCamera->GetOffset();
    float zoom = editorCamera->GetZoom();

    Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(
        offset.x, offset.x + vpWidth / zoom,
        offset.y + vpHeight / zoom, offset.y,
        -100.0f, 100.0f);

    m_renderer.Begin(projection);
    m_ecs.GetSystem<SpriteSystem2D>()->Update();
    m_ecs.GetSystem<TilemapSystem2D>()->Update();
    m_renderer.Flush();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Engine::Close() 
{
    // Clear Jolt instance
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
    JPH::UnregisterTypes();

    // Clear the engine context
    Services::Shutdown();
    m_ecs.DeleteAllEntities();
    m_contentLoader.Cleanup();
    m_window.Exit();
}

void Engine::registerComponents() 
{
    m_ecs.RegisterComponent<Transform2D>();
    m_ecs.RegisterComponent<BehaviorScript>();
    m_ecs.RegisterComponent<Sprite2D>();
    m_ecs.RegisterComponent<RigidBody2D>();
	m_ecs.RegisterComponent<Tilemap2D>();
	m_ecs.RegisterComponent<Camera2D>();
}

void Engine::initializeSystems() 
{
    // Initialize behavior script system
    m_ecs.RegisterSystem<ScriptSystem>();
    m_ecs.AddComponentToSystem<BehaviorScript, ScriptSystem>();

    // Initialize sprite system (2D)
    m_ecs.RegisterSystem<SpriteSystem2D>();
    m_ecs.AddComponentToSystem<Transform2D, SpriteSystem2D>();
    m_ecs.AddComponentToSystem<Sprite2D, SpriteSystem2D>();

	// Initialize camera system (2D)
	m_ecs.RegisterSystem<CameraSystem2D>();
	m_ecs.AddComponentToSystem<Camera2D, CameraSystem2D>();

    // Initialize physics system (2D)
    m_ecs.RegisterSystem<RigidBodySystem2D>();
    m_ecs.AddComponentToSystem<Transform2D, RigidBodySystem2D>();
    m_ecs.AddComponentToSystem<RigidBody2D, RigidBodySystem2D>();

	// Initialize tilemap system (2D)
	m_ecs.RegisterSystem<TilemapSystem2D>();
	m_ecs.AddComponentToSystem<Transform2D, TilemapSystem2D>();
	m_ecs.AddComponentToSystem<Tilemap2D, TilemapSystem2D>();
}