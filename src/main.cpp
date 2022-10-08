#include "Graphics/Renderer.h"
#include "GameState.h"
#include "Util/Assets.h"
#include "Graphics/ModelInfo.h"
#include "Graphics/Scene.h"
#include "Physics/Physics.h"

static Model debugModel;
static std::vector<Vertex3D> debugLines;
void RenderMainScene(GameState* game)
{
	debugLines.clear();
	PH_GetPhysicsVertices(game->physics, debugLines);
	UpdateModelFromVertices(&debugModel, debugLines.data(), debugLines.size());

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
	//RE_RenderScreenSpaceReflection(game->renderer, &game->manager->SSRbuffer, game->manager->AAbuffer.intermediateTexture, 0, game->winWidth, game->winHeight);
	RE_RenderPostProcessingBloom(game->renderer, &game->manager->PPbuffer,
		game->manager->AAbuffer.intermediateTexture, game->manager->AAbuffer.width, game->manager->AAbuffer.height,
		0, game->winWidth, game->winHeight);
}


int main()
{
	MainCallbacks cbs{};
	cbs.renderCallback = RenderMainScene;
	GameState* game = CreateGameState("RacingGame", &cbs, 1600, 900);
	game->manager =  GM_CreateGameManager(game);


	GM_AddPlayerToScene(game->manager, { 0.0f, 12.0f, 0.0f }, 90.0f, 0.0f);

	//Model* model = AM_AddModel(game->assets, "Assets/ScriptFactory.glb", MODEL_LOAD_CONVEX | MODEL_LOAD_CONCAVE);
	//Model* model = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/BoomBox/glTF/BoomBox.gltf");
	Model* model = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/Sponza/glTF/Sponza.gltf", MODEL_LOAD_CONCAVE); 
	model->baseTransform = glm::scale(glm::mat4(1.0f), glm::vec3(0.125f, 0.125f, 0.125f));
	//Model* boomBox = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/BoomBox/glTF-Binary/BoomBox.glb", MODEL_LOAD_CONVEX | MODEL_LOAD_CONCAVE);
	//boomBox->baseTransform = glm::scale(glm::mat4(1.0f), glm::vec3(500.0f, 500.0f, 500.0f));
	Model* boomBox = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/Fox/glTF-Binary/Fox.glb", MODEL_LOAD_CONVEX);
	boomBox->baseTransform = glm::scale(glm::mat4(1.0f), glm::vec3(0.125f, 0.125f, 0.125f));

	PhysicsMaterial* material = PH_AddMaterial(game->physics, 0.5f, 0.5f, 0.5f);

	PhysicsShape* otherShape = PH_AddConcaveShape(game->physics, model->concaveMesh, material, glm::vec3(0.125f, 0.125f, 0.125f));
	PhysicsShape* boomBoxShape = PH_AddConvexShape(game->physics, boomBox->convexMesh, material, glm::vec3(0.125f, 0.125f, 0.125f));
	//PhysicsShape* ground = PH_AddBoxShape(game->physics, material, glm::vec3(4.0f, 4.0f, 4.0f));

	Vertex3D verts[4] = {};
	debugModel = CreateModelFromVertices(verts, 0);
	
	glm::quat def = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	SceneObject base;
	base.entity = nullptr;
	base.material = nullptr;
	base.anim = nullptr;
	base.model = model;
	base.rigidBody = PH_AddStaticRigidBody(game->physics, otherShape, glm::vec3(0.0f), def);
	base.transform = glm::mat4(1.0f);
	SC_AddStaticObject(game->scene, &base);

	base.rigidBody = nullptr;
	base.model = &debugModel;
	SC_AddStaticObject(game->scene, &base);


	// TEST ANIM INSTANCE DATA
	AnimationInstanceData realAnimData; memset(&realAnimData, 0, sizeof(AnimationInstanceData));
	CreateBoneDataFromModel(boomBox, &realAnimData);


	base.model = boomBox;
	for (int i = 0; i < 1; i++)
	{
		base.anim = &realAnimData;
		base.rigidBody = PH_AddDynamicRigidBody(game->physics, boomBoxShape, glm::vec3(0.0f, 20.0f, 0.0f), def);
		SC_AddDynamicObject(game->scene, &base);
	}

	UpateGameState();

	return 0;
}