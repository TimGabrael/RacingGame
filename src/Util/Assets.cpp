#include "Assets.h"
#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "../Graphics/ModelInfo.h"
#include "../Graphics/Renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"
#include "stb_truetype.h"



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

	{
		AM_AddCubemapTexture(out, DEFAULT_CUBE_MAP,
			"Assets/CitySkybox/top.jpg", 
			"Assets/CitySkybox/bottom.jpg", 
			"Assets/CitySkybox/left.jpg", 
			"Assets/CitySkybox/right.jpg", 
			"Assets/CitySkybox/front.jpg", 
			"Assets/CitySkybox/back.jpg");
	}


	return out;
}
void AM_CleanUpAssetManager(AssetManager* assets)
{

}

struct Model* AM_AddModel(AssetManager* m, const char* file)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(file, aiProcess_GenNormals | aiProcess_Triangulate);

	std::string filePath = file;
	{
		size_t idx = filePath.find_last_of("/\\");
		if (idx != -1)
		{
			filePath = filePath.substr(0, idx + 1);
		}
	}

	Model* model = new Model;
	memset(model, 0, sizeof(Model));
	
	model->meshes = new Mesh[scene->mNumMeshes];
	memset(model->meshes, 0, sizeof(Mesh) * scene->mNumMeshes);
	model->textures = new Texture*[scene->mNumTextures];
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
		myMesh->startIdx = numInds;
		myMesh->numInd = localNumInds;

		myMesh->material = &model->materials[mesh->mMaterialIndex];

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
				resTex->width = texture->mWidth;
				resTex->height = texture->mHeight;
				resTex->type = GL_TEXTURE_2D;
				glGenTextures(1, &resTex->uniform);
				glBindTexture(resTex->type, resTex->uniform);
				glTexImage2D(resTex->type, 0, GL_RGBA, resTex->width, resTex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->pcData);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

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
			mat->Get(AI_MATKEY_COLOR_AMBIENT, col);
			float opacity = 0.0f;
			mat->Get(AI_MATKEY_OPACITY, opacity);
			myMat.baseColorFactor = { col.r, col.g, col.b, opacity };

			mat->Get(AI_MATKEY_COLOR_EMISSIVE, col);
			myMat.baseColorFactor = { col.r, col.g, col.b, opacity };

			mat->Get(AI_MATKEY_COLOR_DIFFUSE, col);
			myMat.diffuseFactor = { col.r, col.g, col.b, opacity };

			mat->Get(AI_MATKEY_COLOR_SPECULAR, col);
			myMat.specularFactor = { col.r, col.g, col.b, opacity };

			mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, myMat.roughnessFactor);

			mat->Get(AI_MATKEY_METALLIC_FACTOR, myMat.metallicFactor);

			mat->Get("$mat.gltf.alphaCutoff", 0, 0, myMat.alphaCutoff);

			if (myMat.alphaCutoff < 1.0f)
			{
				myMat.alphaMask = 1.0f;
			}
			// TODO: WORKFLOW SETTING IS NOT SET YET

			CreateMaterialUniform(&myMat);
		}
	}
	importer.FreeScene();
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
struct AudioFile* AM_AddAudioSample(AssetManager* m, const char* file)
{
	return nullptr;
}