#pragma once
#include "Physics.h"

struct FPSUserInput
{
	float deltaYaw;
	float deltaPitch;
	bool forward = false;
	bool back = false;
	bool right = false;
	bool left = false;
	bool actionDown = false;
};


struct DefaultFPSController
{
	void Update(float dt);
	void SetCamera(struct PerspectiveCamera* cam, float camOffsetY);
	PhysicsController* controller;
	FPSUserInput movement;
	glm::vec3 forwardDir;
	glm::vec3 rightDir;
	float yaw;
	float pitch;
	float velocity;
};