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

	//Model* model = AM_AddModel(game->assets, "Assets/ScriptFactory.glb", MODEL_LOAD_CONVEX | MODEL_LOAD_CONCAVE);
	//Model* model = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/BoomBox/glTF/BoomBox.gltf");
	Model* model = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/Sponza/glTF/Sponza.gltf", MODEL_LOAD_CONVEX | MODEL_LOAD_CONCAVE); 
	model->baseTransform = glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 0.25f, 0.25f));
	//Model* boomBox = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/BoomBox/glTF-Binary/BoomBox.glb", MODEL_LOAD_CONVEX | MODEL_LOAD_CONCAVE);
	//boomBox->baseTransform = glm::scale(glm::mat4(1.0f), glm::vec3(500.0f, 500.0f, 500.0f));
	Model* boomBox = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/SimpleSkin/glTF-Embedded/SimpleSkin.gltf", MODEL_LOAD_CONVEX | MODEL_LOAD_CONCAVE);
	boomBox->baseTransform = glm::scale(glm::mat4(1.0f), glm::vec3(5.0f, 5.0f, 5.0f));

	PhysicsMaterial* material = PH_AddMaterial(game->physics, 0.5f, 0.5f, 0.5f);

	PhysicsShape* otherShape = PH_AddConcaveShape(game->physics, model->concaveMesh, material, glm::vec3(0.25f, 0.25f, 0.25f));
	PhysicsShape* boomBoxShape = PH_AddConvexShape(game->physics, boomBox->convexMesh, material, glm::vec3(500.0f, 500.0f, 500.0f));
	PhysicsShape* ground = PH_AddBoxShape(game->physics, material, glm::vec3(4.0f, 4.0f, 4.0f));


	
	glm::quat def = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	SceneObject base;
	base.entity = nullptr;
	base.material = nullptr;
	base.anim = nullptr;
	base.model = model;
	base.rigidBody = PH_AddStaticRigidBody(game->physics, otherShape, glm::vec3(0.0f), def);
	base.transform = glm::mat4(1.0f);
	SC_AddStaticObject(game->scene, &base);



	// TEST ANIM INSTANCE DATA
	AnimationInstanceData::SkinData skinData;
	skinData.numTransforms = 2;
	glGenBuffers(1, &skinData.skinUniform);
	
	AnimationInstanceData animInstance;
	animInstance.data = &skinData;
	animInstance.numSkins = 1;


	AnimationInstanceData realAnimData; memset(&realAnimData, 0, sizeof(AnimationInstanceData));
	CreateBoneDataFromModel(boomBox, &realAnimData);

	{
		BoneData testBoneData{};
		testBoneData.numJoints = 3;
		testBoneData.jointMatrix[0] = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
		testBoneData.jointMatrix[1] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(100.0f));
		testBoneData.jointMatrix[2] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 60.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
		glBindBuffer(GL_UNIFORM_BUFFER, skinData.skinUniform);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(BoneData), &testBoneData, GL_STATIC_DRAW);
	}


	base.model = boomBox;
	for (int i = 0; i < 1; i++)
	{
		base.anim = &realAnimData;
		base.rigidBody = nullptr;// PH_AddDynamicRigidBody(game->physics, boomBoxShape, glm::vec3(-3.0f, 30.0f + i * 50.0f, 0.0f), def);
		SC_AddDynamicObject(game->scene, &base);
	}


	while (true)
	{
		glfwPollEvents();
		if (glfwWindowShouldClose(window)) break;


		static double timer = glfwGetTime();
		double curTime = glfwGetTime();
		float dt = curTime - timer;
		timer = curTime;

		{
			BoneData testBoneData{};
			testBoneData.numJoints = 2;
			testBoneData.jointMatrix[0] = glm::mat4(1.0f);
			testBoneData.jointMatrix[1] = glm::rotate(glm::mat4(1.0f), (float)curTime, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(sin(curTime), sin(curTime), sin(curTime)));

			glBindBuffer(GL_UNIFORM_BUFFER, skinData.skinUniform);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(BoneData), &testBoneData, GL_STATIC_DRAW);
		}
		static float animIdx = 0.0f;
		animIdx += dt;
		UpdateBoneDataFromModel(boomBox, 0, 0, &realAnimData, animIdx);
		if (animIdx > 40.0f) animIdx = 0.0f;


		SC_Update(game->scene, dt);
		GM_Update(game->manager, dt);
		PH_Update(game->physics, dt);
		uint32_t num = 0;
		SceneObject** obj = SC_GetAllSceneObjects(game->scene, &num);
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