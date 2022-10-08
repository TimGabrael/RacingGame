#include "Camera.h"
static constexpr glm::vec3 up = { 0.0f, 1.0f, 0.0f };
static constexpr float nearClipping = 0.1f;
static constexpr float farClipping = 1000.0f;
//static constexpr float FOV = 90.0f;

void CA_InitPerspectiveCamera(PerspectiveCamera* cam, const glm::vec3& pos, float fov, float width, float height)
{
	cam->base.pos = pos;
	cam->fov = fov;
	cam->width = width;
	cam->height = height;
	cam->nearClipping = nearClipping;
	cam->farClipping = farClipping;
	CA_UpdatePerspectiveCamera(cam, {1.0f, 0.0f, 0.0f});
}
void CA_UpdatePerspectiveCamera(PerspectiveCamera* cam, const glm::vec3& front)
{
	cam->base.proj = glm::perspectiveRH(glm::radians(cam->fov), cam->width / cam->height, cam->nearClipping, cam->farClipping);
	if (front.x == 0.0f && front.z == 0.0f)
	{
		cam->base.view = glm::lookAtRH(cam->base.pos, cam->base.pos + front, {0.0f, 0.0f, 1.0f});
	}
	else
	{
		cam->base.view = glm::lookAtRH(cam->base.pos, cam->base.pos + front, up);
	}
}
glm::vec3 CA_GetRight(const glm::vec3& front)
{
	return glm::normalize(glm::cross(front, up));
}


glm::mat4 CA_CreateOrthoView(const glm::vec3& pos, const glm::vec3& dir, float widthHalf, float heightHalf, float nearDepth, float farDepth)
{
	glm::vec3 ndir = glm::normalize(dir);
	glm::mat4 view;
	if (ndir.x == 0.0f && ndir.z == 0.0f)
	{
		view = glm::lookAtRH(pos, pos + ndir, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else
	{
		view = glm::lookAtRH(pos, pos + ndir, up);
	}
	return glm::orthoRH(-widthHalf, widthHalf, -heightHalf, heightHalf, nearDepth, farDepth) * view;
}
glm::mat4 CA_CreatePerspectiveView(const glm::vec3& pos, const glm::vec3& dir, float fov, float width, float height, float near, float far)
{
	glm::vec3 ndir = glm::normalize(dir);
	glm::mat4 view;
	if (ndir.x == 0.0f && ndir.z == 0.0f)
	{
		view = glm::lookAtRH(pos, pos + ndir, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else
	{
		view = glm::lookAtRH(pos, pos + ndir, up);
	}
	return glm::perspectiveRH(fov, width / height, near, far) * view;
}
void CA_CreateOrthoTightFit(const CameraBase* relativeCam, CameraBase* output, const glm::vec3& dir, float splitStart, float splitEnd, float* splitDepth)
{
	const glm::mat4 invCam = glm::inverse(relativeCam->proj * relativeCam->view);
	glm::vec3 frustumCorners[8] = {
		glm::vec3(-1.0f,  1.0f, -1.0f),
		glm::vec3(1.0f,  1.0f, -1.0f),
		glm::vec3(1.0f, -1.0f, -1.0f),
		glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3(-1.0f,  1.0f,  1.0f),
		glm::vec3(1.0f,  1.0f,  1.0f),
		glm::vec3(1.0f, -1.0f,  1.0f),
		glm::vec3(-1.0f, -1.0f,  1.0f),
	};
	for (uint32_t i = 0; i < 8; i++) {
		glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
		frustumCorners[i] = invCorner / invCorner.w;
	}
	for (uint32_t i = 0; i < 4; i++) {
		glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
		frustumCorners[i + 4] = frustumCorners[i] + (dist * splitEnd);
		frustumCorners[i] = frustumCorners[i] + (dist * splitStart);
	}
	glm::vec3 frustumCenter = glm::vec3(0.0f);
	for (uint32_t i = 0; i < 8; i++) {
		frustumCenter += frustumCorners[i];
	}
	frustumCenter /= 8.0f;
	float radius = 0.0f;
	for (uint32_t i = 0; i < 8; i++) {
		float distance = glm::round(glm::length(frustumCorners[i] - frustumCenter));
		radius = glm::max(radius, distance);
	}

	radius = std::ceil(radius * 16.0f) / 16.0f;
	glm::vec3 maxExtents = glm::vec3(radius);
	glm::vec3 minExtents = -maxExtents;

	glm::vec3 outDir = glm::normalize(dir);
	glm::mat4 outViewMatrix;
	if (outDir.x == 0.0f && outDir.z == 0.0f)
	{
		outViewMatrix = glm::lookAt(frustumCenter - outDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else
	{
		outViewMatrix = glm::lookAt(frustumCenter - outDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	glm::mat4 outOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

	const float vSSplitDepth = nearClipping + splitEnd * (farClipping - nearClipping) * -1.0f;
	glm::vec4 splitDepthWorldSpace = relativeCam->proj * glm::vec4(vSSplitDepth, vSSplitDepth, vSSplitDepth, 1.0f);
	*splitDepth = splitDepthWorldSpace.z / splitDepthWorldSpace.w;

	output->pos = glm::round(frustumCenter - outDir * -minExtents.z);
	output->view = outViewMatrix;
	output->proj = outOrthoMatrix;
}

glm::vec3 CA_YawPitchToFoward(float yaw, float pitch)
{
	glm::vec3 front;
	front.x = cosf(glm::radians(yaw)) * cosf(glm::radians(pitch));
	front.y = sinf(glm::radians(pitch));
	front.z = sinf(glm::radians(yaw)) * cosf(glm::radians(pitch));
	return front;
}