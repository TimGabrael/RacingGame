#include "Entitys.h"
#include "Graphics/Renderer.h"
#include "GameState.h"
#include "Physics/Physics.h"



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


