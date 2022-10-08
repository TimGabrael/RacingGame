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
		if (g_gameState->manager)
		{
			GM_OnResizeCallback(g_gameState->manager, w, h);
		}
	}
}
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (g_gameState && g_gameState->manager)
	{
		Player* p = g_gameState->manager->localPlayer;
		if (p)
		{
			if (action == GLFW_PRESS)
			{
				if (key == GLFW_KEY_W) p->controller.movement.forward = true;
				if (key == GLFW_KEY_A) p->controller.movement.left = true;
				if (key == GLFW_KEY_S) p->controller.movement.back = true;
				if (key == GLFW_KEY_D) p->controller.movement.right = true;
			}
			else if(action == GLFW_RELEASE)
			{
				if (key == GLFW_KEY_W) p->controller.movement.forward = false;
				if (key == GLFW_KEY_A) p->controller.movement.left = false;
				if (key == GLFW_KEY_S) p->controller.movement.back = false;
				if (key == GLFW_KEY_D) p->controller.movement.right = false;
			}
		}
	}
}
static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (g_gameState && g_gameState->manager)
	{
		Player* p = g_gameState->manager->localPlayer;
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (action == GLFW_PRESS) p->controller.movement.actionDown = true;
			else if (action == GLFW_RELEASE) p->controller.movement.actionDown = false;
		}
	}
}
static void MousePositionCallback(GLFWwindow* window, double x, double y)
{
	static double oldX = x;
	static double oldY = y;
	if (g_gameState && g_gameState->manager)
	{
		double dx = x - oldX;
		double dy = oldY - y;
		Player* p = g_gameState->manager->localPlayer;
		if (p && p->controller.movement.actionDown)
		{
			p->controller.movement.deltaYaw += dx;
			p->controller.movement.deltaPitch += dy;
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
		if (g_gameState->manager)
		{
			GM_OnResizeCallback(g_gameState->manager, width, height);
		}
	}
}

GameState* CreateGameState(struct GLFWwindow* window, uint32_t windowWidth, uint32_t windowHeight)
{
	if (g_gameState) return g_gameState;
	g_gameState = new GameState;
	g_gameState->swapChainInterval = 2;
	g_gameState->isFullscreen = false;
	g_gameState->window = window;
	g_gameState->winX = 0;
	g_gameState->winY = 0;
	g_gameState->winWidth = windowWidth;
	g_gameState->winHeight = windowHeight;

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
	glEnable(GL_MULTISAMPLE);

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
		g_gameState->winWidth = width;
		g_gameState->winHeight = height;
	}


	g_gameState->manager = GM_CreateGameManager(g_gameState->renderer, g_gameState->assets);

	return g_gameState;
}

GameState* GetGameState()
{
	return g_gameState;
}