#include "pch.h"
#include "PlayerRagdollController.h"

#include "Player.h"

#include "ComCharacterMotor.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
#include "ComPxBoxCollider.h"
#include "ComPxCharacterController.h"
#include "ComPxRagdoll.h"
#include "PhysXRagdollData.h"

NS_BEGIN(Client)

CPlayerRagdollController::CPlayerRagdollController(CPlayer& Owner)
	: m_Owner{ Owner }
{
}

HRESULT CPlayerRagdollController::Initialize()
{
	// [LSY] 에디터에서 제작한 랙돌 정의를 읽고 플레이어의 컴포넌트 컨테이너에 생성한다.
	PX_RAGDOLL_DESC tRagdoll{};
	if (FAILED(CGameInstance::Get().JsonDeSerialize(
		"./Resources/PhysX/Ragdolls/NewRagdoll.ragdoll.json", tRagdoll, "Ragdoll")))
	{
		return E_FAIL;
	}

	CComPxRagdoll::DESC Desc{};
	Desc.tRagdoll = std::move(tRagdoll);
	if (FAILED(m_Owner.AddComponentFromProto(
		ES_EngineProtoMajorType::PHYSX,
		ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRagdoll,
		"ComPxRagdoll", &Desc, &m_pComPxRagdoll)))
	{
		return E_FAIL;
	}

	if (!m_pComPxRagdoll)
		return E_FAIL;

	if (!m_Owner.m_pComModelInstance)
		return E_FAIL;

	auto pModel = m_Owner.m_pComModelInstance->GetModel();
	if (!pModel)
		return E_FAIL;

	if (!m_pComPxRagdoll->BindSkeleton(*pModel))
		return E_FAIL;

	// [LSY] 활성화 전부터 물리 바디가 현재 애니메이션 포즈와 같은 위치를 유지하게 한다.
	auto& CombinedBoneMatrices = m_Owner.m_pComModelInstance->Get_CombinedBoneMatrices();
	if (CombinedBoneMatrices.empty())
		return S_OK;

	if (!m_pComPxRagdoll->CacheAnimationPose(
		CombinedBoneMatrices, m_Owner.GetTransform().GetLoadedCombinedWorldMatrix()))
	{
		return E_FAIL;
	}

	if (!m_pComPxRagdoll->ApplyCachedPoseToKinematicBodies())
		return E_FAIL;

	return S_OK;
}

void CPlayerRagdollController::UpdateGUI()
{
	ImGui::Separator();
	const char* pState = "Kinematic";
	if (IsActive())
		pState = "Active";
	else if (m_bActivationRequested)
		pState = "Pending";

	ImGui::Text("Ragdoll: %s", pState);
	ImGui::DragFloat3("Ragdoll Linear Velocity", &m_vActivationLinearVelocity.x, 0.1f);
	ImGui::DragFloat3(
		"Ragdoll Angular Velocity (Rad/s)", &m_vActivationAngularVelocity.x, 0.1f);

	if (!IsTransitioning())
	{
		if (ImGui::Button("Activate Player Ragdoll"))
		{
			RequestActivation(m_vActivationLinearVelocity, m_vActivationAngularVelocity);
		}
	}
	else if (IsActive())
	{
		if (ImGui::Button("Reset Player Ragdoll"))
			Reset();
	}
}

_bool CPlayerRagdollController::RequestActivation(
	const _float3& vLinearVelocity,
	const _float3& vAngularVelocityRadians)
{
	if (!m_pComPxRagdoll)
		return false;

	if (m_pComPxRagdoll->IsRagdollActive())
		return true;

	m_vActivationLinearVelocity = vLinearVelocity;
	m_vActivationAngularVelocity = vAngularVelocityRadians;
	m_bActivationRequested = true;
	return true;
}

_bool CPlayerRagdollController::RequestFromCurrentMotion()
{
	_float3 vLinearVelocity{};
	if (m_Owner.m_pComCharacterMotor)
		vLinearVelocity = m_Owner.m_pComCharacterMotor->GetVelocity();

	// [LSY] CCT의 순간 속도를 그대로 넘기면 대시나 낙하 중 사망할 때
	// [LSY] 랙돌 전체가 과도하게 튀므로 자연스러운 관성 범위만 승계한다.
	vLinearVelocity.x = std::clamp(vLinearVelocity.x, -4.f, 4.f);
	vLinearVelocity.y = std::clamp(vLinearVelocity.y, -1.f, 2.f);
	vLinearVelocity.z = std::clamp(vLinearVelocity.z, -4.f, 4.f);

	return RequestActivation(vLinearVelocity);
}

