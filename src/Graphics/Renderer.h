#pragma once
#include "GLIncludes.h"
#define MAX_NUM_JOINTS 128

struct BoneData
{
	glm::mat4 jointMatrix[MAX_NUM_JOINTS];
	float numJoints;
};


struct Renderer* RE_CreateRenderer(struct AssetManager* assets);
void RE_CleanUpRenderer(struct Renderer* renderer);

void RE_BeginScene(struct Renderer* renderer, struct Scene* scene);

// the camera base needs to be valid for the entire render loop
void RE_SetCameraBase(struct Renderer* renderer, const struct CameraBase* camBase);


void RE_RenderOpaque(struct Renderer* renderer, struct Scene* scene);
void RE_RenderTransparent(struct Renderer* renderer, struct Scene* scene);
void RE_RenderCubeMap(struct Renderer* renderer, GLuint cubemap);