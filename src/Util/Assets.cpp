#include "Assets.h"
#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "../Graphics/ModelInfo.h"
#include "../Graphics/Renderer.h"

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
		Texture* defaultCubemap = new Texture;
		defaultCubemap->width = 1;
		defaultCubemap->height = 1;
		defaultCubemap->type = GL_TEXTURE_CUBE_MAP;
		glGenTextures(1, &defaultCubemap->uniform);
		glBindTexture(defaultCubemap->type, defaultCubemap->uniform);

		uint32_t data[2] = { 0xFF203040, 0xFF203040 };
		for (uint32_t i = 0; i < 6; i++)
		{

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, defaultCubemap->width, defaultCubemap->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		out->textures[DEFUALT_CUBE_MAP] = defaultCubemap;
	}


	return out;
}
void AM_CleanUpAssetManager(AssetManager* assets)
{

}

struct Model* AM_AddModel(AssetManager* m, const char* file)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(file, aiProcess_GenNormals);

	for (int i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[i];
		Vertex3D* vert = new Vertex3D[mesh->mNumVertices];
		for (int j = 0; j < mesh->mNumVertices; j++)
		{
			vert[j].pos = {mesh->mVertices[j].x,mesh->mVertices[j].y,mesh->mVertices[j].z};
			if (mesh->HasNormals())
			{
				vert[j].nor = { mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z };
			}
			if (mesh->HasTextureCoords(0))
			{
				vert[j].uv1 = { mesh->mTextureCoords[0][j].x,mesh->mTextureCoords[0][j].y };
			}
			if (mesh->HasTextureCoords(1))
			{
				vert[j].uv2 = { mesh->mTextureCoords[1][j].x,mesh->mTextureCoords[1][j].y };
			}
			if (mesh->HasVertexColors(0))
			{
				vert[j].col = RGBA((uint32_t)(mesh->mColors[0][j].r * 255), (uint32_t)(mesh->mColors[0][j].g * 255), (uint32_t)(mesh->mColors[0][j].b * 255), (uint32_t)(mesh->mColors[0][j].a * 255));
			}
		}

		uint32_t numIndices = 0;
		for (int j = 0; j < mesh->mNumFaces; j++)
		{
			numIndices += mesh->mFaces[j].mNumIndices;
		}
		uint32_t curIdx = 0;
		uint32_t* indices = new uint32_t[numIndices];
		for (int j = 0; j < mesh->mNumFaces; j++)
		{
			for (int k = 0; k < mesh->mFaces[j].mNumIndices; k++)
			{
				indices[curIdx] = mesh->mFaces[j].mIndices[k];
				curIdx++;
			}
		}
		
		Model* model = new Model;
		
		GenerateModelVertexBuffer(&model->vao, &model->vertexBuffer, vert, mesh->mNumVertices);
		glGenBuffers(1, &model->indexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->indexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * numIndices, indices, GL_STATIC_DRAW);

		model->bound.min = { mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z };
		model->bound.max = { mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z };
		model->numVertices = mesh->mNumVertices;
		model->numIndices = numIndices;
		memcpy(&model->baseTransform, &scene->mRootNode->mTransformation, sizeof(glm::mat4));
		
		m->models[mesh->mName.C_Str()] = model;


		delete[] vert;
		delete[] indices;
	}

	for (int i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* mat = scene->mMaterials[i];
		aiString textureFile;

		mat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), textureFile);
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

			std::string fullstr = file + std::string(texture->mFilename.C_Str());
			m->textures[fullstr] = resTex;
		}
		else
		{

		}

	}

	
	importer.FreeScene();
	return nullptr;
}
struct Texture* AM_AddTexture(AssetManager* m, const char* file)
{
	return nullptr;
}
struct Animation* AM_AddAnimation(AssetManager* m, const char* file)
{
	return nullptr;
}
struct AudioFile* AM_AddAudioSample(AssetManager* m, const char* file)
{
	return nullptr;
}