_bool CPlayerRagdollController::TryActivateRequested()
{
	if (!m_bActivationRequested)
		return false;

	if (!m_pComPxRagdoll)
		return false;

	if (!m_pComPxRagdoll->ActivateRagdoll(
		m_vActivationLinearVelocity, m_vActivationAngularVelocity))
	{
		m_bActivationRequested = false;
		DEBUG_LOG("[Player][Ragdoll] Activation failed.\n");
		return false;
	}

	// [LSY] 랙돌과 CCT/HurtBox가 동시에 플레이어를 밀지 않도록 게임플레이 물리를 끈다.
	DisableGameplayPhysics();
	m_bActivationRequested = false;
	return true;
}

void CPlayerRagdollController::DisableGameplayPhysics()
{
	if (m_bGameplayPhysicsDisabled)
		return;

	m_bMovementLockedBeforeRagdoll = m_Owner.m_bMovementLocked;
	m_bRootMotionRotationBeforeRagdoll = m_Owner.m_bRootMotionRotationActive;
	m_bRootMotionTranslationBeforeRagdoll = m_Owner.m_bRootMotionTranslationActive;

	m_Owner.m_bMovementLocked = true;
	m_Owner.m_bRootMotionRotationActive = false;
	m_Owner.m_bRootMotionTranslationActive = false;
	m_Owner.m_bRawMoveInput = false;
	m_Owner.m_bSprintRequested = false;
	m_Owner.m_bWalkRequested = false;
	m_Owner.m_vRawMoveDirection = {};
	m_Owner.m_fCurrentMoveSpeed = 0.f;
	m_Owner.m_fControlHoldTime = 0.f;
	m_Owner.m_bDashTriggered = false;

	if (m_Owner.m_pComMoveIntent)
		m_Owner.m_pComMoveIntent->ClearMoveIntent();
	if (m_Owner.m_pComCharacterMotor)
		m_Owner.m_pComCharacterMotor->SetVelocity({});

	if (m_Owner.m_pComCharacterController)
	{
		m_tCCTFilterBeforeRagdoll = m_Owner.m_pComCharacterController->GetFilter();
		PX_FILTER_DESC tDisabledFilter{};
		tDisabledFilter.iLayer = 0;
		tDisabledFilter.iSimulationMask = 0;
		tDisabledFilter.iQueryMask = 0;
		m_Owner.m_pComCharacterController->SetFilter(tDisabledFilter);
	}

	if (m_Owner.m_pComPxBoxCollider)
	{
		m_bHurtBoxSimulationBeforeRagdoll =
			m_Owner.m_pComPxBoxCollider->IsSimulationEnabled();
		m_bHurtBoxQueryBeforeRagdoll = m_Owner.m_pComPxBoxCollider->IsQueryEnabled();
		m_Owner.m_pComPxBoxCollider->SetSimulationEnabled(false);
		m_Owner.m_pComPxBoxCollider->SetQueryEnabled(false);
	}

	m_bGameplayPhysicsDisabled = true;
}

void CPlayerRagdollController::RestoreGameplayPhysics()
{
	if (!m_bGameplayPhysicsDisabled)
		return;

	if (m_Owner.m_pComCharacterController)
	{
		// [LSY] 랙돌 종료 위치에서 CCT가 이어서 움직이도록 먼저 위치를 동기화한다.
		m_Owner.m_pComCharacterController->SetPosition(m_Owner.GetTransform().GetPosition());
		m_Owner.m_pComCharacterController->SetFilter(m_tCCTFilterBeforeRagdoll);
	}

	if (m_Owner.m_pComPxBoxCollider)
	{
		m_Owner.m_pComPxBoxCollider->SetSimulationEnabled(
			m_bHurtBoxSimulationBeforeRagdoll);
		m_Owner.m_pComPxBoxCollider->SetQueryEnabled(
			m_bHurtBoxQueryBeforeRagdoll);
	}

	if (m_Owner.m_pComMoveIntent)
		m_Owner.m_pComMoveIntent->ClearMoveIntent();
	if (m_Owner.m_pComCharacterMotor)
		m_Owner.m_pComCharacterMotor->SetVelocity({});

	m_Owner.m_bMovementLocked = m_bMovementLockedBeforeRagdoll;
	m_Owner.m_bRootMotionRotationActive = m_bRootMotionRotationBeforeRagdoll;
	m_Owner.m_bRootMotionTranslationActive = m_bRootMotionTranslationBeforeRagdoll;
	m_bGameplayPhysicsDisabled = false;
}

