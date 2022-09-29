#include "ModelInfo.h"
#include "Renderer.h"

void GenerateModelVertexBuffer(GLuint* vaoOut, GLuint* vtxBufOut, Vertex3D* vtx, uint32_t num)
{
	glGenVertexArrays(1, vaoOut);
	glGenBuffers(1, vtxBufOut);
	
	glBindVertexArray(*vaoOut);
	glBindBuffer(GL_VERTEX_ARRAY, *vtxBufOut);

	glBufferData(GL_VERTEX_ARRAY, sizeof(Vertex3D) * num, vtx, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), nullptr);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, nor));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, uv1));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, uv2));
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, joint));
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, weights));
	glVertexAttribPointer(6, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, col));


	glEnableVertexArrayAttrib(*vaoOut, 0);
	glEnableVertexArrayAttrib(*vaoOut, 1);
	glEnableVertexArrayAttrib(*vaoOut, 2);
	glEnableVertexArrayAttrib(*vaoOut, 3);
	glEnableVertexArrayAttrib(*vaoOut, 4);
	glEnableVertexArrayAttrib(*vaoOut, 5);
	glEnableVertexArrayAttrib(*vaoOut, 5);
	glEnableVertexArrayAttrib(*vaoOut, 6);

	glBindVertexArray(0);
}

void CreateBoneDataDataFromAnimation(const Animation* anim, GLuint* uniform)
{
	if (anim)
	{

	}
	else
	{
		BoneData boneData = {};
		glGenBuffers(1, uniform);
		glBindBuffer(GL_UNIFORM_BUFFER, *uniform);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(BoneData), &boneData, GL_STATIC_DRAW);
	}
}
void UpdateBoneDataFromAnimation(const Animation* anim, GLuint uniform, float oldTime, float newTime)
{

}