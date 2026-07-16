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
    return S_OK;
}

void CPhysxManagerListener::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
{
    MSG_BOX("onConstraintBreak 미구현");
}

void CPhysxManagerListener::onWake(physx::PxActor** actors, physx::PxU32 count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        physx::PxActor* pActor = actors[i];
        if (!pActor) continue;
		if (auto* pObj = CGameInstance::Get().GetPhysiXManager()->FindGameObject(pActor))
			pObj->OnWake();
    }
}

void CPhysxManagerListener::onSleep(physx::PxActor** actors, physx::PxU32 count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        physx::PxActor* pActor = actors[i];
        if (!pActor) continue;
		if (auto* pObj = CGameInstance::Get().GetPhysiXManager()->FindGameObject(pActor))
			pObj->OnSleep();
    }
}

void CPhysxManagerListener::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
	// 헤더 레벨에서 이번 프레임 이전에 해제(Release)된 액터가 포함되어 있다면 통째로 스킵
	if (pairHeader.flags & (physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1))
	{
		return;
	}

    for (physx::PxU32 i = 0; i < nbPairs; ++i)
    {
        const physx::PxContactPair& cp = pairs[i];
		// 개별 충돌 쌍(Pair) 중에 제거된 셰이프가 있다면 연산에서 제외
		if (cp.flags & (physx::PxContactPairFlag::eREMOVED_SHAPE_0 | physx::PxContactPairFlag::eREMOVED_SHAPE_1))
		{
			continue;
		}

        if (!pairHeader.actors[0] || !pairHeader.actors[1])
        {
            continue;
        }
		// 이미 삭제된 컴포넌트 주소(Dangling Pointer)에 접근하는 것을 원천 차단
		auto* pPhysXManager = CGameInstance::Get().GetPhysiXManager();
		auto* pObjA = pPhysXManager->FindGameObject(pairHeader.actors[0]);
		auto* pObjB = pPhysXManager->FindGameObject(pairHeader.actors[1]);
        if (!pObjA || !pObjB)
        {
            continue;
        }

        if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            pObjA->OnCollisionEnter(pObjB, {});
            pObjB->OnCollisionEnter(pObjA, {});
        }
        else if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            pObjA->OnCollisionExit(pObjB, {});
            pObjB->OnCollisionExit(pObjA, {});
        }
    }
}

void CPhysxManagerListener::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
{
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

		auto* pPhysXManager = CGameInstance::Get().GetPhysiXManager();
		auto* pObjA = pPhysXManager->FindGameObject(tp.triggerActor);
		auto* pObjB = pPhysXManager->FindGameObject(tp.otherActor);
        if (!pObjA || !pObjB)
        {
            continue;
        }
        
        if (tp.status & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            pObjA->OnTriggerEnter(pObjB, {});
            pObjB->OnTriggerEnter(pObjA, {});
        }
        else if (tp.status & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            pObjA->OnTriggerExit(pObjB, {});
            pObjB->OnTriggerExit(pObjA, {});
        }
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
