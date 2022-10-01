#pragma once
#include "Entitys.h"
#include "../Graphics/Renderer.h"

struct GameManager
{
	GLuint lightUniform;
	EnvironmentData env;
	Player* localPlayer;
};

GameManager* GM_CreateGameManager(AssetManager* assets);

void GM_AddPlayerToScene(GameManager* game, const glm::vec3& pos, float yaw, float pitch);