#pragma once
#include "GLIncludes.h"

enum MESH_FLAG
{
	MESH_FLAG_LINE = 1,
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

struct Animation
{
	float duration;
	int numEffectedBones;
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
	Material* material;
	uint32_t flags;
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
	
	uint32_t numMaterials;
	uint32_t numAnimations;
	uint32_t numTextures;
	uint32_t numMeshes;


	GLuint vao;
	GLuint vertexBuffer;
	GLuint indexBuffer;
	uint32_t numVertices;
	uint32_t numIndices;

	AABB bound;
	glm::mat4 baseTransform;
	uint32_t flags;
};


void GenerateModelVertexBuffer(GLuint* vaoOut, GLuint* vtxBufOut, Vertex3D* vtx, uint32_t num);
Model CreateModelFromVertices(Vertex3D* verts, uint32_t numVerts);
void UpdateModelFromVertices(Model* model, Vertex3D* verts, uint32_t numVerts);

void CreateBoneDataFromAnimation(const Animation* anim, GLuint* uniform);
void UpdateBoneDataFromAnimation(const Animation* anim, GLuint uniform, float oldTime, float newTime);

void CreateMaterialUniform(Material* mat);
void UpdateMaterialUniform(Material* mat);