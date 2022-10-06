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
#include <extensions/PxTriangleMeshExt.h>
#include <extensions/PxConvexMeshExt.h>
#include <foundation/PxQuat.h>

#include "../Util/Utility.h"


using namespace physx;

struct PhysicsScene
{
	PxPhysics* physicsSDK = NULL;
	PxDefaultErrorCallback defaultErrorCallback;
	PxDefaultAllocator defaultAllocatorCallback;
	PxSimulationFilterShader defaultFilterShader = PxDefaultSimulationFilterShader;
	PxFoundation* foundation = NULL;
	PxCooking* cooking = NULL;
	PxScene* scene = NULL;
};



struct PhysicsScene* PH_CreatePhysicsScene()
{
	PhysicsScene* out = new PhysicsScene;

	out->foundation = PxCreateFoundation(PX_PHYSICS_VERSION, out->defaultAllocatorCallback, out->defaultErrorCallback);

	// Creating instance of PhysX SDK
	out->physicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *out->foundation, PxTolerancesScale(), true);

	if (out->physicsSDK == NULL) {
		LOG("Error creating PhysX3 device.\n");
		LOG("Exiting...\n");
		exit(1);
	}
	if (!PxInitExtensions(*out->physicsSDK, nullptr))
		LOG("PxInitExtensions failed!\n");

	PxSceneDesc sceneDesc(out->physicsSDK->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.8f, 0.0f);
	PxDefaultCpuDispatcher* dispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = dispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;


	PxCookingParams cookingParams = PxCookingParams::PxCookingParams(out->physicsSDK->getTolerancesScale());
	out->cooking = PxCreateCooking(PX_PHYSICS_VERSION, *out->foundation, cookingParams);

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
	PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
	body->wakeUp();

	return (RigidBody*)body;
}

void PH_AddShapeToRigidBody(RigidBody* body, PhysicsShape* shape)
{
	((PxRigidActor*)body)->attachShape(*(PxShape*)shape);
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
