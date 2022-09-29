#include "Scene.h"

#define NUM_OBJECTS_IN_LIST sizeof(uintptr_t)

struct SceneObjectList
{
	SceneObjectList* next;
	uintptr_t fillMask;
	SceneObject objects[NUM_OBJECTS_IN_LIST];
};

struct Scene
{
	SceneObjectList staticObjects;
	SceneObjectList dynamicObjects;
	uint32_t numStaticObjects;
	uint32_t numDynamicObjects;
	PhysicsScene* physics;
};

static void FreeObject(SceneObject* obj)
{
	// TODO: FREE THE MEMORY ALLOCATED BY THE ENTITY!
}


static SceneObject* AllocateObjectFromList(SceneObjectList* list)
{
	if (list->fillMask == 0xFFFFFFFFFFFFFFFFLL)
	{
		if (list->next)
		{
			return AllocateObjectFromList(list->next);
		}
		else
		{
			list->next = new SceneObjectList;
			memset(list->next, 0, sizeof(SceneObjectList));
			list->next->fillMask = 1;
			return &list->next->objects[0];
		}
	}
	else
	{
		for (uint64_t i = 0; i < NUM_OBJECTS_IN_LIST; i++)
		{
			uint64_t cur = (1LL << i);
			if (!(list->fillMask & cur))
			{
				list->fillMask |= cur;
				return &list->objects[i];
			}
		}
	}
	return nullptr;
}
static void FreeObjectFromSubList(SceneObjectList* oldList, SceneObjectList* list, SceneObject* obj)
{
	const uintptr_t idx = ((uintptr_t)obj - (uintptr_t)&list->objects[0]) / sizeof(SceneObject);
	if (idx >= 0 && idx < NUM_OBJECTS_IN_LIST)
	{
		const uintptr_t bit = (1LL << idx);
		if (list->fillMask & bit)
		{
			list->fillMask &= ~bit;
			FreeObject(&list->objects[idx]);
		}

		if (list->fillMask == 0) // DELETE THE LIST AS IT DOES NOT CONTAIN ANY OBJECTS ANYMORE!!!
		{
			oldList->next = list->next;
			delete list;
		}
	}
	else
	{
		FreeObjectFromSubList(list, list->next, obj);
	}
}
static void FreeObjectFromList(SceneObjectList* list, SceneObject* obj)
{
	const uintptr_t idx = ((uintptr_t)obj - (uintptr_t)&list->objects[0]) / sizeof(SceneObject);
	if (idx >= 0 && idx < NUM_OBJECTS_IN_LIST)
	{
		const uintptr_t bit = (1LL << idx);
		if (list->fillMask & bit)
		{
			list->fillMask &= ~bit;
			FreeObject(&list->objects[idx]);
		}
	}
	else
	{
		FreeObjectFromSubList(list, list->next, obj);
	}
}

// THIS FUNCTION DOES NOT ITERATE OVER ALL CONNECTED LISTS!!!
static void FreeObjectIfExists(SceneObjectList* list, int idx)
{
	const uintptr_t bit = (1LL << idx);
	if (list->fillMask & bit)
	{
		list->fillMask &= ~bit;
		FreeObject(&list->objects[idx]);
	}
}

static void FreeEntireList(SceneObjectList* list)
{
	while (list->next)
	{
		for (int i = 0; i < NUM_OBJECTS_IN_LIST; i++)
		{
			FreeObjectIfExists(list->next, i);
		}
		SceneObjectList* old = list->next;
		list->next = old->next;
		delete old;
	}
	for (int i = 0; i < NUM_OBJECTS_IN_LIST; i++)
	{
		FreeObjectIfExists(list, i);
	}
}



struct Scene* SC_CreateScene(struct PhysicsScene* ph)
{
	Scene* scene = new Scene;
	memset(scene, 0, sizeof(Scene));
	scene->physics = ph;
	return scene;
}
void SC_CleanUpScene(struct Scene* scene)
{
	SC_RemoveAll(scene);
	delete scene;
}
void SC_RemoveAll(struct Scene* scene)
{
	FreeEntireList(&scene->staticObjects);
	FreeEntireList(&scene->dynamicObjects);
	scene->numStaticObjects = 0;
	scene->numDynamicObjects = 0;
}
SceneObject* SC_AddStaticObject(struct Scene* scene, const SceneObject* obj)
{
	SceneObject* out = AllocateObjectFromList(&scene->staticObjects);
	scene->numStaticObjects = scene->numStaticObjects + 1;
	memcpy(out, obj, sizeof(SceneObject));
	return out;
}
void SC_RemoveStaticObject(struct Scene* scene, SceneObject* obj)
{
	FreeObjectFromList(&scene->staticObjects, obj);
	scene->numStaticObjects = scene->numStaticObjects - 1;
}

SceneObject* SC_AddDynamicObject(struct Scene* scene, const SceneObject* obj)
{
	SceneObject* out = AllocateObjectFromList(&scene->dynamicObjects);
	scene->numDynamicObjects = scene->numDynamicObjects + 1;
	memcpy(out, obj, sizeof(SceneObject));
	return out;
}
void SC_RemoveDynamicObject(struct Scene* scene, SceneObject* obj)
{
	FreeObjectFromList(&scene->dynamicObjects, obj);
	scene->numDynamicObjects = scene->numDynamicObjects - 1;
}