#include "pch.h"
#include "ComPxCharacterController.h"
#include "ComPxJoint.h"
#include "PhysXManager.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")

using namespace physx;

namespace Engine
{
	namespace
	{
		PX_CCT_HIT_DATA ConvertHitData(const physx::PxControllerHit& hit, CGameObject* pGameObject)
		{
			PX_CCT_HIT_DATA tResult{};
			tResult.pGameObject = pGameObject;
			tResult.vWorldPosition = {
				static_cast<_float>(hit.worldPos.x),
				static_cast<_float>(hit.worldPos.y),
				static_cast<_float>(hit.worldPos.z) };
			tResult.vWorldNormal = { hit.worldNormal.x, hit.worldNormal.y, hit.worldNormal.z };
			tResult.vMoveDirection = { hit.dir.x, hit.dir.y, hit.dir.z };
			tResult.fMoveLength = hit.length;
			return tResult;
		}

		void FillOtherShapeData(
			const CPhysXManager& manager,
			const physx::PxShape* pShape,
			CHandle& hOther,
			PX_CCT_HIT_DATA& tHit)
		{
			if (!pShape)
				return;

			if (const auto tShapeData = manager.FindShapeUserData(pShape))
			{
				hOther = tShapeData->hGameObject;
				tHit.eOtherShapeType = tShapeData->eType;
				tHit.iOtherShapeSubIndex = tShapeData->iSubIndex;
			}
		}

		physx::PxControllerBehaviorFlags ConvertBehavior(PX_CCT_BEHAVIOR eBehavior)
		{
			const uint8_t iBehavior = static_cast<uint8_t>(eBehavior);
			physx::PxControllerBehaviorFlags tResult{};

			if (iBehavior & static_cast<uint8_t>(PX_CCT_BEHAVIOR::CAN_RIDE))
				tResult |= physx::PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
			if (iBehavior & static_cast<uint8_t>(PX_CCT_BEHAVIOR::SLIDE))
				tResult |= physx::PxControllerBehaviorFlag::eCCT_SLIDE;
			if (iBehavior & static_cast<uint8_t>(PX_CCT_BEHAVIOR::USER_DEFINED_RIDE))
				tResult |= physx::PxControllerBehaviorFlag::eCCT_USER_DEFINED_RIDE;

			return tResult;
		}

		_bool HasCCTBehaviorFlag(
			PX_CCT_BEHAVIOR eBehavior,
			PX_CCT_BEHAVIOR eFlag)
		{
			return (static_cast<uint8_t>(eBehavior) &
				static_cast<uint8_t>(eFlag)) != 0;
		}

	}