_bool CPlayerRagdollController::Reset()
{
	if (!m_pComPxRagdoll)
		return false;

	m_bActivationRequested = false;
	if (!m_pComPxRagdoll->ResetToKinematicPose())
		return false;

	RestoreGameplayPhysics();
	return true;
}

_bool CPlayerRagdollController::IsActive() const
{
	if (!m_pComPxRagdoll)
		return false;

	return m_pComPxRagdoll->IsRagdollActive();
}

_bool CPlayerRagdollController::IsTransitioning() const
{
	if (m_bActivationRequested)
		return true;

	return IsActive();
}

_bool CPlayerRagdollController::TryGetFollowPosition(_float3& OutPosition) const
{
	if (!IsActive())
		return false;

	_float4x4 BodyWorld{};
	if (!m_pComPxRagdoll->GetBodyWorldMatrix(RAGDOLL_FOLLOW_BODY_INDEX, BodyWorld))
		return false;

	OutPosition = { BodyWorld._41, BodyWorld._42, BodyWorld._43 };
	return true;
}

_bool CPlayerRagdollController::PrePriorityUpdate()
{
	if (!IsTransitioning())
		return false;

	m_Owner.m_bRawMoveInput = false;
	m_Owner.m_bSprintRequested = false;
	m_Owner.m_bWalkRequested = false;
	m_Owner.m_vRawMoveDirection = {};
	m_Owner.m_fCurrentMoveSpeed = 0.f;
	m_Owner.m_fControlHoldTime = 0.f;
	m_Owner.m_bDashTriggered = false;
	if (m_Owner.m_pComMoveIntent)
		m_Owner.m_pComMoveIntent->ClearMoveIntent();

	return true;
}

_bool CPlayerRagdollController::PreFixedUpdate()
{
	if (!IsTransitioning())
		return false;

	if (m_Owner.m_pComMoveIntent)
		m_Owner.m_pComMoveIntent->ClearMoveIntent();
	if (m_Owner.m_pComCharacterMotor)
		m_Owner.m_pComCharacterMotor->SetVelocity({});

	if (m_pComPxRagdoll)
	{
		if (!m_pComPxRagdoll->IsRagdollActive())
			m_pComPxRagdoll->ApplyCachedPoseToKinematicBodies();
	}

	return true;
}

void CPlayerRagdollController::PostFixedUpdate()
{
	if (m_pComPxRagdoll)
		m_pComPxRagdoll->ApplyCachedPoseToKinematicBodies();
}

void CPlayerRagdollController::UpdatePoseBridge()
{
	if (!m_pComPxRagdoll)
		return;

	if (!m_Owner.m_pComModelInstance)
		return;

	auto& CombinedBoneMatrices = m_Owner.m_pComModelInstance->Get_CombinedBoneMatrices();
	if (m_pComPxRagdoll->IsRagdollActive())
	{
		// [LSY] 활성 랙돌은 PhysX 결과를 렌더링 본 행렬에 반영한다.
		m_pComPxRagdoll->WritePhysicsPoseToBones(
			CombinedBoneMatrices, m_Owner.GetTransform().GetLoadedCombinedWorldMatrix());
		return;
	}

	// [LSY] 비활성 랙돌은 애니메이션 포즈를 캐시한 뒤 요청된 전환을 수행한다.
	if (!m_pComPxRagdoll->CacheAnimationPose(
		CombinedBoneMatrices, m_Owner.GetTransform().GetLoadedCombinedWorldMatrix()))
		return;

	if (m_bActivationRequested)
		TryActivateRequested();
}

UPtr<CPlayerRagdollController>
CPlayerRagdollController::Create(CPlayer& Owner)
{
	auto pInstance = ToUPtr(new CPlayerRagdollController{ Owner });
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create: CPlayerRagdollController");
		return nullptr;
	}

	return pInstance;
}

NS_END
