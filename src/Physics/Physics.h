#pragma once
#include "../Util/Math.h"
#include <vector>
#include "../Graphics/ModelInfo.h"


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

struct PhysicsScene
{
	physx::PxPhysics* physicsSDK = NULL;
	physx::PxDefaultErrorCallback defaultErrorCallback;
	physx::PxDefaultAllocator defaultAllocatorCallback;
	physx::PxSimulationFilterShader defaultFilterShader = physx::PxDefaultSimulationFilterShader;
	physx::PxControllerManager* manager = NULL;
	physx::PxFoundation* foundation = NULL;
	physx::PxScene* scene = NULL;
	struct MyRaycastCallback* raycastCallback;


	physx::PxConvexMesh* AddConvexMesh(const void* verts, uint32_t numVerts, uint32_t vertStride);
	physx::PxTriangleMesh* AddConcaveMesh(const void* verts, uint32_t numVerts, uint32_t vertStride, const uint32_t* inds, uint32_t numInds);

	
	
	physx::PxController* AddStdCapsuleController(physx::PxMaterial* material, const glm::vec3& pos, float height, float radius);

	physx::PxRigidActor* AddStaticBody(physx::PxShape* shape, const glm::vec3& pos, const glm::quat& rot);
	physx::PxRigidActor* AddDynamicBody(physx::PxShape* shape, const glm::vec3& pos, const glm::quat& rot, float density = 1.0f);
	
	
	
	
	physx::PxRigidActor* RayCast(const glm::vec3& origin, const glm::vec3& dir, float distance);


	


};

struct VehicleDesc
{
	float chassisMass;
	glm::vec3 chassisDims;
	glm::vec3 chassisMOI;
	glm::vec3 chassisCMOffset;
	physx::PxMaterial* chassisMaterial;

	float wheelMass;
	float wheelWidth;
	float wheelRadius;
	float wheelMOI;
	physx::PxMaterial* wheelMaterial;
	uint32_t numWheels;
};

PhysicsScene* PH_CreatePhysicsScene();
void PH_CleanUpPhysicsScene(PhysicsScene* scene);

void PH_Update(PhysicsScene* scene, float dt);

glm::vec3 PH_ControllerGetPosition(physx::PxController* controller);
glm::vec3 PH_ControllerGetVelocity(physx::PxController* controller);
bool PH_ControllerIsOnGround(physx::PxController* controller);

void PH_GetPhysicsVertices(PhysicsScene* scene, std::vector<Vertex3D>& verts);