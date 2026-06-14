#pragma once
#include <Core/EngineContext.h>

// All scenes will extend this
class Scene
{
public:
	Scene(EngineContext* context) : m_context(context) {}
 	virtual void Ready() = 0;
	virtual void Update(float dt) = 0;
	virtual void Leave() = 0;

protected:
	EngineContext* m_context;
};