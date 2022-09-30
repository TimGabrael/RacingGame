#pragma once
#include <unordered_map>
#include <string>
#include "Utility.h"

#define DEFAULT_WHITE_TEXTURE ":DefaultWhite"
#define DEFAULT_BLACK_TEXTURE ":DefaultBlack"

#define DEFUALT_CUBE_MAP ":DefaultCubemap"


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
struct AudioFile* AM_AddAudioSample(AssetManager* m, const char* file);
