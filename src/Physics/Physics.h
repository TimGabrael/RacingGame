#pragma once

struct RigidBody;
struct PhysicsScene;


PhysicsScene* PH_CreatePhysicsScene();
void PH_CleanUpPhysicsScene(PhysicsScene* scene);


struct RigidBody* PH_AddStaticRigidBody(PhysicsScene* scene);
struct RigidBody* PH_AddDynamicRigidBody(PhysicsScene* scene);
void PH_RemoveRigidBody(PhysicsScene* scene, RigidBody* body);