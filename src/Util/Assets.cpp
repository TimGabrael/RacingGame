#include "Assets.h"
#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "../Graphics/ModelInfo.h"
#include "../Graphics/Renderer.h"
#include "assimp/BaseImporter.h"
#include "zlib.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "tiny_gltf.h"
#include "stb_image_resize.h"
#include "stb_truetype.h"

#include "finders_interface.h"


#include "../GameState.h"
#include "../Physics/Physics.h"





AssetManager* AM_CreateAssetManager()
{
	AssetManager* out = new AssetManager;

	{
		Texture* defaultWhite = new Texture;
		defaultWhite->width = 1;
		defaultWhite->height = 1;
		defaultWhite->type = GL_TEXTURE_2D;
		glGenTextures(1, &defaultWhite->uniform);
		glBindTexture(defaultWhite->type, defaultWhite->uniform);
		uint32_t whiteData[2] = { 0xFFFFFFFF, 0xFFFFFFFF };
		glTexImage2D(defaultWhite->type, 0, GL_RGBA, defaultWhite->width, defaultWhite->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, whiteData);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		out->textures[DEFAULT_WHITE_TEXTURE] = defaultWhite;
	}

	{
		Texture* defaultBlack = new Texture;
		defaultBlack->width = 1;
		defaultBlack->height = 1;
		defaultBlack->type = GL_TEXTURE_2D;
		glGenTextures(1, &defaultBlack->uniform);
		glBindTexture(defaultBlack->type, defaultBlack->uniform);
		uint32_t blackData[2] = { 0x0, 0x0 };
		glTexImage2D(defaultBlack->type, 0, GL_RGBA, defaultBlack->width, defaultBlack->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, blackData);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		out->textures[DEFAULT_BLACK_TEXTURE] = defaultBlack;
	}

	return out;
}
void AM_CleanUpAssetManager(AssetManager* assets)
{

}

struct Model* AM_AddModel(AssetManager* m, const char* file, uint32_t flags)
{
	bool isGlb = false;
	bool isGltf = false;
	Model* model = new Model;
	memset(model, 0, sizeof(Model));

	int fileLen = strnlen(file, 250);
	if (strncmp(file + fileLen - 4, ".glb", 4) == 0) isGlb = true;
	else if (strncmp(file + fileLen - 5, ".gltf", 5) == 0) isGltf = true;

