#include "GameManager.h"
#include "../GameState.h"
#include "../Graphics/ModelInfo.h"
#include "../Graphics/Scene.h"


GameManager* GM_CreateGameManager(AssetManager* assets)
{
	GameManager* out = new GameManager;
	GameState* state = GetGameState();
	out->env.environmentMap = assets->textures[DEFAULT_CUBE_MAP]->uniform;
	out->env.width = assets->textures[DEFAULT_CUBE_MAP]->width;
	out->env.height = assets->textures[DEFAULT_CUBE_MAP]->height;
	out->env.mipLevels = 1;
#ifdef CREATE_ACTUAL_ENVIRONMENT
	RE_CreateEnvironment(state->renderer, &out->env);
#else
	out->env.prefilteredMap = assets->textures[DEFAULT_CUBE_MAP]->uniform;
	out->env.irradianceMap = assets->textures[DEFAULT_CUBE_MAP]->uniform;
#endif // CREATE_ACTUAL_ENVIRONMENT

	// LightData lightData{};
	// lightData.dirLights[0].direction = { 0.0f, -1.0f, 0.0f, 0.0f };
	// lightData.dirLights[0].color = { 10.0f, 10.0f, 10.0f, 1.0f };
	// lightData.numDirLights = 1;
	// lightData.ambientColor = { 0.0f, 0.0f, 0.0f, 0.0f };

	LightData lightData{};
	lightData.pointLights[0].pos = { 0.0f, 8.0f, 0.0f, 0.0f };
	lightData.pointLights[0].color = { 10.0f, 10.0f, 10.0f, 1.0f };
	lightData.numPointLights = 1;
	lightData.ambientColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	
	glGenBuffers(1, &out->lightUniform);
	glBindBuffer(GL_UNIFORM_BUFFER, out->lightUniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(lightData), &lightData, GL_STATIC_DRAW);




	out->localPlayer = nullptr;

	return out;
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
		obj.boneData = 0;	// default bone
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