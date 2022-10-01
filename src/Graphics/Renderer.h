#pragma once
#include "GLIncludes.h"
#include "Camera.h"
#define MAX_NUM_JOINTS 128
#define MAX_NUM_LIGHTS 20
#define MAX_BLOOM_MIPMAPS 8

struct EnvironmentData
{
	GLuint environmentMap;
	GLuint prefilteredMap;
	GLuint irradianceMap;
	uint32_t width;
	uint32_t height;
	uint32_t mipLevels;
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

	int baseColorUV;
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
struct PointLight
{
	glm::vec4 color;
	glm::vec4 pos;
	float constant;
	float linear;
	float quadratic;
	float padding;
};
struct DirectionalLight
{
	glm::vec4 direction;
	glm::vec4 color;
};
struct SpotLight
{
	glm::vec4 color;
	glm::vec4 direction;
	glm::vec3 pos;
	float cutOff;
};
struct LightData 
{
	PointLight pointLights[MAX_NUM_LIGHTS];
	DirectionalLight dirLights[MAX_NUM_LIGHTS];
	SpotLight spotLights[MAX_NUM_LIGHTS];
	glm::vec4 ambientColor;
	int numPointLights;
	int numDirLights;
	int numSpotLights;
	int padding;
};

enum CUBE_MAP_SIDE
{
	CUBE_MAP_LEFT,
	CUBE_MAP_RIGHT,
	CUBE_MAP_TOP,
	CUBE_MAP_BOTTOM,
	CUBE_MAP_FRONT,
	CUBE_MAP_BACK,
};


struct DirShadowLight
{
	DirectionalLight light;
	CameraBase cam;
	GLuint framebuffer;
	GLuint texture;
	uint32_t textureWidth;
	uint32_t textureHeight;
	uint32_t numCascades;
	float cascadeSteps[4];
};
struct PointShadowLight
{
	PointLight light;
	CameraBase cam;
	GLuint framebuffer;
	GLuint texture;
	uint32_t textureWidth;
	uint32_t textureHeight;
};


struct AntialiasingRenderData
{
	GLuint fbo;
	GLuint texture;
	GLuint depth;

	// INTERMEDIATE DOESN'T HAVE AA, SO IT CAN BE SAMPLED IN SHADERS
	GLuint intermediateFbo;
	GLuint intermediateTexture;
	GLuint intermediateDepth;
	
	GLuint width;
	GLuint height;
	uint32_t msaaCount;
};
struct PostProcessingRenderData
{
	GLuint* ppFBOs1;
	GLuint* ppFBOs2;
	GLuint ppTexture1;
	GLuint ppTexture2;
	GLuint intermediateTexture;

	uint32_t width;
	uint32_t height;
	float blurRadius;
	float bloomIntensity;
	int numPPFbos; // numPPFbos == min(numMipMaps, MAX_BLOOM_MIPMAPS)
};




struct Renderer* RE_CreateRenderer(struct AssetManager* assets);
void RE_CleanUpRenderer(struct Renderer* renderer);



void RE_CreateAntialiasingData(AntialiasingRenderData* data, uint32_t width, uint32_t height, uint32_t numSamples);
void RE_FinishAntialiasingData(AntialiasingRenderData* data);	// copys contents of msaa texture to intermediate texture
void RE_CleanUpAntialiasingData(AntialiasingRenderData* data);

void RE_CreatePostProcessingRenderData(PostProcessingRenderData* data, uint32_t width, uint32_t height);
void RE_CleanUpPostProcessingRenderData(PostProcessingRenderData* data);


// Creates the prefiltered-/irradiance-Map from the environmentMap or cleanes those to up
void RE_CreateEnvironment(struct Renderer* renderer, EnvironmentData* env);
void RE_CleanUpEnvironment(EnvironmentData* env);

void RE_BeginScene(struct Renderer* renderer, struct SceneObject** objList, uint32_t num);

// the objects passed to the functions need to stay alive for the entire renderpass!!!
void RE_SetCameraBase(struct Renderer* renderer, const struct CameraBase* camBase);
void RE_SetEnvironmentData(struct Renderer* renderer, const EnvironmentData* data);
void RE_SetLightData(struct Renderer* renderer, GLuint lightUniform);


void RE_RenderIrradiance(struct Renderer* renderer, float deltaPhi, float deltaTheta, CUBE_MAP_SIDE side);
void RE_RenderPreFilterCubeMap(struct Renderer* renderer, float roughness, uint32_t numSamples, CUBE_MAP_SIDE side);


void RE_RenderGeometry(struct Renderer* renderer);

void RE_RenderOpaque(struct Renderer* renderer);
void RE_RenderTransparent(struct Renderer* renderer);
void RE_RenderCubeMap(struct Renderer* renderer, GLuint cubemap);


void RE_EndScene(struct Renderer* renderer);