	if (isGltf || isGlb)
	{
		tinygltf::Model gltfModel;

		// LOAD THE GLTF MODEL
		{
			if (isGlb)
			{
				std::string err;
				std::string warn;
				tinygltf::TinyGLTF loader;
				loader.LoadBinaryFromFile(&gltfModel, &err, &warn, file);
				if (!warn.empty()) {
					LOG("WARN: %s\n", warn.c_str());
					delete model;
					return nullptr;
				}

				if (!err.empty()) {
					LOG("ERR: %s\n", err.c_str());
					delete model;
					return nullptr;
				}
			}
			else
			{
				std::string err;
				std::string warn;
				tinygltf::TinyGLTF loader;
				loader.LoadASCIIFromFile(&gltfModel, &err, &warn, file);
				if (!warn.empty()) {
					LOG("WARN: %s\n", warn.c_str());
					delete model;
					return nullptr;
				}

				if (!err.empty()) {
					LOG("ERR: %s\n", err.c_str());
					delete model;
					return nullptr;
				}
			}
		}

		for (auto& m : gltfModel.meshes)
		{
			for (auto& p : m.primitives)
			{
				if ((p.mode == TINYGLTF_MODE_LINE || p.mode == TINYGLTF_MODE_TRIANGLES) && p.indices > -1)
				{
					model->numVertices += gltfModel.accessors[p.attributes.find("POSITION")->second].count;
					model->numIndices += gltfModel.accessors[p.indices].count;
				}
				else if (p.mode == TINYGLTF_MODE_LINE || p.mode == TINYGLTF_MODE_TRIANGLES)
				{
					model->numVertices += gltfModel.accessors[p.attributes.find("POSITION")->second].count;
				}
				else
				{
					LOG("WARNING MODEL HAS UNSUPPORTED MESH\n");
					LOG("p.mode: / p.indices: %d %d\n", p.mode, p.indices);
				}
				
			}
		}
		model->numMeshes = gltfModel.meshes.size();
		model->numAnimations = gltfModel.animations.size();
		model->numTextures = gltfModel.textures.size(); 
		model->numMaterials = gltfModel.materials.size();
		model->numJoints = gltfModel.nodes.size();
		model->numSkins = gltfModel.skins.size();

		model->meshes = new Mesh[model->numMeshes];
		memset(model->meshes, 0, sizeof(Mesh) * model->numMeshes);
		model->textures = new Texture*[model->numTextures];
		memset(model->textures, 0, sizeof(Texture*) * model->numTextures);
		model->materials = new Material[model->numMaterials];
		memset(model->materials, 0, sizeof(Material) * model->numMaterials);
		model->joints = new Joint[model->numJoints];
		memset(model->joints, 0, sizeof(Joint) * model->numJoints);
		model->skins = new Skin[model->numSkins];
		memset(model->skins, 0, sizeof(Skin) * model->numSkins);
		model->animations = new Animation[model->numAnimations];
		memset(model->animations, 0, sizeof(Animation) * model->numAnimations);


		model->baseTransform = glm::mat4(1.0f);

		// GENERATE BUFFER DATA
		{
			Vertex3D* verts = new Vertex3D[model->numVertices];
			uint32_t* inds = nullptr;
			if(model->numIndices > 0) inds = new uint32_t[model->numIndices];

			uint32_t curVertPos = 0;
			uint32_t curIndPos = 0;

			int curMeshIdx = 0;
			for (auto& m : gltfModel.meshes)
			{
				int curPrimIdx = 0;
				Mesh& myMesh = model->meshes[curMeshIdx];
				myMesh.numPrimitives = m.primitives.size();
				myMesh.primitives = new Primitive[myMesh.numPrimitives];
				memset(myMesh.primitives, 0, sizeof(Primitive) * myMesh.numPrimitives);
				myMesh.bound = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };

				for (auto& primitive : m.primitives)
				{
					Primitive& myPrim = myMesh.primitives[curPrimIdx];
					if (primitive.mode == TINYGLTF_MODE_TRIANGLES || primitive.mode == TINYGLTF_MODE_LINE)
					{
						if (primitive.mode == TINYGLTF_MODE_LINE) myPrim.flags |= PRIMITIVE_FLAG_LINE;
						if (primitive.indices <= -1) myPrim.flags |= PRIMITIVE_FLAG_NO_INDEX_BUFFER;

						const uint32_t vertexStart = curVertPos;
						{
							bool hasSkin = false;
							const float* bufferPos = nullptr;
							const float* bufferNormals = nullptr;
							const float* bufferTexCoordSet0 = nullptr;
							const float* bufferTexCoordSet1 = nullptr;
							const void* bufferJoints = nullptr;
							const float* bufferWeights = nullptr;

							int posByteStride = -1;
							int normByteStride = -1;
							int uv0ByteStride = -1;
							int uv1ByteStride = -1;
							int jointByteStride = -1;
							int weightByteStride = -1;

							int jointComponentType;

							const tinygltf::Accessor& posAccessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
							const tinygltf::BufferView& posView = gltfModel.bufferViews[posAccessor.bufferView];
							bufferPos = reinterpret_cast<const float*>(&(gltfModel.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
							posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

							myPrim.bound.min = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
							myPrim.bound.max = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);

							myMesh.bound.min = glm::min(myMesh.bound.min, myPrim.bound.min);
							myMesh.bound.max = glm::max(myMesh.bound.max, myPrim.bound.max);


							if (primitive.material > -1 && primitive.material < model->numMaterials)
							{
								myPrim.material = &model->materials[primitive.material];
							}

							if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
								const tinygltf::Accessor& normAccessor = gltfModel.accessors[primitive.attributes.find("NORMAL")->second];
								const tinygltf::BufferView& normView = gltfModel.bufferViews[normAccessor.bufferView];
								bufferNormals = reinterpret_cast<const float*>(&(gltfModel.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
								normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
							}
							if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
								const tinygltf::Accessor& uvAccessor = gltfModel.accessors[primitive.attributes.find("TEXCOORD_0")->second];
								const tinygltf::BufferView& uvView = gltfModel.bufferViews[uvAccessor.bufferView];
								bufferTexCoordSet0 = reinterpret_cast<const float*>(&(gltfModel.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
								uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
							}
							if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
								const tinygltf::Accessor& uvAccessor = gltfModel.accessors[primitive.attributes.find("TEXCOORD_1")->second];
								const tinygltf::BufferView& uvView = gltfModel.bufferViews[uvAccessor.bufferView];
								bufferTexCoordSet1 = reinterpret_cast<const float*>(&(gltfModel.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
								uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
							}
							if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
								const tinygltf::Accessor& jointAccessor = gltfModel.accessors[primitive.attributes.find("JOINTS_0")->second];
								const tinygltf::BufferView& jointView = gltfModel.bufferViews[jointAccessor.bufferView];
								bufferJoints = &(gltfModel.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]);
								jointComponentType = jointAccessor.componentType;
								jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / tinygltf::GetComponentSizeInBytes(jointComponentType)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
							}
							if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
								const tinygltf::Accessor& weightAccessor = gltfModel.accessors[primitive.attributes.find("WEIGHTS_0")->second];
								const tinygltf::BufferView& weightView = gltfModel.bufferViews[weightAccessor.bufferView];
								bufferWeights = reinterpret_cast<const float*>(&(gltfModel.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
								weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
							}
							
							hasSkin = (bufferJoints && bufferWeights);
							for (size_t v = 0; v < posAccessor.count; v++) {
								Vertex3D& vert = verts[curVertPos];
								vert.pos = glm::make_vec3(&bufferPos[v * posByteStride]);
								model->meshes[curMeshIdx].bound.min = glm::min(model->meshes[curMeshIdx].bound.min, vert.pos);
								model->meshes[curMeshIdx].bound.max = glm::max(model->meshes[curMeshIdx].bound.max, vert.pos);
								vert.nor = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
								vert.uv1 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
								vert.uv2 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);
								vert.col = 0xFFFFFFFF;	// white base color
								if (hasSkin)
								{
									switch (jointComponentType) {
									case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
										const uint16_t* buf = static_cast<const uint16_t*>(bufferJoints);
										vert.joint = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
										break;
									}
									case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
										const uint8_t* buf = static_cast<const uint8_t*>(bufferJoints);
										vert.joint = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
										break;
									}
									default:
										// Not supported by spec
										LOG("Joint component type %d not supported\n", jointComponentType);
										break;
									}
								}
								else {
									vert.joint = glm::vec4(0.0f);
								}
								vert.weights = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);
								// Fix for all zero weights
								if (glm::length(vert.weights) == 0.0f) {
									vert.weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
								}
								curVertPos++;
							}
						}
						if(primitive.indices > -1)
						{
							myPrim.startIdx = curIndPos;
							const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.indices];
							const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
							const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
							const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

							myPrim.numInd = static_cast<uint32_t>(accessor.count);

							switch (accessor.componentType) {
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
								const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
								for (size_t index = 0; index < accessor.count; index++) {
									inds[curIndPos] = buf[index] + vertexStart;
									curIndPos++;
								}
								break;
							}
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
								const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
								for (size_t index = 0; index < accessor.count; index++) {
									inds[curIndPos] = buf[index] + vertexStart;
									curIndPos++;
								}
								break;
							}
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
								const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
								for (size_t index = 0; index < accessor.count; index++) {
									inds[curIndPos] = buf[index] + vertexStart;
									curIndPos++;
								}
								break;
							}
							}
						}
						else
						{
							myPrim.startIdx = vertexStart;
							myPrim.numInd = curVertPos - vertexStart;
						}
					}
					else
					{
						myPrim.flags = PRIMITIVE_FLAG_UNSUPPORTED;
					}
					curPrimIdx++;
				}
				myMesh.numPrimitives = curPrimIdx;
				curMeshIdx++;
			}

			if (flags & MODEL_LOAD_REVERSE_FACE_ORIENTATION)
			{
				if (inds)
				{
					for (uint32_t i = 0; i < curIndPos; i+=3)
					{
						std::swap(inds[i], inds[i + 2]);	
					}
					for (uint32_t i = 0; i < curVertPos; i++)
					{
						verts[i].nor = -verts[i].nor;
					}
				}
				else
				{
					LOG("WARNING REVERSEING THE FACES OF A VERTEX ONLY MESH\n");
				}
			}
			if (flags & MODEL_SET_ORIGIN_CENTER)
			{
				glm::vec3 center = { 0.0f, 0.0f, 0.0f };
				for (uint32_t i = 0; i < curVertPos; i++)
				{
					center += verts[i].pos;
				}
				center /= (float)curVertPos;
				for (uint32_t i = 0; i < curVertPos; i++)
				{
					verts[i].pos -= center;
				}
			}
			GenerateModelVertexBuffer(&model->vao, &model->vertexBuffer, verts, curVertPos);

			if (inds)
			{
				glGenBuffers(1, &model->indexBuffer);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->indexBuffer);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * curIndPos, inds, GL_STATIC_DRAW);
			}

			GameState* game = GetGameState();
			if (flags & MODEL_LOAD_CONVEX)
			{
				model->convexMesh = PH_AddPhysicsConvexMesh(game->physics, verts, curVertPos, sizeof(Vertex3D));
			}
			if (flags & MODEL_LOAD_CONCAVE)
			{
				if (!inds)
				{
					inds = new uint32_t[curVertPos];
					for (uint32_t i = 0; i < curVertPos; i++)
					{
						inds[i] = i;
					}
					curIndPos = curVertPos;
				}
				model->concaveMesh = PH_AddPhysicsConcaveMesh(game->physics, verts, curVertPos, sizeof(Vertex3D), inds, curIndPos);
			}


			delete[] verts;
			if(inds) delete[] inds;

		}
		// LOAD TEXTURES
		for (uint32_t i = 0; i < gltfModel.textures.size(); i++)
		{
			const tinygltf::Texture& tex = gltfModel.textures.at(i);
			const tinygltf::Image& img = gltfModel.images[tex.source];
			unsigned char* buffer = nullptr;
			size_t bufferSize = 0;
			bool deleteBuffer = false;

			if (img.component == 3)
			{
				bufferSize = img.width * img.height * 4;
				buffer = new unsigned char[bufferSize];
				unsigned char* rgba = buffer;
				const unsigned char* rgb = &img.image.at(0);
				for (int32_t i = 0; i < img.width * img.height; i++)
				{
					for (int32_t j = 0; j < 3; j++)
					{
						buffer[j] = rgb[i];
					}
					rgba += 4;
					rgb += 3;
				}
				deleteBuffer = true;
			}
			else
			{
				buffer = (unsigned char*)&img.image.at(0);
				bufferSize = img.image.size();
			}

			Texture* resTex = new Texture;
			resTex->width = glm::min(img.width, 0x8000);
			resTex->height = glm::min(glm::max(img.height, 1), 0x8000);
			resTex->type = GL_TEXTURE_2D;

			glGenTextures(1, &resTex->uniform);
			glBindTexture(GL_TEXTURE_2D, resTex->uniform);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resTex->width, resTex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			if (deleteBuffer)
			{
				delete[] buffer;
			}
			model->textures[i] = resTex;

			std::string fullstr = std::string(":") + file + tex.name;
			m->textures[fullstr] = resTex;
		}

		// GENERATE MATERIALS
		for (uint32_t i = 0; i < gltfModel.materials.size(); i++)
		{
			tinygltf::Material& mat = gltfModel.materials.at(i);
			Material& myMat = model->materials[i];

			myMat.baseColorFactor = glm::vec4(1.0f);
			myMat.emissiveFactor = glm::vec4(1.0f);
			myMat.diffuseFactor = glm::vec4(1.0f);
			myMat.specularFactor = glm::vec4(0.0f);

			myMat.roughnessFactor = 1.0f;
			myMat.metallicFactor = 1.0f;

			myMat.tex.aoUV = -1;
			myMat.tex.emissiveUV = -1;
			myMat.tex.baseColorUV = -1;
			myMat.tex.metallicRoughnessUV = -1;
			myMat.tex.normalUV = -1;

			if (mat.values.find("baseColorTexture") != mat.values.end())
			{
				myMat.tex.baseColor = model->textures[mat.values["baseColorTexture"].TextureIndex()];
				const int texCoord = mat.values["baseColorTexture"].TextureTexCoord();
				myMat.tex.baseColorUV = texCoord;
			}
			if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
				myMat.tex.metallicRoughness = model->textures[mat.values["metallicRoughnessTexture"].TextureIndex()];
				const int texCoord = mat.values["metallicRoughnessTexture"].TextureTexCoord();
				myMat.tex.metallicRoughnessUV = texCoord;
				myMat.workflow = 0.0f;
			}
			if (mat.values.find("roughnessFactor") != mat.values.end()) {
				myMat.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
			}
			if (mat.values.find("metallicFactor") != mat.values.end()) {
				myMat.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
			}
			if (mat.values.find("baseColorFactor") != mat.values.end()) {
				myMat.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
			}
			if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
				myMat.tex.normal = model->textures[mat.additionalValues["normalTexture"].TextureIndex()];
				const int texCoord = mat.additionalValues["normalTexture"].TextureTexCoord();
				myMat.tex.normalUV = texCoord;
			}
			if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
				myMat.tex.emissive = model->textures[mat.additionalValues["emissiveTexture"].TextureIndex()];
				const int texCoord = mat.additionalValues["emissiveTexture"].TextureTexCoord();
				myMat.tex.emissiveUV = texCoord;
			}
			if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
				myMat.tex.ao = model->textures[mat.additionalValues["occlusionTexture"].TextureIndex()];
				const int texCoord = mat.additionalValues["occlusionTexture"].TextureTexCoord();
				myMat.tex.aoUV = texCoord;
			}


			myMat.mode = Material::ALPHA_MODE::ALPHA_MODE_OPAQUE;
			myMat.alphaCutoff = 0.0f;
			if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
				tinygltf::Parameter param = mat.additionalValues["alphaMode"];
				if (param.string_value == "BLEND") {
					myMat.mode = Material::ALPHA_MODE::ALPHA_MODE_BLEND;
					myMat.alphaCutoff = 0.0f;
				}
				if (param.string_value == "MASK") {
					myMat.alphaCutoff = 0.5f;
					myMat.alphaMask = 1.0f;
				}
			}
			if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
				myMat.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
			}

			if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
				myMat.emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
				myMat.tex.emissiveUV = 0;
			}
			if (mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end()) {
				auto ext = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
				if (ext->second.Has("specularGlossinessTexture")) {
					auto index = ext->second.Get("specularGlossinessTexture").Get("index");
					myMat.tex.metallicRoughness = model->textures[index.Get<int>()];
					int texCoord = ext->second.Get("specularGlossinessTexture").Get("texCoord").Get<int>();
					myMat.tex.metallicRoughnessUV = texCoord ? texCoord : -1;
					myMat.workflow = 1.0f;
				}
				if (ext->second.Has("diffuseTexture")) {
					int index = ext->second.Get("diffuseTexture").Get("index").Get<int>();
					myMat.tex.baseColor = model->textures[index];
				}
				if (ext->second.Has("diffuseFactor")) {
					auto factor = ext->second.Get("diffuseFactor");
					for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
						auto val = factor.Get(i);
						myMat.diffuseFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
					}
				}
				if (ext->second.Has("specularFactor")) {
					auto factor = ext->second.Get("specularFactor");
					for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
						auto val = factor.Get(i);
						myMat.specularFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
					}
					if (myMat.workflow == 1.0f) myMat.specularFactor.w = 1.0f;
				}
			}
			CreateMaterialUniform(&myMat);
		}


		// CALCULATE MAIN BOUNDING BOX
		model->bound = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
		for (uint32_t i = 0; i < model->numMeshes; i++)
		{
			model->bound.min = glm::min(model->bound.min, model->meshes[i].bound.min);
			model->bound.max = glm::max(model->bound.max, model->meshes[i].bound.max);
		}
		// LOAD NODES(JOINTS)
		{
			std::function<void(Model* m, tinygltf::Model* gm, Joint* parent, int curNode)> setJointNodeHirarchy = [&](Model* m, tinygltf::Model* gm, Joint* parent, int curNode)
			{
				tinygltf::Node& node = gm->nodes[curNode];
				Joint& j = m->joints[curNode];
				j.matrix = glm::mat4(1.0f);
				j.defMatrix = glm::mat4(1.0f);
				j.skinIndex = node.skin;
				j.model = m;

				if (node.mesh > -1) {
					j.mesh = &m->meshes[node.mesh];
				}
				if (node.skin > -1) {
					j.skin = &m->skins[node.skin];
				}
				
				if(parent) j.parent = parent;
				
				j.children = new Joint*[gm->nodes[curNode].children.size()];
				memset(j.children, 0, sizeof(Joint*) * gm->nodes[curNode].children.size());
				j.numChildren = gm->nodes[curNode].children.size();
				glm::vec3 translation = glm::vec3(0.0f);
				if (node.translation.size() == 3) {
					translation = glm::make_vec3(node.translation.data());
				}
				glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
				if (node.rotation.size() == 4) {
					glm::vec4 r = glm::make_vec4(node.rotation.data());
					rotation = glm::quat(r.w, r.x, r.y, r.z);
				}
				glm::vec3 scale = glm::vec3(1.0f);
				if (node.scale.size() == 3) {
					scale = glm::make_vec3(node.scale.data());
				}
				if (node.matrix.size() == 16) {
					j.matrix = glm::make_mat4x4(node.matrix.data());
				}
				j.translation = translation;
				j.rotation = rotation;
				j.scale = scale;

				for (size_t k = 0; k < j.numChildren; k++)
				{
					int cur = gm->nodes[curNode].children.at(k);
					if (cur > -1 && cur < m->numJoints)
					{
						j.children[k] = &m->joints[cur];
						setJointNodeHirarchy(m, gm, &m->joints[curNode], cur);
					}
					else
					{
						LOG("WARNING: Trying to add child out of joint range\n");
					}
				}
			};
			for (size_t i = 0; i < gltfModel.nodes.size(); i++)
			{
				setJointNodeHirarchy(model, &gltfModel, nullptr, i);
			}
		}

		// LOAD SKINS
		{
			for (size_t i = 0; i < gltfModel.skins.size(); i++)
			{
				tinygltf::Skin& source = gltfModel.skins.at(i);
				Skin* curSkin = &model->skins[i];
				curSkin->numJoints = source.joints.size();
				curSkin->joints = new Joint*[curSkin->numJoints];
				memset(curSkin->joints, 0, sizeof(Joint*) * curSkin->numJoints);

				if (source.skeleton > -1)
				{
					curSkin->skeletonRoot = &model->joints[source.skeleton];
				}
				if (source.inverseBindMatrices > -1)
				{
					const tinygltf::Accessor& accessor = gltfModel.accessors[source.inverseBindMatrices];
					const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
					curSkin->inverseBindMatrices = new glm::mat4[accessor.count];
					if (curSkin->numJoints != accessor.count) LOG("ERROR JOINTS AND BIND MATRICES MISMATCH\n");
					memcpy(curSkin->inverseBindMatrices, &buffer.data[accessor.byteOffset + bufferView.byteOffset], sizeof(glm::mat4) * accessor.count);
				}
				for (size_t j = 0; j < source.joints.size(); j++)
				{
					curSkin->joints[j] = &model->joints[source.joints.at(j)];
				}
			}
		}
		// LOAD ANIMATIONS
		{
			for (size_t i = 0; i < gltfModel.animations.size(); i++)
			{
				tinygltf::Animation& anim = gltfModel.animations.at(i);
				Animation& myAnim = model->animations[i];
				myAnim.start = FLT_MAX;
				myAnim.end = -FLT_MAX;
				myAnim.numSamplers = anim.samplers.size();
				myAnim.numChannels = anim.channels.size();
				myAnim.samplers = new AnimationSampler[myAnim.numSamplers];
				memset(myAnim.samplers, 0, sizeof(AnimationSampler)* myAnim.numSamplers);
				myAnim.channels = new  AnimationChannel[myAnim.numChannels];
				memset(myAnim.channels, 0, sizeof(AnimationChannel)* myAnim.numChannels);

				for (size_t j = 0; j < anim.samplers.size(); j++)
				{
					tinygltf::AnimationSampler& samp = anim.samplers.at(j);
					AnimationSampler& mySamp = myAnim.samplers[j];
					if (samp.interpolation == "LINEAR") {
						mySamp.interpolation = AnimationSampler::InterpolationType::LINEAR;
					}
					if (samp.interpolation == "STEP") {
						mySamp.interpolation = AnimationSampler::InterpolationType::STEP;
					}
					if (samp.interpolation == "CUBICSPLINE") {
						mySamp.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
					}
					// Read sampler input time values
					{
						const tinygltf::Accessor& accessor = gltfModel.accessors[samp.input];
						const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
						const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

						assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
						mySamp.inputs = new float[accessor.count];
						memset(mySamp.inputs, 0, sizeof(float)* accessor.count);
						mySamp.numInOut = accessor.count;

						const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
						const float* buf = static_cast<const float*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++) {
							mySamp.inputs[index] = buf[index];
							if (mySamp.inputs[index] < myAnim.start) {
								myAnim.start = mySamp.inputs[index];
							}
							if (mySamp.inputs[index] > myAnim.end) {
								myAnim.end = mySamp.inputs[index];
							}
						}
					}

					// Read sampler output T/R/S values 
					{
						const tinygltf::Accessor& accessor = gltfModel.accessors[samp.output];
						const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
						const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

						assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
						assert(accessor.count == mySamp.numInOut);
						if (accessor.count != mySamp.numInOut) LOG("WARNING ANIMATION INOUT MISMATCH\n");

						const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

						mySamp.outputs = new glm::vec4[accessor.count];
						memset(mySamp.outputs, 0, sizeof(glm::vec4)* accessor.count);

						switch (accessor.type) {
						case TINYGLTF_TYPE_VEC3: {
							const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++) {
								mySamp.outputs[index] = glm::vec4(buf[index], 0.0f);
							}
							break;
						}
						case TINYGLTF_TYPE_VEC4: {
							const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++) {
								mySamp.outputs[index] = buf[index];
							}
							break;
						}
						default: {
							LOG("WARNING UNKOWN ANIMATION TYPE\n");
							break;
						}
						}
					}
				}


				for (size_t j = 0; j < anim.channels.size(); j++)
				{
					tinygltf::AnimationChannel& channel = anim.channels.at(j);
					AnimationChannel& myChannel = myAnim.channels[j];
					if (channel.target_path == "rotation") {
						myChannel.path = AnimationChannel::PathType::ROTATION;
					}
					if (channel.target_path == "translation") {
						myChannel.path = AnimationChannel::PathType::TRANSLATION;
					}
					if (channel.target_path == "scale") {
						myChannel.path = AnimationChannel::PathType::SCALE;
					}
					if (channel.target_path == "weights") {
						LOG("NOT SUPPORTED\n");
						continue;
					}
					myChannel.samplerIdx = channel.sampler;
					myChannel.joint = &model->joints[channel.target_node];
					if (!myChannel.joint) {
						LOG("WARNING NO SKIN ATTACHED TO ANIMATION\n");
					}
				}
			}
		}

	}
	else
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(file, aiProcess_GenNormals | aiProcess_Triangulate | aiProcess_LimitBoneWeights | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

		if (!scene) {
			delete model;
			return nullptr;
		}
		std::string filePath = file;
		{
			size_t idx = filePath.find_last_of("/\\");
			if (idx != -1)
			{
				filePath = filePath.substr(0, idx + 1);
			}
		}

		model->meshes = new Mesh[scene->mNumMeshes];
		memset(model->meshes, 0, sizeof(Mesh) * scene->mNumMeshes);
		model->textures = new Texture * [scene->mNumTextures];
		memset(model->textures, 0, sizeof(Texture*) * scene->mNumTextures);
		model->materials = new Material[scene->mNumMaterials];
		memset(model->materials, 0, sizeof(Material) * scene->mNumMaterials);
		model->numMeshes = scene->mNumMeshes;
		model->numTextures = scene->mNumTextures;
		model->numMaterials = scene->mNumMaterials;
		model->baseTransform = glm::mat4(1.0f);

		uint32_t numVerts = 0;
		uint32_t numInds = 0;
		for (int i = 0; i < scene->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[i];
			Mesh* myMesh = &model->meshes[i];

			myMesh->bound.min = { mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z };
			myMesh->bound.max = { mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z };

			uint32_t localNumInds = 0;
			for (int j = 0; j < mesh->mNumFaces; j++)
			{
				localNumInds += mesh->mFaces[j].mNumIndices;
			}
			// myMesh->startIdx = numInds;
			// myMesh->numInd = localNumInds;
			// 
			// myMesh->material = &model->materials[mesh->mMaterialIndex];

			numVerts += mesh->mNumVertices;
			numInds += localNumInds;
		}

		// GENERATE BUFFER DATA
		{
			Vertex3D* verts = new Vertex3D[numVerts];
			uint32_t* inds = new uint32_t[numInds];
			memset(verts, 0, sizeof(Vertex3D) * numVerts);
			memset(inds, 0, sizeof(uint32_t) * numInds);

			uint32_t curVertIdx = 0;
			uint32_t curIndIdx = 0;
			for (uint32_t i = 0; i < scene->mNumMeshes; i++)
			{
				aiMesh* mesh = scene->mMeshes[i];

				for (uint32_t j = 0; j < mesh->mNumVertices; j++)
				{
					Vertex3D& myV = verts[j + curVertIdx];
					myV.pos = { mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z };
					if (mesh->HasNormals())
					{
						myV.nor = { mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z };
					}
					if (mesh->HasVertexColors(0))
					{
						const aiColor4D& c = mesh->mColors[0][j];
						myV.col = RGBA((uint32_t)(c.r * 255), (uint32_t)(c.g * 255), (uint32_t)(c.b * 255), (uint32_t)(c.a * 255));
					}
					else
					{
						myV.col = 0xFFFFFFFF;
					}
					if (mesh->HasTextureCoords(0))
					{
						const aiVector3D& uv = mesh->mTextureCoords[0][j];
						myV.uv1 = { uv.x, -uv.y };
					}
					if (mesh->HasTextureCoords(1))
					{
						const aiVector3D& uv = mesh->mTextureCoords[1][j];
						myV.uv2 = { uv.x, -uv.y };
					}

					if (mesh->HasBones())
					{
						struct BoneInfo
						{
							uint32_t joint;
							float weight;
						};
						static constexpr uint32_t maxNumBonesTested = 100;
						BoneInfo allBoneData[maxNumBonesTested]{};
						uint32_t curBoneDataIdx = 0;
						for (uint32_t k = 0; k < mesh->mNumBones; k++)
						{
							for (uint32_t l = 0; mesh->mBones[k]->mNumWeights; l++)
							{
								const aiVertexWeight& weights = mesh->mBones[k]->mWeights[l];
								if (weights.mVertexId == j)
								{
									allBoneData[curBoneDataIdx].joint = k;
									allBoneData[curBoneDataIdx].weight = weights.mWeight;
									curBoneDataIdx += 1;
								}
								if (curBoneDataIdx >= maxNumBonesTested) break;
							}
						}

						if (curBoneDataIdx >= 4)
						{
							std::sort(allBoneData, allBoneData + 100, [](BoneInfo& b1, BoneInfo& b2) {
								if (b1.weight > b2.weight)  return 1;
								else if (b1.weight == b2.weight) return 0;
								return -1;
								});
							glm::vec4 n = glm::normalize(glm::vec4(allBoneData[0].weight, allBoneData[1].weight, allBoneData[2].weight, allBoneData[3].weight));
							allBoneData[0].weight = n.x;
							allBoneData[1].weight = n.y;
							allBoneData[2].weight = n.z;
							allBoneData[3].weight = n.w;
						}

						myV.joint = { allBoneData[0].joint, allBoneData[1].joint, allBoneData[2].joint, allBoneData[3].joint };
						myV.weights = { allBoneData[0].weight,  allBoneData[1].weight,  allBoneData[2].weight,  allBoneData[3].weight };
					}
				}



				for (uint32_t j = 0; j < mesh->mNumFaces; j++)
				{
					for (uint32_t k = 0; k < mesh->mFaces[j].mNumIndices; k++)
					{
						inds[curIndIdx + k] = mesh->mFaces[j].mIndices[k];
					}
					curIndIdx += mesh->mFaces[j].mNumIndices;
				}
				curVertIdx += mesh->mNumVertices;
			}

			GenerateModelVertexBuffer(&model->vao, &model->vertexBuffer, verts, numVerts);

			glGenBuffers(1, &model->indexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * numInds, inds, GL_STATIC_DRAW);

			GameState* game = GetGameState();
			if (flags & MODEL_LOAD_CONVEX)
			{
				model->convexMesh = PH_AddPhysicsConvexMesh(game->physics, verts, numVerts, sizeof(Vertex3D));
			}
			if (flags & MODEL_LOAD_CONCAVE)
			{
				model->concaveMesh = PH_AddPhysicsConcaveMesh(game->physics, verts, numVerts, sizeof(Vertex3D), inds, numInds);
			}

			delete[] verts;
			delete[] inds;
		}

		model->numVertices = numVerts;
		model->numIndices = numInds;

		// GENERATE MATERIAL
		for (int i = 0; i < scene->mNumMaterials; i++)
		{
			aiMaterial* mat = scene->mMaterials[i];
			Material& myMat = model->materials[i];

			auto loadTexture = [&](aiTextureType type, uint8_t* uvOut) -> Texture*
			{
				uint32_t uvIndex = 0;
				aiString textureFile;
				mat->GetTexture(type, 0, &textureFile, 0, &uvIndex);
				*uvOut = uvIndex;
				const aiTexture* texture = scene->GetEmbeddedTexture(textureFile.C_Str());
				if (texture)
				{
					Texture* resTex = new Texture;
					resTex->width = glm::min(texture->mWidth, 0x8000u);
					resTex->height = glm::min(glm::max(texture->mHeight, 1u), 0x8000u);
					resTex->type = GL_TEXTURE_2D;
					uint32_t* data = new uint32_t[resTex->width * resTex->height];
					for (int i = 0; i < resTex->width * resTex->height; i++)
					{
						data[i] = RGBA(texture->pcData[i].r, texture->pcData[i].g, texture->pcData[i].b, 255);
					}

					glGenTextures(1, &resTex->uniform);
					glBindTexture(GL_TEXTURE_2D, resTex->uniform);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resTex->width, resTex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

					glGenerateMipmap(GL_TEXTURE_2D);

					delete[] data;

					std::string fullstr = std::string(":") + file + std::string(texture->mFilename.C_Str());
					m->textures[fullstr] = resTex;
					return resTex;
				}
				else if (textureFile.length > 0)
				{
					return AM_AddTexture(m, (filePath + textureFile.C_Str()).c_str());
				}
				else
				{
					*uvOut = -1;
				}
				return nullptr;
			};
			myMat.tex.baseColor = loadTexture(aiTextureType_BASE_COLOR, &myMat.tex.baseColorUV);
			myMat.tex.normal = loadTexture(aiTextureType_NORMALS, &myMat.tex.normalUV);
			myMat.tex.emissive = loadTexture(aiTextureType_EMISSIVE, &myMat.tex.emissiveUV);
			myMat.tex.ao = loadTexture(aiTextureType_AMBIENT_OCCLUSION, &myMat.tex.aoUV);
			myMat.tex.metallicRoughness = loadTexture(aiTextureType_UNKNOWN, &myMat.tex.metallicRoughnessUV);


			// get material propertys
			{
				aiColor3D col;
				if(mat->Get(AI_MATKEY_COLOR_AMBIENT, col) == aiReturn_SUCCESS) myMat.baseColorFactor = { col.r, col.g, col.b, 1.0f };
				else myMat.baseColorFactor = glm::vec4(1.0f);
				float opacity = 0.0f;
				if (mat->Get(AI_MATKEY_OPACITY, opacity) == aiReturn_SUCCESS) myMat.baseColorFactor.w = opacity;

				if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, col)) myMat.emissiveFactor = { col.r, col.g, col.b, 1.0f };
				else myMat.emissiveFactor = glm::vec4(1.0f);

				if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, col) == aiReturn_SUCCESS) myMat.diffuseFactor = { col.r, col.g, col.b, 1.0f };
				else myMat.diffuseFactor = glm::vec4(1.0f);

				if (mat->Get(AI_MATKEY_COLOR_SPECULAR, col) == aiReturn_SUCCESS) myMat.specularFactor = { col.r, col.g, col.b, opacity };
				else myMat.specularFactor = glm::vec4(0.0f);

				if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, myMat.roughnessFactor) != aiReturn_SUCCESS) myMat.roughnessFactor = 0.0f;

				if (mat->Get(AI_MATKEY_METALLIC_FACTOR, myMat.metallicFactor) != aiReturn_SUCCESS) myMat.metallicFactor = 0.0f;

				if (mat->Get("$mat.gltf.alphaCutoff", 0, 0, myMat.alphaCutoff) != aiReturn_SUCCESS) myMat.alphaCutoff = 0.5f;

				aiBlendMode mode;
				if (mat->Get(AI_MATKEY_BLEND_FUNC, mode) == aiReturn_SUCCESS)
				{
					if (mode == aiBlendMode_Default) myMat.mode = Material::ALPHA_MODE_BLEND;
				}
				else myMat.mode = Material::ALPHA_MODE::ALPHA_MODE_OPAQUE;

				myMat.alphaMask = 1.0f;
				// TODO: WORKFLOW SETTING IS NOT SET YET


				CreateMaterialUniform(&myMat);
			}
		}
		importer.FreeScene();
	}


	// INITIALIZE THE DEFAULT POSE OF ALL JOINTS
	{
		for (uint32_t i = 0; i < model->numJoints; i++)
		{
			Joint& j = model->joints[i];
			if (j.mesh)
			{
				if (j.skin && j.skinIndex > -1)
				{
					j.defMatrix = glm::translate(glm::mat4(1.0f), j.translation) * glm::mat4(j.rotation) * glm::scale(glm::mat4(1.0f), j.scale) * j.matrix;
					glm::mat4 inverse = glm::inverse(j.defMatrix);
					for (uint32_t idx = 0; idx < j.skin->numJoints; idx++)
					{
						Joint* childJoint = j.skin->joints[idx];
						glm::mat4 m = glm::translate(glm::mat4(1.0f), childJoint->translation) * glm::mat4(childJoint->rotation) * glm::scale(glm::mat4(1.0f), childJoint->scale) * childJoint->matrix;
						Joint* parent = childJoint->parent;
						while (parent)
						{
							m = glm::translate(glm::mat4(1.0f), parent->translation) * glm::mat4(parent->rotation) * glm::scale(glm::mat4(1.0f), parent->scale) * parent->matrix * m;
							parent = parent->parent;
						}

						glm::mat4 jointMat = inverse * m * j.skin->inverseBindMatrices[idx];
						childJoint->defMatrix = jointMat;
					}
				}
				else
				{
					j.defMatrix = glm::translate(glm::mat4(1.0f), j.translation) * glm::mat4(j.rotation) * glm::scale(glm::mat4(1.0f), j.scale) * j.matrix;
				}
			}
		}
	}


	// LOAD NODES
	{
		model->numNodes = 0;
		for (uint32_t i = 0; i < model->numJoints; i++)
		{
			if (model->joints[i].mesh) // JOINT IS NODE
			{
				model->numNodes++;
			}
		}
		uint32_t nodeIdx = 0;
		model->nodes = new Joint*[model->numNodes];
		for (uint32_t i = 0; i < model->numJoints; i++)
		{
			if (model->joints[i].mesh) // JOINT IS NODE
			{
				model->nodes[nodeIdx] = &model->joints[i];
				nodeIdx++;
			}
		}
	}

	m->models.push_back(model);
	return model;
}
struct Texture* AM_AddTexture(AssetManager* m, const char* file)
{
	if (m->textures.find(file) != m->textures.end())  return m->textures[file];
	int x = 0, y = 0, comp = 0;
	stbi_uc* c = stbi_load(file, &x, &y, &comp, 4);
	if(c)
	{
		Texture* resTex = new Texture;
		resTex->width = x;
		resTex->height = y;
		resTex->type = GL_TEXTURE_2D;
		glGenTextures(1, &resTex->uniform);
		glBindTexture(resTex->type, resTex->uniform);
		glTexImage2D(resTex->type, 0, GL_RGBA, resTex->width, resTex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, c);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glGenerateMipmap(GL_TEXTURE_2D);
		
		m->textures[file] = resTex;
		stbi_image_free(c);
		return resTex;
	}
	return nullptr;
}
struct Texture* AM_AddCubemapTexture(AssetManager* m, const char* name, const char* top, const char* bottom, const char* left, const char* right, const char* front, const char* back)
{
	if (m->textures.find(name) != m->textures.end()) return m->textures[name];

