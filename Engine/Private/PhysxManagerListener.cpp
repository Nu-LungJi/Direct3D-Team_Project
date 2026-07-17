#include "pch.h"
#include "PhysxManagerListener.h"
#include "PhysXManager.h"

NS_USING(Engine)


CPhysxManagerListener::CPhysxManagerListener()
{

}
CPhysxManagerListener::~CPhysxManagerListener()
{

}

HRESULT CPhysxManagerListener::Initialize()
{
	m_PendingEvents.reserve(128);
    return S_OK;
}

void CPhysxManagerListener::PushEvent(EVENT_TYPE eType, const CHandle& hObjectA, const CHandle& hObjectB)
{
	std::lock_guard lock{ m_PendingEventMutex };
	m_PendingEvents.push_back({ eType, hObjectA, hObjectB });
}

void CPhysxManagerListener::QueueCCTShapeHit(
	const CHandle& hOwner, const CHandle& hOther, const PX_CCT_HIT_DATA& tHit)
{
	PENDING_EVENT tEvent{};
	tEvent.eType = EVENT_TYPE::CCT_SHAPE_HIT;
	tEvent.hObjectA = hOwner;
	tEvent.hObjectB = hOther;
	tEvent.tCCTHit = tHit;
	tEvent.tCCTHit.pGameObject = nullptr;

	std::lock_guard lock{ m_PendingEventMutex };
	m_PendingEvents.push_back(tEvent);
}

void CPhysxManagerListener::QueueCCTControllerHit(
	const CHandle& hOwner, const CHandle& hOther, const PX_CCT_HIT_DATA& tHit)
{
	PENDING_EVENT tEvent{};
	tEvent.eType = EVENT_TYPE::CCT_CONTROLLER_HIT;
	tEvent.hObjectA = hOwner;
	tEvent.hObjectB = hOther;
	tEvent.tCCTHit = tHit;
	tEvent.tCCTHit.pGameObject = nullptr;

	std::lock_guard lock{ m_PendingEventMutex };
	m_PendingEvents.push_back(tEvent);
}

void CPhysxManagerListener::QueueCCTObstacleHit(
	const CHandle& hOwner, const PX_CCT_OBSTACLE_HIT_DATA& tHit)
{
	PENDING_EVENT tEvent{};
	tEvent.eType = EVENT_TYPE::CCT_OBSTACLE_HIT;
	tEvent.hObjectA = hOwner;
	tEvent.tCCTObstacleHit = tHit;

	std::lock_guard lock{ m_PendingEventMutex };
	m_PendingEvents.push_back(tEvent);
}

void CPhysxManagerListener::DispatchPendingEvents()
{
	std::vector<PENDING_EVENT> pendingEvents{};
	{
		std::lock_guard lock{ m_PendingEventMutex };
		pendingEvents.swap(m_PendingEvents);
	}

	for (const PENDING_EVENT& tEvent : pendingEvents)
	{
		CGameObject* pObjectA = CGameInstance::Get().GetGameObjectByHandle(tEvent.hObjectA);
		if (!pObjectA || pObjectA->GetPendingDestroy())
			continue;

		if (tEvent.eType == EVENT_TYPE::WAKE)
		{
			pObjectA->OnWake();
			continue;
		}

		if (tEvent.eType == EVENT_TYPE::SLEEP)
		{
			pObjectA->OnSleep();
			continue;
		}

		if (tEvent.eType == EVENT_TYPE::CCT_SHAPE_HIT ||
			tEvent.eType == EVENT_TYPE::CCT_CONTROLLER_HIT ||
			tEvent.eType == EVENT_TYPE::CCT_OBSTACLE_HIT)
		{
			if (tEvent.eType == EVENT_TYPE::CCT_OBSTACLE_HIT)
			{
				pObjectA->OnCCTObstacleHit(tEvent.tCCTObstacleHit);
				continue;
			}

			PX_CCT_HIT_DATA tHit = tEvent.tCCTHit;
			tHit.pGameObject = CGameInstance::Get().GetGameObjectByHandle(tEvent.hObjectB);
			if (tHit.pGameObject && tHit.pGameObject->GetPendingDestroy())
				tHit.pGameObject = nullptr;

			if (tEvent.eType == EVENT_TYPE::CCT_SHAPE_HIT)
				pObjectA->OnCCTShapeHit(tHit);
			else
				pObjectA->OnCCTControllerHit(tHit);
			continue;
		}

		CGameObject* pObjectB = CGameInstance::Get().GetGameObjectByHandle(tEvent.hObjectB);
		if (!pObjectB || pObjectB->GetPendingDestroy())
			continue;

		switch (tEvent.eType)
		{
		case EVENT_TYPE::COLLISION_ENTER:
			pObjectA->OnCollisionEnter(pObjectB, {});
			pObjectB->OnCollisionEnter(pObjectA, {});
			break;
		case EVENT_TYPE::COLLISION_EXIT:
			pObjectA->OnCollisionExit(pObjectB, {});
			pObjectB->OnCollisionExit(pObjectA, {});
			break;
		case EVENT_TYPE::TRIGGER_ENTER:
			pObjectA->OnTriggerEnter(pObjectB, {});
			pObjectB->OnTriggerEnter(pObjectA, {});
			break;
		case EVENT_TYPE::TRIGGER_EXIT:
			pObjectA->OnTriggerExit(pObjectB, {});
			pObjectB->OnTriggerExit(pObjectA, {});
			break;
		default:
			break;
		}
	}

	pendingEvents.clear();
	{
		std::lock_guard lock{ m_PendingEventMutex };
		if (m_PendingEvents.empty())
			m_PendingEvents.swap(pendingEvents);
	}
}

