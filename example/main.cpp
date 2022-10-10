#include "Graphics/Renderer.h"
#include "GameState.h"
#include "Util/Assets.h"
#include "Graphics/ModelInfo.h"
#include "Graphics/Scene.h"
#include "Physics/Physics.h"
#include "GameManager.h"



int main()
{
	GameState* game = CreateGameState("RacingGame", 1600, 900);
	GameManager* manager =  GM_CreateGameManager(game);
	game->manager =  manager;
	GM_AddPlayerToScene(manager, { 0.0f, 12.0f, 0.0f }, 90.0f, 0.0f);

	SetConsumeMouse(false);

	manager->sponzaModel = AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/Sponza/glTF/Sponza.gltf", MODEL_LOAD_CONCAVE);
	manager->sponzaModel->baseTransform = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)) * glm::inverse(manager->sponzaModel->nodes[0]->defMatrix);
	manager->foxModel= AM_AddModel(game->assets, "C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/BrainStem/glTF-Binary/BrainStem.glb", MODEL_LOAD_CONVEX);
	manager->foxModel->baseTransform = glm::scale(glm::mat4(1.0f), glm::vec3(5.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	PhysicsMaterial* material = PH_AddMaterial(game->physics, 0.5f, 0.5f, 0.5f);

	PhysicsShape* sponzaShape = PH_AddConcaveShape(game->physics, manager->sponzaModel->concaveMesh, material, glm::vec3(0.1f));
	PhysicsShape* foxShape = PH_AddConvexShape(game->physics, manager->foxModel->convexMesh, material, glm::vec3(5.0f));

	Vertex3D verts[4] = {};
	manager->debugModel = CreateModelFromVertices(verts, 0);
	
	glm::quat def = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	SceneObject base;
	base.entity = nullptr;
	base.material = nullptr;
	base.anim = nullptr;
	base.model = manager->sponzaModel;
	base.rigidBody = PH_AddStaticRigidBody(game->physics, sponzaShape, glm::vec3(0.0f), def);
	base.transform = glm::mat4(1.0f);
	SC_AddStaticObject(game->scene, &base);

	base.rigidBody = nullptr;
	base.model = &manager->debugModel;
	//SC_AddStaticObject(game->scene, &base);


	// TEST ANIM INSTANCE DATA
	memset(&manager->foxAnimInstance, 0, sizeof(AnimationInstanceData));
	CreateBoneDataFromModel(manager->foxModel, &manager->foxAnimInstance);


	base.model = manager->foxModel;
	base.anim = &manager->foxAnimInstance;
	for (int i = 0; i < 10; i++)
	{
		base.rigidBody = PH_AddDynamicRigidBody(game->physics, foxShape, glm::vec3(0.0f, 10.0f + 10 * i, 0.0f), def);
		manager->foxSceneObject = SC_AddDynamicObject(game->scene, &base);
	}

	UpateGameState();

	return 0;
}