	const char* files[6] = { right, left, top, bottom, front, back };

	Texture* resTex = nullptr;
	for (int i = 0; i < 6; i++)
	{
		int x = 0, y = 0, comp = 0;
		stbi_uc* c = stbi_load(files[i], &x, &y, &comp, 4);
		if (c)
		{
			if (!resTex)
			{
				resTex = new Texture;
				resTex->width = x;
				resTex->height = y;
				resTex->type = GL_TEXTURE_CUBE_MAP;
				glGenTextures(1, &resTex->uniform);
				glBindTexture(resTex->type, resTex->uniform);
			}
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, resTex->width, resTex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, c);
			stbi_image_free(c);
		}
		else
		{
			LOG("ERROR FAILED TO LOAD FILE: %s\n", files[i]);
		}
	}
	if (resTex)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		m->textures[name] = resTex;
	}
}



using namespace rectpack2D;
constexpr bool allow_flip = false;
const auto runtime_flipping_mode = rectpack2D::flipping_option::DISABLED;
using spaces_type = rectpack2D::empty_spaces<allow_flip, rectpack2D::default_empty_spaces>;
using rect_type = output_rect_t<spaces_type>;
struct AtlasBuildData
{
	static constexpr size_t max = 0x8000;
	uint32_t* copyData = nullptr;
	uint32_t* data = nullptr;
	uint32_t copyWidth = 0;
	uint32_t copyHeight = 0;
	uint32_t curWidth = 0;
	uint32_t curHeight = 0;
	std::vector<rect_type> rects;
	void CopyRectToRect(const rect_type& cpyRec, uint32_t dstStartX, uint32_t dstStartY)
	{
		for (uint32_t y = 0; y < cpyRec.h; y++)
		{
			for (uint32_t x = 0; x < cpyRec.w; x++)
			{
				data[x + dstStartX + (y + dstStartY) * curWidth] = copyData[x + cpyRec.x + (y + cpyRec.y) * copyWidth];
			}
		}
	}
};
struct AtlasBuildData* AM_BeginAtlasTexture()
{
	AtlasBuildData* data = new AtlasBuildData;
	return data;
}



