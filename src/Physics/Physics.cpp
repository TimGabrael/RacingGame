#include "Physics.h"

#include <PxPhysicsAPI.h> 
#include <extensions\PxExtensionsAPI.h>
#include <extensions\PxDefaultErrorCallback.h>
#include <extensions\PxDefaultAllocator.h> 
#include <extensions\PxDefaultSimulationFilterShader.h>
#include <extensions\PxDefaultCpuDispatcher.h>
#include <extensions\PxShapeExt.h>
#include <foundation\PxMat33.h> 
#include <extensions\PxSimpleFactory.h>

#include "../Util/Utility.h"


using namespace physx;

struct PhysicsScene
{
	PxPhysics* physicsSDK = NULL;
	PxDefaultErrorCallback defaultErrorCallback;
	PxDefaultAllocator defaultAllocatorCallback;
	PxSimulationFilterShader defaultFilterShader = PxDefaultSimulationFilterShader;
	PxFoundation* foundation = NULL;
	PxScene* scene = NULL;
};



struct PhysicsScene* PH_CreatePhysicsScene()
{
	PhysicsScene* out = new PhysicsScene;

	out->foundation = PxCreateFoundation(PX_PHYSICS_VERSION, out->defaultAllocatorCallback, out->defaultErrorCallback);

	// Creating instance of PhysX SDK
	out->physicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *out->foundation, PxTolerancesScale());

	if (out->physicsSDK == NULL) {
		LOG("Error creating PhysX3 device.\n");
		LOG("Exiting...\n");
		exit(1);
	}
	if (!PxInitExtensions(*out->physicsSDK, nullptr))
		LOG("PxInitExtensions failed!\n");

	PxSceneDesc sceneDesc(out->physicsSDK->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.8f, 0.0f);
	if (!sceneDesc.cpuDispatcher) {
		PxDefaultCpuDispatcher* mCpuDispatcher = PxDefaultCpuDispatcherCreate(1);
		if (!mCpuDispatcher)
			LOG("PxDefaultCpuDispatcherCreate failed!\n");
		sceneDesc.cpuDispatcher = mCpuDispatcher;
	}
	if (!sceneDesc.filterShader)
		sceneDesc.filterShader = out->defaultFilterShader;

	out->scene = out->physicsSDK->createScene(sceneDesc);

	return out;
}

void PH_CleanUpPhysicsScene(struct PhysicsScene* scene)
{
	scene->scene->release();
	scene->physicsSDK->release();
}

void PH_Update(PhysicsScene* scene, float dt)
{
	scene->scene->simulate(dt);
	while (!scene->scene->fetchResults())
	{

	}
}


RigidBody* PH_AddStaticRigidBody(struct PhysicsScene* scene)
{
	PxTransform pose = PxTransform(PxVec3(0.0f, 0, 0.0f), PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
	PxRigidActor* body = scene->physicsSDK->createRigidStatic(pose);
	return (RigidBody*)body;
}
RigidBody* PH_AddDynamicRigidBody(struct PhysicsScene* scene)
{
	PxTransform pose = PxTransform(PxVec3(0.0f, 0, 0.0f), PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
	PxRigidActor* body = scene->physicsSDK->createRigidDynamic(pose);
	return (RigidBody*)body;
}
void PH_RemoveRigidBody(struct PhysicsScene* scene, RigidBody* body)
{
	scene->scene->removeActor(*(PxRigidActor*)body);
}