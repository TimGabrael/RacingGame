#include "GameState.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "Graphics/Renderer.h"
#include "Graphics/Scene.h"
#include "Physics/Physics.h"
#include "Audio/AudioManager.h"

static GameState* g_gameState = nullptr;

static void WindowPositionCallback(GLFWwindow* window, int x, int y)
{
	if (g_gameState)
	{
		g_gameState->accumulatedTime = 0.0f;
		g_gameState->winX = x;
		g_gameState->winY = y;
		if (g_gameState->manager) g_gameState->manager->OnWindowPositionChanged(x, y);
	}
}

static void WindowResizeCallback(GLFWwindow* window, int w, int h)
{
	if (g_gameState)
	{
		g_gameState->accumulatedTime = 0.0f;
		g_gameState->winWidth = w;
		g_gameState->winHeight = h;
		if (g_gameState->manager) g_gameState->manager->OnWindowResize(w, h);
		
	}
}
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (g_gameState && g_gameState->manager)
	{
		g_gameState->manager->OnKey(key, scancode, action, mods);
		if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
		{
			if (g_gameState->isFullscreen)
			{
				SetWindowed(g_gameState, 1600, 900);
			}
			else
			{
				SetFullscreen(g_gameState, 0, nullptr, nullptr);
			}
		}
	}
}
static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (g_gameState && g_gameState->manager)
	{
		if(!ImGui::GetIO().WantCaptureMouse)
			g_gameState->manager->OnMouseButton(button, action, mods);
	}
}
static void MousePositionCallback(GLFWwindow* window, double x, double y)
{
	if (g_gameState && g_gameState->manager)
	{
		const double dx = x - static_cast<double>(g_gameState->mouseX);
		const double dy = static_cast<double>(g_gameState->mouseY) - y;

		if (!ImGui::GetIO().WantCaptureMouse)
			g_gameState->manager->OnMousePositionChanged(static_cast<float>(x), static_cast<float>(y), static_cast<float>(dx), static_cast<float>(dy));
		
		g_gameState->mouseX = static_cast<float>(x);
		g_gameState->mouseY = static_cast<float>(y);
	}
}
static void WindowFocusCallback(GLFWwindow* window, int focused)
{
	if (g_gameState)
	{
		g_gameState->hasFocus = static_cast<bool>(focused);
	}
}
static void WindowMaximizedCallback(GLFWwindow* window, int maximized)
{
	if (g_gameState)
	{
		int width, height = 0;
		glfwGetWindowSize(window, &width, &height);
		g_gameState->winWidth = width;
		g_gameState->winHeight = height;
		if (g_gameState->manager) g_gameState->manager->OnWindowResize(width, height);	
	}
}
static void JoystickCallback(int jid, int event)
{
	if (event == GLFW_CONNECTED)
	{
		g_gameState->numGamepads++;
	}
	else if (event == GLFW_DISCONNECTED)
	{
		g_gameState->numGamepads--;
	}
}

