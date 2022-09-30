#include "Graphics/Renderer.h"
#include "GameState.h"
#include "Util/Assets.h"
#include "Graphics/ModelInfo.h"
#include "Graphics/Scene.h"


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

	AddPlayerToScene(game, { 0.0f, 0.0f, 0.0f }, 90.0f, 0.0f);

	//Model* model = AM_AddModel(game->assets, "Assets/ScriptFactory.glb");
	Model* model = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/BoomBox/glTF/BoomBox.gltf");
	SceneObject base;
	base.boneData = 0;
	base.entity = nullptr;
	base.material = nullptr;
	base.model = model;
	base.rigidBody = nullptr;
	base.transform = glm::mat4(1.0f);
	SC_AddStaticObject(game->scene, &base);



	// !!!!ONLY DO THIS WHILE TESTING AFTER THAT DELETE ELSE ERROR!!!!
	{
		auto defaultCubemap = game->assets->textures[DEFUALT_CUBE_MAP];
		EnvironmentData data{};
		data.environmentMap = defaultCubemap->uniform;
		data.width = defaultCubemap->width;
		data.height = defaultCubemap->height;
	
		RE_CreateEnvironment(game->renderer, &data);
		defaultCubemap->uniform = data.prefilteredMap;
	}





	while (true)
	{
		glfwPollEvents();
		if (glfwWindowShouldClose(window)) break;

		SC_Update(game->scene, 0.0f);


		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
		glDepthMask(GL_TRUE);

		glClearColor(0.2f, 0.2f, 0.6f, 1.0f);
		glClearDepthf(1.0f);
		glViewport(0, 0, game->winWidth, game->winHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		
		if (game->localPlayer)
		{
			uint32_t numSceneObjects = 0;
			SceneObject** objs = SC_GetAllSceneObjects(game->scene, &numSceneObjects);

			RE_BeginScene(game->renderer, objs, numSceneObjects);
			RE_SetCameraBase(game->renderer, &game->localPlayer->camera.base);


			RE_RenderCubeMap(game->renderer, game->assets->textures[DEFUALT_CUBE_MAP]->uniform);

			RE_RenderOpaque(game->renderer);

		}

		glfwSwapBuffers(window);

	}

	return 0;
}