bool AM_AtlasAddFromFile(struct AtlasBuildData* data, const char* file)
{
	int x, y, comp;
	stbi_uc* fileData = stbi_load(file, &x, &y, &comp, 4);
	bool res = false;
	if (fileData)
	{
		res = AM_AtlasAddRawData(data, (uint32_t*)fileData, x, y);
		stbi_image_free(fileData);
	}
	return res;
}
FontMetrics* AM_AtlasAddGlyphRangeFromFile(struct AtlasBuildData* data, const char* fontFile, uint32_t first, uint32_t last, float fontSize)
{
	uint8_t* fontData = nullptr;
	{
		FILE* fileHandle = NULL;
		errno_t err = fopen_s(&fileHandle, fontFile, "rb");
		if (err != 0) {
			return nullptr;
		}
		fseek(fileHandle, 0, SEEK_END);
		int size = ftell(fileHandle);
		fseek(fileHandle, 0, SEEK_SET);

		fontData = new uint8_t[size];
		fread(fontData, size, 1, fileHandle);
		fclose(fileHandle);
	}

	stbtt_fontinfo info;
	if (!stbtt_InitFont(&info, fontData, 0))
	{
		delete[] fontData;
		return nullptr;
	}
	const float scale = stbtt_ScaleForPixelHeight(&info, fontSize);

	uint32_t tempWidthHeight = (int)(fontSize + 100);
	uint32_t* tempColor = new uint32_t[tempWidthHeight * tempWidthHeight];
	uint8_t* tempBitmap = new uint8_t[tempWidthHeight * tempWidthHeight];

	FontMetrics* metrics = new FontMetrics;
	memset(metrics, 0, sizeof(FontMetrics));
	metrics->size = fontSize;
	metrics->atlasIdx = data->rects.size();

	int ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

	metrics->ascent = roundf(ascent * scale);
	metrics->descent = roundf(descent * scale);
	metrics->lineGap = roundf(lineGap * scale);
	metrics->firstCharacter = first;
	metrics->numGlyphs = last - first;
	metrics->glyphs = new Glyph[metrics->numGlyphs];
	memset(metrics->glyphs, 0, sizeof(Glyph) * metrics->numGlyphs);

	for (uint32_t i = first; i < last; i++)
	{
		Glyph& g = metrics->glyphs[i - first];
		int advance = 0, leftSideBearing = 0;
		stbtt_GetGlyphHMetrics(&info, i, &advance, &leftSideBearing);
		g.advance = advance * scale;
		g.leftSideBearing = leftSideBearing * scale;

		memset(tempBitmap, 0, sizeof(uint8_t) * tempWidthHeight * tempWidthHeight);

		int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
		stbtt_GetCodepointBitmapBox(&info, i, scale, scale, &x1, &y1, &x2, &y2);

		g.width = (float)(x2 - x1);
		g.height = (float)(y2 - y1);
		g.leftSideBearing = x1;
		g.yStart = (float)y1;

		stbtt_MakeCodepointBitmap(&info, tempBitmap, g.width, g.height, tempWidthHeight, scale, scale, i);

		for(uint32_t j = 0; j < tempWidthHeight * tempWidthHeight; j++)
		{
			tempColor[j] = 0xFFFFFF | (tempBitmap[j] << 24);
		}

		if (!AM_AtlasAddSubRawData(data, tempColor, 0, 0, g.width, g.height, tempWidthHeight))
		{
			delete[] fontData;
			delete[] metrics->glyphs;
			delete[] tempBitmap;
			delete[] tempColor;
			delete metrics;
			return nullptr;
		}

	}
	
	delete[] fontData;
	delete[] tempBitmap;
	delete[] tempColor;


	return metrics;
}
bool AM_AtlasAddRawData(struct AtlasBuildData* data, uint32_t* rawData, uint32_t width, uint32_t height)
{
	return AM_AtlasAddSubRawData(data, rawData, 0, 0, width, height, width);
}
bool AM_AtlasAddSubRawData(struct AtlasBuildData* data, uint32_t* rawData, uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY, uint32_t rawWidth)
{
	const uint32_t width = endX - startX;
	const uint32_t height = endY - startY;
	std::vector<rect_type> old = data->rects;
	data->rects.emplace_back(0, 0, width, height);
	const int discard_step = -4;
	bool failed = false;
	auto report_successful = [&](rect_type&) { failed = false;  return callback_result::CONTINUE_PACKING; };
	auto report_unsuccessful = [&](rect_type&) { failed = true; return callback_result::ABORT_PACKING; };


	const rect_wh result_size = find_best_packing<spaces_type>(data->rects, make_finder_input(AtlasBuildData::max, discard_step, report_successful, report_unsuccessful, runtime_flipping_mode));
	if (!failed)
	{
		bool sizeChanged = false;
		if (!data->data)
		{
			data->curWidth = result_size.w + 100;
			data->curHeight = result_size.h + 100;
			data->copyWidth = data->curWidth;
			data->copyHeight = data->copyHeight;
			const uint32_t dataSize = data->curWidth * data->curHeight;

			data->data = new uint32_t[dataSize];
			data->copyData = new uint32_t[dataSize];
			memset(data->data, 0, sizeof(uint32_t) * dataSize);
			memset(data->copyData, 0, sizeof(uint32_t) * dataSize);
		}
		memcpy(data->copyData, data->data, sizeof(uint32_t) * data->curWidth * data->curHeight);
		if (data->curWidth <= result_size.w || data->curHeight <= result_size.h)
		{
			sizeChanged = true;
			delete[] data->data;
			data->curWidth = result_size.w + 100;
			data->curHeight = result_size.h + 100;
			data->data = new uint32_t[data->curWidth * data->curHeight];
		}
		memset(data->data, 0, sizeof(uint32_t) * data->curWidth * data->curHeight);
		for (uint32_t i = 0; i < old.size(); i++)
		{
			data->CopyRectToRect(old.at(i), data->rects.at(i).x, data->rects.at(i).y);
		}

		const rect_type& last = data->rects.at(data->rects.size() - 1);
		for (uint32_t y = 0; y < height; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				data->data[last.x + x + (y + last.y) * data->curWidth] = rawData[startX + x + (startY + y) * rawWidth];
			}
		}

		if (sizeChanged)
		{
			delete[] data->copyData;
			data->copyWidth = data->curWidth;
			data->copyHeight = data->curHeight;
			data->copyData = new uint32_t[data->curWidth * data->curHeight];
			memset(data->copyData, 0, sizeof(uint32_t) * data->curWidth * data->curHeight);
		}
		return true;
	}
	else
	{
		data->rects = old;
		return false;
	}
}

