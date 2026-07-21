#pragma once
#include "Engine_Defines.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"
#include <cooking/PxCooking.h>
#include <extensions/PxExtensionsAPI.h>

#pragma pop_macro("new")

NS_BEGIN(Engine)

class CPhysxManagerListener final
	: public CEngineBase
	, public physx::PxSimulationEventCallback
{
private:
	CPhysxManagerListener();
	~CPhysxManagerListener() override;

private:
	HRESULT Initialize();

	// PxSimulationEventCallback
public:
	void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;
	void onWake(physx::PxActor** actors, physx::PxU32 count) override;
	void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
	void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)override;
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
	void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override;

public:
	static UPtr<CPhysxManagerListener> Create();
};

NS_END
