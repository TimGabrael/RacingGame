#pragma once
#include "Entitys.h"
#include "../Graphics/Renderer.h"

struct GameManager
{
	AntialiasingRenderData AAbuffer;
	PostProcessingRenderData PPbuffer;
	GLuint lightUniform;
	EnvironmentData env;
	Player* localPlayer;
};

GameManager* GM_CreateGameManager(AssetManager* assets);
void GM_CleanUpGameManager(GameManager* manager);

void GM_AddPlayerToScene(GameManager* game, const glm::vec3& pos, float yaw, float pitch);



void GM_OnResizeCallback(GameManager* game, int width, int height);