#pragma once
#include "Graphics/Camera.h"
#include "Physics/Controller.h"
#include "Graphics/Scene.h"


struct Player : public Entity
{
	virtual ~Player() override = default;
	virtual void Update(float dt) override;

	struct SceneObject* sceneObject;
	DefaultFPSController controller;
	PerspectiveCamera camera;
};

