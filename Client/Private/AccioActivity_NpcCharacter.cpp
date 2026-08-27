#include "pch.h"
#include "AccioActivity_NpcCharacter.h"

#include "Client_Resources.h"
#include "ComAnimator.h"
#include "ComCharacterMotor.h"
#include "ComCharacterMoveIntent.h"
#include "ComCollider.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComPxCharacterController.h"
#include "GameInstance.h"
#include "Player_Weapon.h"
#include "ResPhysXMaterial.h"
#include "Resources.h"

NS_USING(Client)

namespace
{
	constexpr _char IDLE_ANIMATION[] =
		"AN_ElegantStudent_PrettyGirl2_Rig_ESPG2_Hu_BM_LF_Idle_anm.bin";
	constexpr _char MOVE_ANIMATION[] =
		"AN_ElegantStudent_PrettyGirl2_Rig_ESPG2_Hu_BM_Jog_Loop_Fwd_anm.bin";
	constexpr _char AIM_ANIMATION[] =
		"AN_ElegantStudent_PrettyGirl2_Rig_ESPG2_Hu_Cmbt_RMB_WandReady_POSE_anm.bin";
	constexpr _char PULL_ANIMATION[] =
		"AN_ElegantStudent_PrettyGirl2_Rig_ESPG2_Hu_Cmbt_Atk_Cast_AccioPull_anm.bin";
}

CAccioActivity_NpcCharacter::CAccioActivity_NpcCharacter() = default;

CAccioActivity_NpcCharacter::CAccioActivity_NpcCharacter(
	const CAccioActivity_NpcCharacter& prototype)
	: CWorldAgent{ prototype }
{
}

HRESULT CAccioActivity_NpcCharacter::InitializePrototype(void* pArg)
{
	return CWorldAgent::InitializePrototype(pArg);
}

