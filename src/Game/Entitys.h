#pragma once
#include "../Graphics/Camera.h"

struct PlayerInputData
{
	bool forward;
	bool back;
	bool right;
	bool left;
};


struct Entity
{
	virtual ~Entity() = default;
	virtual void Update(float dt) = 0;
};

struct Player : public Entity
{
	virtual ~Player() override = default;
	virtual void Update(float dt) override;

	struct SceneObject* sceneObject;
	PlayerInputData input;
	PerspectiveCamera camera;
};

