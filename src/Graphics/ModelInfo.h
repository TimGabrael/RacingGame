#pragma once
#include "GLIncludes.h"

enum MESH_FLAG
{
	MESH_FLAG_LINE = 1,
	MESH_FLAG_NO_INDEX_BUFFER = (1 << 1),
	MESH_FLAG_UNSUPPORTED = (1 << 2),
};

struct AnimationTransformation
{
	glm::vec3 translation{};
	glm::vec3 scale{ 1.0f };
	glm::quat rotation{};
};
struct AABB
{
	glm::vec3 min;
	glm::vec3 max;
};

struct Vertex3D
{
	glm::vec3 pos;
	glm::vec3 nor;
	glm::vec2 uv1;
	glm::vec2 uv2;
	glm::vec4 joint;
	glm::vec4 weights;
	uint32_t col;
};

struct AnimationChannel
{
	enum PathType {TRANSLATION, ROTATION, SCALE};
	struct Joint* joint;
	uint32_t samplerIdx;
	PathType path;
};
struct AnimationSampler
{
	enum InterpolationType {LINEAR, STEP, CUBICSPLINE};
	float* inputs;
	glm::vec4* outputs;
	uint32_t numInOut;
	InterpolationType interpolation;
};
struct Animation
{
	AnimationSampler* samplers;
	AnimationChannel* channels;
	uint32_t numSamplers;
	uint32_t numChannels;
	float start;
	float end;
};

struct Joint
{
	Joint* parent;
	Joint** children;
	struct Model* model;
	struct Mesh* mesh;
	struct Skin* skin;
	int32_t skinIndex = -1;
	uint32_t numChildren;
	glm::vec3 translation{};
	glm::vec3 scale{ 1.0f };
	glm::quat rotation{};
	glm::mat4 matrix;
	glm::mat4 defMatrix;
	glm::mat4 GetMatrix();
};

struct Skin
{
	Joint* skeletonRoot = nullptr;
	glm::mat4* inverseBindMatrices;
	Joint** joints;
	uint32_t numJoints;
};

struct Texture
{
	GLuint uniform;
	uint32_t width;
	uint32_t height;
	GLenum type;
};
struct Material
{
	enum ALPHA_MODE
	{
		ALPHA_MODE_OPAQUE,
		ALPHA_MODE_BLEND,
	};
	struct TextureInfo
	{
		Texture* baseColor;
		Texture* normal;
		Texture* emissive;
		Texture* ao;
		Texture* metallicRoughness;
		uint8_t baseColorUV;
		uint8_t normalUV;
		uint8_t emissiveUV;
		uint8_t aoUV;
		uint8_t metallicRoughnessUV;
	}tex;

	glm::vec4 baseColorFactor;
	glm::vec4 emissiveFactor;
	glm::vec4 diffuseFactor;
	glm::vec4 specularFactor;

	float roughnessFactor = 0.0f;
	float metallicFactor = 0.0f;
	float alphaMask = 0.0f;
	float alphaCutoff = 0.0f;

	float workflow = 0.0f;

	int mode = 0;

	GLuint uniform;
};

struct Mesh
{
	uint32_t startIdx;
	uint32_t numInd;
	uint32_t flags;
	uint32_t skinIdx;
	struct Material* material;
	AABB bound;
};
struct Model
{
	struct PhysicsConvexMesh* convexMesh;
	struct PhysicsConcaveMesh* concaveMesh;

	Material* materials;
	Animation* animations;
	Mesh* meshes;
	Texture** textures;
	Joint* joints;
	Skin* skins;
	Joint** nodes;	// renderable objects
	
	uint32_t numMaterials;
	uint32_t numAnimations;
	uint32_t numTextures;
	uint32_t numMeshes;
	uint32_t numJoints;
	uint32_t numSkins;
	uint32_t numNodes;

	GLuint vao;
	GLuint vertexBuffer;
	GLuint indexBuffer;
	uint32_t numVertices;
	uint32_t numIndices;

	AABB bound;
	glm::mat4 baseTransform;
	uint32_t flags;
};


struct AnimationInstanceData
{
	struct SkinData
	{
		AnimationTransformation* current;
		glm::mat4 baseTransform;
		GLuint skinUniform;
		uint32_t numTransforms;
	};
	SkinData* data;
	uint32_t numSkins;
};



void GenerateModelVertexBuffer(GLuint* vaoOut, GLuint* vtxBufOut, Vertex3D* vtx, uint32_t num);
Model CreateModelFromVertices(Vertex3D* verts, uint32_t numVerts);
void UpdateModelFromVertices(Model* model, Vertex3D* verts, uint32_t numVerts);

void CreateBoneDataFromModel(const Model* model, AnimationInstanceData* anim);
void UpdateBoneDataFromModel(const Model* model, uint32_t animIdx, uint32_t skinIdx, AnimationInstanceData* anim, float time);

void CreateMaterialUniform(Material* mat);
void UpdateMaterialUniform(Material* mat);