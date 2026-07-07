#pragma once
#include "Engine_Defines.h"

#ifdef _DEBUG
// 라이브러리 설정 전후로 매크로 잠시 해제
#undef new
#endif

#include "PxPhysicsAPI.h"
#include <cooking/PxCooking.h>
#include <extensions/PxExtensionsAPI.h>

#ifdef _DEBUG
#define new DBG_NEW
#endif

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
