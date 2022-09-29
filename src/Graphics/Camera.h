#pragma once
#include "../Util/Math.h"

struct CameraBase
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 pos;
};

struct PerspectiveCamera
{
	CameraBase base;
	glm::vec3 front;
	float yaw;
	float pitch;
	float width;
	float height;
};


void CA_InitPerspectiveCamera(PerspectiveCamera* cam, const glm::vec3& pos, float yaw, float pitch, float width, float height);
void CA_UpdatePerspectiveCamera(PerspectiveCamera* cam);
glm::vec3 CA_GetPerspectiveRightDir(const PerspectiveCamera* cam);