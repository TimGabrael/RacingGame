#pragma once
#include "Renderable.h"
#define MAX_NUM_JOINTS 128
#define MAX_NUM_LIGHTS 20
#define MAX_BLOOM_MIPMAPS 8


struct EnvironmentData
{
	GLuint environmentMap;
	GLuint prefilteredMap;
	GLuint irradianceMap;
	uint32_t irradianceMapWidth;
	uint32_t irradianceMapHeight;
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
	int projIdx;
};
struct PointShadowLight
{
	PointLight light;
};
struct DirectionalLight
{
	glm::vec3 direction;
	int isCascaded;
	glm::vec3 color;
	int projIdx;
};
struct DirShadowLight
{
	DirectionalLight light;
	glm::vec3 pos;
	float cascadeSteps[4];
};
struct SpotLight
{
	glm::vec4 color;
	glm::vec3 direction;
	int projIdx;
	glm::vec3 pos;
	float cutOff;
};
struct SpotShadowLight
{
	SpotLight light;
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


struct AntialiasingRenderData
{
	GLuint fbo;
	GLuint texture;
	GLuint depth;

	// INTERMEDIATE DOESN'T HAVE AA, SO IT CAN BE SAMPLED IN SHADERS
	GLuint intermediateFbo;
	GLuint intermediateTexture;
	GLuint intermediateDepth;
	
	uint32_t width;
	uint32_t height;
	uint32_t msaaCount;
};
struct PostProcessingRenderData
{
	GLuint* ppFBOs1;
	GLuint* ppFBOs2;
	GLuint ppTexture1;
	GLuint ppTexture2;

	GLuint intermediateFbo;
	GLuint intermediateTexture;

