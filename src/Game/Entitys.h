#pragma once
#include "../Graphics/Camera.h"
#include "../Physics/Controller.h"


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
	DefaultFPSController controller;
	PerspectiveCamera camera;
};

