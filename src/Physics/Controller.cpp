#include "Controller.h"
#include "../Graphics/Camera.h"

static glm::vec3 GenerateVelocityFromFPSUserInput(FPSUserInput& movement, const glm::vec3& front, const glm::vec3& right, float velocity, float sprintMultiplier)
{
	glm::vec3 vel = {};
	if (movement.forward || movement.back || movement.left || movement.right)	// move player from input
	{

		float forwardSpeed = 0.0f;
		float rightSpeed = 0.0f;
		if (movement.forward) forwardSpeed = 1.0f;
		if (movement.back) forwardSpeed -= 1.0f;
		if (movement.right) rightSpeed = 1.0f;
		if (movement.left) rightSpeed -= 1.0f;
		glm::vec3 horizontalForwardDir = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
		glm::vec3 horizontalRightDir = glm::normalize(glm::vec3(right.x, 0.0f, right.z));
		vel = glm::normalize(forwardSpeed * horizontalForwardDir + rightSpeed * horizontalRightDir) * velocity;
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
static void UpdateFPSFromKey(FPSUserInput& movement, int key, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_W) movement.forward = true;
		else if (key == GLFW_KEY_A) movement.left = true;
		else if (key == GLFW_KEY_S) movement.back = true;
		else if (key == GLFW_KEY_D) movement.right = true;
		else if (key == GLFW_KEY_LEFT_SHIFT) movement.sprintDown = true;
		else if (key == GLFW_KEY_SPACE) movement.jumpDown = true;
		else if (key == GLFW_KEY_LEFT_CONTROL) movement.crouchDown = true;
	}
	else if (action == GLFW_RELEASE)
	{
		if (key == GLFW_KEY_W) movement.forward = false;
		else if (key == GLFW_KEY_A) movement.left = false;
		else if (key == GLFW_KEY_S) movement.back = false;
		else if (key == GLFW_KEY_D) movement.right = false;
		else if (key == GLFW_KEY_LEFT_SHIFT) movement.sprintDown = false;
		else if (key == GLFW_KEY_SPACE) movement.jumpDown = false;
		else if (key == GLFW_KEY_LEFT_CONTROL) movement.crouchDown = false;
	}
}








void DefaultFPSController::Update(float dt)
{
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
void DefaultFPSController::HandleMouseButton(int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS) movement.actionDown = true;
		else if (action == GLFW_RELEASE) movement.actionDown = false;
	}
}
void DefaultFPSController::HandleKey(int key, int scancode, int action, int mods)
{
	UpdateFPSFromKey(movement, key, action, mods);
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
	UpdateFPSUserInput(movement, forwardDir, rightDir, &yaw, &pitch);
	glm::vec3 vel = GenerateVelocityFromFPSUserInput(movement, forwardDir, rightDir, velocity, sprintModifier);
	pos += vel * dt;
}
void FreecamFPSController::SetCamera(struct PerspectiveCamera* cam)
{
	cam->base.pos = pos;
	CA_UpdatePerspectiveCamera(cam, forwardDir);
	
}
void FreecamFPSController::HandleMouseButton(int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS) movement.actionDown = true;
		else if (action == GLFW_RELEASE) movement.actionDown = false;
	}
}
void FreecamFPSController::HandleKey(int key, int scancode, int action, int mods)
{
	UpdateFPSFromKey(movement, key, action, mods);
}
void FreecamFPSController::HandleMouseMovement(int dx, int dy)
{
	if (movement.actionDown)
	{
		movement.deltaYaw += dx;
		movement.deltaPitch += dy;
	}
}