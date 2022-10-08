#include "Entitys.h"
#include "../Graphics/Scene.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Renderer.h"
#include "../GameState.h"
#include "../Physics/Physics.h"

void Player::Update(float dt)
{
	controller.SetCamera(&camera, 8.0f);
	controller.Update(dt);
}