	struct CComPxCharacterController::Impl :
		// 캐릭터 컨트롤러가 move() 함수를 호출하여 이동하는 도중에 무언가와 부딪혔을 때 호출
		public physx::PxUserControllerHitReport, 
		// 캐릭터가 특정 객체 위에 닿았을 때(예: 바닥을 밟거나 움직이는는 엘리베이터 위에 올라탔을 때),
		// 이 객체 위에서 캐릭터가 어떻게 행동할지(미끄러질지, 올라탈 수 있는지)를 물리 엔진에 알려주는 역할
		public physx::PxControllerBehaviorCallback,
		public physx::PxQueryFilterCallback,
		public physx::PxControllerFilterCallback
	{
		CComPxCharacterController* pOwner = nullptr;
		physx::PxControllerCollisionFlags collisionFlags{};
		_bool bPreventUpwardProjection{};
		_bool bConstrainHorizontalToInput{};
		_bool bApplyingDirectionalCorrection{};

		void BeginMove()
		{
			bPreventUpwardProjection = false;
			bConstrainHorizontalToInput = false;
			bApplyingDirectionalCorrection = false;
		}

		void AccumulateBehavior(PX_CCT_BEHAVIOR eBehavior)
		{
			if (HasCCTBehaviorFlag(
				eBehavior,
				PX_CCT_BEHAVIOR::PREVENT_UPWARD_PROJECTION))
			{
				bPreventUpwardProjection = true;
			}
			if (HasCCTBehaviorFlag(
				eBehavior,
				PX_CCT_BEHAVIOR::CONSTRAIN_HORIZONTAL_TO_INPUT))
			{
				bConstrainHorizontalToInput = true;
			}
		}

		physx::PxControllerCollisionFlags RemoveUnrequestedHorizontalDisplacement(
			physx::PxController& controller,
			const physx::PxExtendedVec3& vPositionBeforeMove,
			const XMFLOAT3& vRequestedDisplacement,
			_float fMinDistance,
			_float fTimeStep,
			const physx::PxFilterData& tMoveFilter)
		{
			physx::PxControllerCollisionFlags correctionFlags{};
			if (!bConstrainHorizontalToInput)
				return correctionFlags;

			const physx::PxExtendedVec3 vPositionAfterMove =
				controller.getPosition();
			const double fActualX =
				vPositionAfterMove.x - vPositionBeforeMove.x;
			const double fActualZ =
				vPositionAfterMove.z - vPositionBeforeMove.z;
			const double fRequestedHorizontalLengthSq =
				static_cast<double>(vRequestedDisplacement.x) *
				vRequestedDisplacement.x +
				static_cast<double>(vRequestedDisplacement.z) *
				vRequestedDisplacement.z;
			double fLateralX = fActualX;
			double fLateralZ = fActualZ;

			if (fRequestedHorizontalLengthSq > 1e-10)
			{
				const double fRequestedHorizontalLength =
					sqrt(fRequestedHorizontalLengthSq);
				const double fDirectionX =
					vRequestedDisplacement.x / fRequestedHorizontalLength;
				const double fDirectionZ =
					vRequestedDisplacement.z / fRequestedHorizontalLength;
				const double fForwardDistance =
					fActualX * fDirectionX + fActualZ * fDirectionZ;
				fLateralX -= fDirectionX * fForwardDistance;
				fLateralZ -= fDirectionZ * fForwardDistance;
			}

			const double fLateralLengthSq =
				fLateralX * fLateralX + fLateralZ * fLateralZ;
			if (fLateralLengthSq <= 1e-10)
				return correctionFlags;

			bApplyingDirectionalCorrection = true;
			correctionFlags = controller.move(
				physx::PxVec3{
					static_cast<float>(-fLateralX),
					0.f,
					static_cast<float>(-fLateralZ) },
				fMinDistance,
				fTimeStep,
				physx::PxControllerFilters(
					&tMoveFilter,
					this,
					this));
			bApplyingDirectionalCorrection = false;
			return correctionFlags;
		}

		void ClampUnrequestedUpwardDisplacement(
			physx::PxController& controller,
			const physx::PxExtendedVec3& vPositionBeforeMove,
			const XMFLOAT3& vRequestedDisplacement) const
		{
			if (!bPreventUpwardProjection)
				return;

			physx::PxExtendedVec3 vCorrectedPosition =
				controller.getPosition();
			const double fRequestedUpwardDisplacement =
				std::max(0.f, vRequestedDisplacement.y);
			const double fMaximumAllowedY =
				vPositionBeforeMove.y + fRequestedUpwardDisplacement;
			if (vCorrectedPosition.y <= fMaximumAllowedY)
				return;

			vCorrectedPosition.y = fMaximumAllowedY;
			controller.setPosition(vCorrectedPosition);
		}

		void onShapeHit(const physx::PxControllerShapeHit& hit) override
		{
			if (!pOwner || !pOwner->GetGameObject())
				return;
			if (bApplyingDirectionalCorrection)
				return;

			auto* pManager = CGameInstance::Get().GetPhysXManager();
			if (!pManager)
				return;

			CHandle hOther{};
			CGameObject* pOtherGameObject =
				pManager->FindGameObject(hit.actor);
			if (const auto userData = pManager->FindActorUserData(hit.actor))
				hOther = userData->hGameObject;

			const PX_CCT_BEHAVIOR eBehavior =
				pOwner->GetGameObject()->GetCCTShapeBehavior(
					pOtherGameObject);
			AccumulateBehavior(eBehavior);

			PX_CCT_HIT_DATA tHit = ConvertHitData(hit, nullptr);
			FillOtherShapeData(*pManager, hit.shape, hOther, tHit);

			pManager->QueueCCTShapeHit(
				pOwner->GetGameObject()->GetHandle(), hOther, tHit);
		}

		void onControllerHit(const physx::PxControllersHit& hit) override
		{
			if (!pOwner || !pOwner->GetGameObject())
				return;
			if (bApplyingDirectionalCorrection)
				return;

			auto* pManager = CGameInstance::Get().GetPhysXManager();
			if (!pManager)
				return;

			const physx::PxRigidActor* pActor = hit.other ? hit.other->getActor() : nullptr;
			CHandle hOther{};
			if (const auto userData = pManager->FindActorUserData(pActor))
				hOther = userData->hGameObject;
			PX_CCT_HIT_DATA tHit = ConvertHitData(hit, nullptr);
			physx::PxShape* pOtherShape{};
			if (pActor && pActor->getShapes(&pOtherShape, 1) == 1)
				FillOtherShapeData(*pManager, pOtherShape, hOther, tHit);

			pManager->QueueCCTControllerHit(
				pOwner->GetGameObject()->GetHandle(), hOther, tHit);
		}

		void onObstacleHit(const physx::PxControllerObstacleHit& hit) override
		{
			if (!pOwner || !pOwner->GetGameObject())
				return;
			if (bApplyingDirectionalCorrection)
				return;

			PX_CCT_OBSTACLE_HIT_DATA tResult{};
			tResult.pUserData = hit.userData;
			tResult.vWorldPosition = {
				static_cast<_float>(hit.worldPos.x),
				static_cast<_float>(hit.worldPos.y),
				static_cast<_float>(hit.worldPos.z) };
			tResult.vWorldNormal = { hit.worldNormal.x, hit.worldNormal.y, hit.worldNormal.z };
			tResult.vMoveDirection = { hit.dir.x, hit.dir.y, hit.dir.z };
			tResult.fMoveLength = hit.length;
			if (auto* pManager = CGameInstance::Get().GetPhysXManager())
				pManager->QueueCCTObstacleHit(pOwner->GetGameObject()->GetHandle(), tResult);
		}
		
		PxControllerBehaviorFlags getBehaviorFlags(const PxObstacle& obstacle) override
		{
			CGameObject* pGameObject = pOwner ? pOwner->GetGameObject() : nullptr;
			const PX_CCT_BEHAVIOR eBehavior = pGameObject
				? pGameObject->GetCCTObstacleBehavior(obstacle.mUserData)
				: PX_CCT_BEHAVIOR::CAN_RIDE;
			return ConvertBehavior(eBehavior);
		}

		PxControllerBehaviorFlags getBehaviorFlags(const PxShape& shape, const PxActor& actor) override
		{
			auto* pManager = CGameInstance::Get().GetPhysXManager();
			CGameObject* pGameObject = pManager ? pManager->FindGameObject(&actor) : nullptr;
			CGameObject* pOwnerObject = pOwner ? pOwner->GetGameObject() : nullptr;
			const PX_CCT_BEHAVIOR eBehavior = pOwnerObject
				? pOwnerObject->GetCCTShapeBehavior(pGameObject)
				: PX_CCT_BEHAVIOR::CAN_RIDE;

			// [LSY] Hit Report가 발생하지 않는 겹침 복구 경로에서도 Shape 정책은
			// 조회되므로, 이번 move에 적용할 엔진 확장 정책을 여기서도 기록한다.
			AccumulateBehavior(eBehavior);

			return ConvertBehavior(eBehavior);
		}

		PxControllerBehaviorFlags getBehaviorFlags(const PxController& controller) override
		{
			auto* pManager = CGameInstance::Get().GetPhysXManager();
			CGameObject* pGameObject = pManager ? pManager->FindGameObject(controller.getActor()) : nullptr;
			CGameObject* pOwnerObject = pOwner ? pOwner->GetGameObject() : nullptr;
			const PX_CCT_BEHAVIOR eBehavior = pOwnerObject
				? pOwnerObject->GetCCTControllerBehavior(pGameObject)
				: PX_CCT_BEHAVIOR::NONE;
			return ConvertBehavior(eBehavior);
		}

		PxQueryHitType::Enum preFilter(
			const PxFilterData& filterData,
			const PxShape* shape,
			const PxRigidActor* actor,
			PxHitFlags& queryFlags) override
		{
			if (!pOwner || !shape || !actor)
				return PxQueryHitType::eNONE;

			if (pOwner->m_pController && actor == pOwner->m_pController->getActor())
				return PxQueryHitType::eNONE;

			if (shape->getFlags().isSet(PxShapeFlag::eTRIGGER_SHAPE))
				return PxQueryHitType::eNONE;

			const PxFilterData tTargetFilter = shape->getQueryFilterData();
			return (pOwner->m_tFilter.iQueryMask & tTargetFilter.word0) != 0
				? PxQueryHitType::eBLOCK
				: PxQueryHitType::eNONE;
		}

		PxQueryHitType::Enum postFilter(
			const PxFilterData& filterData,
			const PxQueryHit& hit,
			const PxShape* shape,
			const PxRigidActor* actor) override
		{
			return PxQueryHitType::eBLOCK;
		}

		bool filter(const PxController& a, const PxController& b) override
		{
			const PxRigidDynamic* pActorA = a.getActor();
			const PxRigidDynamic* pActorB = b.getActor();
			if (!pActorA || !pActorB)
				return false;

			PxShape* pShapeA{};
			PxShape* pShapeB{};
			if (pActorA->getShapes(&pShapeA, 1) != 1 || !pShapeA ||
				pActorB->getShapes(&pShapeB, 1) != 1 || !pShapeB)
				return false;

			const PxFilterData tFilterA = pShapeA->getQueryFilterData();
			const PxFilterData tFilterB = pShapeB->getQueryFilterData();
			return (tFilterA.word1 & tFilterB.word0) != 0 &&
				(tFilterB.word1 & tFilterA.word0) != 0;
		}
	};

}


