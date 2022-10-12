#include "GameManager.h"
#include "GameState.h"
#include "Graphics/ModelInfo.h"
#include "Graphics/Scene.h"
#include "Physics/Physics.h"

GameManager* GM_CreateGameManager(GameState* state)
{
	GameManager* out = new GameManager;

	if (!AM_LoadEnvironment(&out->env, "Assets/Environment.menv"))
	{
		Texture* envMap = AM_AddCubemapTexture(state->assets, DEFAULT_CUBE_MAP,
			"Assets/CitySkybox/top.jpg",
			"Assets/CitySkybox/bottom.jpg",
			"Assets/CitySkybox/left.jpg",
			"Assets/CitySkybox/right.jpg",
			"Assets/CitySkybox/front.jpg",
			"Assets/CitySkybox/back.jpg");
		out->env.environmentMap = envMap->uniform;
		out->env.width = envMap->width;
		out->env.height = envMap->height;
		out->env.mipLevels = 1;
		RE_CreateEnvironment(state->renderer, &out->env, 128, 128);
		AM_StoreEnvironment(&out->env, "Assets/Environment.menv");
	}

	out->defaultLightGroup = RELI_AddLightGroup(state->renderer);

	//DirectionalLight* l = RELI_AddDirectionalLight(out->defaultLightGroup);
	//l->direction = { 0.0f, -1.0f, 0.0f };
	//l->color = { 2.0f, 2.0f, 0.0f };

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
	
	//PointLight* point = RELI_AddPointLight(out->defaultLightGroup);
	//point->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	//point->pos = { 0.0f, 30.0f, 0.0f, 0.0f };
	
	//PointShadowLight* point = RELI_AddPointShadowLight(out->defaultLightGroup, 2048, 2048);
	//point->light.color = { 3.0f, 6.0f, 3.0f, 1.0f };
	//point->light.pos = { 0.0f, 30.0f, 0.0f, 0.0f };

	//point = RELI_AddPointShadowLight(out->defaultLightGroup, 2048, 2048);
	//point->light.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	//point->light.pos = { 0.0f, 20.0f, 10.0f, 0.0f };

	//point = RELI_AddPointShadowLight(out->defaultLightGroup, 2048, 2048);
	//point->light.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	//point->light.pos = { 0.0f, 20.0f, -10.0f, 0.0f };

	//RELI_SetAmbientLightColor(out->defaultLightGroup, glm::vec3(0.2f));

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
		game->localPlayer->controller.pitch = pitch;
		game->localPlayer->controller.yaw = yaw;
		game->localPlayer->controller.forwardDir = CA_YawPitchToFoward(yaw, pitch);
		CA_InitPerspectiveCamera(&game->localPlayer->camera, pos, 90.0f, state->winWidth, state->winHeight);
	}
	else
	{
		SceneObject obj;
		obj.flags = 0;	// for now
		obj.model = nullptr;
		obj.rigidBody = nullptr;
		Player* player = new Player;
		obj.entity = player;
		obj.transform = glm::mat4(1.0f);

		CA_InitPerspectiveCamera(&player->camera, pos, 90.0f, state->winWidth, state->winHeight);

		PhysicsMaterial* mat = PH_AddMaterial(state->physics, 1.0f, 1.0f, 1.0f);

		player->sceneObject = SC_AddDynamicObject(state->scene, &obj);
#ifndef FREE_CAM
		player->controller.controller = PH_AddCapsuleController(state->physics, mat, pos, 10.0f, 2.0f);
		player->sceneObject->rigidBody = player->controller.controller->GetRigidBody();
#else
		player->sceneObject->rigidBody = nullptr;
#endif
		player->controller.yaw = yaw;
		player->controller.pitch = pitch;
		player->controller.forwardDir = CA_YawPitchToFoward(yaw, pitch);
		player->controller.rightDir = CA_GetRight(player->controller.forwardDir);
		game->localPlayer = player;
	}
	game->localPlayer->controller.velocity = 20.0f;
	game->localPlayer->controller.sprintModifier = 2.0f;
#ifndef FREE_CAM
	game->localPlayer->controller.SetCamera(&game->localPlayer->camera, 8.0f);
#else
	game->localPlayer->controller.SetCamera(&game->localPlayer->camera);
#endif

}


