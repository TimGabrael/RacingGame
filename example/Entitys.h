#pragma once
#include "Graphics/Camera.h"
#include "Physics/Controller.h"
#include "Graphics/Scene.h"


//#define FREE_CAM
struct Player : public Entity
{
	Player();
	virtual ~Player() override;
	virtual void Update(float dt) override;
	virtual void UpdateFrame(float dt) {};

#ifndef FREE_CAM
	DefaultFPSController controller;
#else
	FreecamFPSController controller;
#endif // !FREE_CAM

	PerspectiveCamera camera;
	physx::PxRigidActor* body;
};

struct FoxEntity : public Entity
{
	FoxEntity(Model* m, AnimationInstanceData* data);
	virtual ~FoxEntity() override = default;
	virtual void Update(float dt) override {};
	virtual void UpdateFrame(float dt);
	Model* model;
	physx::PxRigidActor* body;
	AnimationInstanceData* anim;
	PBRRenderable* renderable;
};

struct SponzaEntity : public Entity
{
	SponzaEntity(Model* m);
	virtual ~SponzaEntity() override = default;
	virtual void Update(float dt) override {};
	virtual void UpdateFrame(float dt);
	physx::PxRigidActor* body;
	Model* model;
	PBRRenderable* renderable;
	
};