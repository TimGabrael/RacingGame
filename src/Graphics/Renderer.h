#pragma once
#include "GLIncludes.h"
#define MAX_NUM_JOINTS 128

struct BoneData
{
	glm::mat4 jointMatrix[MAX_NUM_JOINTS];
	float numJoints;
	float _align1;
	float _align2;
	float _align3;
};


struct Renderer* RE_CreateRenderer(struct AssetManager* assets);
void RE_CleanUpRenderer(struct Renderer* renderer);




void RE_BeginScene(struct Renderer* renderer, struct SceneObject** objList, uint32_t num);

// the camera base needs to be valid for the entire render loop
void RE_SetCameraBase(struct Renderer* renderer, const struct CameraBase* camBase);


void RE_RenderOpaque(struct Renderer* renderer);
void RE_RenderTransparent(struct Renderer* renderer);
void RE_RenderCubeMap(struct Renderer* renderer, GLuint cubemap);


void RE_EndScene(struct Renderer* renderer);