HRESULT CAccioActivity_NpcCharacter::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CAnimationObject::Initialize(pArg)))
		return E_FAIL;

	{
		CComConstantBuffer::DESC desc{};
		desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ConstantBuffer,
			"ComCBufferPerObject",
			&desc,
			&m_pComCBufferPerObject)))
		{
			return E_FAIL;
		}
	}

	{
		CComModelInstance::DESC desc{};
		desc.sGroupTag = pDesc->sResourceGroup;
		desc.sResTag = pDesc->sModelResourceTag;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ModelInstance,
			"ComCModelInstance",
			&desc,
			&m_pComModelInstance)))
		{
			return E_FAIL;
		}
	}

	{
		CComAnimator::DESC desc{};
		desc.sComTag = "ComCModelInstance";
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_Animator,
			"ComCModelAnimator",
			&desc,
			&m_pModelAnimator)))
		{
			return E_FAIL;
		}
	}

	{
		// [LSY] 렌더 컬링과 캐스케이드 그림자 Bounds에 사용할 가벼운 논리 Collider다.
		CComCollider::DESC desc{};
		desc.eCollType = CollType::Box;
		desc.vCenter = { 0.f, 1.f, 0.f };
		desc.vExtents = { 0.5f, 1.f, 0.5f };
		if (FAILED(AddComponentFromProto(
			"COLLIDER",
			"Prototype_Component_Collider",
			"ComCollider",
			&desc,
			&m_pComCollider)))
		{
			return E_FAIL;
		}
	}

	{
		// [LSY] 학생의 월드 충돌과 지면 추종은 Transform 순간이동이 아니라 CCT가 담당한다.
		CComPxCharacterController::DESC desc{};
		desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		desc.fHeight = pDesc->fCCTHeight;
		desc.fRadius = pDesc->fCCTRadius;
		desc.fStepOffset = pDesc->fCCTStepOffset;
		desc.vPosition = {
			pDesc->vInitialPosition.x + pDesc->vCCTCenterOffset.x,
			pDesc->vInitialPosition.y + pDesc->vCCTCenterOffset.y,
			pDesc->vInitialPosition.z + pDesc->vCCTCenterOffset.z
		};
		desc.tFilter = pDesc->tPhysicsFilter;
		if (!desc.pResMaterial || FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,
			"ComPxCharacterController",
			&desc,
			&m_pCharacterController)))
		{
			return E_FAIL;
		}
	}

	{
		CComCharacterMoveIntent::DESC desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMoveIntent,
			"ComCharacterMoveIntent",
			&desc,
			&m_pMoveIntent)))
		{
			return E_FAIL;
		}
	}

	{
		CComCharacterMotor::DESC desc{};
		desc.pMoveIntent = m_pMoveIntent;
		desc.pCharacterController = m_pCharacterController;
		desc.fGravity = -9.81f;
		desc.vControllerCenterOffset = pDesc->vCCTCenterOffset;
		desc.bUseGravity = true;
		desc.bSyncTransform = true;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMotor,
			"ComCharacterMotor",
			&desc,
			&m_pCharacterMotor)))
		{
			return E_FAIL;
		}
	}

	if (!m_pComModelInstance || !m_pComModelInstance->GetModel() || !m_pModelAnimator)
		return E_FAIL;

	m_iIdleAnimation = FindAnimationIndex(IDLE_ANIMATION);
	// [LSY] 다른 Skeleton의 공용 Walk Clip은 본 매핑과 루트 축이 달라 자세가 깨질 수 있다.
	// 이 학생 모델과 함께 변환된 전용 Jog Clip만 사용한다.
	m_iMoveAnimation = FindAnimationIndex(MOVE_ANIMATION);
	m_iAimAnimation = FindAnimationIndex(AIM_ANIMATION);
	m_iPullAnimation = FindAnimationIndex(PULL_ANIMATION);
	if (m_iIdleAnimation < 0 || m_iMoveAnimation < 0 || m_iPullAnimation < 0)
		return E_FAIL;
	if (m_iAimAnimation < 0)
		m_iAimAnimation = m_iIdleAnimation;

	m_fPullHoldRatio = std::clamp(pDesc->fPullHoldRatio, 0.f, 0.95f);
	m_fTurnSpeed = std::max(pDesc->fTurnSpeed, 1.f);
	m_vCCTCenterOffset = pDesc->vCCTCenterOffset;
	m_sResourceGroup = pDesc->sResourceGroup;
	m_sWeaponResourceTag = pDesc->sWeaponResourceTag;
	m_sWeaponLayerTag = pDesc->sWeaponLayerTag;
	m_iWandAttachBoneIndex = FindWandAttachBoneIndex();
	m_iWandTipBoneIndex =
		m_pComModelInstance->GetModel()->Get_BoneIndex("WandSocketTip");
	if (m_iWandAttachBoneIndex < 0)
		return E_FAIL;
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pModelAnimator->Build_BoneMatrices_CPU(0.f);

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	GetTransform().SetScale(pDesc->vInitialScale);
	GetTransform().Update();
	PlayAction(ACTION::IDLE, 0.f);
	return S_OK;
}

void CAccioActivity_NpcCharacter::OnRegisteredToManager()
{
	CPlayer_Weapon::WEAPON_DESC desc{};
	desc.sObjectTag = "AccioActivity_NpcCharacter_Wand";
	desc.LevelTag = m_sResourceGroup;
	desc.WeaponName = m_sWeaponResourceTag;
	desc.ParentHandle = GetHandle();
	desc.iBoneIndex = m_iWandAttachBoneIndex;
	desc.iSpawnBoneIndex = m_iWandTipBoneIndex;

	const auto hWeapon = CGameInstance::Get().AddGameObjectToLayer(
		m_sResourceGroup,
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerWeapon,
		m_sWeaponLayerTag,
		&desc);
	if (!hWeapon)
	{
		DEBUG_LOG("[AccioActivity] Failed to attach the student wand.\n");
		return;
	}

	// [LSY] 완드는 기존 Player 장비 객체를 재사용하며 외곽선 대상에는 등록하지 않는다.
	m_hWeapon = *hWeapon;
}

void CAccioActivity_NpcCharacter::FixedUpdate(_float fTimeDelta)
{
	if (m_pCharacterMotor)
		m_pCharacterMotor->FixedUpdate(fTimeDelta);

	// [LSY] AI가 다음 FixedUpdate용 이동 의도를 다시 기록하도록 1회 적용 후 비운다.
	if (m_pMoveIntent)
	{
		m_pMoveIntent->ClearMoveIntent();
		m_pMoveIntent->ClearFacingIntent();
	}
}