void AM_StoreTextureAtlas(struct AtlasBuildData* data)
{

}
AtlasTexture* AM_EndTextureAtlas(struct AtlasBuildData* data)
{
	AtlasTexture* atlas = new AtlasTexture;
	memset(atlas, 0, sizeof(AtlasTexture));

	atlas->texture.width = data->curWidth;
	atlas->texture.height = data->curHeight;
	atlas->texture.type = GL_TEXTURE_2D;
	glGenTextures(1, &atlas->texture.uniform);
	glBindTexture(GL_TEXTURE_2D, atlas->texture.uniform);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data->curWidth, data->curHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data->data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenerateMipmap(GL_TEXTURE_2D);

	atlas->numBounds = data->rects.size();
	atlas->bounds = new AtlasTexture::UVBound[atlas->numBounds];
	memset(atlas->bounds, 0, sizeof(AtlasTexture::UVBound) * atlas->numBounds);
	for (uint32_t i = 0; i < atlas->numBounds; i++)
	{
		rect_type& r = data->rects.at(i);
		atlas->bounds[i].start = { (float)r.x / (float)atlas->texture.width, (float)r.y / (float)atlas->texture.height };
		atlas->bounds[i].end = { (float)(r.x + r.w) / (float)atlas->texture.width, (float)(r.y + r.h) / (float)atlas->texture.height };
	}
	delete data;

	return atlas;
}









