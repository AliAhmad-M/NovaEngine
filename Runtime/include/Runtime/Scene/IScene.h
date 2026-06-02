#pragma once

// All scenes will extend this
class IScene
{
public:
	virtual ~IScene() = default;
	virtual void Ready() = 0;
	virtual void Update(float dt) = 0;
	virtual void Leave() = 0;
};