void CComPxCharacterController::UpdateGUI()
{
	CComponent::UpdateGUI();
	ImGui::PushID(this);

	if (!m_pController)
	{
		ImGui::Text("Controller: nullptr");
		ImGui::PopID();
		return;
	}

	ImGui::Text("CCT Callback Target: Owner GameObject");
	ImGui::Text("Collision Side: %s", IsCollidingSide() ? "true" : "false");
	ImGui::Text("Collision Up: %s", IsCollidingUp() ? "true" : "false");
	ImGui::Text("Grounded: %s", IsGrounded() ? "true" : "false");

	_float3 vPosition = GetPosition();
	float fPosition[3] = { vPosition.x, vPosition.y, vPosition.z };
	if (ImGui::DragFloat3("Position", fPosition, 0.1f))
		SetPosition({ fPosition[0], fPosition[1], fPosition[2] });

	if (m_pController->getType() == PxControllerShapeType::eCAPSULE)
	{
		auto* pCapsule = static_cast<PxCapsuleController*>(m_pController);
		float fHeight = pCapsule->getHeight();
		if (ImGui::DragFloat("Height", &fHeight, 0.05f, 0.01f, 100.f))
			Resize(fHeight);

		float fRadius = pCapsule->getRadius();
		if (ImGui::DragFloat("Radius", &fRadius, 0.05f, 0.01f, 100.f))
			SetRadius(fRadius);
	}

	float fStepOffset = GetStepOffset();
	if (ImGui::DragFloat("Step Offset", &fStepOffset, 0.01f, 0.f, 100.f))
		SetStepOffset(fStepOffset);

	float fSlopeLimit = GetSlopeLimit();
	if (ImGui::SliderFloat("Slope Limit (cos)", &fSlopeLimit, 0.f, 1.f))
		SetSlopeLimit(fSlopeLimit);

	float fContactOffset = GetContactOffset();
	if (ImGui::DragFloat("Contact Offset", &fContactOffset, 0.001f, 0.0001f, 10.f, "%.4f"))
		SetContactOffset(fContactOffset);

	PX_FILTER_DESC tFilter = GetFilter();
	bool bFilterChanged{};
	if (auto* pPhysXManager = CGameInstance::Get().GetPhysXManager())
	{
		bFilterChanged |= pPhysXManager->EditCollisionLayerGUI(
			"Layer", tFilter.iLayer);
		bFilterChanged |= pPhysXManager->EditCollisionLayerMaskGUI(
			"Simulation Mask", tFilter.iSimulationMask);
		bFilterChanged |= pPhysXManager->EditCollisionLayerMaskGUI(
			"Query Mask", tFilter.iQueryMask);
	}
	if (bFilterChanged)
		SetFilter(tFilter);

	static float s_fTestDisplacement[3]{};
	static float s_fTestTimeStep = 1.f / 60.f;
	ImGui::Separator();
	ImGui::DragFloat3("Test Displacement", s_fTestDisplacement, 0.01f);
	ImGui::DragFloat("Test Time Step", &s_fTestTimeStep, 0.001f, 0.0001f, 1.f, "%.4f");
	if (ImGui::Button("Move Once"))
	{
		Move(
			{ s_fTestDisplacement[0], s_fTestDisplacement[1], s_fTestDisplacement[2] },
			s_fTestTimeStep);
	}

	ImGui::PopID();
}

