#include "ModelInfo.h"
#include "Renderer.h"

void GenerateModelVertexBuffer(GLuint* vaoOut, GLuint* vtxBufOut, Vertex3D* vtx, uint32_t num)
{
	glGenVertexArrays(1, vaoOut);
	glGenBuffers(1, vtxBufOut);
	
	glBindVertexArray(*vaoOut);
	glBindBuffer(GL_ARRAY_BUFFER, *vtxBufOut);

	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex3D) * num, vtx, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), nullptr);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, nor));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, uv1));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, uv2));
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, joint));
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, weights));
	glVertexAttribPointer(6, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, col));


	glEnableVertexArrayAttrib(*vaoOut, 0);
	glEnableVertexArrayAttrib(*vaoOut, 1);
	glEnableVertexArrayAttrib(*vaoOut, 2);
	glEnableVertexArrayAttrib(*vaoOut, 3);
	glEnableVertexArrayAttrib(*vaoOut, 4);
	glEnableVertexArrayAttrib(*vaoOut, 5);
	glEnableVertexArrayAttrib(*vaoOut, 6);


	glBindVertexArray(0);
}

void CreateBoneDataFromAnimation(const Animation* anim, GLuint* uniform)
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

void CreateMaterialUniform(Material* mat)
{
	MaterialData data;
	data.baseColorFactor = mat->baseColorFactor;
	data.emissiveFactor = mat->emissiveFactor;
	data.diffuseFactor = mat->diffuseFactor;
	data.specularFactor = mat->specularFactor;
	data.baseColorUV = mat->tex.baseColorUV > 5 ? -1 : mat->tex.baseColorUV;
	data.normalUV = mat->tex.normalUV > 5 ? -1 : mat->tex.normalUV;
	data.emissiveUV = mat->tex.emissiveUV > 5 ? -1 : mat->tex.emissiveUV;
	data.aoUV = mat->tex.aoUV > 5 ? -1 : mat->tex.aoUV;
	data.metallicRoughnessUV = mat->tex.metallicRoughnessUV > 5 ? -1 : mat->tex.metallicRoughnessUV;
	data.roughnessFactor = mat->roughnessFactor;
	data.metallicFactor = mat->metallicFactor;
	data.alphaMask = mat->alphaMask;
	data.alphaCutoff = mat->alphaCutoff;
	data.workflow = mat->workflow;
	data._align1 = 0;
	data._align2 = 0;

	data.baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
	data.emissiveFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
	data.baseColorUV = 0;

#ifdef DEBUG_PRINT_MATERIAL
	LOG("BASECOLORFACTOR: (%f, %f, %f, %f)\n", data.baseColorFactor.r, data.baseColorFactor.g, data.baseColorFactor.b, data.baseColorFactor.a);
	LOG("EMISSIVEFACTOR: (%f, %f, %f, %f)\n", data.emissiveFactor.r, data.emissiveFactor.g, data.emissiveFactor.b, data.emissiveFactor.a);
	LOG("DIFFUSEFACTOR: (%f, %f, %f, %f)\n", data.diffuseFactor.r, data.diffuseFactor.g, data.diffuseFactor.b, data.diffuseFactor.a);
	LOG("SPECULARFACTOR: (%f, %f, %f, %f)\n", data.specularFactor.r, data.specularFactor.g, data.specularFactor.b, data.specularFactor.a);
	LOG("BASECOLORUV: %d\n", data.baseColorUV);
	LOG("NORMALUV: %d\n", data.normalUV);
	LOG("EMISSIVEUV: %d\n", data.emissiveUV);
	LOG("AOUV: %d\n", data.aoUV);
	LOG("METALLICROUGHNESSUV: %d\n", data.metallicRoughnessUV);
	LOG("ROUGHNESSFACTOR: %f\n", data.roughnessFactor);
	LOG("METALLICFACTOR: %f\n", data.metallicFactor);
	LOG("ALPHAMASK: %f\n", data.alphaMask);
	LOG("ALPHACUTOFF: %f\n", data.alphaCutoff);
	LOG("WORKFLOW: %f\n", data.workflow);
#endif

	glGenBuffers(1, &mat->uniform);
	glBindBuffer(GL_UNIFORM_BUFFER, mat->uniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialData), &data, GL_STATIC_DRAW);


}
void UpdateMaterialUniform(Material* mat)
{
	MaterialData data;
	data.baseColorFactor = mat->baseColorFactor;
	data.emissiveFactor = mat->emissiveFactor;
	data.diffuseFactor = mat->diffuseFactor;
	data.specularFactor = mat->specularFactor;
	data.baseColorUV = mat->tex.baseColorUV > 5 ? -1 : mat->tex.baseColorUV;
	data.normalUV = mat->tex.normalUV > 5 ? -1 : mat->tex.normalUV;
	data.emissiveUV = mat->tex.emissiveUV > 5 ? -1 : mat->tex.emissiveUV;
	data.aoUV = mat->tex.aoUV > 5 ? -1 : mat->tex.aoUV;
	data.metallicRoughnessUV = mat->tex.metallicRoughnessUV > 5 ? -1 : mat->tex.metallicRoughnessUV;
	data.roughnessFactor = mat->roughnessFactor;
	data.metallicFactor = mat->metallicFactor;
	data.alphaMask = mat->alphaMask;
	data.alphaCutoff = mat->alphaCutoff;
	data.workflow = mat->workflow;
	data._align1 = 0;
	data._align2 = 0;

	glBindBuffer(GL_UNIFORM_BUFFER, mat->uniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialData), &data, GL_STATIC_DRAW);
}