void CAccioActivity_NpcCharacter::PriorityUpdate(_float fTimeDelta)
{
	// [LSY] 이 Pawn은 CWorldAgent의 행동트리를 사용하지 않는다.
	// 부모 구현은 m_pBeHavior를 전제로 하므로 GameObject 기본 단계만 수행한다.
	CGameObject::PriorityUpdate(fTimeDelta);
}

void CAccioActivity_NpcCharacter::Update(_float fTimeDelta)
{
	CAnimationObject::Update(fTimeDelta);
	if (!m_pModelAnimator)
		return;

	m_pModelAnimator->Update(fTimeDelta);
	UpdatePullAnimation();
}

void CAccioActivity_NpcCharacter::LateUpdate(_float fTimeDelta)
{
	CGameObject::LateUpdate(fTimeDelta);
	GetTransform().Update();

	if (!m_pComModelInstance || !m_pModelAnimator ||
		!m_pComModelInstance->GetModel())
	{
		return;
	}

	CGameInstance::Get().Add_Instance(
		m_pComModelInstance,
		m_pModelAnimator,
		*GetTransform().GetCombinedWorldMatrix());
}

void CAccioActivity_NpcCharacter::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::Text("Action: %s", GetActionName(m_eAction));
	ImGui::Text("Animation Ratio: %.2f",
		m_pModelAnimator ? m_pModelAnimator->GetPlayAnimRatio() : 0.f);
	ImGui::DragFloat("Pull Hold Ratio", &m_fPullHoldRatio, 0.01f, 0.f, 0.95f, "%.2f");
	ImGui::DragFloat("Turn Speed", &m_fTurnSpeed, 1.f, 1.f, 720.f, "%.0f deg/s");
}

void CAccioActivity_NpcCharacter::SetAction(ACTION eAction)
{
	if (!m_pModelAnimator)
		return;

	if (m_eAction == ACTION::PULL && eAction != ACTION::PULL)
	{
		// [LSY] 당기는 동안 멈춘 자세에서 버튼을 놓으면 남은 AccioPull 동작을 끝낸다.
		m_ePendingAction = eAction;
		m_bFinishingPull = true;
		m_pModelAnimator->SetPlay(true);
		return;
	}

	if (m_eAction == eAction && !m_bFinishingPull &&
		!m_bDialogueAnimationPlaying)
		return;

	PlayAction(eAction);
}

void CAccioActivity_NpcCharacter::SetWorldPosition(const _float3& vWorldPosition)
{
	if (m_pMoveIntent)
		m_pMoveIntent->ClearMoveIntent();

	if (m_pCharacterController)
	{
		m_pCharacterController->SetPosition({
			vWorldPosition.x + m_vCCTCenterOffset.x,
			vWorldPosition.y + m_vCCTCenterOffset.y,
			vWorldPosition.z + m_vCCTCenterOffset.z
		});
	}

	GetTransform().SetPosition(vWorldPosition);
}

void CAccioActivity_NpcCharacter::SetMoveIntent(
	const _float3& vDirection,
	_float fSpeed)
{
	if (m_pMoveIntent)
		m_pMoveIntent->SetMoveIntent(vDirection, fSpeed);
}

void CAccioActivity_NpcCharacter::FaceTowards(const _float3& vWorldPosition)
{
	const _float3 vPosition = GetTransform().GetPosition();
	const _float3 vDirection{
		vWorldPosition.x - vPosition.x,
		0.f,
		vWorldPosition.z - vPosition.z
	};
	if (vDirection.x * vDirection.x + vDirection.z * vDirection.z <= FLT_EPSILON)
		return;

	if (m_pMoveIntent)
	{
		// [LSY] CharacterMotor가 초당 회전량을 제한해 급격한 방향 전환을 보간한다.
		m_pMoveIntent->SetFacingIntent(vDirection, m_fTurnSpeed);
		return;
	}

	const _float fYaw = XMConvertToDegrees(atan2f(vDirection.x, vDirection.z));
	GetTransform().SetRotationEuler({ 0.f, fYaw, 0.f });
}

_bool CAccioActivity_NpcCharacter::IsPullAnimationFinished() const
{
	return m_eAction != ACTION::PULL && !m_bFinishingPull;
}

