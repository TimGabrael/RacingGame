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
		if (p) p->controller.HandleKey(key, scancode, action, mods);
		if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
		{
			SetFullscreen(g_gameState, 0, nullptr, nullptr);
		}
	}
}
static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (g_gameState && g_gameState->manager)
	{
		Player* p = g_gameState->manager->localPlayer;
		if(p) p->controller.HandleMouseButton(button, action, mods);
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
		if (p) p->controller.HandleMouseMovement(dx, dy);
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

GameState* CreateGameState(const char* windowName, MainCallbacks* cbs, uint32_t windowWidth, uint32_t windowHeight)
{
	if (g_gameState) return g_gameState;

	if (!glfwInit())
		return nullptr;


	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, windowName, NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return nullptr;
	}


	g_gameState = new GameState;
	g_gameState->swapChainInterval = 2;
	g_gameState->isFullscreen = false;
	g_gameState->window = window;
	g_gameState->winX = 0;
	g_gameState->winY = 0;
	g_gameState->winWidth = windowWidth;
	g_gameState->winHeight = windowHeight;

	g_gameState->callbacks = *cbs;

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

	return g_gameState;
}

GameState* GetGameState()
{
	return g_gameState;
}

void SetFullscreen(GameState* state, int monitorIdx, int* width, int* height)
{
	int numMonitors = 0;
	GLFWmonitor** monitors = glfwGetMonitors(&numMonitors);
	if (numMonitors <= monitorIdx) return;

	if (width && height)
	{
		glfwSetWindowMonitor(state->window, monitors[monitorIdx], 0, 0, *width, *height, 0);
	}
	else
	{
		int xPos, yPos, w, h;
		glfwGetMonitorWorkarea(monitors[monitorIdx], &xPos, &yPos, &w, &h);
		glfwSetWindowMonitor(state->window, monitors[monitorIdx], xPos, yPos, w, h, 0);
	}
}

void UpateGameState()
{
	static constexpr float TIME_STEP = 1.0f / 60.0f;
	while (true)
	{
		glfwPollEvents();
		if (glfwWindowShouldClose(g_gameState->window)) break;


		static double timer = glfwGetTime();
		static float accTime = 0.0f;
		double curTime = glfwGetTime();

		float dt = curTime - timer;
		timer = curTime;
		accTime += dt;

		while (accTime >= TIME_STEP)
		{
			if (g_gameState->callbacks.presceneUpdate) g_gameState->callbacks.presceneUpdate(g_gameState, TIME_STEP);
			SC_Update(g_gameState->scene, TIME_STEP);
			if (g_gameState->callbacks.prephysicsUpdate) g_gameState->callbacks.prephysicsUpdate(g_gameState, TIME_STEP);
			GM_Update(g_gameState->manager, TIME_STEP);
			PH_Update(g_gameState->physics, TIME_STEP);
			if (g_gameState->callbacks.postphysicsUpdate) g_gameState->callbacks.postphysicsUpdate(g_gameState, TIME_STEP);
			accTime -= TIME_STEP;
			break;
		}
		
		if (g_gameState->winWidth > 0 && g_gameState->winHeight > 0)
		{
			uint32_t num = 0;
			SceneObject** obj = SC_GetAllSceneObjects(g_gameState->scene, &num);
			for (int i = 0; i < num; i++)
			{
				if (obj[i]->rigidBody)
				{
					PH_SetTransformation(obj[i]->rigidBody, obj[i]->transform);
				}
			}


			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CCW);
			glDepthMask(GL_TRUE);

			if (g_gameState->callbacks.renderCallback) g_gameState->callbacks.renderCallback(g_gameState);

			glfwSwapBuffers(g_gameState->window);
		}

	}
}