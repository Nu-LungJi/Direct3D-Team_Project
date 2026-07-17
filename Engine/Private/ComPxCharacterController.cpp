#include "pch.h"
#include "ComPxCharacterController.h"
#include "PhysXManager.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")

using namespace physx;

namespace Engine
{
	struct CComPxCharacterController::Impl :
		// 캐릭터 컨트롤러가 move() 함수를 호출하여 이동하는 도중에 무언가와 부딪혔을 때 호출
		public physx::PxUserControllerHitReport, 
		// 캐릭터가 특정 객체 위에 닿았을 때(예: 바닥을 밟거나 움직이는는 엘리베이터 위에 올라탔을 때),
		// 이 객체 위에서 캐릭터가 어떻게 행동할지(미끄러질지, 올라탈 수 있는지)를 물리 엔진에 알려주는 역할
		public physx::PxControllerBehaviorCallback 
	{
		CComPxCharacterController* pOwner = nullptr;
		physx::PxControllerCollisionFlags collisionFlags{};

		
		virtual void onShapeHit(const physx::PxControllerShapeHit& hit) override
		{
			// 1. 충돌한 대상이 동적 물체(RigidDynamic)인지 확인
			physx::PxActor* pActor = hit.shape->getActor();
			physx::PxRigidDynamic* pDynamic = pActor ? pActor->is<physx::PxRigidDynamic>() : nullptr;

			if (pDynamic)
			{
				// 2. 중요: 물체가 잠들어 있으면(Sleeping) 힘을 줘도 안 움직임!
				pDynamic->wakeUp();

				// 3. 밀어낼 방향 결정
				// hit.worldNormal은 충돌한 면의 법선입니다. 
				// 컨트롤러가 물체 쪽으로 이동 중이므로, 이 법선 방향으로 힘을 가하면 물체가 밀려납니다.
				physx::PxVec3 vPushDir = hit.worldNormal;
				vPushDir.y = 0.0f; // 캐릭터가 물체를 밟고 공중으로 솟구치는 걸 방지 (좌우로만 밀기)

				if (vPushDir.magnitudeSquared() > 0.001f)
				{
					vPushDir.normalize();

					// 4. 힘 가하기
					// eIMPULSE: 순간적인 충격 (툭 치는 느낌)
					// eVELOCITY_CHANGE: 속도를 강제로 변화시킴 (더 잘 밀리는 느낌)
					float fForce = 110.0f; // 이 수치를 키우면 더 세게 밀어냅니다.
					pDynamic->addForce(vPushDir * fForce, physx::PxForceMode::eIMPULSE);
				}
			}
		}
		virtual void onControllerHit(const physx::PxControllersHit& hit) override {}
		virtual void onObstacleHit(const physx::PxControllerObstacleHit& hit) override {}
		
		virtual PxControllerBehaviorFlags getBehaviorFlags(const PxObstacle& obstacle)
		{
			return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
		}
		virtual PxControllerBehaviorFlags getBehaviorFlags(const PxShape& shape, const PxActor& actor) override
		{
			// 모든 충돌체에 대해 "올라탈 수 있음"을 반환
			return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
		}

		virtual PxControllerBehaviorFlags getBehaviorFlags(const PxController& controller) override
		{
			return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
		}
	};

}


void CComPxCharacterController::UpdateGUI()
{
}

CComPxCharacterController::CComPxCharacterController() { }
CComPxCharacterController::CComPxCharacterController(const CComPxCharacterController& rhs)
	: CComponent{rhs}
{
}
CComPxCharacterController::~CComPxCharacterController() { }

