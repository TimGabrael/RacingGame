#pragma once
#include "Entitys.h"
#include "../Graphics/Renderer.h"

struct GameManager
{
	AntialiasingRenderData AAbuffer;
	PostProcessingRenderData PPbuffer;
	ScreenSpaceReflectionRenderData SSRbuffer;
	struct LightGroup* defaultLightGroup;
	EnvironmentData env;
	Player* localPlayer;
};

GameManager* GM_CreateGameManager(struct GameState* state);
void GM_CleanUpGameManager(GameManager* manager);

void GM_AddPlayerToScene(GameManager* game, const glm::vec3& pos, float yaw, float pitch);


void GM_Update(GameManager* game, float dt);
void GM_OnResizeCallback(GameManager* game, int width, int height);