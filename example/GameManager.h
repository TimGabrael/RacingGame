#pragma once
#include "Entitys.h"
#include "Graphics/Renderer.h"
#include "GameState.h"

struct GameManager : public BaseGameManager
{
	virtual void RenderCallback(GameState* state) override;
	virtual void Update(float dt) override;


	virtual void OnWindowPositionChanged(int x, int y) override;
	virtual void OnWindowResize(int w, int h) override;
	virtual void OnKey(int key, int scancode, int action, int mods) override;
	virtual void OnMouseButton(int button, int action, int mods) override;
	virtual void OnMousePositionChanged(float x, float y, float dx, float dy) override;

	AntialiasingRenderData AAbuffer;
	PostProcessingRenderData PPbuffer;
	ScreenSpaceReflectionRenderData SSRbuffer;
	struct LightGroup* defaultLightGroup;
	EnvironmentData env;
	Player* localPlayer;

	Model* sponzaModel;
	Model* foxModel;
	AnimationInstanceData foxAnimInstance;
	Model debugModel;
};

GameManager* GM_CreateGameManager(struct GameState* state);
void GM_CleanUpGameManager(GameManager* manager);

void GM_AddPlayerToScene(GameManager* game, const glm::vec3& pos, float yaw, float pitch);
