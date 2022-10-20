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
		Player* player = new Player;
		
		CA_InitPerspectiveCamera(&player->camera, pos, 90.0f, state->winWidth, state->winHeight);


#ifndef FREE_CAM
		physx::PxMaterial* mat = state->physics->physicsSDK->createMaterial(1.0f, 1.0f, 1.0f);
		player->controller.controller = state->physics->AddStdCapsuleController(mat, pos, 10.0f, 2.0f);
#else

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

void DrawString(AtlasTexture* atlas, FontMetrics* metrics, const glm::vec2& start, const char* text);

static std::vector<Vertex3D> debugLines;
void GameManager::RenderCallback(GameState* state, float dt)
{
	//debugLines.clear();
	//PH_GetPhysicsVertices(state->physics, debugLines);
	//UpdateModelFromVertices(&debugModel, debugLines.data(), debugLines.size());
	
	RELI_Update(defaultLightGroup, &localPlayer->camera.base);

	static float updatetimer = 0.0f;
	UpdateBoneDataFromModel(foxModel, 0, 0, &foxAnimInstance, updatetimer);
	updatetimer += dt;
	if (updatetimer > 10.0f) updatetimer = 0.0f;


	if (localPlayer)
	{

		glm::vec3 mouseWorldPos;
		glm::vec3 mouseDir = CA_GetMouseWorldSpace(&localPlayer->camera.base, {state->mouseX / (float)state->winWidth, state->mouseY / (float)state->winHeight}, mouseWorldPos);

		physx::PxRigidActor* hitObj = state->physics->RayCast(mouseWorldPos, mouseDir, 1000.0f);

		RE_BeginScene(state->renderer, state->scene);
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
		
		
		if (hitObj && hitObj->userData)
		{
			RE_RenderOutline(state->renderer, ((FoxEntity*)hitObj->userData)->renderable, {2.0f, 0.0f, 0.0f, 1.0f}, 0.1f);
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

	DrawString(atlas, metrics, { 100.0f, 100.0f }, "So Funktionierts");

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DrawString(AtlasTexture* atlas, FontMetrics* metrics, const glm::vec2& start, const char* text)
{
	glm::vec2 curPos = start;
	const int len = strnlen(text, 1000);
	const float unkownAdvance = roundf(metrics->size * 2.0f / 4.0f);
	ImDrawList* list = ImGui::GetForegroundDrawList();
	for (int i = 0; i < len; i++)
	{
		char c = text[i];
		if (c == ' ') { curPos.x += unkownAdvance; continue; }
		uint32_t idx = c - metrics->firstCharacter;
		if (idx < metrics->numGlyphs)
		{
			Glyph& g = metrics->glyphs[idx];
			
			const ImVec2 tl = { roundf(g.leftSideBearing + curPos.x),  roundf(metrics->ascent + g.yStart + curPos.y) };
			const ImVec2 br = { roundf(tl.x + g.width), roundf(tl.y + g.height) };

			auto bound = atlas->bounds[metrics->atlasIdx + idx];
			list->AddImage((ImTextureID)atlas->texture.uniform, tl, br, { bound.start.x, bound.start.y }, { bound.end.x, bound.end.y });

			curPos.x += g.advance;
		}
	}
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
