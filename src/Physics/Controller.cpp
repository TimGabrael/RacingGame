#include "Controller.h"
#include "../Graphics/Camera.h"
#include "GameState.h"

static void GenerateFPSUserInputFromKeys(FPSUserInput& movement)
{
	movement.forward = 0.0f; movement.right = 0.0f;
	if (GetKey(GLFW_KEY_W)) movement.forward = 1.0f;
	if (GetKey(GLFW_KEY_S))movement.forward -= 1.0f;
	if (GetKey(GLFW_KEY_D))movement.right = 1.0f;
	if (GetKey(GLFW_KEY_A))movement.right -= 1.0f;
	movement.jumpDown = GetKey(GLFW_KEY_SPACE);
	movement.actionDown = GetMouseButton(GLFW_MOUSE_BUTTON_LEFT);
	movement.crouchDown = GetKey(GLFW_KEY_LEFT_CONTROL);
	movement.sprintDown = GetKey(GLFW_KEY_LEFT_SHIFT);
}

static glm::vec3 GenerateVelocityFromFPSUserInput(FPSUserInput& movement, const glm::vec3& front, const glm::vec3& right, float velocity, float sprintMultiplier)
{
	glm::vec3 vel = {};
	if (movement.forward != 0.0f || movement.right != 0.0f)	// move player from input
	{
		glm::vec3 horizontalForwardDir = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
		glm::vec3 horizontalRightDir = glm::normalize(glm::vec3(right.x, 0.0f, right.z));
		vel = glm::normalize(movement.forward * horizontalForwardDir + movement.right * horizontalRightDir) * velocity;
		if (movement.sprintDown) vel *= sprintMultiplier;
	}
	return vel;
}
static void UpdateFPSUserInput(FPSUserInput& movement, glm::vec3& front, glm::vec3& right, float* yaw, float* pitch)
{
	if (movement.deltaYaw != 0.0f || movement.deltaPitch != 0.0f)
	{
		*yaw += movement.deltaYaw;
		*pitch += movement.deltaPitch;
		*pitch = glm::max(glm::min(*pitch, 89.9f), -89.9f);
		front = CA_YawPitchToFoward(*yaw, *pitch);
		right = CA_GetRight(front);
		movement.deltaYaw = 0.0f;
		movement.deltaPitch = 0.0f;
	}
}








void DefaultFPSController::Update(float dt)
{
	GenerateFPSUserInputFromKeys(movement);
	glm::vec3 curVel = (controller->GetVelocity() - glm::vec3(0.0f, 9.81f, 0.0f) * 0.5f);
	curVel.x = 0.0f; curVel.z = 0.0f;
	UpdateFPSUserInput(movement, forwardDir, rightDir, &yaw, &pitch);
	glm::vec3 vel = (GenerateVelocityFromFPSUserInput(movement, forwardDir, rightDir, velocity, sprintModifier) + curVel) * dt;
	if (controller->IsOnGround() && movement.jumpDown) {
		vel.y = 1.5f;
	}
	controller->Move(vel);
}
void DefaultFPSController::SetCamera(PerspectiveCamera* cam, float camOffsetY)
{
	cam->base.pos = controller->GetPos() + glm::vec3(0.0f, camOffsetY, 0.0f);
	CA_UpdatePerspectiveCamera(cam, forwardDir);
}
void DefaultFPSController::HandleMouseMovement(int dx, int dy)
{
	if (movement.actionDown)
	{
		movement.deltaYaw += dx;
		movement.deltaPitch += dy;
	}
}






void FreecamFPSController::Update(float dt)
{
	GenerateFPSUserInputFromKeys(movement);
	UpdateFPSUserInput(movement, forwardDir, rightDir, &yaw, &pitch);
	glm::vec3 vel = GenerateVelocityFromFPSUserInput(movement, forwardDir, rightDir, velocity, sprintModifier);
	if (movement.jumpDown) vel.y = velocity;
	if (movement.crouchDown) vel.y -= velocity;
	if (movement.sprintDown) vel.y *= sprintModifier;
	pos += vel * dt;
}
void FreecamFPSController::SetCamera(struct PerspectiveCamera* cam)
{
	cam->base.pos = pos;
	CA_UpdatePerspectiveCamera(cam, forwardDir);
}
void FreecamFPSController::HandleMouseMovement(int dx, int dy)
{
	if (movement.actionDown)
	{
		movement.deltaYaw += dx;
		movement.deltaPitch += dy;
	}
}