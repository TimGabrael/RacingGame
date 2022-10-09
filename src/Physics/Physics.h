#pragma once
#include "../Util/Math.h"
#include <vector>
#include "../Graphics/ModelInfo.h"

struct RigidBody;
struct PhysicsScene;
struct PhysicsMaterial;
struct PhysicsConvexMesh;
struct PhysicsConcaveMesh;
struct PhysicsShape;

struct PhysicsController
{
	void Move(const glm::vec3& mov);
	glm::vec3 GetPos();
	glm::vec3 GetVelocity();
	RigidBody* GetRigidBody();
	bool IsOnGround();
};

struct VehicleDesc
{
	float chassisMass;
	glm::vec3 chassisDims;
	glm::vec3 chassisMOI;
	glm::vec3 chassisCMOffset;
	PhysicsMaterial* chassisMaterial;

	float wheelMass;
	float wheelWidth;
	float wheelRadius;
	float wheelMOI;
	PhysicsMaterial* wheelMaterial;
	uint32_t numWheels;
};

PhysicsScene* PH_CreatePhysicsScene();
void PH_CleanUpPhysicsScene(PhysicsScene* scene);

void PH_Update(PhysicsScene* scene, float dt);


PhysicsMaterial* PH_AddMaterial(PhysicsScene* scene, float staticFriction, float dynamicFriction, float restitution);

PhysicsConvexMesh* PH_AddPhysicsConvexMesh(PhysicsScene* scene, const void* verts, uint32_t numVerts, uint32_t vertStride);
PhysicsConcaveMesh* PH_AddPhysicsConcaveMesh(PhysicsScene* scene, const void* verts, uint32_t numVerts, uint32_t vertStride, const uint32_t* inds, uint32_t numInds);


PhysicsShape* PH_AddConvexShape(PhysicsScene* scene, PhysicsConvexMesh* mesh, const PhysicsMaterial* material, const glm::vec3& scale);
PhysicsShape* PH_AddConcaveShape(PhysicsScene* scene, PhysicsConcaveMesh* mesh, const PhysicsMaterial* material, const glm::vec3& scale);

PhysicsShape* PH_AddBoxShape(PhysicsScene* scene, const PhysicsMaterial* material, const glm::vec3& halfExtent);


RigidBody* PH_AddStaticRigidBody(PhysicsScene* scene, PhysicsShape* shape, const glm::vec3& pos, const glm::quat& rot);
RigidBody* PH_AddDynamicRigidBody(PhysicsScene* scene, PhysicsShape* shape, const glm::vec3& pos, const glm::quat& rot);

PhysicsController* PH_AddCapsuleController(PhysicsScene* scene, const PhysicsMaterial* material, const glm::vec3& pos, float height, float radius);



void PH_AddShapeToRigidBody(RigidBody* body, PhysicsShape* shape);


void PH_RemoveController(PhysicsController* controller);
void PH_RemoveRigidBody(PhysicsScene* scene, RigidBody* body);



void PH_SetTransformation(RigidBody* body, glm::mat4& m);




void PH_GetPhysicsVertices(PhysicsScene* scene, std::vector<Vertex3D>& verts);