void CPhysxManagerListener::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
{
    MSG_BOX("onConstraintBreak 미구현");
}

void CPhysxManagerListener::onWake(physx::PxActor** actors, physx::PxU32 count)
{
	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager)
		return;

    for (uint32_t i = 0; i < count; ++i)
    {
        physx::PxActor* pActor = actors[i];
		if (!pActor)
			continue;

		const auto userData = pPhysXManager->FindActorUserData(pActor);
		if (userData)
			PushEvent(EVENT_TYPE::WAKE, userData->hGameObject);
    }
}

void CPhysxManagerListener::onSleep(physx::PxActor** actors, physx::PxU32 count)
{
	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager)
		return;

    for (uint32_t i = 0; i < count; ++i)
    {
        physx::PxActor* pActor = actors[i];
		if (!pActor)
			continue;

		const auto userData = pPhysXManager->FindActorUserData(pActor);
		if (userData)
			PushEvent(EVENT_TYPE::SLEEP, userData->hGameObject);
    }
}

void CPhysxManagerListener::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
	// 헤더 레벨에서 이번 프레임 이전에 해제(Release)된 액터가 포함되어 있다면 통째로 스킵
	if (pairHeader.flags & (physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1))
	{
		return;
	}

	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager || !pairHeader.actors[0] || !pairHeader.actors[1])
		return;

	const auto userDataA = pPhysXManager->FindActorUserData(pairHeader.actors[0]);
	const auto userDataB = pPhysXManager->FindActorUserData(pairHeader.actors[1]);
	if (!userDataA || !userDataB)
		return;

	for (physx::PxU32 i = 0; i < nbPairs; ++i)
    {
        const physx::PxContactPair& cp = pairs[i];
		// 개별 충돌 쌍(Pair) 중에 제거된 셰이프가 있다면 연산에서 제외
		if (cp.flags & (physx::PxContactPairFlag::eREMOVED_SHAPE_0 | physx::PxContactPairFlag::eREMOVED_SHAPE_1))
		{
			continue;
		}

        if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
			PushEvent(EVENT_TYPE::COLLISION_ENTER, userDataA->hGameObject, userDataB->hGameObject);
		if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
			PushEvent(EVENT_TYPE::COLLISION_EXIT, userDataA->hGameObject, userDataB->hGameObject);
    }
}

void CPhysxManagerListener::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
{
	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager)
		return;

    for (physx::PxU32 i = 0; i < count; ++i)
    {
        const physx::PxTriggerPair& tp = pairs[i];

		// 트리거 이벤트 중 이미 삭제된 액터가 관여하고 있다면 안전하게 패스
		if (tp.flags & (physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
		{
			continue;
		}

        if (!tp.triggerActor || !tp.otherActor)
        {
            continue;
        }

		const auto userDataA = pPhysXManager->FindActorUserData(tp.triggerActor);
		const auto userDataB = pPhysXManager->FindActorUserData(tp.otherActor);
		if (!userDataA || !userDataB)
			continue;
        
        if (tp.status & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
			PushEvent(EVENT_TYPE::TRIGGER_ENTER, userDataA->hGameObject, userDataB->hGameObject);
		if (tp.status & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
			PushEvent(EVENT_TYPE::TRIGGER_EXIT, userDataA->hGameObject, userDataB->hGameObject);
    }
}

void CPhysxManagerListener::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
{
    MSG_BOX("onAdvance 미구현");
}

UPtr<CPhysxManagerListener> CPhysxManagerListener::Create()
{
    auto pInstance = ToUPtr(new CPhysxManagerListener{ });
    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CPhysxManagerListener");
        return nullptr;
    }
    return pInstance;
}
