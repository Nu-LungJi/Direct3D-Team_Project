#include "pch.h"
#include "NpcRagdollController.h"

#include "WorldAgent.h"

#include "ComCharacterMotor.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
#include "ComPxCharacterController.h"
#include "ComPxRagdoll.h"
#include "ComPxSphereCollider.h"
#include "PhysXRagdollData.h"

NS_BEGIN(Client)

CNpcRagdollController::CNpcRagdollController(CWorldAgent& Owner)
	: m_Owner{ Owner }
{
}

HRESULT CNpcRagdollController::Initialize()
{
	PX_RAGDOLL_DESC tRagdoll{};
	if (FAILED(CGameInstance::Get().JsonDeSerialize(
		"./Resources/PhysX/Ragdolls/NewRagdoll.ragdoll.json",
		tRagdoll,
		"Ragdoll")))
	{
		return E_FAIL;
	}

	CComPxRagdoll::DESC Desc{};
	Desc.tRagdoll = std::move(tRagdoll);
	if (FAILED(m_Owner.AddComponentFromProto(
		ES_EngineProtoMajorType::PHYSX,
		ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRagdoll,
		"ComPxNpcRagdoll",
		&Desc,
		&m_pComPxRagdoll)))
	{
		return E_FAIL;
	}

	if (!m_pComPxRagdoll || !m_Owner.m_pComModelInstance)
		return E_FAIL;

	const auto pModel = m_Owner.m_pComModelInstance->GetModel();
	if (!pModel || !m_pComPxRagdoll->BindSkeleton(*pModel))
	{
		DEBUG_LOG("[NPC][Ragdoll] Skeleton binding failed.\n");
		return E_FAIL;
	}

	auto& CombinedBoneMatrices =
		m_Owner.m_pComModelInstance->Get_CombinedBoneMatrices();
	if (CombinedBoneMatrices.empty())
		return S_OK;

	if (!m_pComPxRagdoll->CacheAnimationPose(
		CombinedBoneMatrices,
		m_Owner.GetTransform().GetLoadedCombinedWorldMatrix()) ||
		!m_pComPxRagdoll->ApplyCachedPoseToKinematicBodies())
	{
		return E_FAIL;
	}

	return S_OK;
}

_bool CNpcRagdollController::EnsureInitialized()
{
	if (m_bInitialized)
		return true;
	if (m_bInitializationAttempted)
		return false;

	m_bInitializationAttempted = true;
	m_bInitialized = SUCCEEDED(Initialize());
	return m_bInitialized;
}

void CNpcRagdollController::UpdateGUI()
{
	ImGui::Separator();
	const char* pState = "Kinematic";
	if (IsActive())
		pState = "Active";
	else if (m_bActivationRequested)
		pState = "Pending";

	ImGui::Text("NPC Ragdoll: %s", pState);
	ImGui::DragFloat3(
		"NPC Ragdoll Linear Velocity",
		&m_vActivationLinearVelocity.x,
		0.1f);
	ImGui::DragFloat3(
		"NPC Ragdoll Angular Velocity (Rad/s)",
		&m_vActivationAngularVelocity.x,
		0.1f);

	if (!IsTransitioning())
	{
		if (ImGui::Button("Activate NPC Ragdoll"))
		{
			RequestActivation(
				m_vActivationLinearVelocity,
				m_vActivationAngularVelocity);
		}
	}
	else if (IsActive())
	{
		if (ImGui::Button("Reset NPC Ragdoll"))
			Reset();
	}
}

_bool CNpcRagdollController::RequestActivation(
	const _float3& vLinearVelocity,
	const _float3& vAngularVelocityRadians)
{
	// 월드의 모든 NPC에 16개 바디와 15개 조인트를 미리 만들지 않고,
	// 실제로 랙돌이 필요한 첫 순간에만 PhysX 컴포넌트를 생성한다.
	if (!EnsureInitialized() || !m_pComPxRagdoll)
		return false;

	if (m_pComPxRagdoll->IsRagdollActive())
		return true;

	m_vActivationLinearVelocity = vLinearVelocity;
	m_vActivationAngularVelocity = vAngularVelocityRadians;
	m_bActivationRequested = true;
	return true;
}

_bool CNpcRagdollController::RequestFromCurrentMotion()
{
	_float3 vLinearVelocity{};
	if (m_Owner.m_pCharacterMotor)
		vLinearVelocity = m_Owner.m_pCharacterMotor->GetVelocity();

	// CCT의 순간 속도를 그대로 넘겨 사망 랙돌이 과도하게 튀는 것을 막는다.
	vLinearVelocity.x = std::clamp(vLinearVelocity.x, -4.f, 4.f);
	vLinearVelocity.y = std::clamp(vLinearVelocity.y, -1.f, 2.f);
	vLinearVelocity.z = std::clamp(vLinearVelocity.z, -4.f, 4.f);
	return RequestActivation(vLinearVelocity);
}

_bool CNpcRagdollController::TryActivateRequested()
{
	if (!m_bActivationRequested || !m_pComPxRagdoll)
		return false;

	if (!m_pComPxRagdoll->ActivateRagdoll(
		m_vActivationLinearVelocity,
		m_vActivationAngularVelocity))
	{
		m_bActivationRequested = false;
		DEBUG_LOG("[NPC][Ragdoll] Activation failed.\n");
		return false;
	}

	DisableGameplayPhysics();
	m_bActivationRequested = false;
	return true;
}