	uint32_t width;
	uint32_t height;
	float blurRadius;
	float bloomIntensity;
	float exposure;
	float gamma;
	int numPPFbos; // numPPFbos == min(numMipMaps, MAX_BLOOM_MIPMAPS)
};
struct ScreenSpaceReflectionRenderData
{
	GLuint fbo;
	GLuint normalTexture;
	GLuint metallicTexture;
	GLuint depthTexture;
	uint32_t width;
	uint32_t height;
};



struct Renderer* RE_CreateRenderer(struct AssetManager* assets);
void RE_DestroyRenderer(struct Renderer* renderer);










void RE_CreateAntialiasingData(AntialiasingRenderData* data, uint32_t width, uint32_t height, uint32_t numSamples);
void RE_FinishAntialiasingData(AntialiasingRenderData* data);	// copys contents of msaa texture to intermediate texture
void RE_CopyAntialiasingDataToFBO(AntialiasingRenderData* data, GLuint dstFbo, uint32_t dstWidth, uint32_t dstHeight);
void RE_CleanUpAntialiasingData(AntialiasingRenderData* data);

void RE_CreatePostProcessingRenderData(PostProcessingRenderData* data, uint32_t width, uint32_t height);
void RE_CleanUpPostProcessingRenderData(PostProcessingRenderData* data);

void RE_CreateScreenSpaceReflectionRenderData(ScreenSpaceReflectionRenderData* data, uint32_t width, uint32_t height);
void RE_CleanUpScreenSpaceReflectionRenderData(ScreenSpaceReflectionRenderData* data);















struct LightGroup* RELI_AddLightGroup(struct Renderer* renderer);
void RELI_RemoveLightGroup(struct Renderer* renderer, struct LightGroup* group);
void RELI_Update(struct LightGroup* group, const CameraBase* relativeCam);

void RELI_SetAmbientLightColor(struct LightGroup* group, const glm::vec3& col);


DirectionalLight* RELI_AddDirectionalLight(struct LightGroup* group);
void RELI_RemoveDirectionalLight(struct LightGroup* group, DirectionalLight* light);

PointLight* RELI_AddPointLight(struct LightGroup* group);
void RELI_RemovePointLight(struct LightGroup* group, PointLight* light);

SpotLight* RELI_AddSpotLight(struct LightGroup* group);
void RELI_RemoveSpotLight(struct LightGroup* group, SpotLight* light);




DirShadowLight* RELI_AddDirectionalShadowLight(struct LightGroup* group, uint16_t shadowWidth, uint16_t shadowHeight, bool useCascade);
void RELI_RemoveDirectionalShadowLight(struct LightGroup* group, DirShadowLight* light);
SpotShadowLight* RELI_AddSpotShadowLight(struct LightGroup* group, uint16_t shadowWidth, uint16_t shadowHeight);
void RELI_RemoveSpotShadowLight(struct LightGroup* group, SpotShadowLight* light);
PointShadowLight* RELI_AddPointShadowLight(struct LightGroup* group, uint16_t shadowWidth, uint16_t shadowHeight);
void RELI_RemovePointShadowLight(struct LightGroup* group, PointShadowLight* light);













// Creates the prefiltered-/irradiance-Map from the environmentMap or cleanes those two up
void RE_CreateEnvironment(struct Renderer* renderer, EnvironmentData* env, uint32_t irrWidth, uint32_t irrHeight);
void RE_CleanUpEnvironment(EnvironmentData* env);

void RE_BeginScene(struct Renderer* renderer, struct Scene* scene);

// the objects passed to the functions need to stay alive for the entire renderpass!!!
void RE_SetCameraBase(struct Renderer* renderer, const struct CameraBase* camBase);
void RE_SetEnvironmentData(struct Renderer* renderer, const EnvironmentData* data);
void RE_SetLightData(struct Renderer* renderer, struct LightGroup* group);

// Renders the shadow map of the current light group
void RE_RenderShadows(struct Renderer* renderer);

void RE_RenderIrradiance(struct Renderer* renderer, float deltaPhi, float deltaTheta, CUBE_MAP_SIDE side);
void RE_RenderPreFilterCubeMap(struct Renderer* renderer, float roughness, uint32_t numSamples, CUBE_MAP_SIDE side);


void RE_RenderGeometry(struct Renderer* renderer);

void RE_RenderOpaque(struct Renderer* renderer);
void RE_RenderAdditionalOpaque(struct Renderer* renderer);	// renders additional lights ontop of first render opaque
void RE_RenderTransparent(struct Renderer* renderer);



void RE_RenderOutline(struct Renderer* renderer, const Renderable* obj, const glm::vec4& color, float thickness);


void RE_RenderCubeMap(struct Renderer* renderer, GLuint cubemap);





void RE_RenderScreenSpaceReflection(struct Renderer* renderer, const ScreenSpaceReflectionRenderData* ssrData, GLuint srcTexture, GLuint targetFBO, uint32_t targetWidth, uint32_t targetHeight);


// THIS DOESN'T REALLY WORK, BUT MAYBE THE UNIFORM DATA NEEDS TO BE CONFIGURED BETTER
void RE_RenderScreenSpaceAmbientOcclusion_Experimental(struct Renderer* renderer, GLuint srcColorTexture, GLuint srcDepthTexture, uint32_t srcWidth, uint32_t srcHeight, GLuint targetFBO, uint32_t targetWidth, uint32_t targetHeight);


// TONEMAPS DIRECTLY AFTERWARDS
void RE_RenderPostProcessingBloom(struct Renderer* renderer, const PostProcessingRenderData* ppData, GLuint srcTexture, uint32_t srcWidth, uint32_t srcHeight, GLuint targetFBO, uint32_t targetWidth, uint32_t targetHeight);


void RE_RenderPostProcessingToneMap(struct Renderer* renderer, const PostProcessingRenderData* ppData, GLuint srcTexture, GLuint targetFBO, uint32_t targetWidth, uint32_t targetHeight);




void RE_EndScene(struct Renderer* renderer);


