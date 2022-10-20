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
}
void FoxEntity::UpdateFrame(float dt)
{
	glm::mat4 mat;
	PH_SetTransformation(body, mat);
	renderable->Update(model, anim, mat);
}
SponzaEntity::SponzaEntity(Model* m)
{
	this->model = m;
}
void SponzaEntity::UpdateFrame(float dt)
{
	glm::mat4 mat;
	PH_SetTransformation(body, mat);
	renderable->Update(model, nullptr, mat);
}