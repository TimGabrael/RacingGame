#pragma once
#include "Graphics/Camera.h"
#include "Physics/Controller.h"
#include "Graphics/Scene.h"


#define FREE_CAM
struct Player : public Entity
{
	virtual ~Player() override = default;
	virtual void Update(float dt) override;

	struct SceneObject* sceneObject;
#ifndef FREE_CAM
	DefaultFPSController controller;
#else
	FreecamFPSController controller;
#endif // !FREE_CAM

	PerspectiveCamera camera;
};

