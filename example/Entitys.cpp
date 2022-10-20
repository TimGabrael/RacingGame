#include "Entitys.h"
#include "Graphics/Renderer.h"
#include "GameState.h"
#include "Physics/Physics.h"


Player::Player()
{
}
Player::~Player()
{
	// CLEANUP THE RIGIDBODY OR SOMETHING
#ifndef FREE_CAM
	if (controller.controller)
	{
		controller.controller->release();
	}
#endif
	
}
void Player::Update(float dt)
{

#ifndef FREE_CAM
	controller.SetCamera(&camera, 8.0f);
	controller.Update(dt);
#else
	controller.SetCamera(&camera);
	controller.Update(dt);
#endif

}


FoxEntity::FoxEntity(Model* m, AnimationInstanceData* data)
{
	this->model = m;
	this->anim = data;
	renderable = nullptr;
	body = nullptr;
}
FoxEntity::~FoxEntity()
{
	body->release();
	body = nullptr;
	delete renderable;
	renderable = nullptr;
}
void FoxEntity::UpdateFrame(float dt)
{
	glm::mat4 mat;

	physx::PxMat44 m = physx::PxMat44(body->getGlobalPose());
	memcpy(&mat, &m, sizeof(glm::mat4));
	renderable->Update(model, anim, mat);
}
SponzaEntity::SponzaEntity(Model* m)
{
	this->model = m;
	body = nullptr;
	renderable = nullptr;
}
SponzaEntity::~SponzaEntity()
{
	body->release();
	body = nullptr;
	delete renderable;
	renderable = nullptr;
}
void SponzaEntity::UpdateFrame(float dt)
{
	glm::mat4 mat;
	physx::PxMat44 m = physx::PxMat44(body->getGlobalPose());
	memcpy(&mat, &m, sizeof(glm::mat4));
	renderable->Update(model, nullptr, mat);
}