CComPxCharacterController::CComPxCharacterController() { }
CComPxCharacterController::CComPxCharacterController(const CComPxCharacterController& rhs)
	: CComponent{rhs}
{
}
CComPxCharacterController::~CComPxCharacterController() { }

PxRigidActor* CComPxCharacterController::GetActor() const
{
	return m_pController ? m_pController->getActor() : nullptr;
}

void CComPxCharacterController::RegisterJoint(
	CComPxJoint* pJoint)
{
	if (pJoint)
		m_Joints.insert(pJoint);
}

void CComPxCharacterController::UnregisterJoint(
	CComPxJoint* pJoint)
{
	if (pJoint)
		m_Joints.erase(pJoint);
}

void CComPxCharacterController::ReleaseConnectedJoints()
{
	while (!m_Joints.empty())
	{
		CComPxJoint* pJoint = *m_Joints.begin();
		if (!pJoint)
		{
			m_Joints.erase(m_Joints.begin());
			continue;
		}

		pJoint->OnCharacterControllerReleased(this);
	}
}

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

	m_tFilter = pDesc->tFilter;
	m_iShapeSubIndex = pDesc->iShapeSubIndex;
	
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

	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	auto* pActor = m_pController->getActor();
	if (!pActor)
		return E_FAIL;

	PxShape* pShape{};
	if (pActor->getShapes(&pShape, 1) != 1 || !pShape)
		return E_FAIL;

	PxFilterData simulationFilter{};
	simulationFilter.word0 = pDesc->tFilter.iLayer;
	simulationFilter.word1 = pDesc->tFilter.iSimulationMask;
	simulationFilter.word2 = pDesc->tFilter.iNotifyFlags;
	pShape->setSimulationFilterData(simulationFilter);

	PxFilterData queryFilter{};
	queryFilter.word0 = pDesc->tFilter.iLayer;
	queryFilter.word1 = pDesc->tFilter.iQueryMask;
	pShape->setQueryFilterData(queryFilter);

	PX_ACTOR_USER_DATA userData{};
	userData.hGameObject = GetGameObject()->GetHandle();
	userData.eType = PX_ACTOR_TYPE::CHARACTER_CONTROLLER;
	if (!pPhysXManager || !pPhysXManager->RegisterActor(pActor, userData))
		return E_FAIL;

	PX_SHAPE_USER_DATA shapeUserData{};
	shapeUserData.hGameObject = GetGameObject()->GetHandle();
	shapeUserData.eType = PX_SHAPE_TYPE::CAPSULE;
	shapeUserData.iSubIndex = m_iShapeSubIndex;
	if (!pPhysXManager->RegisterShape(pShape, shapeUserData))
	{
		pPhysXManager->UnregisterActor(pActor);
		return E_FAIL;
	}

	pActor->userData = nullptr;
	pShape->userData = nullptr;
	return S_OK;
}

