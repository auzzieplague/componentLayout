#include <PxSimulationEventCallback.h>
#include <PxPhysicsAPI.h>
#include <cooking/PxCooking.h>
#include "PhysicsLayer.h"
#include "../components/Terrain.h"

void PhysicsLayer::onAttach(Scene *) {
    Debug::show("[>] Physics Attached");
    initPhysicsWorld();
}

void PhysicsLayer::update(Scene *scene) {
    this->processSpawnQueue(scene);
    // process spawn queue

    physx::PxTransform transform;
    //update positions of models
    for (auto model: scene->modelsWithPhysics) {
        transform = model->mPhysicsBody->getGlobalPose();
        model->applyPxTransform(transform);
    }

    mScene->simulate(1.0f / 60.0f);
    mScene->fetchResults(true);
}

void PhysicsLayer::initPhysicsWorld() {
    // init physx
    mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mDefaultAllocatorCallback, mDefaultErrorCallback);
    if (!mFoundation) throw ("PxCreateFoundation failed!");
    mPvd = PxCreatePvd(*mFoundation);
    physx::PxPvdTransport *transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);

    mPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
    mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, physx::PxTolerancesScale(), true, mPvd);
    mToleranceScale.length = 100;        // typical length of an object
    mToleranceScale.speed = 981;         // typical speed of an object, gravity*1s is a reasonable choice

    physx::PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
    sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
    mDispatcher = physx::PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = mDispatcher;
    sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
    mScene = mPhysics->createScene(sceneDesc);

    physx::PxPvdSceneClient *pvdClient = mScene->getScenePvdClient();
    if (pvdClient) {
        pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }
}

physx::PxTriangleMesh *PhysicsLayer::createTriangleMeshForModel(Model *model) {
// Create the triangle mMesh descriptor
    auto vertices = model->mMesh->getVertices();
    auto indices = model->mMesh->getIndices();

    // Create the triangle mMesh descriptor
    physx::PxTriangleMeshDesc meshDesc;
    meshDesc.points.count = vertices.size();
    meshDesc.points.stride = sizeof(glm::vec3);
    meshDesc.points.data = vertices.data();
    meshDesc.triangles.count = indices.size() / 3;
    meshDesc.triangles.stride = 3 * sizeof(unsigned int);
    meshDesc.triangles.data = indices.data();

    // triangles appeared to be coming out backwards in PVD
    meshDesc.flags = physx::PxMeshFlags(physx::PxMeshFlag::eFLIPNORMALS); // <-- set the eFLIPNORMALS flag

    // Create the cooking object
    physx::PxTolerancesScale scale;
    physx::PxCookingParams params(scale);
    physx::PxCooking *cooking = PxCreateCooking(PX_PHYSICS_VERSION, mPhysics->getFoundation(), params);

    // Create the triangle mMesh in the PhysX SDK
    physx::PxTriangleMesh *triangleMesh = nullptr;
    physx::PxDefaultMemoryOutputStream writeBuffer;
    if (cooking->cookTriangleMesh(meshDesc, writeBuffer)) {
        physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
        triangleMesh = mPhysics->createTriangleMesh(readBuffer);
    }
    cooking->release();

    return triangleMesh;
}


physx::PxHeightFieldGeometry PhysicsLayer::createHeightGeometry(Terrain *model) {
    HeightMap heightMap = model->getHeightMap();
    physx::PxHeightFieldGeometry hfGeom;
    physx::PxHeightFieldDesc hfDesc;
    hfDesc.format = physx::PxHeightFieldFormat::eS16_TM;
    hfDesc.nbColumns = heightMap.width;
    hfDesc.nbRows = heightMap.height;
    hfDesc.samples.data = heightMap.vertexHeights.data();
    hfDesc.samples.stride = sizeof(physx::PxHeightFieldSample); //maybe


    // Create the cooking object
    physx::PxTolerancesScale scale;
    physx::PxCookingParams params(scale);
    physx::PxCooking *cooking = PxCreateCooking(PX_PHYSICS_VERSION, mPhysics->getFoundation(), params);

    // Create height field geometry
    physx::PxHeightField *heightField = nullptr;

    physx::PxDefaultMemoryOutputStream writeBuffer;
    if (cooking->cookHeightField(hfDesc, writeBuffer)) {
        physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
        heightField = mPhysics->createHeightField(readBuffer);
        hfGeom = physx::PxHeightFieldGeometry(heightField, physx::PxMeshGeometryFlags(), heightMap.scale, heightMap.scale,
                                            heightMap.scale);
    }

    cooking->release();

    return hfGeom;
}

void PhysicsLayer::processSpawnQueue(Scene *scene) {

    if (scene->modelsWithPhysicsQueue.size() == 0) return;
    // pull off the next item this frame
    auto model = scene->modelsWithPhysicsQueue.front();
    scene->modelsWithPhysicsQueue.pop_front();

    // use the ColliderConfig for physics config
    ColliderConfig config = model->mCollider->getConfig();
    physx::PxMaterial *mMaterial = mPhysics->createMaterial(config.material.staticFriction,
                                                            config.material.dynamicFriction,
                                                            config.material.restitution);

    physx::PxShape *shape;
    physx::PxTriangleMesh *triangleMesh; // refactor and remove after testing
    physx::PxHeightFieldGeometry hfGeom; // refactor and remove

    switch (config.shape) {
        case config.Box:
            shape = mPhysics->createShape(physx::PxBoxGeometry(config.size, config.size, config.size), *mMaterial);
            break;
        case config.HeightMap:
            hfGeom = createHeightGeometry(dynamic_cast<Terrain *>(model));
            shape = mPhysics->createShape(physx::PxHeightFieldGeometry(hfGeom), *mMaterial);
            // punch in any holes
            break;
        case config.Mesh:
            triangleMesh = createTriangleMeshForModel(model);
            if (triangleMesh) {
                shape = mPhysics->createShape(physx::PxTriangleMeshGeometry(triangleMesh), *mMaterial);
            }
            break;
        default:
            shape = mPhysics->createShape(physx::PxSphereGeometry(config.size), *mMaterial);
            break;
    };

    glm::vec3 p = model->getPosition();
    physx::PxTransform t(physx::PxVec3(p.x, p.y, p.z));

    switch (config.type) {
        case config.Dynamic:
            model->mPhysicsBody = mPhysics->createRigidDynamic(t);
            break;
        default:
            model->mPhysicsBody = mPhysics->createRigidStatic(t);
    }

    model->mPhysicsBody->attachShape(*shape);
    mScene->addActor(*model->mPhysicsBody);
    shape->release();

    // now everything is ready, push this model into the physics array
    scene->modelsWithPhysics.push_back(model);
}



