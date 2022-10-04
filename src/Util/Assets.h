#pragma once
#include <unordered_map>
#include <string>
#include "Utility.h"

#define DEFAULT_WHITE_TEXTURE ":DefaultWhite"
#define DEFAULT_BLACK_TEXTURE ":DefaultBlack"

#define DEFAULT_CUBE_MAP ":DefaultCubemap"


struct AssetManager
{
	std::vector<struct Model*> models;
	std::unordered_map<std::string, struct Texture*> textures;
	std::vector<struct AudioFile*> audioSamples;
};

AssetManager* AM_CreateAssetManager();
void AM_CleanUpAssetManager(AssetManager* assets);


struct Model* AM_AddModel(AssetManager* m, const char* file);
struct Texture* AM_AddTexture(AssetManager* m, const char* file);
struct Texture* AM_AddCubemapTexture(AssetManager* m, const char* name, const char* top, const char* bottom, const char* left, const char* right, const char* front, const char* back);
struct AudioFile* AM_AddAudioSample(AssetManager* m, const char* file);



void AM_StoreEnvironment(const struct EnvironmentData* env, const char* fileName);
bool AM_LoadEnvironment(struct EnvironmentData* env, const char* fileName);