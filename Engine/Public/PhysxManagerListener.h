#pragma once
#include "Engine_Defines.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"
#include <cooking/PxCooking.h>
#include <extensions/PxExtensionsAPI.h>
#include <mutex>

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
	void DispatchPendingEvents();
	void QueueCCTShapeHit(const CHandle& hOwner, const CHandle& hOther, const PX_CCT_HIT_DATA& tHit);
	void QueueCCTControllerHit(const CHandle& hOwner, const CHandle& hOther, const PX_CCT_HIT_DATA& tHit);
	void QueueCCTObstacleHit(const CHandle& hOwner, const PX_CCT_OBSTACLE_HIT_DATA& tHit);

private:
	enum class EVENT_TYPE : uint8_t
	{
		WAKE,
		SLEEP,
		COLLISION_ENTER,
		COLLISION_EXIT,
		TRIGGER_ENTER,
		TRIGGER_EXIT,
		CCT_SHAPE_HIT,
		CCT_CONTROLLER_HIT,
		CCT_OBSTACLE_HIT
	};

	struct PENDING_EVENT
	{
		EVENT_TYPE eType{};
		CHandle hObjectA{};
		CHandle hObjectB{};
		PX_CCT_HIT_DATA tCCTHit{};
		PX_CCT_OBSTACLE_HIT_DATA tCCTObstacleHit{};
	};

	void PushEvent(EVENT_TYPE eType, const CHandle& hObjectA, const CHandle& hObjectB = {});

private:
	std::mutex m_PendingEventMutex{};
	std::vector<PENDING_EVENT> m_PendingEvents{};

public:
	static UPtr<CPhysxManagerListener> Create();
};

NS_END