static std::vector<Vertex3D> debugLines;
void GameManager::RenderCallback(GameState* state)
{
	//debugLines.clear();
	//PH_GetPhysicsVertices(state->physics, debugLines);
	//UpdateModelFromVertices(&debugModel, debugLines.data(), debugLines.size());
	
	RELI_Update(defaultLightGroup, &localPlayer->camera.base);

	static float updatetimer = 0.0f;
	UpdateBoneDataFromModel(foxModel, 0, 0, &foxAnimInstance, updatetimer);
	updatetimer += 1.0f / 60.0f;
	if (updatetimer > 10.0f) updatetimer = 0.0f;


	if (localPlayer)
	{

		glm::vec3 mouseWorldPos;
		glm::vec3 mouseDir = CA_GetMouseWorldSpace(&localPlayer->camera.base, {state->mouseX / (float)state->winWidth, state->mouseY / (float)state->winHeight}, mouseWorldPos);

		SceneObject* hitObj = SC_Raycast(state->scene, mouseWorldPos, mouseDir, 1000.0f);

		uint32_t numSceneObjects = 0;
		SceneObject** objs = SC_GetAllSceneObjects(state->scene, &numSceneObjects);

		RE_BeginScene(state->renderer, objs, numSceneObjects);
		RE_SetCameraBase(state->renderer, &localPlayer->camera.base);
		RE_SetEnvironmentData(state->renderer, &env);
		RE_SetLightData(state->renderer, defaultLightGroup);
		
		RE_RenderShadows(state->renderer);
		
		glBindFramebuffer(GL_FRAMEBUFFER, AAbuffer.fbo);
		glClearColor(0.2f, 0.2f, 0.6f, 1.0f);
		glClearDepthf(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, AAbuffer.width, AAbuffer.height);
		
		RE_RenderGeometry(state->renderer);
		
		RE_RenderCubeMap(state->renderer, env.environmentMap);
		
		RE_RenderOpaque(state->renderer);
		
		
		if (hitObj)
		{
			RE_RenderOutline(state->renderer, hitObj, { 2.0f, 0.0f, 0.0f, 1.0f }, 0.1f);
		}
		RE_RenderTransparent(state->renderer);
	}


	RE_FinishAntialiasingData(&AAbuffer);

	//RE_CopyAntialiasingDataToFBO(&AAbuffer, 0, state->winWidth, state->winHeight);
	
	// RENDER SSR WITH BLOOM
	//RE_RenderScreenSpaceReflection(state->renderer, &SSRbuffer, AAbuffer.intermediateTexture,
	//	PPbuffer.intermediateFbo, PPbuffer.width, PPbuffer.height);
	//RE_RenderPostProcessingBloom(state->renderer, &PPbuffer,
	//	PPbuffer.intermediateTexture, PPbuffer.width, PPbuffer.height,
	//	0, state->winWidth, state->winHeight);

	// RENDER SSR
	//RE_RenderScreenSpaceReflection(state->renderer, &SSRbuffer, AAbuffer.intermediateTexture, PPbuffer.intermediateFbo, PPbuffer.width, PPbuffer.height);
	//RE_RenderPostProcessingToneMap(state->renderer, &PPbuffer, PPbuffer.intermediateTexture, 0, state->winWidth, state->winHeight);

	// RENDER BLOOM
	RE_RenderPostProcessingBloom(state->renderer, &PPbuffer,
		AAbuffer.intermediateTexture, AAbuffer.width, AAbuffer.height,
		0, state->winWidth, state->winHeight);

	RE_EndScene(state->renderer);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_::ImGuiDockNodeFlags_PassthruCentralNode);

	ImGui::ShowDemoWindow(nullptr);
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GameManager::Update(float dt)
{
	
}

void GameManager::OnWindowPositionChanged(int x, int y)
{
}

void GameManager::OnWindowResize(int width, int height)
{
	if (width > 0 && height > 0)
	{
		uint32_t prevSamples = AAbuffer.msaaCount;
		RE_CleanUpAntialiasingData(&AAbuffer);
		RE_CleanUpPostProcessingRenderData(&PPbuffer);
		RE_CleanUpScreenSpaceReflectionRenderData(&SSRbuffer);

		RE_CreateAntialiasingData(&AAbuffer, width, height, prevSamples);
		RE_CreatePostProcessingRenderData(&PPbuffer, width, height);
		RE_CreateScreenSpaceReflectionRenderData(&SSRbuffer, width, height);
	}
}

void GameManager::OnKey(int key, int scancode, int action, int mods)
{
}

void GameManager::OnMouseButton(int button, int action, int mods)
{
}

void GameManager::OnMousePositionChanged(float x, float y, float dx, float dy)
{
	if (localPlayer) localPlayer->controller.HandleMouseMovement(dx, dy);
}
