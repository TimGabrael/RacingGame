#include "Entitys.h"
#include "../Graphics/Scene.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Renderer.h"
#include "../GameState.h"
#include "../Physics/Physics.h"

void Player::Update(float dt)
{
	if (input.forward || input.back || input.left || input.right)	// move player from input
	{
		const glm::vec3 right = CA_GetPerspectiveRightDir(&camera);

		float forwardSpeed = 0.0f;
		float rightSpeed = 0.0f;
		if (input.forward) forwardSpeed = 1.0f;
		if (input.back) forwardSpeed -= 1.0f;
		if (input.right) rightSpeed = 1.0f;
		if (input.left) rightSpeed -= 1.0f;
		glm::vec3 vel = forwardSpeed * camera.front + rightSpeed * right;
		camera.base.pos += vel;
		CA_UpdatePerspectiveCamera(&camera);

		vel.y -= 9.81f;

		controller->Move(vel);
	}


}