HRESULT CComPxCharacterController::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

	if (FAILED(CComponent::Initialize(pArg)))
		return E_FAIL;

	if (!pDesc->pResMaterial)
	{
		MSG_BOX("Character Controller needs a PhysX Material");
		return E_FAIL;
	}

	
	m_pImpl = std::make_unique<CComPxCharacterController::Impl>();
	m_pImpl->pOwner = this;
	PxControllerManager* pManager = CGameInstance::Get().PxGetControllerManager();
	if (!pManager)
		return E_FAIL;

	PxCapsuleControllerDesc desc;
	desc.reportCallback = m_pImpl.get();
	desc.behaviorCallback = m_pImpl.get();
	desc.height = pDesc->fHeight;
	desc.radius = pDesc->fRadius;
	desc.stepOffset = pDesc->fStepOffset; // 캐릭터가 점프 없이 자동으로 타고 올라갈 수 있는 장애물
	desc.slopeLimit = pDesc->fSlopeLimit; // 캐릭터가 미끄러지지 않고 걸어 올라갈 수 있는 최대 경사면의 각도입니다. 수치 입력은 코사인(Cosine) 값
	desc.contactOffset = 0.001f; // 캡슐 표면을 감싸는 보이지 않는 '버퍼 존(Skin Width)'의 두께입니다.
	desc.material = pDesc->pResMaterial->GetMaterial(); //[cite: 10]
	if (!desc.material)
		return E_FAIL;
	desc.position = PxExtendedVec3(pDesc->vPosition.x, pDesc->vPosition.y, pDesc->vPosition.z);

	// 캐릭터의 위쪽 방향 설정 (일반적으로 Y축)
	desc.upDirection = PxVec3(0, 1, 0);

	// 이 컴포넌트의 포인터를 UserData에 넣어 충돌 콜백에서 활용
	desc.userData = nullptr;

	m_pController = pManager->createController(desc);
	if (m_pController == nullptr)
		return E_FAIL;

	auto* pPhysXManager = CGameInstance::Get().GetPhysiXManager();
	auto* pActor = m_pController->getActor();
	PHYSX_ACTOR_USER_DATA userData{};
	userData.hGameObject = GetGameObject()->GetHandle();
	userData.eType = PHYSX_ACTOR_TYPE::CHARACTER_CONTROLLER;
	if (!pPhysXManager || !pActor || !pPhysXManager->RegisterActor(pActor, userData))
		return E_FAIL;

	pActor->userData = nullptr;
	return S_OK;
}

void CComPxCharacterController::Move(const XMFLOAT3& vDisplacement, float fTimeStep)
{
	if (!m_pController) return;

	PxVec3 disp(vDisplacement.x, vDisplacement.y, vDisplacement.z);
	// move 함수는 '속도'가 아니라 '변위(Displacement = Velocity * dt)'를 받습니다.
	m_pImpl->collisionFlags = m_pController->move(
		disp,
		0.00f,      // 최소 이동 거리 (이보다 작으면 연산 생략해 최적화)
		fTimeStep,
		PxControllerFilters() // 필터링 설정 (적이나 아군 통과 여부 등)
	);
}

bool CComPxCharacterController::IsGrounded() const
{
	return m_pImpl->collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN);
}

void CComPxCharacterController::SetPosition(const XMFLOAT3& vPosition)
{
	physx::PxExtendedVec3 pos = { vPosition.x, vPosition.y, vPosition.z };

	// 이 함수가 물리 엔진 내의 컨트롤러 위치를 즉시 이동시킵니다.
	m_pController->setPosition(pos);
}


UPtr<CComPxCharacterController> CComPxCharacterController::Create()
{
	auto pInstance = ToUPtr(new CComPxCharacterController{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComPxCharacterController");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CComPxCharacterController::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComPxCharacterController{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned: CComPxCharacterController");
		return nullptr;
	}
	return pInstance;
}

void CComPxCharacterController::Free()
{
	if (m_pController)
	{
		if (auto* pActor = m_pController->getActor())
		{
			if (auto* pPhysXManager = CGameInstance::Get().GetPhysiXManager())
				pPhysXManager->UnregisterActor(pActor);

			pActor->userData = nullptr;
		}

		m_pController->release();
		m_pController = nullptr;
	}
	CComponent::Free();
}