GameState* CreateGameState(const char* windowName, uint32_t windowWidth, uint32_t windowHeight, int numConcurrentAudio)
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
	g_gameState->numGamepads = 0;
	g_gameState->swapChainInterval = 2;
	g_gameState->isFullscreen = false;
	g_gameState->window = window;
	g_gameState->winX = 0;
	g_gameState->winY = 0;
	g_gameState->winWidth = windowWidth;
	g_gameState->winHeight = windowHeight;
	g_gameState->mouseX = 0;
	g_gameState->mouseY = 0;
	g_gameState->tickMultiplier = 1.0f;
    g_gameState->accumulatedTime = 0.0f;

	glfwMakeContextCurrent(window);
	glfwSetWindowAspectRatio(window, 16, 9);
	glfwSetFramebufferSizeCallback(window, WindowResizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, MousePositionCallback);
	glfwSetWindowFocusCallback(window, WindowFocusCallback);
	glfwSetWindowPosCallback(window, WindowPositionCallback);
	glfwSetWindowMaximizeCallback(window, WindowMaximizedCallback);
	glfwSetJoystickCallback(JoystickCallback);
	glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	gladLoadGL();
	glfwSwapInterval(g_gameState->swapChainInterval);
	glEnable(GL_MULTISAMPLE);


	// INITIALIZE IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg].w = 0.4f;


	g_gameState->assets = AM_CreateAssetManager();
	g_gameState->physics = PH_CreatePhysicsScene();
	g_gameState->scene = SC_CreateScene();
	g_gameState->renderer = RE_CreateRenderer(g_gameState->assets);
	g_gameState->audio = AU_CreateAudioManager(numConcurrentAudio);


	if (g_gameState->isFullscreen)
	{
		int count = 0;
		GLFWmonitor** m = glfwGetMonitors(&count);
		if (count > 0)
		{
			int width, height;
			glfwGetMonitorPhysicalSize(m[0], &width, &height);
			glfwSetWindowMonitor(window, m[0], 0, 0, width, height, 0);
			g_gameState->winWidth = width;
			g_gameState->winHeight = height;
		}
	}

	for (int i = 0; i < 16; i++)
	{
		if (glfwJoystickPresent(i))
		{
			if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1 + i))
			{
				g_gameState->numGamepads++;
			}
		}
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

	const GLFWvidmode* mode = glfwGetVideoMode(monitors[monitorIdx]);

	g_gameState->isFullscreen = true;
	g_gameState->accumulatedTime = 0.0f;
	if (width && height)
	{
		glfwSetWindowMonitor(state->window, monitors[monitorIdx], 0, 0, *width, *height, mode->refreshRate);
	}
	else
	{
		int xPos, yPos, w, h;
		glfwGetMonitorWorkarea(monitors[monitorIdx], &xPos, &yPos, &w, &h);
		glfwSetWindowMonitor(state->window, monitors[monitorIdx], xPos, yPos, w, h, mode->refreshRate);
	}
}
void SetWindowed(GameState* state, int width, int height)
{
	g_gameState->isFullscreen = false;
	g_gameState->accumulatedTime = 0.0f;
	glfwSetWindowMonitor(state->window, nullptr, 0, 32, width, height, 0);
}

void SetConsumeMouse(bool consume)
{
	if (consume)
	{
		glfwSetInputMode(g_gameState->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}
	else
	{
		glfwSetInputMode(g_gameState->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}
bool GetKey(int key)
{
	if (ImGui::GetIO().WantCaptureKeyboard) return false;
	return glfwGetKey(g_gameState->window, key);
}
bool GetMouseButton(int button)
{
	if (ImGui::GetIO().WantCaptureMouse) return false;
	return glfwGetMouseButton(g_gameState->window, button);
}

void UpateGameState()
{
	static constexpr float TIME_STEP = 1.0f / 60.0f;
	while (true)
	{
		glfwPollEvents();
		if (glfwWindowShouldClose(g_gameState->window)) {
            break;
        }


		static double timer = glfwGetTime();
		double curTime = glfwGetTime();

		const float dt = static_cast<float>(curTime - timer) * g_gameState->tickMultiplier;
		timer = curTime;
		g_gameState->accumulatedTime += dt;

		while (g_gameState->accumulatedTime >= TIME_STEP)
		{
			g_gameState->manager->PreUpdate(TIME_STEP);
			SC_Update(g_gameState->scene, TIME_STEP);
			g_gameState->manager->Update(TIME_STEP);
			PH_Update(g_gameState->physics, TIME_STEP);
			g_gameState->manager->PostUpdate(TIME_STEP);
			g_gameState->accumulatedTime -= TIME_STEP;
		}
		
		if (g_gameState->winWidth > 0 && g_gameState->winHeight > 0)
		{
			SC_UpdateFrame(g_gameState->scene, dt);

			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CCW);
			glDepthMask(GL_TRUE);

			g_gameState->manager->RenderCallback(g_gameState, dt);

			glfwSwapBuffers(g_gameState->window);
		}

	}
}
void DestroyGameState(GameState* state) {
    if(state == g_gameState) {
        g_gameState = nullptr;
    }
    

    glfwDestroyWindow(state->window);
	
	AM_DestroyAssetManager(state->assets);
    SC_DestroyScene(state->scene);
    RE_DestroyRenderer(state->renderer);
    PH_DestroyPhysicsScene(state->physics);
    AU_DestroyAudioManager(state->audio);
    
	state->assets = nullptr;
	state->physics = nullptr;
	state->scene = nullptr;
	state->renderer = nullptr;
	state->audio = nullptr;

    delete state;
    
}

