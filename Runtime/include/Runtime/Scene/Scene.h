#pragma once

// All scenes will implement this
class IScene
{
public:
 	virtual void Ready() = 0;
	virtual void Update(float dt) = 0;
	virtual void Leave() = 0;
};