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
	bool sprintDown = false;
	bool jumpDown = false;
	bool actionDown = false;
	bool crouchDown = false;
};


struct DefaultFPSController
{
	void Update(float dt);
	void SetCamera(struct PerspectiveCamera* cam, float camOffsetY);

	void HandleMouseButton(int button, int action, int mods);
	void HandleKey(int key, int scancode, int action, int mods);
	void HandleMouseMovement(int dx, int dy);

	PhysicsController* controller;
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

	void HandleMouseButton(int button, int action, int mods);
	void HandleKey(int key, int scancode, int action, int mods);
	void HandleMouseMovement(int dx, int dy);

	FPSUserInput movement;
	glm::vec3 pos;
	glm::vec3 forwardDir;
	glm::vec3 rightDir;
	float yaw;
	float pitch;
	float velocity;
	float sprintModifier;
};