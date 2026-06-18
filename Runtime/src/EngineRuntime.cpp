#include <Runtime/EngineRuntime.h>
#include <Core/Components.h>
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

EngineContext* EngineRuntime::GetContext()
{
    return &m_context;
}

bool EngineRuntime::Init(bool isEditor)
{
    if (isEditor) 
    {
        // Setup ECS
        m_ecs.Init();
        registerComponents();
        initializeSystems();

        m_context.Register(&m_ecs);
        m_context.Register(&m_contentLoader);

        return true;
    }

    // Create window
    if (!m_window.Init(1280, 720, "Star Platinum Engine")) 
    {
        return false;
    }

    // Init renderer
    m_renderer.Init();

    // Register engine context
    m_context.Register(&m_ecs);
    m_context.Register(&m_window);
    m_context.Register(&m_inputMap);
    m_context.Register(&m_renderer);
    m_context.Register(&m_contentLoader);
    m_context.Register(&m_sceneManager);

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

void EngineRuntime::Update()
{
    const double targetFrameTime = 1.0 / 60.0;
    double lastTime = glfwGetTime();

    while (m_running) {
        double currentTime = glfwGetTime();
        double frameStartTime = currentTime;
        m_deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (m_deltaTime > 0.1) m_deltaTime = targetFrameTime;

        m_window.PollEvents();
        if (m_window.ShouldClose()) m_running = false;

        m_inputMap.Update(m_window.GetNativeWindow());

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Update systems
		m_ecs.GetSystem<CameraSystem2D>()->Update((float)m_deltaTime);
		m_ecs.GetSystem<RigidBodySystem2D>()->Update((float)m_deltaTime);
		m_ecs.GetSystem<TilemapSystem2D>()->Update((float)m_deltaTime); 

        // Update the active scene
        m_sceneManager.UpdateScene((float)m_deltaTime);

        m_window.SwapBuffers();

        while (glfwGetTime() < frameStartTime + targetFrameTime)
            std::this_thread::yield();
    }
}

void EngineRuntime::Close() 
{
    // Clear Jolt instance
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
    JPH::UnregisterTypes();

    // Clear the engine context
    m_ecs.DeleteAllEntities();
    m_contentLoader.Cleanup();
    m_window.Exit();
}

void EngineRuntime::registerComponents() 
{
    m_ecs.RegisterComponent<Transform2D>();
    m_ecs.RegisterComponent<Sprite2D>();
    m_ecs.RegisterComponent<RigidBody2D>();
	m_ecs.RegisterComponent<Tilemap2D>();
	m_ecs.RegisterComponent<Camera2D>();
}

void EngineRuntime::initializeSystems() 
{
    // Initialize sprite system (2D)
    m_ecs.RegisterSystem<SpriteSystem2D>(&m_context);
    m_ecs.AddComponentToSystem<Transform2D, SpriteSystem2D>();
    m_ecs.AddComponentToSystem<Sprite2D, SpriteSystem2D>();

	// Initialize camera system (2D)
	m_ecs.RegisterSystem<CameraSystem2D>(&m_context);
	m_ecs.AddComponentToSystem<Camera2D, CameraSystem2D>();

    // Initialize physics system (2D)
    m_ecs.RegisterSystem<RigidBodySystem2D>(&m_context);
    m_ecs.AddComponentToSystem<Transform2D, RigidBodySystem2D>();
    m_ecs.AddComponentToSystem<RigidBody2D, RigidBodySystem2D>();

	// Initialize tilemap system (2D)
	m_ecs.RegisterSystem<TilemapSystem2D>(&m_context);
	m_ecs.AddComponentToSystem<Transform2D, TilemapSystem2D>();
	m_ecs.AddComponentToSystem<Tilemap2D, TilemapSystem2D>();
}