PX_CCT_COLLISION_FLAG CComPxCharacterController::Move(const XMFLOAT3& vDisplacement, float fTimeStep, float fMinDistance)
{
	if (!m_pController || !m_pImpl)
		return PX_CCT_COLLISION_FLAG::NONE;

	const PxExtendedVec3 vPositionBeforeMove =
		m_pController->getPosition();
	m_pImpl->BeginMove();

	PxVec3 disp(vDisplacement.x, vDisplacement.y, vDisplacement.z);
	PxFilterData tMoveFilter{};
	tMoveFilter.word0 = m_tFilter.iQueryMask;
	// move 함수는 '속도'가 아니라 '변위(Displacement = Velocity * dt)'를 받습니다.
	m_pImpl->collisionFlags = m_pController->move(
		disp,
		fMinDistance,
		fTimeStep,
		PxControllerFilters(
			&tMoveFilter,
			m_pImpl.get(),
			m_pImpl.get())
	);

	// [LSY] 회전 문이 입력에 없던 횡이동을 추가했으면 반대 방향 CCT Sweep으로
	// 제거한다. 강제 위치 설정이 아니므로 다른 Shape가 막으면 안전하게 정지한다.
	m_pImpl->collisionFlags |=
		m_pImpl->RemoveUnrequestedHorizontalDisplacement(
			*m_pController,
			vPositionBeforeMove,
			vDisplacement,
			fMinDistance,
			fTimeStep,
			tMoveFilter);

	// [LSY] HitReport가 없는 단순 Standing 접촉도 같은 정책을 적용한다.
	if (!m_pImpl->bPreventUpwardProjection)
	{
		PxControllerState tState{};
		m_pController->getState(tState);
		auto* pManager = CGameInstance::Get().GetPhysXManager();
		CGameObject* pStandingObject = pManager && tState.touchedActor
			? pManager->FindGameObject(tState.touchedActor)
			: nullptr;
		CGameObject* pOwnerObject = GetGameObject();
		if (pOwnerObject && pStandingObject)
		{
			m_pImpl->bPreventUpwardProjection = HasCCTBehaviorFlag(
				pOwnerObject->GetCCTShapeBehavior(pStandingObject),
				PX_CCT_BEHAVIOR::PREVENT_UPWARD_PROJECTION);
		}
	}

	// [LSY] 사용자가 요청한 상승은 보존하고 충돌 투영이 추가한 상승만 제거한다.
	m_pImpl->ClampUnrequestedUpwardDisplacement(
		*m_pController,
		vPositionBeforeMove,
		vDisplacement);

	uint8_t iResult{};
	if (m_pImpl->collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_SIDES))
		iResult |= static_cast<uint8_t>(PX_CCT_COLLISION_FLAG::SIDE);
	if (m_pImpl->collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_UP))
		iResult |= static_cast<uint8_t>(PX_CCT_COLLISION_FLAG::UP);
	if (m_pImpl->collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN))
		iResult |= static_cast<uint8_t>(PX_CCT_COLLISION_FLAG::DOWN);

	return static_cast<PX_CCT_COLLISION_FLAG>(iResult);
}

