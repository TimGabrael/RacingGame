#pragma once
#include <stdint.h>
#include "Util/Assets.h"
#include "Util/Math.h"
#include "Game/Entitys.h"

struct GameState
{
	struct Renderer* renderer;
	struct Scene* scene;
	struct PhysicsScene* physics;
	AssetManager* assets;
	struct Player* localPlayer;
	struct GLFWwindow* window;
	uint32_t winX;
	uint32_t winY;
	uint32_t winWidth;
	uint32_t winHeight;
	uint32_t swapChainInterval;
	bool hasFocus;
	bool isFullscreen;
	bool isMouseCaptured;
};

GameState* CreateGameState(struct GLFWwindow* window, uint32_t windowWidth, uint32_t windowHeight);
GameState* GetGameState();

void AddPlayerToScene(GameState* game, const glm::vec3& pos, float yaw, float pitch);