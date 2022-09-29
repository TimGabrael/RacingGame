#pragma once
#include "GLIncludes.h"
#include "../Util/Math.h"



struct SceneObject
{
	struct Model* model;
	struct Material* material;
	struct Entity* entity;
	struct RigidBody* rigidBody;
	glm::mat4 transform;
	GLuint boneData;
	uint32_t flags;
};


struct Scene* SC_CreateScene(struct PhysicsScene* ph);
void SC_CleanUpScene(struct Scene* scene);


void SC_RemoveAll(struct Scene* scene);

SceneObject* SC_AddStaticObject(struct Scene* scene, const SceneObject* obj);
void SC_RemoveStaticObject(struct Scene* scene, SceneObject* obj);

SceneObject* SC_AddDynamicObject(struct Scene* scene, const SceneObject* obj);
void SC_RemoveDynamicObject(struct Scene* scene, SceneObject* obj);