void CNpcRagdollController::DisableGameplayPhysics()
{
	if (m_bGameplayPhysicsDisabled)
		return;

	m_bRootMotionRotationBeforeRagdoll =
		m_Owner.m_bRootMotionRotationActive;
	m_bRootMotionTranslationBeforeRagdoll =
		m_Owner.m_bRootMotionTranslationActive;
	m_Owner.m_bRootMotionRotationActive = false;
	m_Owner.m_bRootMotionTranslationActive = false;

	if (m_Owner.m_pMoveIntent)
	{
		m_Owner.m_pMoveIntent->ClearMoveIntent();
		m_Owner.m_pMoveIntent->ClearFacingIntent();
	}
	if (m_Owner.m_pCharacterMotor)
		m_Owner.m_pCharacterMotor->SetVelocity({});

	if (m_Owner.m_pCharacterController)
	{
		m_tCCTFilterBeforeRagdoll =
			m_Owner.m_pCharacterController->GetFilter();
		PX_FILTER_DESC tDisabledFilter{};
		m_Owner.m_pCharacterController->SetFilter(tDisabledFilter);
	}

	if (m_Owner.m_pComSphereCol)
	{
		m_bHurtBoxSimulationBeforeRagdoll =
			m_Owner.m_pComSphereCol->IsSimulationEnabled();
		m_bHurtBoxQueryBeforeRagdoll =
			m_Owner.m_pComSphereCol->IsQueryEnabled();
		m_Owner.m_pComSphereCol->SetSimulationEnabled(false);
		m_Owner.m_pComSphereCol->SetQueryEnabled(false);
	}

	m_bGameplayPhysicsDisabled = true;
}

void CNpcRagdollController::RestoreGameplayPhysics()
{
	if (!m_bGameplayPhysicsDisabled)
		return;

	if (m_Owner.m_pCharacterController)
	{
		m_Owner.m_pCharacterController->SetPosition(
			m_Owner.GetTransform().GetPosition());
		m_Owner.m_pCharacterController->SetFilter(
			m_tCCTFilterBeforeRagdoll);
	}

	if (m_Owner.m_pComSphereCol)
	{
		m_Owner.m_pComSphereCol->SetSimulationEnabled(
			m_bHurtBoxSimulationBeforeRagdoll);
		m_Owner.m_pComSphereCol->SetQueryEnabled(
			m_bHurtBoxQueryBeforeRagdoll);
	}

	if (m_Owner.m_pMoveIntent)
	{
		m_Owner.m_pMoveIntent->ClearMoveIntent();
		m_Owner.m_pMoveIntent->ClearFacingIntent();
	}
	if (m_Owner.m_pCharacterMotor)
		m_Owner.m_pCharacterMotor->SetVelocity({});

	m_Owner.m_bRootMotionRotationActive =
		m_bRootMotionRotationBeforeRagdoll;
	m_Owner.m_bRootMotionTranslationActive =
		m_bRootMotionTranslationBeforeRagdoll;
	m_bGameplayPhysicsDisabled = false;
}

_bool CNpcRagdollController::Reset()
{
	if (!m_pComPxRagdoll)
		return false;

	m_bActivationRequested = false;
	if (!m_pComPxRagdoll->ResetToKinematicPose())
		return false;

	RestoreGameplayPhysics();
	return true;
}

_bool CNpcRagdollController::IsActive() const
{
	return m_pComPxRagdoll &&
		m_pComPxRagdoll->IsRagdollActive();
}

_bool CNpcRagdollController::IsTransitioning() const
{
	return m_bActivationRequested || IsActive();
}

_bool CNpcRagdollController::PrePriorityUpdate()
{
	if (!IsTransitioning())
		return false;

	if (m_Owner.m_pMoveIntent)
	{
		m_Owner.m_pMoveIntent->ClearMoveIntent();
		m_Owner.m_pMoveIntent->ClearFacingIntent();
	}
	return true;
}

_bool CNpcRagdollController::PreFixedUpdate()
{
	if (!IsTransitioning())
		return false;

	if (m_Owner.m_pMoveIntent)
	{
		m_Owner.m_pMoveIntent->ClearMoveIntent();
		m_Owner.m_pMoveIntent->ClearFacingIntent();
	}
	if (m_Owner.m_pCharacterMotor)
		m_Owner.m_pCharacterMotor->SetVelocity({});

	if (m_pComPxRagdoll && !m_pComPxRagdoll->IsRagdollActive())
		m_pComPxRagdoll->ApplyCachedPoseToKinematicBodies();
	return true;
}

void CNpcRagdollController::PostFixedUpdate()
{
	if (m_pComPxRagdoll && !m_pComPxRagdoll->IsRagdollActive())
		m_pComPxRagdoll->ApplyCachedPoseToKinematicBodies();
}

void CNpcRagdollController::UpdatePoseBridge()
{
	if (!m_pComPxRagdoll || !m_Owner.m_pComModelInstance)
		return;

	auto& CombinedBoneMatrices =
		m_Owner.m_pComModelInstance->Get_CombinedBoneMatrices();
	if (m_pComPxRagdoll->IsRagdollActive())
	{
		m_pComPxRagdoll->WritePhysicsPoseToBones(
			CombinedBoneMatrices,
			m_Owner.GetTransform().GetLoadedCombinedWorldMatrix());
		return;
	}

	if (!m_pComPxRagdoll->CacheAnimationPose(
		CombinedBoneMatrices,
		m_Owner.GetTransform().GetLoadedCombinedWorldMatrix()))
	{
		return;
	}

	if (m_bActivationRequested)
		TryActivateRequested();
}

UPtr<CNpcRagdollController> CNpcRagdollController::Create(
	CWorldAgent& Owner)
{
	auto pInstance = ToUPtr(new CNpcRagdollController{ Owner });
	return pInstance;
}

NS_END
