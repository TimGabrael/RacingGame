#include "Scene.h"
#include "Physics/Physics.h"
#include "GameState.h"
#define NUM_OBJECTS_IN_LIST sizeof(uintptr_t) * 8

struct SceneEntity
{
	Entity* ent;
	bool isAlive;
};

struct Scene
{
	std::vector<SceneEntity> entitys;


	// renderables don't need anything else, as they are not supposed to delete themselves or others
	std::vector<Renderable*> renderables;


	// this will be the output of SC_GetAllEntitys 
	std::vector<Entity*> userEntList;

	bool containsInvalidEntity;
};


Entity::Entity()
{
	SC_AddEntity(GetGameState()->scene, this);
}
Entity::~Entity()
{
	SC_RemoveEntity(GetGameState()->scene, this);
}


struct Scene* SC_CreateScene()
{
	Scene* scene = new Scene;
	return scene;
}
void SC_CleanUpScene(struct Scene* scene)
{
	SC_RemoveAll(scene);
	delete scene;
}

void SC_RemoveAll(struct Scene* scene)
{
	scene->entitys.clear();
	scene->renderables.clear();
}

void SC_AddEntity(struct Scene* scene, Entity* ent)
{
	scene->entitys.push_back({ ent, true });
}
void SC_RemoveEntity(struct Scene* scene, Entity* ent)
{
	scene->containsInvalidEntity = true;
	for (size_t i = 0; i < scene->entitys.size(); i++)
	{
		if (scene->entitys.at(i).ent == ent)
		{
			scene->entitys.at(i).isAlive = false;
		}
	}
}

void SC_AddRenderable(struct Scene* scene, struct Renderable* r)
{
	scene->renderables.push_back(r);
}
void SC_RemoveRenderable(struct Scene* scene, struct Renderable* r)
{
	for (size_t i = 0; i < scene->renderables.size(); i++)
	{
		if (scene->renderables.at(i) == r)
		{
			scene->renderables.erase(scene->renderables.begin() + i);
		}
	}
}


static void SC_RemoveInvalid(struct Scene* scene)
{
	if (scene->containsInvalidEntity)
	{
		std::vector<SceneEntity> newList;
		for (SceneEntity& e : scene->entitys)
		{
			if (e.isAlive)
			{
				newList.push_back(e);
			}
		}
		scene->entitys = std::move(newList);
		scene->containsInvalidEntity = false;
	}
}


Entity** SC_GetAllEntitys(struct Scene* scene, size_t* num)
{
	SC_RemoveInvalid(scene);
	*num = scene->entitys.size();
	for (uint32_t i = 0; i < scene->entitys.size(); i++)
	{
		scene->userEntList.push_back(scene->entitys.at(i).ent);
	}
	return scene->userEntList.data();
}
struct Renderable** SC_GetAllRenderables(struct Scene* scene, size_t* num)
{
	*num = scene->renderables.size();
	return scene->renderables.data();
}




void SC_Update(struct Scene* scene, float dt)
{
	for (uint32_t i = 0; i < scene->entitys.size(); i++)
	{
		if (scene->entitys.at(i).isAlive)
		{
			scene->entitys.at(i).ent->Update(dt);
		}
	}
	SC_RemoveInvalid(scene);
}
void SC_UpdateFrame(struct Scene* scene, float dt)
{
	for (uint32_t i = 0; i < scene->entitys.size(); i++)
	{
		if (scene->entitys.at(i).isAlive)
		{
			scene->entitys.at(i).ent->UpdateFrame(dt);
		}
	}
	SC_RemoveInvalid(scene);
}


Entity* SC_Raycast(const glm::vec3& origin, const glm::vec3& dir, float distance)
{
	GameState* state = GetGameState();
	RigidBody* hit = PH_RayCast(state->physics, origin, dir, distance);
	if (hit)
	{
		return (Entity*)PH_GetRigidBodyUserData(hit);
	}
	return nullptr;
}