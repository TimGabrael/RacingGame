#include "Graphics/Renderer.h"
#include "GameState.h"
#include "Util/Assets.h"
#include "Graphics/ModelInfo.h"

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

	AM_AddModel(game->assets, "Assets/ScriptFactory.glb");

	while (true)
	{
		glfwPollEvents();
		if (glfwWindowShouldClose(window)) break;

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepthf(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (game->localPlayer)
		{
			RE_BeginScene(game->renderer, game->scene);
			RE_SetCameraBase(game->renderer, &game->localPlayer->camera.base);
			RE_RenderCubeMap(game->renderer, game->assets->textures[DEFUALT_CUBE_MAP]->uniform);
		}

		glfwSwapBuffers(window);

	}

	return 0;
}