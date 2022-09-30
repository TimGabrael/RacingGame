#include "GameState.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "Graphics/Renderer.h"
#include "Graphics/Scene.h"
#include "Physics/Physics.h"
#include "Game/Entitys.h"

static GameState* g_gameState = nullptr;

static void WindowPositionCallback(GLFWwindow* window, int x, int y)
{
	if (g_gameState)
	{
		g_gameState->winX = x;
		g_gameState->winY = y;
	}
}

static void WindowResizeCallback(GLFWwindow* window, int w, int h)
{
	if (g_gameState)
	{
		g_gameState->winWidth = w;
		g_gameState->winHeight = h;
	}
}
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (g_gameState)
	{
		if (g_gameState->localPlayer)
		{
			if (action == GLFW_PRESS)
			{
				if (key == GLFW_KEY_W) g_gameState->localPlayer->input.forward = true;
				if (key == GLFW_KEY_A) g_gameState->localPlayer->input.left = true;
				if (key == GLFW_KEY_S) g_gameState->localPlayer->input.back = true;
				if (key == GLFW_KEY_D) g_gameState->localPlayer->input.right = true;
			}
			else if(action == GLFW_RELEASE)
			{
				if (key == GLFW_KEY_W) g_gameState->localPlayer->input.forward = false;
				if (key == GLFW_KEY_A) g_gameState->localPlayer->input.left = false;
				if (key == GLFW_KEY_S) g_gameState->localPlayer->input.back = false;
				if (key == GLFW_KEY_D) g_gameState->localPlayer->input.right = false;
			}
		}
	}
}
static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (g_gameState)
	{

	}
}
static void MousePositionCallback(GLFWwindow* window, double x, double y)
{
	static double oldX = x;
	static double oldY = y;
	if (g_gameState)
	{
		double dx = x - oldX;
		double dy = oldY - y;
		if (g_gameState->localPlayer)
		{
			g_gameState->localPlayer->camera.yaw += dx;
			g_gameState->localPlayer->camera.pitch += dy;
			g_gameState->localPlayer->camera.pitch = glm::max(glm::min(g_gameState->localPlayer->camera.pitch, 89.9f), -89.9f);
			CA_UpdatePerspectiveCamera(&g_gameState->localPlayer->camera);
		}
		oldX = x;
		oldY = y;
	}
}
static void WindowFocusCallback(GLFWwindow* window, int focused)
{
	if (g_gameState)
	{
		g_gameState->hasFocus = focused;
	}
}
static void WindowMaximizedCallback(GLFWwindow* window, int maximized)
{
	if (g_gameState)
	{
		int width, height = 0;
		glfwGetWindowSize(window, &width, &height);
		g_gameState->isFullscreen = maximized;
		g_gameState->winWidth = width;
		g_gameState->winHeight = height;
	}
}

GameState* CreateGameState(struct GLFWwindow* window, uint32_t windowWidth, uint32_t windowHeight)
{
	if (g_gameState) return g_gameState;
	g_gameState = new GameState;
	g_gameState->swapChainInterval = 2;
	g_gameState->isFullscreen = false;
	g_gameState->localPlayer = nullptr;

	glfwMakeContextCurrent(window);
	glfwSetWindowAspectRatio(window, 16, 9);
	glfwSetFramebufferSizeCallback(window, WindowResizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, MousePositionCallback);
	glfwSetWindowFocusCallback(window, WindowFocusCallback);
	glfwSetWindowPosCallback(window, WindowPositionCallback);
	glfwSetWindowMaximizeCallback(window, WindowMaximizedCallback);

	gladLoadGL();
	glfwSwapInterval(g_gameState->swapChainInterval);

	g_gameState->assets = AM_CreateAssetManager();
	g_gameState->physics = PH_CreatePhysicsScene();
	g_gameState->scene = SC_CreateScene(g_gameState->physics);
	g_gameState->renderer = RE_CreateRenderer(g_gameState->assets);

	if (g_gameState->isFullscreen)
	{
		int count = 0;
		GLFWmonitor** m = glfwGetMonitors(&count);
		int width, height;
		glfwGetMonitorPhysicalSize(m[0], &width, &height);
		glfwSetWindowMonitor(window, m[0], 0, 0, width, height, 60);
	}
	g_gameState->window = window;
	g_gameState->winX = 0;
	g_gameState->winY = 0;
	g_gameState->winWidth = windowWidth;
	g_gameState->winHeight = windowHeight;

	return g_gameState;
}

GameState* GetGameState()
{
	return g_gameState;
}





void AddPlayerToScene(GameState* game, const glm::vec3& pos, float yaw, float pitch)
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

	CA_InitPerspectiveCamera(&player->camera, pos, yaw, pitch, game->winWidth, game->winHeight);


	player->sceneObject = SC_AddDynamicObject(game->scene, &obj);
	game->localPlayer = player;
}