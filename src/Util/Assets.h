#pragma once
#include <unordered_map>
#include <string>
#include "Utility.h"
#include "../Graphics/ModelInfo.h"

#define DEFAULT_WHITE_TEXTURE ":DefaultWhite"
#define DEFAULT_BLACK_TEXTURE ":DefaultBlack"

#define DEFAULT_CUBE_MAP ":DefaultCubemap"

enum MODEL_LOADING_FLAGS
{
	MODEL_LOAD_NONE = 0,
	MODEL_LOAD_CONVEX = 1,
	MODEL_LOAD_CONCAVE = 2,
	MODEL_LOAD_REVERSE_FACE_ORIENTATION = 4,
	MODEL_SET_ORIGIN_CENTER = 8,
};


struct AssetManager
{
	std::vector<struct Model*> models;
	std::unordered_map<std::string, struct Texture*> textures;
};


struct AtlasTexture
{
	Texture texture;
	struct UVBound
	{
		glm::vec2 start;
		glm::vec2 end;
	};
	UVBound* bounds;
	uint32_t numBounds;
};
struct Glyph
{
	float leftSideBearing;
	float yStart;
	float advance;
	float width;
	float height;
};
struct FontMetrics
{
	Glyph* glyphs;
	uint32_t numGlyphs;
	uint32_t firstCharacter;
	uint32_t atlasIdx;
	float ascent;
	float descent;
	float lineGap;
	float size;
};



AssetManager* AM_CreateAssetManager();
void AM_CleanUpAssetManager(AssetManager* assets);




struct Model* AM_AddModel(AssetManager* m, const char* file, uint32_t flags);
struct Texture* AM_AddTexture(AssetManager* m, const char* file);
struct Texture* AM_AddCubemapTexture(AssetManager* m, const char* name, const char* top, const char* bottom, const char* left, const char* right, const char* front, const char* back);



struct AtlasBuildData* AM_BeginAtlasTexture();

bool AM_AtlasAddFromFile(struct AtlasBuildData* data, const char* file);
FontMetrics* AM_AtlasAddGlyphRangeFromFile(struct AtlasBuildData* data, const char* fontFile, uint32_t first, uint32_t last, float fontSize);
bool AM_AtlasAddRawData(struct AtlasBuildData* data, uint32_t* rawData, uint32_t width, uint32_t height);
bool AM_AtlasAddSubRawData(struct AtlasBuildData* data, uint32_t* rawData, uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY, uint32_t rawWidth);

void AM_StoreTextureAtlas(struct AtlasBuildData* data);
AtlasTexture* AM_EndTextureAtlas(struct AtlasBuildData* data);



void AM_DeleteModel(struct Model* model);


void AM_StoreEnvironment(const struct EnvironmentData* env, const char* fileName);
bool AM_LoadEnvironment(struct EnvironmentData* env, const char* fileName);