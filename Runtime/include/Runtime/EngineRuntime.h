#pragma once

#include <Core/ECS/ECS.h>
#include <Platform/Window.h>
#include <Platform/InputMap.h>
#include <Graphics/Renderer.h>
#include <Runtime/ContentLoader.h>
#include <Runtime/Scene/SceneManager.h>

class EngineRuntime 
{
public:
	bool Init(bool isEditor=false);
	void Update();
	void Close();

private:
	void registerComponents();
	void initializeSystems();

	ECS           m_ecs;
	Window        m_window;
	InputMap      m_inputMap;
	Renderer      m_renderer;
	ContentLoader m_contentLoader;
	SceneManager  m_sceneManager;

	bool          m_running = true;
	double        m_deltaTime = 0;
};