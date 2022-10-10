#include "Physics.h"

#define PX_PHYSX_CHARACTER_STATIC_LIB
#include <PxPhysicsAPI.h> 
#include <extensions/PxExtensionsAPI.h>
#include <extensions/PxDefaultErrorCallback.h>
#include <extensions/PxDefaultAllocator.h> 
#include <extensions/PxDefaultSimulationFilterShader.h>
#include <extensions/PxDefaultCpuDispatcher.h>
#include <extensions/PxShapeExt.h>
#include <foundation/PxMat33.h> 
#include <extensions/PxSimpleFactory.h>
#include <extensions/PxTriangleMeshExt.h>
#include <extensions/PxConvexMeshExt.h>
#include <vehicle/PxVehicleSDK.h>
#include <foundation/PxQuat.h>
#include "vehicle/PxVehicleUtil.h"

#include "../Util/Utility.h"


using namespace physx;
class MyRaycastCallback : public PxRaycastCallback
{
public:
	MyRaycastCallback() : PxRaycastCallback(new PxRaycastHit[4], 4)
	{

	}
	virtual ~MyRaycastCallback()
	{
		delete[] this->touches;
		this->touches = nullptr;
	}
	virtual PxAgain processTouches(const PxRaycastHit* buffer, PxU32 nbHits)
	{
		for (uint32_t i = 0; i < nbHits; i++)
		{
		}
		return false;
	}
};

struct PhysicsScene
{
	PxPhysics* physicsSDK = NULL;
	PxDefaultErrorCallback defaultErrorCallback;
	PxDefaultAllocator defaultAllocatorCallback;
	PxSimulationFilterShader defaultFilterShader = PxDefaultSimulationFilterShader;
	PxControllerManager* manager = NULL;
	PxFoundation* foundation = NULL;
	PxCooking* cooking = NULL;
	PxScene* scene = NULL;
	MyRaycastCallback* raycastCallback;
};



void PhysicsController::Move(const glm::vec3& mov)
{
	PxController* c = (PxController*)this;
	PxControllerFilters filters;
	filters.mFilterCallback = NULL;
	filters.mCCTFilterCallback = NULL;
	filters.mFilterData = NULL;
	c->move({ mov.x, mov.y, mov.z }, 0.01f, 1.0f / 60.0f, filters);
}
glm::vec3 PhysicsController::GetPos()
{
	PxController* c = (PxController*)this;
	const PxExtendedVec3& pos = c->getPosition();
	return {pos.x, pos.y, pos.z};
}
glm::vec3 PhysicsController::GetVelocity()
{
	PxController* c = (PxController*)this;
	const PxVec3& vel = c->getActor()->getLinearVelocity();
	return { vel.x, vel.y, vel.z };
}
RigidBody* PhysicsController::GetRigidBody()
{
	PxController* c = (PxController*)this;
	return (RigidBody*)c->getActor();
}
bool PhysicsController::IsOnGround()
{
	PxController* c = (PxController*)this;
	PxControllerState state;
	c->getState(state);
	return state.touchedShape;
}


struct PhysicsScene* PH_CreatePhysicsScene()
{
	PhysicsScene* out = new PhysicsScene;

	out->foundation = PxCreateFoundation(PX_PHYSICS_VERSION, out->defaultAllocatorCallback, out->defaultErrorCallback);

	// Creating instance of PhysX SDK
	out->physicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *out->foundation, PxTolerancesScale(), true);
	PxInitVehicleSDK(*out->physicsSDK);

	if (out->physicsSDK == NULL) {
		LOG("Error creating PhysX3 device.\n");
		LOG("Exiting...\n");
		exit(1);
	}
	if (!PxInitExtensions(*out->physicsSDK, nullptr))
		LOG("PxInitExtensions failed!\n");

	PxSceneDesc sceneDesc(out->physicsSDK->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	PxDefaultCpuDispatcher* dispatcher = PxDefaultCpuDispatcherCreate(1);
	sceneDesc.cpuDispatcher = dispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;

	PxCookingParams cookingParams = PxCookingParams::PxCookingParams(out->physicsSDK->getTolerancesScale());
	out->cooking = PxCreateCooking(PX_PHYSICS_VERSION, *out->foundation, cookingParams);

	out->scene = out->physicsSDK->createScene(sceneDesc);
	out->manager = PxCreateControllerManager(*out->scene);

	out->raycastCallback = new MyRaycastCallback();

	out->scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
	out->scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);

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
	scene->scene->fetchResults(true);
}


struct PhysicsMaterial* PH_AddMaterial(PhysicsScene* scene, float staticFriction, float dynamicFriction, float restitution)
{
	return (PhysicsMaterial*)scene->physicsSDK->createMaterial(staticFriction, dynamicFriction, restitution);
}