void AM_DeleteModel(struct Model* model)
{
	if (model->animations) delete[] model->animations;
	if (model->indexBuffer) glDeleteBuffers(1, &model->indexBuffer);
	if (model->joints)
	{
		for (uint32_t i = 0; i < model->numJoints; i++)
		{
			if (model->joints[i].children)
			{
				delete[] model->joints[i].children;
				model->joints[i].children = nullptr;
			}
		}
		delete[] model->joints;
	}
	if (model->nodes) delete[] model->nodes;
	if (model->materials) delete[] model->materials;
	if (model->meshes)
	{
		for (uint32_t i = 0; i < model->numMeshes; i++)
		{
			if(model->meshes[i].primitives) delete[] model->meshes[i].primitives;
			model->meshes[i].primitives = nullptr;
		}
		delete[] model->meshes;
	}
	if (model->skins)
	{
		for (uint32_t i = 0; i < model->numSkins; i++)
		{
			if(model->skins[i].joints) delete[] model->skins[i].joints;
			model->skins[i].joints = nullptr;
			if (model->skins[i].inverseBindMatrices) delete[] model->skins[i].inverseBindMatrices;
			model->skins[i].inverseBindMatrices = nullptr;
		}
		delete[] model->skins;
	}

	// MAYBE EACH MODEL SHOULD DELETE THEIR TEXTURES, BUT FOR NOW THE ASSET MANAGER KEEPS THEM
	if (model->textures)
	{
		delete[] model->textures;
	}

	if(model->vao) glDeleteVertexArrays(1, &model->vao);
	if(model->vertexBuffer) glDeleteBuffers(1, &model->vertexBuffer);

	model->vao = 0;
	model->vertexBuffer = 0;
	model->indexBuffer = 0;
	model->textures = nullptr;
	model->meshes = nullptr;
	model->skins = nullptr;
	model->joints = nullptr;
	model->nodes = nullptr;
	model->materials = nullptr;
	model->animations = nullptr;
}


















