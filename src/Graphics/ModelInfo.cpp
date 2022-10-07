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
Model CreateModelFromVertices(Vertex3D* verts, uint32_t numVerts)
{
	Model m; memset(&m, 0, sizeof(Model));
	GenerateModelVertexBuffer(&m.vao, &m.vertexBuffer, verts, numVerts);
	m.bound = { glm::vec3(-FLT_MAX), glm::vec3(FLT_MAX) };
	
	m.meshes = new Mesh;
	memset(m.meshes, 0, sizeof(Mesh));
	m.numMeshes = 1;

	m.meshes->bound = { glm::vec3(-FLT_MAX), glm::vec3(FLT_MAX) };
	m.meshes->material = nullptr;
	m.meshes->numInd = numVerts;
	m.meshes->startIdx = 0;
	m.baseTransform = glm::mat4(1.0f);

	m.numIndices = numVerts;
	m.numVertices = numVerts;
	uint32_t* indices = new uint32_t[numVerts];
	for (uint32_t i = 0; i < numVerts; i++)
	{
		indices[i] = i;
	}
	glGenBuffers(1, &m.indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * numVerts, indices, GL_DYNAMIC_DRAW);
	delete[] indices;
	return m;
}
void UpdateModelFromVertices(Model* model, Vertex3D* verts, uint32_t numVerts)
{
	if (numVerts == 0) return;
	glBindVertexArray(model->vao);
	glBindBuffer(GL_ARRAY_BUFFER, model->vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->indexBuffer);

	model->numVertices = numVerts;
	model->numIndices = numVerts;

	model->meshes->bound = { glm::vec3(-FLT_MAX), glm::vec3(FLT_MAX) };
	model->meshes->material = nullptr;
	model->meshes->numInd = numVerts;
	model->meshes->startIdx = 0;
	model->baseTransform = glm::mat4(1.0f);
	model->meshes->flags = MESH_FLAG_LINE;

	uint32_t* indices = new uint32_t[numVerts];
	for (uint32_t i = 0; i < numVerts; i++)
	{
		indices[i] = i;
	}
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex3D) * numVerts, verts, GL_DYNAMIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * numVerts, indices, GL_DYNAMIC_DRAW);


	delete[] indices;
}

void CreateBoneDataFromModel(const Model* model, AnimationInstanceData* anim)
{
	anim->data = new AnimationInstanceData::SkinData[model->numSkins];
	anim->numSkins = model->numSkins;
	for (uint32_t i = 0; i < model->numSkins; i++)
	{
		Skin& skin = model->skins[i];
		AnimationInstanceData::SkinData& cur = anim->data[i];
		cur.numTransforms = skin.numJoints;
		cur.current = new AnimationTransformation[skin.numJoints];
		
		BoneData boneData{};
		const uint32_t numJoints = glm::min(skin.numJoints, (uint32_t)MAX_NUM_JOINTS);
		boneData.numJoints = numJoints;

		for (uint32_t j = 0; j < numJoints; j++)
		{
			Joint* joint = skin.joints[j];
			if (joint->mesh)
			{
				glm::mat4 inverse = glm::inverse(joint->defMatrix);
				for (uint32_t k = 0; k < numJoints; k++)
				{
					Joint* childJoint = skin.joints[k];
					glm::mat4 jointMat = inverse * childJoint->defMatrix * skin.inverseBindMatrices[i];
					boneData.jointMatrix[k] = jointMat;
				}
				break;
			}
		}
		glGenBuffers(1, &cur.skinUniform);
		glBindBuffer(GL_UNIFORM_BUFFER, cur.skinUniform);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(BoneData), &boneData, GL_DYNAMIC_DRAW);
	}
}
void UpdateBoneDataFromModel(const Model* model, uint32_t animIdx, uint32_t skinIdx, AnimationInstanceData* animInstance, float time)
{
	if (model->numAnimations <= animIdx) { LOG("WARNING TRYING TO UPDATE INVALID ANIMATION\n"); return; }
	if (model->numSkins <= skinIdx) { LOG("WARNING TRYING TO UPDATE ANIMATION OF INVALID SKIN\n"); return; }
	if (animInstance->numSkins <= skinIdx) { LOG("WARNING TRYING TO UPDATE ANIMATION INSTANCE WITH MISMATCHING ANIMATION INFO\n"); return; }
	
	Animation& anim = model->animations[animIdx];
	bool updated = false;
	for (uint32_t i = 0; i < anim.numChannels; i++) 
	{
		AnimationChannel& channel = anim.channels[i];
		AnimationSampler& sampler = anim.samplers[channel.samplerIdx];

		for (uint32_t j = 0; j < sampler.numInOut - 1; j++) 
		{
			if ((time >= sampler.inputs[j]) && (time <= sampler.inputs[j + 1]))
			{
				float u = glm::max(0.0f, time - sampler.inputs[j]) / (sampler.inputs[j + 1] - sampler.inputs[j]);
				if (u <= 1.0f) 
				{
					// TODO: CHECK IF THIS ALSO APPLYS SKELETEL ANIMATION
					switch(channel.path)
					{
					case AnimationChannel::TRANSLATION: {
						glm::vec4 trans = glm::mix(sampler.outputs[j], sampler.outputs[j + 1], u);
						channel.joint->translation = glm::vec3(trans);	// 
						break;
					}
					case AnimationChannel::SCALE: {
						glm::vec4 scale = glm::mix(sampler.outputs[j], sampler.outputs[j + 1], u);
						channel.joint->scale = glm::vec3(scale);
						break;
					}
					case AnimationChannel::ROTATION: {
						glm::quat q1;
						q1.x = sampler.outputs[j].x;
						q1.y = sampler.outputs[j].y;
						q1.z = sampler.outputs[j].z;
						q1.w = sampler.outputs[j].w;
						glm::quat q2;
						q2.x = sampler.outputs[j].x;
						q2.y = sampler.outputs[j].y;
						q2.z = sampler.outputs[j].z;
						q2.w = sampler.outputs[j].w;
						channel.joint->rotation = glm::normalize(glm::slerp(q1, q2, u));
						break;
					}
					}
					updated = true;
				}
			}
		}
	}
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