struct PhysicsConvexMesh* PH_AddPhysicsConvexMesh(PhysicsScene* scene, const void* verts, uint32_t numVerts, uint32_t vertStride)
{
	PxConvexMeshDesc desc;
	desc.points.data = verts;
	desc.points.count = numVerts;
	desc.points.stride = vertStride;
	desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
	
	PxDefaultMemoryOutputStream buf;
	if (!scene->cooking->cookConvexMesh(desc, buf))
	{
		LOG("FAILED TO COOK CONVEX MESH\n");
		return nullptr;
	}
	PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	PxConvexMesh* convexMesh = scene->physicsSDK->createConvexMesh(input);
	return (PhysicsConvexMesh*)convexMesh;
}
struct PhysicsConcaveMesh* PH_AddPhysicsConcaveMesh(PhysicsScene* scene, const void* verts, uint32_t numVerts, uint32_t vertStride, const uint32_t* inds, uint32_t numInds)
{
	PxTriangleMeshDesc desc;
	desc.points.data = verts;
	desc.points.count = numVerts;
	desc.points.stride = vertStride;
	desc.triangles.count = numInds / 3;
	desc.triangles.stride = 3 * sizeof(uint32_t);
	desc.triangles.data = inds;

	PxDefaultMemoryOutputStream buf;
	if (!scene->cooking->cookTriangleMesh(desc, buf))
	{
		LOG("FAILED TO COOK CONCAVE MESH\n");
		return nullptr;
	}

	PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	PxTriangleMesh* concaveMesh = scene->physicsSDK->createTriangleMesh(input);
	return (PhysicsConcaveMesh*)concaveMesh;
}


PhysicsShape* PH_AddConvexShape(PhysicsScene* scene, PhysicsConvexMesh* mesh, const PhysicsMaterial* material, const glm::vec3& scale)
{
	PxConvexMeshGeometry geom((PxConvexMesh*)mesh, PxMeshScale(PxVec3(scale.x, scale.y, scale.z)));
	return (PhysicsShape*)scene->physicsSDK->createShape(geom, *(PxMaterial*)material);
}
PhysicsShape* PH_AddConcaveShape(PhysicsScene* scene, PhysicsConcaveMesh* mesh, const PhysicsMaterial* material, const glm::vec3& scale)
{
	PxTriangleMeshGeometry geom((PxTriangleMesh*)mesh, PxMeshScale(PxVec3(scale.x, scale.y, scale.z)));
	PxShape* shape = scene->physicsSDK->createShape(geom, *(PxMaterial*)material);
	return (PhysicsShape*)shape;
}
PhysicsShape* PH_AddBoxShape(PhysicsScene* scene, const PhysicsMaterial* material, const glm::vec3& halfExtent)
{
	PxBoxGeometry geom(halfExtent.x, halfExtent.y, halfExtent.z);
	PxShape* shape = scene->physicsSDK->createShape(geom, *(PxMaterial*)material);
	return (PhysicsShape*)shape;
}

RigidBody* PH_AddStaticRigidBody(struct PhysicsScene* scene, PhysicsShape* shape, const glm::vec3& pos, const glm::quat& rot)
{
	PxTransform pose = PxTransform(PxVec3(pos.x, pos.y, pos.z), PxQuat(rot.x, rot.y, rot.z, rot.w));
	PxRigidStatic* body = scene->physicsSDK->createRigidStatic(pose);
	body->attachShape(*(PxShape*)shape);
	scene->scene->addActor(*body);
	return (RigidBody*)body;
}
RigidBody* PH_AddDynamicRigidBody(struct PhysicsScene* scene, PhysicsShape* shape, const glm::vec3& pos, const glm::quat& rot)
{
	PxTransform pose = PxTransform(PxVec3(pos.x, pos.y, pos.z), PxQuat(rot.x, rot.y, rot.z, rot.w));
	PxRigidDynamic* body = scene->physicsSDK->createRigidDynamic(pose);
	body->attachShape(*(PxShape*)shape);
	scene->scene->addActor(*body);
	PxRigidBodyExt::updateMassAndInertia(*body, 1.0f);
	body->wakeUp();

	return (RigidBody*)body;
}