struct EnvironmentFileHeader
{
	uint8_t numMipMaps;
	uint16_t width;
	uint16_t height;
	uint16_t irrWidth;
	uint16_t irrHeight;
};
void AM_StoreEnvironment(const struct EnvironmentData* env, const char* fileName)
{

	uint32_t dataSize = sizeof(EnvironmentFileHeader);
	for (int i = 0; i < env->mipLevels; i++)
	{
		dataSize += (glm::max(env->width, 1u) * glm::max(env->height, 1u) * 4) * 6;
	}
	dataSize += env->irradianceMapWidth * env->irradianceMapHeight * 6;

	uint8_t* fullData = new uint8_t[dataSize];

	EnvironmentFileHeader* header = (EnvironmentFileHeader*)fullData;
	header->numMipMaps = env->mipLevels;
	header->width = env->width;
	header->height = env->height;
	header->irrWidth = env->irradianceMapWidth;
	header->irrHeight = env->irradianceMapHeight;
	uint8_t* curData = fullData + sizeof(EnvironmentFileHeader);

	for (int i = 0; i < env->mipLevels; i++)
	{
		const uint32_t cw = glm::max(env->width >> i, 1u);
		const uint32_t ch = glm::max(env->height >> i, 1u);

		int testW = 0;
		int testH = 0;
		
		glBindTexture(GL_TEXTURE_CUBE_MAP, env->prefilteredMap);

		glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, i, GL_TEXTURE_WIDTH, &testW);
		glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, i, GL_TEXTURE_HEIGHT, &testH);
		if (testW != cw)
		{
			LOG("(env->prefilteredMap): ERROR LEVEL WIDTH DOES NOT EQUAL REAL WIDTH: %d != %d\n", testW, cw);
		}
		if (testH != ch)
		{
			LOG("(env->prefilteredMap): ERROR LEVEL HEIGHT DOES NOT EQUAL REAL HEIGHT: %d != %d\n", testH, ch);
		}

		for (int j = 0; j < 6; j++)
		{
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, GL_RGBA, GL_UNSIGNED_BYTE, curData);
			curData += 4 * cw * ch;
		}
	}

	int testW = 0;
	int testH = 0;
	glBindTexture(GL_TEXTURE_CUBE_MAP, env->irradianceMap);
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &testW);
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_HEIGHT, &testH);
	if (testW != env->irradianceMapWidth)
	{
		LOG("(env->irradianceMap): ERROR LEVEL WIDTH DOES NOT EQUAL REAL WIDTH: %d != %d\n", testW, env->irradianceMapWidth);
	}
	if (testH != env->irradianceMapHeight)
	{
		LOG("(env->irradianceMap): ERROR LEVEL HEIGHT DOES NOT EQUAL REAL HEIGHT: %d != %d\n", testH, env->irradianceMapHeight);
	}

	for (int j = 0; j < 6; j++)
	{
		glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_RGBA, GL_UNSIGNED_BYTE, curData);
		curData += 4 * env->irradianceMapWidth * env->irradianceMapHeight;
	}



	uLongf dataLen = dataSize;
	uint8_t* compressData = new uint8_t[dataSize];
	if (compress(compressData, &dataLen, fullData, dataSize) != Z_OK)
	{
		delete[] compressData;
		delete[] fullData;
		return;
	}

	FILE* f = NULL;
	fopen_s(&f, fileName, "wb");
	if (!f)
	{
		delete[] compressData;
		delete[] fullData;
		return;
	}
	fwrite(&dataSize, sizeof(uint32_t), 1, f);
	fwrite(compressData, dataLen, 1, f);
	fclose(f);


	delete[] compressData;
	delete[] fullData;
}
bool AM_LoadEnvironment(struct EnvironmentData* env, const char* fileName)
{
	FILE* f = NULL;
	fopen_s(&f, fileName, "rb");
	if (!f)
	{
		return false;
	}
	fseek(f, 0, SEEK_END);
	int sz = ftell(f);
	fseek(f, 0, SEEK_SET);

	uint8_t* fileData = new uint8_t[sz];
	fread(fileData, sz, 1, f);
	fclose(f);

	const uint32_t uncompressedSize = *(uint32_t*)fileData;
	uint8_t* curFileData = fileData + sizeof(uint32_t);

	uint8_t* uncompressedData = new uint8_t[uncompressedSize];

	uLongf outSize = uncompressedSize;
	int res = uncompress(uncompressedData, &outSize, curFileData, sz - sizeof(uint32_t));
	delete[] fileData;
	if (res != Z_OK)
	{
		LOG("ERROR FAILED TO LOAD ENVIRONMENT: %s %d\n", fileName, res);
		delete[] uncompressedData;
		return false;
	}


	EnvironmentFileHeader* header = (EnvironmentFileHeader*)uncompressedData;
	uint8_t* curData = uncompressedData + sizeof(EnvironmentFileHeader);

	env->mipLevels = header->numMipMaps;
	env->width = header->width;
	env->height = header->height;
	env->irradianceMapWidth = header->irrWidth;
	env->irradianceMapHeight = header->irrHeight;

	glGenTextures(1, &env->irradianceMap);
	glGenTextures(1, &env->prefilteredMap);
	
	
	glBindTexture(GL_TEXTURE_CUBE_MAP, env->prefilteredMap);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	for (int i = 0; i < env->mipLevels; i++)
	{
		uint32_t cw = glm::max(header->width >> i, 1);
		uint32_t ch = glm::max(header->height >> i, 1);

		for (unsigned int j = 0; j < 6; j++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, GL_RGBA, cw, ch, 0, GL_RGBA, GL_UNSIGNED_BYTE, curData);
			curData += 4 * cw * ch;
		}
	}



	glBindTexture(GL_TEXTURE_CUBE_MAP, env->irradianceMap);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	for (unsigned int j = 0; j < 6; j++)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_RGBA, header->irrWidth, header->irrHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, curData);
		curData += 4 * header->irrWidth * header->irrHeight;
	}
	
	env->environmentMap = env->prefilteredMap;

	delete[] uncompressedData;
	return true;
}