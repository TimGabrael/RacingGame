#include "Graphics/Renderer.h"
#include "GameState.h"
#include "Util/Assets.h"
#include "Graphics/ModelInfo.h"
#include "Graphics/Scene.h"
#include "Physics/Physics.h"
#include "GameManager.h"

#include "finders_interface.h"

int main()
{

	GameState* game = CreateGameState("RacingGame", 1600, 900, 20);
	GameManager* manager =  GM_CreateGameManager(game);
	game->manager =  manager;
	GM_AddPlayerToScene(manager, { 0.0f, 12.0f, 0.0f }, 90.0f, 0.0f);

	uint32_t numMetrics = 0;
	manager->atlas = AM_LoadTextureAtlas("Assets/atlas.atl", &manager->metrics, &numMetrics, false);
	if (!manager->atlas)
	{
		AtlasBuildData* build = AM_BeginAtlasTexture();
		manager->metrics = AM_AtlasAddGlyphRangeFromFile(build, "Assets/consola.ttf", 'A', 'z' + 1, 13.0f);
		AM_StoreTextureAtlas("Assets/atlas.atl", build, &manager->metrics, 1);
		manager->atlas = AM_EndTextureAtlas(build, false);
	}
	SetConsumeMouse(false);

	manager->sponzaModel = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/Sponza/glTF/Sponza.gltf", MODEL_LOAD_CONCAVE);
	//manager->sponzaModel = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/RocketLeagueStadium.glb", MODEL_LOAD_CONCAVE);
	manager->sponzaModel->baseTransform = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)) * glm::inverse(manager->sponzaModel->nodes[0]->defMatrix);
	manager->foxModel= AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/BrainStem/glTF-Binary/BrainStem.glb", MODEL_LOAD_CONVEX);
	manager->foxModel->baseTransform = glm::scale(glm::mat4(1.0f), glm::vec3(5.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	physx::PxMaterial* material = game->physics->physicsSDK->createMaterial(0.5f, 0.5f, 0.5f);

	physx::PxShape* sponzaShape = nullptr;
	physx::PxShape* foxShape = nullptr;
	physx::PxShape* planeShape = nullptr;

	{
		physx::PxTriangleMeshGeometry geomTriangle(manager->sponzaModel->concaveMesh, physx::PxMeshScale(physx::PxVec3(0.1f, 0.1f, 0.1f)));
		sponzaShape = game->physics->physicsSDK->createShape(geomTriangle, *material);

		physx::PxConvexMeshGeometry geomConvex(manager->foxModel->convexMesh, physx::PxMeshScale(physx::PxVec3(5.0f, 5.0f, 5.0f)));
		foxShape = game->physics->physicsSDK->createShape(geomConvex, *material);

		physx::PxBoxGeometry geomBox(1000.0f, 1.0f, 1000.f);
		planeShape = game->physics->physicsSDK->createShape(geomBox, *material);
	}
	Vertex3D verts[4] = {};
	manager->debugModel = CreateModelFromVertices(verts, 0);
	
	glm::quat def = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	SponzaEntity* ent = new SponzaEntity(manager->sponzaModel);
	ent->body = game->physics->AddStaticBody(planeShape, glm::vec3(0.0f), def);
	ent->renderable = new PBRRenderable(manager->sponzaModel, nullptr, glm::mat4(1.0f));
	ent->model = manager->sponzaModel;
	
	// TEST ANIM INSTANCE DATA
	memset(&manager->foxAnimInstance, 0, sizeof(AnimationInstanceData));
	CreateBoneDataFromModel(manager->foxModel, &manager->foxAnimInstance);
	

	for (int i = 0; i < 10; i++)
	{
		manager->fox = new FoxEntity(manager->foxModel, &manager->foxAnimInstance);
		manager->fox->body = game->physics->AddDynamicBody(foxShape, glm::vec3(0.0f, 10.0f + 10 * i, 0.0f), def);
		manager->fox->renderable = new PBRRenderable(manager->foxModel, &manager->foxAnimInstance, glm::mat4(1.0f));
		manager->fox->body->userData = manager->fox;
	}
	
	UpateGameState();

	return 0;
}