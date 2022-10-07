#include "GameManager.h"
#include "../GameState.h"
#include "../Graphics/ModelInfo.h"
#include "../Graphics/Scene.h"

GameManager* GM_CreateGameManager(struct Renderer* renderer, AssetManager* assets)
{
	GameManager* out = new GameManager;
	GameState* state = GetGameState();
	out->env.environmentMap = assets->textures[DEFAULT_CUBE_MAP]->uniform;
	out->env.width = assets->textures[DEFAULT_CUBE_MAP]->width;
	out->env.height = assets->textures[DEFAULT_CUBE_MAP]->height;
	out->env.mipLevels = 1;

	if (!AM_LoadEnvironment(&out->env, "Assets/Environment.menv"))
	{
		RE_CreateEnvironment(state->renderer, &out->env);
		AM_StoreEnvironment(&out->env, "Assets/Environment.menv");
	}

	out->defaultLightGroup = RELI_AddLightGroup(renderer);

	DirectionalLight* l = RELI_AddDirectionalLight(out->defaultLightGroup);
	l->direction = { 0.0f, -1.0f, 0.0f };
	l->color = { 1.0f, 1.0f, 1.0f };

	DirShadowLight* sl = RELI_AddDirectionalShadowLight(out->defaultLightGroup, 0x1000, 0x1000, false);
	sl->pos = { 0.0f, 30.0f, 0.0f };
	sl->light.direction = { 0.0f, -1.0f, 0.0f };
	sl->light.color = { 2.0f, 2.0f, 2.0f };
	//
	//SpotLight* spot = RELI_AddSpotLight(out->defaultLightGroup);
	//spot->color = { 2.0f, 2.0f, 2.0f, 1.0f };
	//spot->direction = { 0.0f, -1.0f, 0.0f };
	//spot->cutOff = M_PI_4;
	//spot->pos = { 0.0f, 25.0f, 0.0f };
	//
	//SpotShadowLight* spot = RELI_AddSpotShadowLight(out->defaultLightGroup, 2048, 2048);
	//spot->light.color = { 2.0f, 2.0f, 2.0f, 1.0f };
	//spot->light.direction = { 0.0f, -1.0f, 0.0f };
	//spot->light.cutOff = M_PI_4;
	//spot->light.pos = { 0.0f, 30.0f, 0.0f };
	//
	//PointLight* point = RELI_AddPointLight(out->defaultLightGroup);
	//point->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	//point->pos = { 0.0f, 30.0f, 0.0f, 0.0f };

	//PointShadowLight* point = RELI_AddPointShadowLight(out->defaultLightGroup, 2048, 2048);
	//point->light.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	//point->light.pos = { 0.0f, 30.0f, 0.0f, 0.0f };

	//point = RELI_AddPointShadowLight(out->defaultLightGroup, 2048, 2048);
	//point->light.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	//point->light.pos = { 0.0f, 20.0f, 10.0f, 0.0f };

	//point = RELI_AddPointShadowLight(out->defaultLightGroup, 2048, 2048);
	//point->light.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	//point->light.pos = { 0.0f, 20.0f, -10.0f, 0.0f };

	RELI_Update(out->defaultLightGroup, nullptr);


	RE_CreateAntialiasingData(&out->AAbuffer, state->winWidth, state->winHeight, 0);
	RE_CreatePostProcessingRenderData(&out->PPbuffer, state->winWidth, state->winHeight);
	
	RE_CreateScreenSpaceReflectionRenderData(&out->SSRbuffer, state->winWidth, state->winHeight);

	out->localPlayer = nullptr;

	return out;
}
void GM_CleanUpGameManager(GameManager* manager)
{
	RE_CleanUpAntialiasingData(&manager->AAbuffer);
	RE_CleanUpPostProcessingRenderData(&manager->PPbuffer);
}

void GM_AddPlayerToScene(GameManager* game, const glm::vec3& pos, float yaw, float pitch)
{
	GameState* state = GetGameState();
	if (game->localPlayer)
	{

		CA_InitPerspectiveCamera(&game->localPlayer->camera, pos, yaw, pitch, state->winWidth, state->winHeight);
	}
	else
	{
		SceneObject obj;
		obj.flags = 0;	// for now
		obj.material = nullptr;
		obj.model = nullptr;
		obj.rigidBody = nullptr;
		Player* player = new Player;
		obj.entity = player;
		obj.transform = glm::mat4(1.0f);

		CA_InitPerspectiveCamera(&player->camera, pos, yaw, pitch, state->winWidth, state->winHeight);


		player->sceneObject = SC_AddDynamicObject(state->scene, &obj);
		game->localPlayer = player;
	}
}


void GM_Update(GameManager* game, float dt)
{
	RELI_Update(game->defaultLightGroup, &game->localPlayer->camera.base);



}
void GM_OnResizeCallback(GameManager* game, int width, int height)
{
	uint32_t prevSamples = game->AAbuffer.msaaCount;
	RE_CleanUpAntialiasingData(&game->AAbuffer);
	RE_CleanUpPostProcessingRenderData(&game->PPbuffer);
	RE_CleanUpScreenSpaceReflectionRenderData(&game->SSRbuffer);

	RE_CreateAntialiasingData(&game->AAbuffer, width, height, prevSamples);
	RE_CreatePostProcessingRenderData(&game->PPbuffer, width, height);
}