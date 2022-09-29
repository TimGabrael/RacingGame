#include "Camera.h"
static constexpr glm::vec3 up = { 0.0f, 1.0f, 0.0f };
static constexpr float nearClipping = 0.1f;
static constexpr float farClipping = 1000.0f;
static constexpr float FOV = 90.0f;

void CA_InitPerspectiveCamera(PerspectiveCamera* cam, const glm::vec3& pos, float yaw, float pitch, float width, float height)
{
	cam->base.pos = pos;
	cam->yaw = yaw;
	cam->pitch = pitch;
	cam->width = width;
	cam->height = height;
	CA_UpdatePerspectiveCamera(cam);
}
void CA_UpdatePerspectiveCamera(PerspectiveCamera* cam)
{
	cam->front.x = cosf(glm::radians(cam->yaw)) * cosf(glm::radians(cam->pitch));
	cam->front.y = sinf(glm::radians(cam->pitch));
	cam->front.z = sinf(glm::radians(cam->yaw)) * cosf(glm::radians(cam->pitch));

	cam->base.proj = glm::perspectiveRH(glm::radians(FOV), cam->width / cam->height, nearClipping, farClipping);
	cam->base.view = glm::lookAtRH(cam->base.pos, cam->base.pos + cam->front, up);
}
glm::vec3 CA_GetPerspectiveRightDir(const PerspectiveCamera* cam)
{
	return glm::normalize(glm::cross(cam->front, up));
}