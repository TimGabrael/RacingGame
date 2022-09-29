#pragma once
#include "GLIncludes.h"

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

struct Model
{
	GLuint vao;
	GLuint vertexBuffer;
	GLuint indexBuffer;
	uint32_t numVertices;
	uint32_t numIndices;
	AABB bound;
	glm::mat4 baseTransform;
};

void GenerateModelVertexBuffer(GLuint* vaoOut, GLuint* vtxBufOut, Vertex3D* vtx, uint32_t num);


void CreateBoneDataDataFromAnimation(const Animation* anim, GLuint* uniform);
void UpdateBoneDataFromAnimation(const Animation* anim, GLuint uniform, float oldTime, float newTime);