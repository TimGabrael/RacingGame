#include "Controller.h"
#include "../Graphics/Camera.h"

void DefaultFPSController::Update(float dt)
{
	if (movement.deltaYaw != 0.0f || movement.deltaPitch != 0.0f)
	{
		yaw += movement.deltaYaw;
		pitch += movement.deltaPitch;
		pitch = glm::max(glm::min(pitch, 89.9f), -89.9f);

		forwardDir = CA_YawPitchToFoward(yaw, pitch);
		rightDir = CA_GetRight(forwardDir);
		movement.deltaYaw = 0.0f;
		movement.deltaPitch = 0.0f;
	}
	glm::vec3 vel = {};
	if (movement.forward || movement.back || movement.left || movement.right)	// move player from input
	{

		float forwardSpeed = 0.0f;
		float rightSpeed = 0.0f;
		if (movement.forward) forwardSpeed = 1.0f;
		if (movement.back) forwardSpeed -= 1.0f;
		if (movement.right) rightSpeed = 1.0f;
		if (movement.left) rightSpeed -= 1.0f;
		glm::vec3 horizontalForwardDir = glm::normalize(glm::vec3(forwardDir.x, 0.0f, forwardDir.z));
		glm::vec3 horizontalRightDir = glm::normalize(glm::vec3(rightDir.x, 0.0f, rightDir.z));
		vel = glm::normalize(forwardSpeed * horizontalForwardDir + rightSpeed * horizontalRightDir);
	}
	vel.y -= 9.81f;
	controller->Move(vel);

}
void DefaultFPSController::SetCamera(PerspectiveCamera* cam, float camOffsetY)
{
	CA_UpdatePerspectiveCamera(cam, forwardDir);
	cam->base.pos = controller->GetPos() + glm::vec3(0.0f, camOffsetY, 0.0f);
}