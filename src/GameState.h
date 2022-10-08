#pragma once

// FOR CONTROLLER INPUT
#include <Windows.h>
#include <Xinput.h>


#include <stdint.h>
#include "Util/Assets.h"
#include "Util/Math.h"
#include "Game/Entitys.h"
#include "Game/GameManager.h"


typedef void (*UPDATE_CALLBACK)(struct GameState* state, float dt);
typedef void (*GENERAL_CALLBACK)(struct GameState* state);

struct MainCallbacks
{
	UPDATE_CALLBACK presceneUpdate;
	UPDATE_CALLBACK prephysicsUpdate;
	UPDATE_CALLBACK postphysicsUpdate;
	GENERAL_CALLBACK renderCallback;
};

struct GameState
{
	struct Renderer* renderer;
	struct Scene* scene;
	struct PhysicsScene* physics;
	AssetManager* assets;
	GameManager* manager;
	struct GLFWwindow* window;
	uint32_t winX;
	uint32_t winY;
	uint32_t winWidth;
	uint32_t winHeight;
	uint32_t swapChainInterval;
	MainCallbacks callbacks;
	bool hasFocus;
	bool isFullscreen;
	bool isMouseCaptured;
};

GameState* CreateGameState(const char* windowName, MainCallbacks* cbs, uint32_t windowWidth, uint32_t windowHeight);
GameState* GetGameState();
void SetFullscreen(GameState* state, int monitorIdx, int* width, int* height);

void UpateGameState();