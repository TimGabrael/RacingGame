#include "Physics.h"
#include <cooking/PxCooking.h>

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


PhysicsScene* PH_CreatePhysicsScene()
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
	
	out->scene = out->physicsSDK->createScene(sceneDesc);
	out->manager = PxCreateControllerManager(*out->scene);
	
	out->raycastCallback = new MyRaycastCallback();
	
	out->scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
	out->scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);

	return out;
}

void PH_DestroyPhysicsScene(PhysicsScene* scene)
{
	delete scene->raycastCallback;
	scene->manager->release();
	scene->scene->release();
	scene->physicsSDK->release();

	// Foundation destruction failed due to pending module references. Close/release all depending modules first.
	// 
	// this comes up in the console whenever the foundation is released, but I don't really understand if this is
	// trying to point out memory leaks or not.
	// (cause I removed EVERY physics component creation except for the scene and it works but the scene always adds the message again to the output,
	//  even though nothing is attached to the scene...)
    scene->foundation->release();
    delete scene;
}

void PH_Update(PhysicsScene* scene, float dt)
{
	scene->scene->simulate(dt);
	scene->scene->fetchResults(true);
}

physx::PxConvexMesh* PhysicsScene::AddConvexMesh(const void* verts, uint32_t numVerts, uint32_t vertStride)
{
	PxCookingParams cookingParams = PxCookingParams(this->physicsSDK->getTolerancesScale());
	PxConvexMeshDesc desc = {};
	desc.points.data = verts;
	desc.points.count = numVerts;
	desc.points.stride = vertStride;
	desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

	PxDefaultMemoryOutputStream buf = {};
	if (!PxCookConvexMesh(cookingParams, desc, buf))
	{
		LOG("FAILED TO COOK CONVEX MESH\n");
		return nullptr;
	}
	PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	PxConvexMesh* convexMesh = physicsSDK->createConvexMesh(input);
	return convexMesh;

}
physx::PxTriangleMesh* PhysicsScene::AddConcaveMesh(const void* verts, uint32_t numVerts, uint32_t vertStride, const uint32_t* inds, uint32_t numInds)
{
	PxCookingParams cookingParams = PxCookingParams(this->physicsSDK->getTolerancesScale());
	PxTriangleMeshDesc desc = {};
	desc.points.data = verts;
	desc.points.count = numVerts;
	desc.points.stride = vertStride;
	desc.triangles.count = numInds / 3;
	desc.triangles.stride = 3 * sizeof(uint32_t);
	desc.triangles.data = inds;

	PxDefaultMemoryOutputStream buf = {};
	if (!PxCookTriangleMesh(cookingParams, desc, buf))
	{
		LOG("FAILED TO COOK CONCAVE MESH\n");
		return nullptr;
	}

	PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	PxTriangleMesh* concaveMesh = physicsSDK->createTriangleMesh(input);
	return concaveMesh;
}
physx::PxController* PhysicsScene::AddStdCapsuleController(physx::PxMaterial* material, const glm::vec3& pos, float height, float radius)
{
	PxCapsuleControllerDesc desc;
	desc.behaviorCallback = NULL;
	desc.climbingMode = PxCapsuleClimbingMode::eEASY;
	desc.contactOffset = 0.001f;
	desc.density = 10.0f;
	desc.height = height;
	desc.invisibleWallHeight = 0.0f;
	desc.material = material;
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
	PxController* controller = manager->createController(desc);
	controller->invalidateCache();

	return controller;
}
physx::PxRigidActor* PhysicsScene::AddStaticBody(physx::PxShape* shape, const glm::vec3& pos, const glm::quat& rot)
{
	PxTransform pose = PxTransform(PxVec3(pos.x, pos.y, pos.z), PxQuat(rot.x, rot.y, rot.z, rot.w));
	PxRigidStatic* body = physicsSDK->createRigidStatic(pose);
	body->attachShape(*(PxShape*)shape);
	scene->addActor(*body);
	return body;
}
physx::PxRigidActor* PhysicsScene::AddDynamicBody(physx::PxShape* shape, const glm::vec3& pos, const glm::quat& rot, float density)
{
	PxTransform pose = PxTransform(PxVec3(pos.x, pos.y, pos.z), PxQuat(rot.x, rot.y, rot.z, rot.w));
	PxRigidDynamic* body = physicsSDK->createRigidDynamic(pose);
	body->attachShape(*(PxShape*)shape);
	scene->addActor(*body);
	PxRigidBodyExt::updateMassAndInertia(*body, density);
	body->wakeUp();
	return body;
}
physx::PxRigidActor* PhysicsScene::RayCast(const glm::vec3& origin, const glm::vec3& dir, float distance)
{
	bool res = scene->raycast({ origin.x, origin.y, origin.z }, { dir.x, dir.y, dir.z }, distance, *raycastCallback);
	if (res)
	{
		PxRigidActor* closest = nullptr;
		float distance = FLT_MAX;
		for (uint32_t i = 0; i < raycastCallback->nbTouches; i++)
		{
			if (raycastCallback->touches[i].distance < distance)
			{
				distance = raycastCallback->touches[i].distance;
				closest = raycastCallback->touches[i].actor;
			}
		}
		return closest;
	}
	return nullptr;
}

glm::vec3 PH_ControllerGetPosition(physx::PxController* controller)
{
	const PxExtendedVec3& pos = controller->getPosition();
	return {pos.x, pos.y, pos.z};
}
glm::vec3 PH_ControllerGetVelocity(physx::PxController* controller)
{
	const PxVec3& vel = controller->getActor()->getLinearVelocity();
	return { vel.x, vel.y, vel.z };
}
bool PH_ControllerIsOnGround(physx::PxController* controller)
{
	PxControllerState state;
	controller->getState(state);
	return state.touchedShape;
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