_bool CAccioActivity_NpcCharacter::TryGetWandSpawnWorldMatrix(
	_float4x4& outWorld) const
{
	const auto* pWeapon = CGameInstance::Get().
		GetGameObjectByHandleT<CPlayer_Weapon>(m_hWeapon);
	if (!pWeapon || pWeapon->GetPendingDestroy())
		return false;

	outWorld = pWeapon->GetSpawnWorldMatrix();
	return true;
}

_bool CAccioActivity_NpcCharacter::PlayDialogueAnimation(
	const _string& sAnimationName,
	_bool bLoop)
{
	if (sAnimationName.empty() || !m_pModelAnimator)
		return false;

	const int32_t iAnimation = FindAnimationIndex(sAnimationName.c_str());
	if (iAnimation < 0)
		return false;

	m_bFinishingPull = false;
	m_bDialogueAnimationPlaying = true;
	m_pModelAnimator->Play_Anim(iAnimation, bLoop);
	return true;
}

int32_t CAccioActivity_NpcCharacter::FindAnimationIndex(
	const _char* pAnimationName) const
{
	if (!pAnimationName || !m_pComModelInstance ||
		!m_pComModelInstance->GetModel())
	{
		return -1;
	}

	const auto& animations = m_pComModelInstance->GetModel()->GetAnimations();
	for (size_t i = 0; i < animations.size(); ++i)
	{
		if (animations[i] && animations[i]->GetAnimName() == pAnimationName)
			return static_cast<int32_t>(i);
	}
	return -1;
}

int32_t CAccioActivity_NpcCharacter::FindWandAttachBoneIndex() const
{
	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return -1;

	const _char* pCandidates[] =
	{
		"RightHandWandSocket",
		"SKT_FX_RightHandSocket",
		"SKT_RightHandSocket",
		"RightHand",
		"SKT_RightHand"
	};

	for (const _char* pBoneName : pCandidates)
	{
		const int32_t iBoneIndex =
			m_pComModelInstance->GetModel()->Get_BoneIndex(pBoneName);
		if (iBoneIndex >= 0)
			return iBoneIndex;
	}

	return -1;
}

void CAccioActivity_NpcCharacter::PlayAction(
	ACTION eAction,
	_float fBlendDuration)
{
	int32_t iAnimation = m_iIdleAnimation;
	_bool bLoop = true;
	switch (eAction)
	{
	case ACTION::MOVE:
		iAnimation = m_iMoveAnimation;
		break;
	case ACTION::AIM:
		iAnimation = m_iAimAnimation;
		break;
	case ACTION::PULL:
		iAnimation = m_iPullAnimation;
		bLoop = false;
		break;
	case ACTION::IDLE:
	default:
		break;
	}

	if (iAnimation < 0)
		return;

	m_eAction = eAction;
	m_ePendingAction = eAction;
	m_bFinishingPull = false;
	m_bDialogueAnimationPlaying = false;
	m_pModelAnimator->SetPlay(true);
	m_pModelAnimator->Play_Anim(iAnimation, bLoop, fBlendDuration);
}

void CAccioActivity_NpcCharacter::UpdatePullAnimation()
{
	if (m_eAction != ACTION::PULL || !m_pModelAnimator)
		return;

	const _float fRatio = m_pModelAnimator->GetPlayAnimRatio();
	if (m_bFinishingPull)
	{
		if (fRatio >= 0.99f)
			PlayAction(m_ePendingAction);
		return;
	}

	if (fRatio >= m_fPullHoldRatio)
		m_pModelAnimator->SetPlay(false);
}

const _char* CAccioActivity_NpcCharacter::GetActionName(ACTION eAction)
{
	switch (eAction)
	{
	case ACTION::IDLE:
		return "Idle";
	case ACTION::MOVE:
		return "Move";
	case ACTION::AIM:
		return "Aim";
	case ACTION::PULL:
		return "Pull";
	default:
		return "Unknown";
	}
}

UPtr<CAccioActivity_NpcCharacter> CAccioActivity_NpcCharacter::Create()
{
	auto pInstance = ToUPtr(new CAccioActivity_NpcCharacter{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CAccioActivity_NpcCharacter::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CAccioActivity_NpcCharacter{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
