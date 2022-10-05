#include "Graphics/Renderer.h"
#include "GameState.h"
#include "Util/Assets.h"
#include "Graphics/ModelInfo.h"
#include "Graphics/Scene.h"
#include "Physics/Physics.h"



int main()
{
	if (!glfwInit())
		return 1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	GLFWwindow* window = glfwCreateWindow(1600, 900, "RacingGame", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return 1;
	}
	GameState* game = CreateGameState(window, 1600, 900);
	GM_AddPlayerToScene(game->manager, { 0.0f, 0.0f, 0.0f }, 90.0f, 0.0f);

	//Model* model = AM_AddModel(game->assets, "Assets/ScriptFactory.glb");
	//Model* model = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/BoomBox/glTF/BoomBox.gltf");
	Model* model = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/Sponza/glTF/Sponza.gltf"); 
	model->baseTransform = glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 0.25f, 0.25f));
	Model* boomBox = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/BoomBox/glTF-Binary/BoomBox.glb"); 
	boomBox->baseTransform = glm::scale(glm::mat4(1.0f), glm::vec3(500.0f, 500.0f, 500.0f));
	
	SceneObject base;
	base.boneData = 0;
	base.entity = nullptr;
	base.material = nullptr;
	base.model = model;
	base.rigidBody = nullptr;
	base.transform = glm::mat4(1.0f);
	SC_AddStaticObject(game->scene, &base);

	base.model = boomBox;
	base.transform = glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, 15.0f, 0.0f));
	SC_AddStaticObject(game->scene, &base);


	while (true)
	{
		glfwPollEvents();
		if (glfwWindowShouldClose(window)) break;

		static double timer = glfwGetTime();
		double curTime = glfwGetTime();
		float dt = timer - curTime;
		SC_Update(game->scene, dt);
		PH_Update(game->physics, dt);
		

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
		glDepthMask(GL_TRUE);


		Player* localPlayer = game->manager->localPlayer;
		if (localPlayer)
		{
			uint32_t numSceneObjects = 0;
			SceneObject** objs = SC_GetAllSceneObjects(game->scene, &numSceneObjects);
		
			RE_BeginScene(game->renderer, objs, numSceneObjects);
			RE_SetCameraBase(game->renderer, &localPlayer->camera.base);
			RE_SetEnvironmentData(game->renderer, &game->manager->env);
			RE_SetLightData(game->renderer, game->manager->defaultLightGroup);

			
			RE_RenderShadows(game->renderer);
			glBindFramebuffer(GL_FRAMEBUFFER, game->manager->AAbuffer.fbo);
			glClearColor(0.2f, 0.2f, 0.6f, 1.0f);
			glClearDepthf(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, game->manager->AAbuffer.width, game->manager->AAbuffer.height);

			RE_RenderGeometry(game->renderer);

			RE_RenderCubeMap(game->renderer, game->manager->env.environmentMap);

			RE_RenderOpaque(game->renderer);
		}


		RE_FinishAntialiasingData(&game->manager->AAbuffer);
		RE_RenderPostProcessingBloom(game->renderer, &game->manager->PPbuffer, 
			game->manager->AAbuffer.intermediateTexture, game->manager->AAbuffer.width, game->manager->AAbuffer.height,
			0, game->winWidth, game->winHeight);
		
		glfwSwapBuffers(window);

	}

	return 0;
}