bool CComPxCharacterController::IsGrounded() const
{
	return m_pImpl && m_pImpl->collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN);
}

bool CComPxCharacterController::IsCollidingUp() const
{
	return m_pImpl && m_pImpl->collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_UP);
}

bool CComPxCharacterController::IsCollidingSide() const
{
	return m_pImpl && m_pImpl->collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_SIDES);
}

std::optional<PX_CCT_STANDING_DATA> CComPxCharacterController::GetStandingShapeData() const
{
	if (!m_pController)
		return std::nullopt;

	PxControllerState state{};
	m_pController->getState(state);

	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager)
		return std::nullopt;

	if (state.touchedShape)
	{
		if (const auto userData = pPhysXManager->FindShapeUserData(state.touchedShape))
		{
			PX_CCT_STANDING_DATA tResult{};
			tResult.hGameObject = userData->hGameObject;
			tResult.eShapeType = userData->eType;
			tResult.iShapeSubIndex = userData->iSubIndex;
			tResult.bHasShapeData = true;
			return tResult;
		}
	}

	if (state.touchedActor)
	{
		if (const auto userData = pPhysXManager->FindActorUserData(state.touchedActor))
		{
			PX_CCT_STANDING_DATA tResult{};
			tResult.hGameObject = userData->hGameObject;
			return tResult;
		}
	}

	return std::nullopt;
}

std::optional<CHandle> CComPxCharacterController::GetStandingGameObjectHandle() const
{
	const auto tStandingData = GetStandingShapeData();
	if (!tStandingData)
		return std::nullopt;

	return tStandingData->hGameObject;
}

