#pragma once
#include "Physics.h"

struct FPSUserInput
{
	float deltaYaw = 0.0f;
	float deltaPitch = 0.0f;
	float forward = 0.0f;
	float right = 0.0f;
	bool sprintDown = false;
	bool jumpDown = false;
	bool actionDown = false;
	bool crouchDown = false;
};


struct DefaultFPSController
{
	void Update(float dt);
	void SetCamera(struct PerspectiveCamera* cam, float camOffsetY);

	void HandleMouseMovement(float dx, float dy);

	physx::PxController* controller;
	FPSUserInput movement;
	glm::vec3 forwardDir;
	glm::vec3 rightDir;
	float yaw;
	float pitch;
	float velocity;
	float sprintModifier;
	float jumpSpeed;
};

struct FreecamFPSController
{
	void Update(float dt);
	void SetCamera(struct PerspectiveCamera* cam);

	void HandleMouseMovement(float dx, float dy);

	FPSUserInput movement;
	glm::vec3 pos;
	glm::vec3 forwardDir;
	glm::vec3 rightDir;
	float yaw;
	float pitch;
	float velocity;
	float sprintModifier;
};