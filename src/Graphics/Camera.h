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
	float fov;
	float nearClipping;
	float farClipping;
	float width;
	float height;
};


void CA_InitPerspectiveCamera(PerspectiveCamera* cam, const glm::vec3& pos, float fov, float width, float height);
void CA_UpdatePerspectiveCamera(PerspectiveCamera* cam, const glm::vec3& front);


glm::mat4 CA_CreateOrthoView(const glm::vec3& pos, const glm::vec3& dir, float widthHalf, float heightHalf, float nearDepth, float farDepth);
glm::mat4 CA_CreatePerspectiveView(const glm::vec3& pos, const glm::vec3& dir, float fov, float width, float height, float near, float far);

void CA_CreateOrthoTightFit(const CameraBase* relativeCam, CameraBase* output, const glm::vec3& dir, float splitStart, float splitEnd, float* splitDepth);

glm::vec3 CA_GetRight(const glm::vec3& front);
glm::vec3 CA_YawPitchToFoward(float yaw, float pitch);


glm::vec3 CA_GetMouseWorldSpace(const CameraBase* cam, const glm::vec2& mousePos, glm::vec3& mouseWorldSpace);