void CComPxCharacterController::SetPosition(const XMFLOAT3& vPosition)
{
	if (!m_pController)
		return;

	physx::PxExtendedVec3 pos = { vPosition.x, vPosition.y, vPosition.z };

	// 이 함수가 물리 엔진 내의 컨트롤러 위치를 즉시 이동시킵니다.
	m_pController->setPosition(pos);
}

_float3 CComPxCharacterController::GetPosition() const
{
	if (!m_pController)
		return {};

	const PxExtendedVec3 vPosition = m_pController->getPosition();
	return {
		static_cast<_float>(vPosition.x),
		static_cast<_float>(vPosition.y),
		static_cast<_float>(vPosition.z) };
}

_float3 CComPxCharacterController::GetFootPosition() const
{
	if (!m_pController)
		return {};

	const PxExtendedVec3 vPosition = m_pController->getFootPosition();
	return {
		static_cast<_float>(vPosition.x),
		static_cast<_float>(vPosition.y),
		static_cast<_float>(vPosition.z) };
}

_bool CComPxCharacterController::Resize(_float fHeight)
{
	if (!m_pController || fHeight <= 0.f)
		return false;

	m_pController->resize(fHeight);
	return true;
}

_bool CComPxCharacterController::SetRadius(_float fRadius)
{
	if (!m_pController || fRadius <= 0.f || m_pController->getType() != PxControllerShapeType::eCAPSULE)
		return false;

	return static_cast<PxCapsuleController*>(m_pController)->setRadius(fRadius);
}

void CComPxCharacterController::SetStepOffset(_float fStepOffset)
{
	if (m_pController && fStepOffset >= 0.f)
		m_pController->setStepOffset(fStepOffset);
}

_float CComPxCharacterController::GetStepOffset() const
{
	return m_pController ? m_pController->getStepOffset() : 0.f;
}

void CComPxCharacterController::SetSlopeLimit(_float fSlopeLimit)
{
	if (m_pController && fSlopeLimit >= 0.f && fSlopeLimit <= 1.f)
		m_pController->setSlopeLimit(fSlopeLimit);
}

_float CComPxCharacterController::GetSlopeLimit() const
{
	return m_pController ? m_pController->getSlopeLimit() : 0.f;
}

void CComPxCharacterController::SetContactOffset(_float fContactOffset)
{
	if (m_pController && fContactOffset > 0.f)
		m_pController->setContactOffset(fContactOffset);
}

_float CComPxCharacterController::GetContactOffset() const
{
	return m_pController ? m_pController->getContactOffset() : 0.f;
}

_bool CComPxCharacterController::SetFilter(const PX_FILTER_DESC& tFilter)
{
	if (!m_pController)
		return false;

	PxRigidDynamic* pActor = m_pController->getActor();
	if (!pActor)
		return false;

	PxShape* pShape{};
	if (pActor->getShapes(&pShape, 1) != 1 || !pShape)
		return false;

	m_tFilter = tFilter;

	PxFilterData tSimulationFilter{};
	tSimulationFilter.word0 = tFilter.iLayer;
	tSimulationFilter.word1 = tFilter.iSimulationMask;
	tSimulationFilter.word2 = tFilter.iNotifyFlags;
	pShape->setSimulationFilterData(tSimulationFilter);

	PxFilterData tQueryFilter{};
	tQueryFilter.word0 = tFilter.iLayer;
	tQueryFilter.word1 = tFilter.iQueryMask;
	pShape->setQueryFilterData(tQueryFilter);

	if (PxScene* pScene = pActor->getScene())
		pScene->resetFiltering(*pActor);
	m_pController->invalidateCache();

	return true;
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
	ReleaseConnectedJoints();

	if (m_pController)
	{
		if (auto* pActor = m_pController->getActor())
		{
			if (auto* pPhysXManager = CGameInstance::Get().GetPhysXManager())
			{
				PxShape* pShape{};
				if (pActor->getShapes(&pShape, 1) == 1 && pShape)
				{
					pPhysXManager->UnregisterShape(pShape);
					pShape->userData = nullptr;
				}

				pPhysXManager->UnregisterActor(pActor);
			}

			pActor->userData = nullptr;
		}

		m_pController->release();
		m_pController = nullptr;
	}
	m_pImpl.reset();
	CComponent::Free();
}
