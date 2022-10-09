#pragma once
#include "GLIncludes.h"
#include "../Util/Math.h"


enum SCENE_OBJECT_FLAG
{
	SCENE_OBJECT_FLAG_DYNAMIC = 1,
	SCENE_OBJECT_FLAG_VISIBLE = 2,
};

struct Entity
{
	virtual ~Entity() = default;
	virtual void Update(float dt) = 0;
};


struct SceneObject
{
	struct Model* model;
	struct Material* material;
	struct Entity* entity;
	struct RigidBody* rigidBody;
	struct AnimationInstanceData* anim;
	glm::mat4 transform;
	uint32_t flags;
};


struct Scene* SC_CreateScene(struct PhysicsScene* ph);
void SC_CleanUpScene(struct Scene* scene);


void SC_RemoveAll(struct Scene* scene);

SceneObject* SC_AddStaticObject(struct Scene* scene, const SceneObject* obj);
void SC_RemoveStaticObject(struct Scene* scene, SceneObject* obj);

SceneObject* SC_AddDynamicObject(struct Scene* scene, const SceneObject* obj);
void SC_RemoveDynamicObject(struct Scene* scene, SceneObject* obj);



SceneObject** SC_GetAllSceneObjects(struct Scene* scene, uint32_t* num);

void SC_Update(struct Scene* scene, float dt);