PhysicsController* PH_AddCapsuleController(PhysicsScene* scene, const PhysicsMaterial* material, const glm::vec3& pos, float height, float radius)
{
	PxCapsuleControllerDesc desc;
	desc.behaviorCallback = NULL;
	desc.climbingMode = PxCapsuleClimbingMode::eEASY;
	desc.contactOffset = 0.001f;
	desc.density = 10.0f;
	desc.height = height;
	desc.invisibleWallHeight = 0.0f;
	desc.material = (PxMaterial*)material;
	desc.maxJumpHeight = 2.0f;
	//desc.nonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
	desc.nonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING;
	desc.position = { pos.x, pos.y, pos.z };
	desc.radius = radius;
	desc.registerDeletionListener = false;
	desc.reportCallback = nullptr;
	desc.scaleCoeff = 1.0f;
	desc.slopeLimit = 0.1f;
	desc.stepOffset = 0.5f;
	desc.upDirection = { 0.0f, 1.0f, 0.0f };
	desc.userData = nullptr;
	desc.volumeGrowth = 1.5f;
	PxController* controller = scene->manager->createController(desc);
	controller->invalidateCache();
	
	
	return (PhysicsController*)controller;
}





void PH_AddShapeToRigidBody(RigidBody* body, PhysicsShape* shape)
{
	((PxRigidActor*)body)->attachShape(*(PxShape*)shape);
}



RigidBody* PH_RayCast(PhysicsScene* scene, const glm::vec3& origin, const glm::vec3& dir, float distance)
{
	bool res = scene->scene->raycast({ origin.x, origin.y, origin.z }, { dir.x, dir.y, dir.z }, distance, *scene->raycastCallback);
	if (res)
	{
		PxRigidActor* closest = nullptr;
		float distance = FLT_MAX;
		for (uint32_t i = 0; i < scene->raycastCallback->nbTouches; i++)
		{
			if (scene->raycastCallback->touches[i].distance < distance)
			{
				distance = scene->raycastCallback->touches[i].distance;
				closest = scene->raycastCallback->touches[i].actor;
			}
		}
		return (RigidBody*)closest;
	}
	return nullptr;
}


void PH_RemoveController(PhysicsController* controller)
{
	PxController* c = (PxController*)controller;
	c->release();
}
void PH_RemoveRigidBody(struct PhysicsScene* scene, RigidBody* body)
{
	scene->scene->removeActor(*(PxRigidActor*)body);
}

void PH_SetTransformation(RigidBody* body, glm::mat4& m)
{
	PxMat44 transform = PxMat44(((PxRigidActor*)body)->getGlobalPose());
	memcpy(&m, &transform, sizeof(PxMat44));
}
void PH_SetRigidBodyUserData(RigidBody* body, void* userData)
{
	((PxRigidActor*)body)->userData = userData;
}
void* PH_GetRigidBodyUserData(RigidBody* body)
{
	return ((PxRigidActor*)body)->userData;
}






void PH_GetPhysicsVertices(PhysicsScene* scene, std::vector<Vertex3D>& verts)
{
#define LINES
#ifdef LINES
	verts.resize(scene->scene->getRenderBuffer().getNbLines() * 2);
	const PxDebugLine* lines = scene->scene->getRenderBuffer().getLines();
	
	memset(verts.data(), 0, sizeof(Vertex3D) * verts.size());
	for (size_t i = 0; i < verts.size(); i+=2)
	{
		size_t curLine = i / 2;
		verts.at(i).pos = { lines[curLine].pos0.x, lines[curLine].pos0.y, lines[curLine].pos0.z };
		verts.at(i+1).pos = { lines[curLine].pos1.x, lines[curLine].pos1.y, lines[curLine].pos1.z };
		verts.at(i).col = lines[curLine].color0;
		verts.at(i+1).col = lines[curLine].color1;
		verts.at(i).nor = { 1.0f, 0.0f, 0.0f };
		verts.at(i+1).nor = { 1.0f, 0.0f, 0.0f };
	}
#else
	verts.resize(scene->scene->getRenderBuffer().getNbTriangles() * 3);
	const PxDebugTriangle* trig = scene->scene->getRenderBuffer().getTriangles();

	memset(verts.data(), 0, sizeof(Vertex3D) * verts.size());
	for (size_t i = 0; i < verts.size(); i += 3)
	{
		size_t curTrig = i / 3;
		verts.at(i).pos = { trig[curTrig].pos0.x, trig[curTrig].pos0.y, trig[curTrig].pos0.z };
		verts.at(i + 1).pos = { trig[curTrig].pos1.x, trig[curTrig].pos1.y, trig[curTrig].pos1.z };
		verts.at(i + 2).pos = { trig[curTrig].pos2.x, trig[curTrig].pos2.y, trig[curTrig].pos2.z };
		verts.at(i).col = trig[curTrig].color0;
		verts.at(i + 1).col = trig[curTrig].color1;
		verts.at(i + 2).col = trig[curTrig].color2;
		verts.at(i).nor = { 1.0f, 0.0f, 0.0f };
		verts.at(i + 1).nor = { 1.0f, 0.0f, 0.0f };
		verts.at(i + 2).nor = { 1.0f, 0.0f, 0.0f };
	}
#endif
}
