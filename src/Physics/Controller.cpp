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
		vel = glm::normalize(forwardSpeed * horizontalForwardDir + rightSpeed * horizontalRightDir) * velocity;
	}
	vel.y -= 9.81f;
	vel *= dt;
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
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_W) movement.forward = true;
		if (key == GLFW_KEY_A) movement.left = true;
		if (key == GLFW_KEY_S) movement.back = true;
		if (key == GLFW_KEY_D) movement.right = true;
	}
	else if (action == GLFW_RELEASE)
	{
		if (key == GLFW_KEY_W) movement.forward = false;
		if (key == GLFW_KEY_A) movement.left = false;
		if (key == GLFW_KEY_S) movement.back = false;
		if (key == GLFW_KEY_D) movement.right = false;
	}
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
		vel = glm::normalize(forwardSpeed * horizontalForwardDir + rightSpeed * horizontalRightDir) * velocity;
	}
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
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_W) movement.forward = true;
		if (key == GLFW_KEY_A) movement.left = true;
		if (key == GLFW_KEY_S) movement.back = true;
		if (key == GLFW_KEY_D) movement.right = true;
	}
	else if (action == GLFW_RELEASE)
	{
		if (key == GLFW_KEY_W) movement.forward = false;
		if (key == GLFW_KEY_A) movement.left = false;
		if (key == GLFW_KEY_S) movement.back = false;
		if (key == GLFW_KEY_D) movement.right = false;
	}
}
void FreecamFPSController::HandleMouseMovement(int dx, int dy)
{
	if (movement.actionDown)
	{
		movement.deltaYaw += dx;
		movement.deltaPitch += dy;
	}
}