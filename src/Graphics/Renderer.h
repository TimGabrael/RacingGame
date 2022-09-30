#pragma once
#include "GLIncludes.h"
#define MAX_NUM_JOINTS 128

struct EnvironmentData
{
	GLuint environmentMap;
	GLuint prefilteredMap;
	GLuint irradianceMap;
};

struct BoneData
{
	glm::mat4 jointMatrix[MAX_NUM_JOINTS];
	float numJoints;
	float _align1;
	float _align2;
	float _align3;
};
struct MaterialData
{
	glm::vec4 baseColorFactor;
	glm::vec4 emissiveFactor;
	glm::vec4 diffuseFactor;
	glm::vec4 specularFactor;

	int diffuseUV;
	int normalUV;
	int emissiveUV;
	int aoUV;
	int metallicRoughnessUV;

	float roughnessFactor;
	float metallicFactor;
	float alphaMask;
	float alphaCutoff;

	float workflow;
	float _align1;
	float _align2;
};


struct Renderer* RE_CreateRenderer(struct AssetManager* assets);
void RE_CleanUpRenderer(struct Renderer* renderer);




void RE_BeginScene(struct Renderer* renderer, struct SceneObject** objList, uint32_t num);

// the objects passed to the functions need to stay alive for the entire renderpass!!!
void RE_SetCameraBase(struct Renderer* renderer, const struct CameraBase* camBase);
void RE_SetEnvironmentData(struct Renderer* renderer, const EnvironmentData* data);


void RE_RenderIrradiance(struct Renderer* renderer, float deltaPhi, float deltaTheta);


void RE_RenderOpaque(struct Renderer* renderer);
void RE_RenderTransparent(struct Renderer* renderer);
void RE_RenderCubeMap(struct Renderer* renderer, GLuint cubemap);


void RE_EndScene(struct Renderer* renderer);