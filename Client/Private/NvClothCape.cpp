#include "pch.h"
#include "NvClothCape.h"

#include "ComConstantBuffer.h"
#include "ComCharacterMotor.h"
#include "ComModelInstance.h"
#include "ComNvCloth.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "NvClothManager.h"
#include "Player.h"
#include "Player_Broom.h"
#include "ResModel.h"
#include "ResModelBone.h"
#include "ResNvClothMesh.h"
#include "Resources.h"

NS_USING(Client)

namespace Client::NvClothCapeDetail
{
	_bool MakeRigidMatrix(
		_fmatrix Matrix,
		_matrix& OutRigidMatrix)
	{
		_vector vScale{};
		_vector qRotation{};
		_vector vTranslation{};
		if (!XMMatrixDecompose(
			&vScale,
			&qRotation,
			&vTranslation,
			Matrix))
		{
			return false;
		}

		qRotation = XMQuaternionNormalize(qRotation);
		OutRigidMatrix =
			XMMatrixRotationQuaternion(qRotation) *
			XMMatrixTranslationFromVector(vTranslation);
		return true;
	}

	_matrix MakePoseMatrix(
		const _float3& vPosition,
		const _float4& vRotation)
	{
		return
			XMMatrixRotationQuaternion(
				XMQuaternionNormalize(
					XMLoadFloat4(&vRotation))) *
			XMMatrixTranslation(
				vPosition.x,
				vPosition.y,
				vPosition.z);
	}
}

CNvClothCape::CNvClothCape()
	: CGameObject{}
{
	XMStoreFloat4x4(
		&m_ParentWorld,
		XMMatrixIdentity());
}

CNvClothCape::CNvClothCape(
	const CNvClothCape& Prototype)
	: CGameObject{ Prototype },
	  m_pVertexShader{ Prototype.m_pVertexShader },
	  m_pShadowVertexShader{
		  Prototype.m_pShadowVertexShader },
	  m_pPointShadowVertexShader{
		  Prototype.m_pPointShadowVertexShader },
	  m_pPixelShader{ Prototype.m_pPixelShader }
{
	XMStoreFloat4x4(
		&m_ParentWorld,
		XMMatrixIdentity());
}

CNvClothCape::~CNvClothCape()
{
}

HRESULT CNvClothCape::InitializePrototype(void*)
{
	m_pVertexShader =
		CGameInstance::Get().
		GetResourceFirst<CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_NvCloth");
	m_pPixelShader =
		CGameInstance::Get().
		GetResourceFirst<CResPixelShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"PS_NvCloth");
	m_pShadowVertexShader =
		CGameInstance::Get().
		GetResourceFirst<CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_NvClothShadow");
	m_pPointShadowVertexShader =
		CGameInstance::Get().
		GetResourceFirst<CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_NvClothPointShadow");
	return m_pVertexShader &&
		m_pShadowVertexShader &&
		m_pPointShadowVertexShader &&
		m_pPixelShader ?
		S_OK : E_FAIL;
}

HRESULT CNvClothCape::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<const DESC*>(pArg);
	if (!pDesc ||
		pDesc->sResourceGroup.hash == 0 ||
		pDesc->sModelResourceTag.hash == 0 ||
		pDesc->sClothMeshResourceTag.hash == 0 ||
		pDesc->sTargetModelComponentTag.hash == 0 ||
		pDesc->sAttachBoneName.empty() ||
		!std::isfinite(pDesc->fTeleportDistance) ||
		pDesc->fTeleportDistance < 0.f ||
		!std::isfinite(pDesc->fTeleportAngleDegrees) ||
		pDesc->fTeleportAngleDegrees < 0.f ||
		pDesc->fTeleportAngleDegrees > 180.f ||
		!std::isfinite(pDesc->fBackstopRadius) ||
		pDesc->fBackstopRadius <= 0.f ||
		!std::isfinite(pDesc->fBackstopOffset) ||
		!std::isfinite(
			pDesc->fBackstopFullRatio) ||
		pDesc->fBackstopFullRatio < 0.f ||
		pDesc->fBackstopFullRatio > 1.f ||
		!std::isfinite(
			pDesc->fBackstopFadeEndRatio) ||
		pDesc->fBackstopFadeEndRatio < 0.f ||
		pDesc->fBackstopFadeEndRatio > 1.f ||
		pDesc->fBackstopFullRatio >=
			pDesc->fBackstopFadeEndRatio ||
		!std::isfinite(
			pDesc->fBackstopFadeDepth) ||
		pDesc->fBackstopFadeDepth < 0.f ||
		!std::isfinite(pDesc->fBroomBackstopOffset) ||
		!std::isfinite(
			pDesc->fBroomBackstopFullRatio) ||
		pDesc->fBroomBackstopFullRatio < 0.f ||
		pDesc->fBroomBackstopFullRatio > 1.f ||
		!std::isfinite(
			pDesc->fBroomBackstopFadeEndRatio) ||
		pDesc->fBroomBackstopFadeEndRatio < 0.f ||
		pDesc->fBroomBackstopFadeEndRatio > 1.f ||
		pDesc->fBroomBackstopFullRatio >=
			pDesc->fBroomBackstopFadeEndRatio ||
		!std::isfinite(
			pDesc->fBroomBackstopFadeDepth) ||
		pDesc->fBroomBackstopFadeDepth < 0.f ||
		!std::isfinite(
			pDesc->fBroomBackstopFullInfluenceRatio) ||
		pDesc->fBroomBackstopFullInfluenceRatio < 0.f ||
		pDesc->fBroomBackstopFullInfluenceRatio > 1.f ||
		!std::isfinite(
			pDesc->fBroomBackstopMinimumInfluenceDepthRatio) ||
		pDesc->fBroomBackstopMinimumInfluenceDepthRatio < 0.f ||
		pDesc->fBroomBackstopMinimumInfluenceDepthRatio > 1.f ||
		pDesc->fBroomBackstopFullInfluenceRatio >=
			pDesc->fBroomBackstopMinimumInfluenceDepthRatio ||
		!std::isfinite(
			pDesc->fBroomBackstopMinimumInfluence) ||
		pDesc->fBroomBackstopMinimumInfluence < 0.f ||
		pDesc->fBroomBackstopMinimumInfluence > 1.f ||
		!std::isfinite(pDesc->fSelfCollisionDistance) ||
		pDesc->fSelfCollisionDistance < 0.f ||
		!std::isfinite(pDesc->fSelfCollisionStiffness) ||
		pDesc->fSelfCollisionStiffness < 0.f ||
		pDesc->fSelfCollisionStiffness > 1.f ||
		!std::isfinite(
			pDesc->fStructuralPhaseStiffness) ||
		pDesc->fStructuralPhaseStiffness < 0.f ||
		pDesc->fStructuralPhaseStiffness > 1.f ||
		!std::isfinite(
			pDesc->fShearingPhaseStiffness) ||
		pDesc->fShearingPhaseStiffness < 0.f ||
		pDesc->fShearingPhaseStiffness > 1.f ||
		!std::isfinite(
			pDesc->fBendingPhaseStiffness) ||
		pDesc->fBendingPhaseStiffness < 0.f ||
		pDesc->fBendingPhaseStiffness > 1.f ||
		!std::isfinite(pDesc->fVelocityWindScale) ||
		pDesc->fVelocityWindScale < 0.f ||
		!std::isfinite(
			pDesc->fVerticalVelocityWindScale) ||
		pDesc->fVerticalVelocityWindScale < 0.f ||
		pDesc->fVerticalVelocityWindScale > 1.f ||
		!std::isfinite(pDesc->fBroomVerticalInertia) ||
		pDesc->fBroomVerticalInertia < 0.f ||
		pDesc->fBroomVerticalInertia > 1.f ||
		!std::isfinite(pDesc->fBaseSolverFrequency) ||
		pDesc->fBaseSolverFrequency <= 0.f ||
		!std::isfinite(pDesc->fHighLoadSolverFrequency) ||
		pDesc->fHighLoadSolverFrequency <
			pDesc->fBaseSolverFrequency ||
		!std::isfinite(
			pDesc->fHighLoadEnterVerticalSpeed) ||
		pDesc->fHighLoadEnterVerticalSpeed < 0.f ||
		!std::isfinite(
			pDesc->fHighLoadExitVerticalSpeed) ||
		pDesc->fHighLoadExitVerticalSpeed < 0.f ||
		pDesc->fHighLoadExitVerticalSpeed >
			pDesc->fHighLoadEnterVerticalSpeed ||
		!std::isfinite(
			pDesc->fHighLoadEnterVerticalAcceleration) ||
		pDesc->fHighLoadEnterVerticalAcceleration < 0.f ||
		!std::isfinite(
			pDesc->fHighLoadExitVerticalAcceleration) ||
		pDesc->fHighLoadExitVerticalAcceleration < 0.f ||
		pDesc->fHighLoadExitVerticalAcceleration >
			pDesc->fHighLoadEnterVerticalAcceleration ||
		!std::isfinite(pDesc->fMaxWindSpeed) ||
		pDesc->fMaxWindSpeed < 0.f ||
		!std::isfinite(pDesc->fWindResponse) ||
		pDesc->fWindResponse <= 0.f ||
		!std::isfinite(pDesc->fWindDragCoefficient) ||
		pDesc->fWindDragCoefficient < 0.f ||
		pDesc->fWindDragCoefficient > 1.f ||
		!std::isfinite(pDesc->fWindLiftCoefficient) ||
		pDesc->fWindLiftCoefficient < 0.f ||
		pDesc->fWindLiftCoefficient > 1.f ||
		!std::isfinite(pDesc->fWindFluidDensity) ||
		pDesc->fWindFluidDensity < 0.f ||
		!std::isfinite(pDesc->fWindFlutterStrength) ||
		pDesc->fWindFlutterStrength < 0.f ||
		!std::isfinite(pDesc->fWindFlutterFrequency) ||
		pDesc->fWindFlutterFrequency <= 0.f ||
		!std::isfinite(pDesc->fWindGustStrength) ||
		pDesc->fWindGustStrength < 0.f ||
		pDesc->fWindGustStrength > 1.f ||
		!std::isfinite(pDesc->fWindGustFrequency) ||
		pDesc->fWindGustFrequency <= 0.f ||
		!std::isfinite(pDesc->fHighSpeedWindStart) ||
		pDesc->fHighSpeedWindStart < 0.f ||
		!std::isfinite(pDesc->fHighSpeedWindFull) ||
		pDesc->fHighSpeedWindFull <= pDesc->fHighSpeedWindStart ||
		!std::isfinite(pDesc->fHighSpeedFlutterStrength) ||
		pDesc->fHighSpeedFlutterStrength < 0.f ||
		!std::isfinite(pDesc->fHighSpeedFlutterFrequency) ||
		pDesc->fHighSpeedFlutterFrequency <= 0.f ||
		!std::isfinite(pDesc->fHighSpeedGustStrength) ||
		pDesc->fHighSpeedGustStrength < 0.f ||
		pDesc->fHighSpeedGustStrength > 1.f ||
		!std::isfinite(pDesc->fMotionConstraintScale) ||
		pDesc->fMotionConstraintScale < 0.f ||
		(!pDesc->tBodyCollisionRig.Shapes.empty() &&
			pDesc->tBodyCollisionRig.iVersion !=
				NVCLOTH_COLLISION_RIG_VERSION) ||
		(!pDesc->tBroomBodyCollisionRig.Shapes.empty() &&
			pDesc->tBroomBodyCollisionRig.iVersion !=
				NVCLOTH_COLLISION_RIG_VERSION) ||
		(!pDesc->tBroomObjectCollisionRig.Shapes.empty() &&
			pDesc->tBroomObjectCollisionRig.iVersion !=
				NVCLOTH_COLLISION_RIG_VERSION) ||
		FAILED(CGameObject::Initialize(pArg)))
	{
		return E_INVALIDARG;
	}

	m_hTarget = pDesc->hTarget;
	m_sTargetModelComponentTag =
		pDesc->sTargetModelComponentTag;
	m_sAttachBoneName =
		pDesc->sAttachBoneName;
	m_fTeleportDistance =
		pDesc->fTeleportDistance;
	m_fTeleportAngleDegrees =
		pDesc->fTeleportAngleDegrees;
	m_bUseBackstop =
		pDesc->bUseBackstop;
	m_bFlipBackstopNormal =
		pDesc->bFlipBackstopNormal;
	m_fBackstopRadius =
		pDesc->fBackstopRadius;
	m_fBackstopOffset =
		pDesc->fBackstopOffset;
	m_fBackstopFullRatio =
		pDesc->fBackstopFullRatio;
	m_fBackstopFadeEndRatio =
		pDesc->fBackstopFadeEndRatio;
	m_fBackstopFadeDepth =
		pDesc->fBackstopFadeDepth;
	m_fBroomBackstopOffset =
		pDesc->fBroomBackstopOffset;
	m_fBroomBackstopFullRatio =
		pDesc->fBroomBackstopFullRatio;
	m_fBroomBackstopFadeEndRatio =
		pDesc->fBroomBackstopFadeEndRatio;
	m_fBroomBackstopFadeDepth =
		pDesc->fBroomBackstopFadeDepth;
	m_fBroomBackstopFullInfluenceRatio =
		pDesc->fBroomBackstopFullInfluenceRatio;
	m_fBroomBackstopMinimumInfluenceDepthRatio =
		pDesc->fBroomBackstopMinimumInfluenceDepthRatio;
	m_fBroomBackstopMinimumInfluence =
		pDesc->fBroomBackstopMinimumInfluence;
	m_bUseVirtualParticles =
		pDesc->bUseVirtualParticles;
	m_bUseSelfCollision =
		pDesc->bUseSelfCollision;
	m_fSelfCollisionDistance =
		pDesc->fSelfCollisionDistance;
	m_fSelfCollisionStiffness =
		pDesc->fSelfCollisionStiffness;
	m_bUseVelocityWind =
		pDesc->bUseVelocityWind;
	m_fVelocityWindScale =
		pDesc->fVelocityWindScale;
	m_fVerticalVelocityWindScale =
		pDesc->fVerticalVelocityWindScale;
	m_fBroomVerticalInertia =
		pDesc->fBroomVerticalInertia;
	m_fBaseSolverFrequency =
		pDesc->fBaseSolverFrequency;
	m_fHighLoadSolverFrequency =
		pDesc->fHighLoadSolverFrequency;
	m_fHighLoadEnterVerticalSpeed =
		pDesc->fHighLoadEnterVerticalSpeed;
	m_fHighLoadExitVerticalSpeed =
		pDesc->fHighLoadExitVerticalSpeed;
	m_fHighLoadEnterVerticalAcceleration =
		pDesc->fHighLoadEnterVerticalAcceleration;
	m_fHighLoadExitVerticalAcceleration =
		pDesc->fHighLoadExitVerticalAcceleration;
	m_fMaxWindSpeed =
		pDesc->fMaxWindSpeed;
	m_fWindResponse =
		pDesc->fWindResponse;
	m_fWindDragCoefficient =
		pDesc->fWindDragCoefficient;
	m_fWindLiftCoefficient =
		pDesc->fWindLiftCoefficient;
	m_fWindFluidDensity =
		pDesc->fWindFluidDensity;
	m_bUseWindFlutter =
		pDesc->bUseWindFlutter;
	m_fWindFlutterStrength =
		pDesc->fWindFlutterStrength;
	m_fWindFlutterFrequency =
		pDesc->fWindFlutterFrequency;
	m_fWindGustStrength =
		pDesc->fWindGustStrength;
	m_fWindGustFrequency =
		pDesc->fWindGustFrequency;
	m_fHighSpeedWindStart =
		pDesc->fHighSpeedWindStart;
	m_fHighSpeedWindFull =
		pDesc->fHighSpeedWindFull;
	m_fHighSpeedFlutterStrength =
		pDesc->fHighSpeedFlutterStrength;
	m_fHighSpeedFlutterFrequency =
		pDesc->fHighSpeedFlutterFrequency;
	m_fHighSpeedGustStrength =
		pDesc->fHighSpeedGustStrength;
	m_fMotionConstraintScale =
		pDesc->fMotionConstraintScale;
	m_GroundBodyCollisionRig =
		pDesc->tBodyCollisionRig;
	m_BroomBodyCollisionRig =
		pDesc->tBroomBodyCollisionRig;
	m_BroomObjectCollisionRig =
		pDesc->tBroomObjectCollisionRig;
	m_BodyCollisionRig =
		m_GroundBodyCollisionRig;
	GetTransform().SetPosition(
		pDesc->vLocalPosition);

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = {
			TAG_RES_GRP_PERMANENT_BUFFER,
			TAG_RES_CBUFFER_OBJECT
		};
		if (FAILED(AddComponentFromProto(
			"PERMANENT",
			"Prototype_Component_ConstantBuffer",
			"ComCBufferPerObject",
			&Desc,
			&m_pComCBufferPerObject)))
		{
			return E_FAIL;
		}
	}

	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = pDesc->sResourceGroup;
		Desc.sResTag = pDesc->sModelResourceTag;
		if (FAILED(AddComponentFromProto(
			"PERMANENT",
			"Prototype_Component_ModelInstance",
			"ComModelInstance",
			&Desc,
			&m_pComModelInstance)))
		{
			return E_FAIL;
		}
	}

	m_pClothMesh =
		CGameInstance::Get().
		GetResourceFirst<CResNvClothMesh>(
			pDesc->sResourceGroup,
			pDesc->sClothMeshResourceTag);
	if (!m_pClothMesh ||
		m_pClothMesh->GetSections().empty())
	{
		return E_FAIL;
	}

	{
		CComNvCloth::DESC Desc{};
		Desc.tFabric =
			m_pClothMesh->GetFabricDesc();
		Desc.tCloth.vGravity =
			{ 0.f, -18.f, 0.f };
		Desc.tCloth.vDamping =
			{ 0.08f, 0.08f, 0.08f };
		// [LSY] 60Hz FixedUpdate에서 과도한 솔버 반복과 Self Collision 비용이
		// 발생하지 않도록 프리퀀시를 물리 갱신 주기와 동일하게 사용한다.
		Desc.tCloth.fSolverFrequency =
			m_fBaseSolverFrequency;
		Desc.tCloth.fStiffnessFrequency =
			m_fBaseSolverFrequency;
		Desc.tCloth.fPhaseStiffness = 0.9f;
		Desc.tCloth.tPhaseStiffness.fVertical =
			pDesc->fStructuralPhaseStiffness;
		Desc.tCloth.tPhaseStiffness.fHorizontal =
			pDesc->fStructuralPhaseStiffness;
		Desc.tCloth.tPhaseStiffness.fShearing =
			pDesc->fShearingPhaseStiffness;
		Desc.tCloth.tPhaseStiffness.fBending =
			pDesc->fBendingPhaseStiffness;
		Desc.tCloth.fStretchLimit = 1.05f;
		if (m_bUseSelfCollision)
		{
			Desc.tCloth.fSelfCollisionDistance =
				m_fSelfCollisionDistance;
			Desc.tCloth.fSelfCollisionStiffness =
				m_fSelfCollisionStiffness;
		}
		Desc.tCloth.bUseVirtualParticles =
			m_bUseVirtualParticles;
		Desc.tCloth.vLinearInertia =
			{ 1.f, 1.f, 1.f };
		Desc.tCloth.vAngularInertia =
			{ 0.8f, 0.8f, 0.8f };
		Desc.tCloth.vCentrifugalInertia =
			{ 0.8f, 0.8f, 0.8f };
		if (FAILED(AddComponentFromProto(
			"PHYSX",
			"Prototype_Component_ComNvCloth",
			"ComNvCloth",
			&Desc,
			&m_pComNvCloth)))
		{
			return E_FAIL;
		}
	}

	if (!ResolveAttachment() ||
		!UpdateAttachment(true) ||
		!UpdateBodyCollisions())
	{
		return E_FAIL;
	}

	return S_OK;
}

void CNvClothCape::PriorityUpdate(_float fTimeDelta)
{
	if (m_fClothWindImpulseRemaining > 0.f)
	{
		m_fClothWindImpulseRemaining = std::max(
			0.f,
			m_fClothWindImpulseRemaining - fTimeDelta);
		if (m_fClothWindImpulseRemaining <= 0.f)
			m_vClothWindImpulseVelocity = {};
	}
}

void CNvClothCape::FixedUpdate(_float fTimeDelta)
{
	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	auto* pPlayer = Cast<CPlayer>(pTarget);
	const _bool bOwnerRenderSuppressed =
		pPlayer && pPlayer->GetRenderInfluence();
	const _bool bSuppressionChanged =
		bOwnerRenderSuppressed !=
		m_bOwnerRenderSuppressed;
	COLLISION_RIG_PROFILE_CANDIDATE
		CollisionRigCandidate{};
	if (!PrepareCollisionRigProfile(
		pPlayer,
		CollisionRigCandidate))
	{
		return;
	}
	_bool bAppliedCandidateCollision{};
	if (CollisionRigCandidate.bChanged)
	{
		// [LSY] 기존 리그 상태로 이번 물리 틱의 Simulation Transform을 먼저
		// 맞춘 뒤 후보 Collision을 적용한다. 실패하면 기존 프로필을 유지한다.
		if (!UpdateAttachment(
			true,
			bOwnerRenderSuppressed ||
				bSuppressionChanged))
		{
			return;
		}
		if (!UpdateBodyCollisions(
			CollisionRigCandidate.Rig,
			CollisionRigCandidate.BoneIndices))
		{
			return;
		}
		bAppliedCandidateCollision = true;
		CommitCollisionRigProfile(
			std::move(CollisionRigCandidate));
	}
	if (!UpdateRuntimeSimulationProfile(
		fTimeDelta,
		pPlayer))
	{
		return;
	}

	const _bool bForceSimulationReset =
		bSuppressionChanged ||
		m_bCollisionRigProfileChanged;

	// [LSY] 대시 중에는 망토가 보이지 않으므로 이동 관성을 누적하지 않는다.
	// 종료 시에는 현재 애니메이션 자세로 복원한 뒤 다시 표시한다.
	if (!UpdateAttachment(
			true,
			bOwnerRenderSuppressed ||
				bForceSimulationReset))
	{
		return;
	}

	if (bForceSimulationReset &&
		!ResetSimulationToAnimationPose())
	{
		return;
	}

	if (!ValidateAndRecoverSimulation())
		return;

	if (!UpdateVirtualWind(
		fTimeDelta,
		pPlayer,
		bOwnerRenderSuppressed))
	{
		return;
	}

	// [LSY] 리그 전환 틱은 후보 Collision을 이미 적용했다. 일반 틱만 현재
	// Bone 자세를 반영해 Collision을 갱신한다.
	if (!bAppliedCandidateCollision &&
		!UpdateBodyCollisions())
	{
		return;
	}

	// [LSY] 중간 갱신 실패 시 완료 상태를 기록하지 않아 다음 Fixed Tick에서
	// 자세 복원과 런타임 설정을 다시 시도한다.
	m_bOwnerRenderSuppressed =
		bOwnerRenderSuppressed;
	m_bCollisionRigProfileChanged = false;
}

void CNvClothCape::Update(_float)
{
}

void CNvClothCape::LateUpdate(_float)
{
	// [LSY] Collision 프로필 전환이 끝나지 않은 틱의 부분 갱신 자세는
	// 렌더하지 않고 다음 FixedUpdate의 재동기화를 기다린다.
	if (!m_bRenderCape ||
		m_bCollisionRigProfileChanged)
	{
		return;
	}

	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return;
	if (auto* pPlayer = Cast<CPlayer>(pTarget))
	{
		// [LSY] 실시간 상태는 대시 진입을 즉시 반영하고, 캐시 상태는 FixedUpdate의
		// 자세 복원과 충돌 갱신이 모두 끝날 때까지 재표시를 보류한다.
		if (pPlayer->GetRenderInfluence() ||
			m_bOwnerRenderSuppressed)
		{
			return;
		}
	}

	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
	CGameInstance::Get().AddShadowRenderGroup(ACTORTYPE::DYNAMIC, this);
}

void CNvClothCape::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::PushID(this);

	ImGui::Checkbox(
		"Render Cape",
		&m_bRenderCape);
	ImGui::Text(
		"Simulation Recoveries: %u",
		m_iSimulationRecoveryCount);
	ImGui::Text(
		"Runaway Recoveries: %u",
		m_iSimulationRunawayRecoveryCount);
	ImGui::Text(
		"Simulation Distance Warnings: %u",
		m_iSimulationDistanceWarningCount);
	ImGui::Text(
		"Last Outliers: %zu, Max Distance: %.2f",
		m_iLastDistanceWarningParticleCount,
		m_fLastMaximumParticleDistance);
	const char* szSolverProfile = "Base";
	if (m_bHighLoadSolverEnabled)
		szSolverProfile = "Vertical High Load";
	ImGui::Text(
		"Solver Profile: %s",
		szSolverProfile);
	ImGui::Text(
		"Vertical Speed: %.2f, Filtered Acceleration: %.2f",
		m_fCurrentVerticalVelocity,
		m_fFilteredVerticalAcceleration);

	if (ImGui::TreeNode("Virtual Wind"))
	{
		ImGui::Checkbox(
			"Use Flight Velocity Wind",
			&m_bUseVelocityWind);
		ImGui::DragFloat(
			"Velocity Wind Scale",
			&m_fVelocityWindScale,
			0.05f,
			0.f,
			5.f);
		ImGui::DragFloat(
			"Vertical Velocity Wind Scale",
			&m_fVerticalVelocityWindScale,
			0.01f,
			0.f,
			1.f);
		ImGui::DragFloat(
			"Broom Vertical Inertia",
			&m_fBroomVerticalInertia,
			0.01f,
			0.f,
			1.f);
		ImGui::DragFloat(
			"Base Solver Frequency",
			&m_fBaseSolverFrequency,
			1.f,
			30.f,
			240.f);
		ImGui::DragFloat(
			"High Load Solver Frequency",
			&m_fHighLoadSolverFrequency,
			1.f,
			30.f,
			240.f);
		ImGui::DragFloat(
			"High Load Enter Vertical Speed",
			&m_fHighLoadEnterVerticalSpeed,
			0.1f,
			0.f,
			100.f);
		ImGui::DragFloat(
			"High Load Exit Vertical Speed",
			&m_fHighLoadExitVerticalSpeed,
			0.1f,
			0.f,
			100.f);
		ImGui::DragFloat(
			"High Load Enter Vertical Acceleration",
			&m_fHighLoadEnterVerticalAcceleration,
			0.1f,
			0.f,
			500.f);
		ImGui::DragFloat(
			"High Load Exit Vertical Acceleration",
			&m_fHighLoadExitVerticalAcceleration,
			0.1f,
			0.f,
			500.f);
		ImGui::DragFloat(
			"Max Wind Speed",
			&m_fMaxWindSpeed,
			0.5f,
			0.f,
			100.f);
		ImGui::DragFloat(
			"Wind Response",
			&m_fWindResponse,
			0.25f,
			0.1f,
			30.f);
		ImGui::DragFloat(
			"Wind Drag",
			&m_fWindDragCoefficient,
			0.01f,
			0.f,
			1.f);
		ImGui::DragFloat(
			"Wind Lift",
			&m_fWindLiftCoefficient,
			0.01f,
			0.f,
			1.f);
		ImGui::DragFloat(
			"Wind Fluid Density",
			&m_fWindFluidDensity,
			0.05f,
			0.f,
			5.f);
		ImGui::Checkbox(
			"Use Wind Flutter",
			&m_bUseWindFlutter);
		if (m_bUseWindFlutter)
		{
			ImGui::DragFloat(
				"Flutter Strength",
				&m_fWindFlutterStrength,
				0.02f,
				0.f,
				2.f);
			ImGui::DragFloat(
				"Flutter Frequency",
				&m_fWindFlutterFrequency,
				0.1f,
				0.1f,
				20.f);
			ImGui::DragFloat(
				"Gust Strength",
				&m_fWindGustStrength,
				0.02f,
				0.f,
				1.f);
			ImGui::DragFloat(
				"Gust Frequency",
				&m_fWindGustFrequency,
				0.05f,
				0.1f,
				5.f);
			ImGui::Text("High Speed Response");
			ImGui::DragFloat(
				"High Speed Start",
				&m_fHighSpeedWindStart,
				0.25f,
				0.f,
				100.f);
			ImGui::DragFloat(
				"High Speed Full",
				&m_fHighSpeedWindFull,
				0.25f,
				0.f,
				100.f);
			ImGui::DragFloat(
				"High Speed Flutter Strength",
				&m_fHighSpeedFlutterStrength,
				0.02f,
				0.f,
				2.f);
			ImGui::DragFloat(
				"High Speed Flutter Frequency",
				&m_fHighSpeedFlutterFrequency,
				0.1f,
				0.1f,
				20.f);
			ImGui::DragFloat(
				"High Speed Gust Strength",
				&m_fHighSpeedGustStrength,
				0.02f,
				0.f,
				1.f);
			ImGui::Text(
				"High Speed Blend: %.2f",
				m_fHighSpeedWindBlend);
		}
		ImGui::Text(
			"Current Wind: %.2f, %.2f, %.2f",
			m_vCurrentWindVelocity.x,
			m_vCurrentWindVelocity.y,
			m_vCurrentWindVelocity.z);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Animation Constraint"))
	{
		ImGui::DragFloat(
			"Motion Constraint Scale",
			&m_fMotionConstraintScale,
			0.02f,
			0.f,
			3.f);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Self Collision"))
	{
		_bool bSettingsChanged = ImGui::Checkbox(
			"Use Self Collision",
			&m_bUseSelfCollision);
		bSettingsChanged |= ImGui::DragFloat(
			"Self Collision Distance",
			&m_fSelfCollisionDistance,
			0.002f,
			0.f,
			0.5f);
		bSettingsChanged |= ImGui::DragFloat(
			"Self Collision Stiffness",
			&m_fSelfCollisionStiffness,
			0.01f,
			0.f,
			1.f);

		if (bSettingsChanged && m_pComNvCloth)
		{
			_float fDistance{};
			_float fStiffness{};
			if (m_bUseSelfCollision)
			{
				fDistance = m_fSelfCollisionDistance;
				fStiffness = m_fSelfCollisionStiffness;
			}
			m_pComNvCloth->SetSelfCollision(
				fDistance,
				fStiffness);
		}
		ImGui::TextDisabled(
			"Higher values reduce folding but cost more and can look stiff.");
		ImGui::TreePop();
	}

	auto vLocalPosition =
		GetTransform().GetPosition();
	if (ImGui::DragFloat3(
		"Cape Local Offset",
		&vLocalPosition.x,
		0.005f))
	{
		GetTransform().SetPosition(vLocalPosition);
		m_bSimulationTransformInitialized = false;
	}

	if (ImGui::TreeNode("Body Collision"))
	{
		ImGui::Checkbox(
			"Continuous Collision",
			&m_bContinuousBodyCollision);
		ImGui::DragFloat(
			"Collision Mass Scale",
			&m_fCollisionMassScale,
			0.1f,
			0.f,
			100.f);
		ImGui::DragFloat(
			"Collision Friction",
			&m_fCollisionFriction,
			0.01f,
			0.f,
			1.f);
		ImGui::Separator();
		ImGui::Checkbox(
			"Use Backstop",
			&m_bUseBackstop);
		ImGui::Checkbox(
			"Flip Backstop Normal",
			&m_bFlipBackstopNormal);
		ImGui::DragFloat(
			"Backstop Radius",
			&m_fBackstopRadius,
			0.005f,
			0.001f,
			20.f);
		ImGui::DragFloat(
			"Backstop Outward Offset",
			&m_fBackstopOffset,
			0.002f,
			-0.2f,
			0.2f);
		ImGui::DragFloat(
			"Backstop Full Ratio",
			&m_fBackstopFullRatio,
			0.01f,
			0.f,
			1.f);
		ImGui::DragFloat(
			"Backstop Fade End Ratio",
			&m_fBackstopFadeEndRatio,
			0.01f,
			0.f,
			1.f);
		ImGui::DragFloat(
			"Backstop Fade Depth",
			&m_fBackstopFadeDepth,
			0.01f,
			0.f,
			2.f);
		if (ImGui::TreeNode("Broom Backstop Profile"))
		{
			ImGui::DragFloat(
				"Broom Outward Offset",
				&m_fBroomBackstopOffset,
				0.002f,
				-0.2f,
				0.2f);
			ImGui::DragFloat(
				"Broom Full Ratio",
				&m_fBroomBackstopFullRatio,
				0.01f,
				0.f,
				1.f);
			ImGui::DragFloat(
				"Broom Fade End Ratio",
				&m_fBroomBackstopFadeEndRatio,
				0.01f,
				0.f,
				1.f);
			ImGui::DragFloat(
				"Broom Fade Depth",
				&m_fBroomBackstopFadeDepth,
				0.01f,
				0.f,
				2.f);
			ImGui::DragFloat(
				"Broom Full Influence Ratio",
				&m_fBroomBackstopFullInfluenceRatio,
				0.01f,
				0.f,
				1.f);
			ImGui::DragFloat(
				"Broom Minimum Influence Depth Ratio",
				&m_fBroomBackstopMinimumInfluenceDepthRatio,
				0.01f,
				0.f,
				1.f);
			ImGui::DragFloat(
				"Broom Minimum Influence",
				&m_fBroomBackstopMinimumInfluence,
				0.01f,
				0.f,
				1.f);
			ImGui::TreePop();
		}
		_bool bUseVirtualParticles =
			m_bUseVirtualParticles;
		if (ImGui::Checkbox(
				"Use Virtual Particles",
				&bUseVirtualParticles) &&
			m_pComNvCloth &&
			m_pComNvCloth->SetVirtualParticles(
				bUseVirtualParticles))
		{
			m_bUseVirtualParticles =
				bUseVirtualParticles;
		}
		ImGui::Checkbox(
			"Debug Body Collision",
			&m_bDebugBodyCollisions);
		ImGui::Text(
			"Collision Rig Shapes: %zu",
			m_BodyCollisionRig.Shapes.size());
		ImGui::Text(
			"Broom Object Rig Shapes: %zu",
			m_BroomObjectCollisionRig.Shapes.size());
		ImGui::Text(
			"Broom Object Bones: %zu / %zu",
			m_iResolvedBroomObjectShapeCount,
			m_BroomObjectCollisionRig.Shapes.size());
		if (m_bBroomObjectCollisionRequested)
		{
			if (m_bBroomObjectCollisionApplied)
			{
				ImGui::TextColored(
					{ 0.35f, 0.9f, 0.45f, 1.f },
					"Broom Object Collision: Applied");
			}
			else
			{
				ImGui::TextColored(
					{ 0.95f, 0.35f, 0.3f, 1.f },
					"Broom Object Collision: Failed");
			}
		}
		else
		{
			ImGui::TextDisabled(
				"Broom Object Collision: Inactive");
		}
		if (m_bUsingBroomCollisionRig)
			ImGui::TextUnformatted("Collision Profile: Broom");
		else
			ImGui::TextUnformatted("Collision Profile: Ground");
		ImGui::Text(
			"NvCloth: %zu spheres, %zu capsules, "
			"%zu planes, %zu convexes",
			m_LastBodyCollisionDesc.vecSpheres.size(),
			m_LastBodyCollisionDesc.vecCapsules.size(),
			m_LastBodyCollisionDesc.vecPlanes.size(),
			m_LastBodyCollisionDesc.vecConvexes.size());

		if (m_BodyCollisionRig.Shapes.empty())
		{
			ImGui::Separator();
			ImGui::TextUnformatted(
				"Fallback Bone Spheres");
			for (auto& Binding :
				m_BodyCollisionBones)
			{
				ImGui::DragFloat(
					Binding.sBoneName.c_str(),
					&Binding.fRadius,
					0.005f,
					0.01f,
					1.f);
			}
		}
		ImGui::TreePop();
	}

	if (m_bDebugBodyCollisions)
		DebugDrawBodyCollisions();

	ImGui::PopID();
}

void CNvClothCape::RequestClothWindImpulse(
	const _float3& vVelocity,
	_float fDuration)
{
	if (!std::isfinite(vVelocity.x) ||
		!std::isfinite(vVelocity.y) ||
		!std::isfinite(vVelocity.z) ||
		!std::isfinite(fDuration) ||
		fDuration <= 0.f)
	{
		return;
	}

	m_vClothWindImpulseVelocity = vVelocity;
	m_fClothWindImpulseRemaining = fDuration;
}

_bool CNvClothCape::ResolveAttachment()
{
	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return false;

	auto* pModelInstance =
		pTarget->GetComponent<CComModelInstance>(
			m_sTargetModelComponentTag);
	if (!pModelInstance ||
		!pModelInstance->GetModel())
	{
		return false;
	}

	m_iAttachBoneIndex =
		pModelInstance->GetModel()->
		Get_BoneIndex(m_sAttachBoneName.c_str());
	if (m_iAttachBoneIndex < 0)
		return false;

	const auto& SkinBoneNames =
		m_pClothMesh->GetSkinBoneNames();
	m_ResolvedSkinBoneIndices.resize(
		SkinBoneNames.size(),
		-1);
	m_SkinBoneToSimulationMatrices.resize(
		SkinBoneNames.size());
	size_t iResolvedSkinBoneCount{};
	for (size_t i = 0;
		i < SkinBoneNames.size();
		++i)
	{
		m_ResolvedSkinBoneIndices[i] =
			pModelInstance->GetModel()->
			Get_BoneIndex(
				SkinBoneNames[i].c_str());
		if (m_ResolvedSkinBoneIndices[i] >= 0)
			++iResolvedSkinBoneCount;
	}

	char szLog[256]{};
	sprintf_s(
		szLog,
		"[NvClothCape] Skin bones resolved: %zu / %zu. "
		"Particles without a matching bone use their Rest Pose.\n",
		iResolvedSkinBoneCount,
		SkinBoneNames.size());
	DEBUG_LOG(szLog);

	const char* szProfileName = "Ground Body";
	if (m_bUsingBroomCollisionRig)
		szProfileName = "Broom Body";
	if (!ResolveCollisionRigBones(
		m_BodyCollisionRig,
		*pModelInstance,
		m_CollisionRigBoneIndices,
		szProfileName))
		return false;

	for (auto& Binding : m_BodyCollisionBones)
	{
		Binding.iBoneIndex =
			pModelInstance->GetModel()->
			Get_BoneIndex(
				Binding.sBoneName.c_str());
		if (Binding.iBoneIndex < 0)
			return false;
	}

	return true;
}

_bool CNvClothCape::ResolveCollisionRigBones(
	const NVCLOTH_COLLISION_RIG_DESC& Rig,
	CComModelInstance& ModelInstance,
	std::vector<int32_t>& OutBoneIndices,
	const char* szProfileName)
{
	if (!ModelInstance.GetModel() || !szProfileName)
		return false;

	OutBoneIndices.assign(
		Rig.Shapes.size(),
		-1);
	size_t iResolvedShapeCount{};
	for (size_t i = 0;
		i < Rig.Shapes.size();
		++i)
	{
		OutBoneIndices[i] =
			ModelInstance.GetModel()->Get_BoneIndex(
				Rig.Shapes[i].
					sBoneName.c_str());
		if (OutBoneIndices[i] >= 0)
			++iResolvedShapeCount;
	}

	if (!Rig.Shapes.empty())
	{
		char szLog[256]{};
		sprintf_s(
			szLog,
			"[NvClothCape] %s collision rig shapes "
			"resolved: %zu / %zu.\n",
			szProfileName,
			iResolvedShapeCount,
			Rig.Shapes.size());
		DEBUG_LOG(szLog);
	}

	return true;
}

_bool CNvClothCape::PrepareCollisionRigProfile(
	CPlayer* pPlayer,
	COLLISION_RIG_PROFILE_CANDIDATE& OutCandidate)
{
	OutCandidate = {};
	const _bool bUseBroomRig =
		pPlayer &&
		pPlayer->IsBroomVisible() &&
		!m_BroomBodyCollisionRig.Shapes.empty();
	if (bUseBroomRig == m_bUsingBroomCollisionRig)
		return true;
	OutCandidate.bChanged = true;
	OutCandidate.bUseBroomRig = bUseBroomRig;

	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return false;

	auto* pModelInstance =
		pTarget->GetComponent<CComModelInstance>(
			m_sTargetModelComponentTag);
	if (!pModelInstance ||
		!pModelInstance->GetModel())
	{
		return false;
	}

	// [LSY] JSON은 초기화 때 이미 로드했다. 탑승 상태가 변할 때만
	// 메모리에 보관한 리그와 Bone 매핑을 교체한다.
	const auto* pTargetRig =
		&m_GroundBodyCollisionRig;
	const char* szProfileName = "Ground Body";
	if (bUseBroomRig)
	{
		pTargetRig = &m_BroomBodyCollisionRig;
		szProfileName = "Broom Body";
	}
	if (!ResolveCollisionRigBones(
		*pTargetRig,
		*pModelInstance,
		OutCandidate.BoneIndices,
		szProfileName))
	{
		return false;
	}
	OutCandidate.Rig = *pTargetRig;
	return true;
}

void CNvClothCape::CommitCollisionRigProfile(
	COLLISION_RIG_PROFILE_CANDIDATE&& Candidate)
{
	// [LSY] 후보 Collision이 NvCloth에 적용된 뒤에만 활성 프로필을 확정한다.
	m_bUsingBroomCollisionRig =
		Candidate.bUseBroomRig;
	m_BodyCollisionRig =
		std::move(Candidate.Rig);
	m_CollisionRigBoneIndices =
		std::move(Candidate.BoneIndices);
	m_bCollisionRigProfileChanged = true;
	m_bRuntimeSimulationProfileInitialized = false;
	m_bVerticalVelocityInitialized = false;
}

_bool CNvClothCape::UpdateAnimationConstraints(
	CComModelInstance& ModelInstance,
	_fmatrix AttachmentWorld,
	_bool bResetPreviousParticles)
{
	if (!m_pComNvCloth ||
		!m_pClothMesh ||
		!ModelInstance.GetModel())
	{
		return false;
	}

	const auto& Bindings =
		m_pClothMesh->GetParticleSkinBindings();
	const auto& RestPositions =
		m_pClothMesh->GetFabricDesc().vecPositions;
	if (Bindings.empty() ||
		Bindings.size() != RestPositions.size() ||
		m_ResolvedSkinBoneIndices.size() !=
			m_pClothMesh->GetSkinBoneNames().size() ||
		m_SkinBoneToSimulationMatrices.size() !=
			m_ResolvedSkinBoneIndices.size())
	{
		return false;
	}

	_vector vDeterminant{};
	const _matrix InverseAttachmentWorld =
		XMMatrixInverse(
			&vDeterminant,
			AttachmentWorld);
	if (!std::isfinite(XMVectorGetX(vDeterminant)) ||
		std::fabs(XMVectorGetX(vDeterminant)) <=
			FLT_EPSILON)
	{
		return false;
	}

	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return false;

	const _matrix TargetWorld =
		pTarget->GetTransform().
		GetLoadedCombinedWorldMatrix();
	for (size_t i = 0;
		i < m_ResolvedSkinBoneIndices.size();
		++i)
	{
		const int32_t iTargetBoneIndex =
			m_ResolvedSkinBoneIndices[i];
		if (iTargetBoneIndex < 0)
		{
			XMStoreFloat4x4(
				&m_SkinBoneToSimulationMatrices[i],
				XMMatrixIdentity());
			continue;
		}

		_matrix BoneMatrix{};
		if (!GetTargetBoneMatrix(
			ModelInstance,
			iTargetBoneIndex,
			BoneMatrix))
		{
			return false;
		}

		_matrix BoneWorldRigid{};
		if (!NvClothCapeDetail::MakeRigidMatrix(
			BoneMatrix * TargetWorld,
			BoneWorldRigid))
		{
			return false;
		}

		XMStoreFloat4x4(
			&m_SkinBoneToSimulationMatrices[i],
			BoneWorldRigid *
				InverseAttachmentWorld);
	}

	auto& Desc = m_AnimationConstraintDesc;
	Desc.vecTargetPositions.resize(Bindings.size());
	Desc.vecMaxDistances.resize(Bindings.size());
	if (m_bUseBackstop)
	{
		Desc.vecSeparationCenters.resize(
			Bindings.size());
		Desc.vecSeparationRadii.resize(
			Bindings.size());
	}
	else
	{
		Desc.vecSeparationCenters.clear();
		Desc.vecSeparationRadii.clear();
	}
	float fMaxAnimationDistance{};
	for (const auto& Binding : Bindings)
	{
		fMaxAnimationDistance =
			std::max(
				fMaxAnimationDistance,
				Binding.fMaxDistance *
					m_fMotionConstraintScale);
	}
	for (size_t i = 0;
		i < Bindings.size();
		++i)
	{
		_vector vTarget = XMVectorZero();
		_vector vNormal = XMVectorZero();
		float fTotalWeight{};
		for (const auto& Influence :
			Bindings[i].Influences)
		{
			if (Influence.fWeight <= 0.f ||
				Influence.iSourceBoneIndex >=
					m_SkinBoneToSimulationMatrices.size() ||
				m_ResolvedSkinBoneIndices[
					Influence.iSourceBoneIndex] < 0)
			{
				continue;
			}

			vTarget +=
				XMVector3TransformCoord(
					XMLoadFloat3(
						&Influence.vBoneLocalPosition),
					XMLoadFloat4x4(
						&m_SkinBoneToSimulationMatrices[
							Influence.iSourceBoneIndex])) *
				Influence.fWeight;
			vNormal +=
				XMVector3TransformNormal(
					XMLoadFloat3(
						&Influence.vBoneLocalNormal),
					XMLoadFloat4x4(
						&m_SkinBoneToSimulationMatrices[
							Influence.iSourceBoneIndex])) *
				Influence.fWeight;
			fTotalWeight += Influence.fWeight;
		}

		if (fTotalWeight > FLT_EPSILON)
		{
			vTarget /= fTotalWeight;
			vNormal /= fTotalWeight;
		}
		else
		{
			vTarget = XMLoadFloat3(&RestPositions[i]);
			vNormal = XMLoadFloat3(
				&Bindings[i].vRestSimulationNormal);
		}

		const float fNormalLengthSq =
			XMVectorGetX(
				XMVector3LengthSq(vNormal));
		if (!std::isfinite(fNormalLengthSq) ||
			fNormalLengthSq <= FLT_EPSILON)
		{
			vNormal = XMLoadFloat3(
				&Bindings[i].vRestSimulationNormal);
		}
		vNormal = XMVector3Normalize(vNormal);
		if (m_bFlipBackstopNormal)
			vNormal = -vNormal;

		XMStoreFloat3(
			&Desc.vecTargetPositions[i],
			vTarget);
		Desc.vecMaxDistances[i] =
			Bindings[i].fMaxDistance *
				m_fMotionConstraintScale;
		if (m_bUseBackstop)
		{
			const float fDepthRatio =
				fMaxAnimationDistance > FLT_EPSILON ?
				std::clamp(
					Desc.vecMaxDistances[i] /
						fMaxAnimationDistance,
					0.f,
					1.f) :
				0.f;
			float fProfileOffset =
				m_fBackstopOffset;
			float fProfileFullRatio =
				m_fBackstopFullRatio;
			float fProfileFadeEndRatio =
				m_fBackstopFadeEndRatio;
			float fProfileFadeDepth =
				m_fBackstopFadeDepth;
			float fBackstopInfluence = 1.f;
			if (m_bUsingBroomCollisionRig)
			{
				fProfileOffset =
					m_fBroomBackstopOffset;
				fProfileFullRatio =
					m_fBroomBackstopFullRatio;
				fProfileFadeEndRatio =
					m_fBroomBackstopFadeEndRatio;
				fProfileFadeDepth =
					m_fBroomBackstopFadeDepth;

				const float fInfluenceRatio =
					std::clamp(
						(fDepthRatio -
							m_fBroomBackstopFullInfluenceRatio) /
						std::max(
							m_fBroomBackstopMinimumInfluenceDepthRatio -
								m_fBroomBackstopFullInfluenceRatio,
							0.001f),
						0.f,
						1.f);
				const float fSmoothInfluence =
					fInfluenceRatio * fInfluenceRatio *
					(3.f - 2.f * fInfluenceRatio);
				const float fMinimumInfluence =
					std::clamp(
						m_fBroomBackstopMinimumInfluence,
						0.f,
						1.f);
				fBackstopInfluence =
					1.f - fSmoothInfluence *
						(1.f - fMinimumInfluence);
			}

			const float fRadius =
				m_fBackstopRadius *
				fBackstopInfluence;
			const float fFullRatio =
				std::clamp(
					fProfileFullRatio,
					0.f,
					1.f);
			const float fFadeEndRatio =
				std::clamp(
					std::max(
						fProfileFadeEndRatio,
						fFullRatio + 0.001f),
					0.f,
					1.f);
			const float fLinearFade =
				std::clamp(
					(fDepthRatio - fFullRatio) /
						std::max(
							fFadeEndRatio -
								fFullRatio,
							0.001f),
					0.f,
					1.f);
			const float fSmoothFade =
				fLinearFade * fLinearFade *
				(3.f - 2.f * fLinearFade);
			const float fOutwardOffset =
				std::clamp(
					fProfileOffset,
					-m_fBackstopRadius,
					m_fBackstopRadius * 0.95f);
			const float fFadeDepth =
				std::max(
					fProfileFadeDepth,
					0.f) *
				fSmoothFade;
			_vector vCenter = vTarget;
			if (fRadius > FLT_EPSILON)
			{
				vCenter -=
					vNormal *
						(fRadius -
							fOutwardOffset +
							fFadeDepth);
			}
			XMStoreFloat3(
				&Desc.vecSeparationCenters[i],
				vCenter);
			Desc.vecSeparationRadii[i] =
				fRadius;
		}
	}

	Desc.bUpdateFixedParticles = true;
	Desc.bResetPreviousParticles =
		bResetPreviousParticles ||
		!m_bAnimationConstraintInitialized;
	if (!m_pComNvCloth->
		SetAnimationConstraints(Desc))
	{
		return false;
	}

	m_bAnimationConstraintInitialized = true;
	return true;
}

_bool CNvClothCape::GetTargetBoneMatrix(
	CComModelInstance& ModelInstance,
	int32_t iBoneIndex,
	_matrix& OutBoneMatrix) const
{
	if (iBoneIndex < 0 ||
		!ModelInstance.GetModel())
	{
		return false;
	}

	const auto iIndex =
		static_cast<size_t>(iBoneIndex);
	const auto& CombinedBones =
		ModelInstance.Get_CombinedBoneMatrices();
	if (iIndex < CombinedBones.size())
	{
		OutBoneMatrix =
			XMLoadFloat4x4(
				&CombinedBones[iIndex]);
		return true;
	}

	const auto& Bones =
		ModelInstance.GetModel()->GetBones();
	if (iIndex >= Bones.size() ||
		!Bones[iIndex])
	{
		return false;
	}

	OutBoneMatrix =
		Bones[iIndex]->
		Get_CombinedTransformationMatrix();
	return true;
}

_bool CNvClothCape::AppendCollisionsFromRig(
	const NVCLOTH_COLLISION_RIG_DESC& Rig,
	const std::vector<int32_t>& BoneIndices,
	CComModelInstance& ModelInstance,
	_fmatrix TargetWorld,
	_fmatrix InverseSimulationWorld,
	NVCLOTH_COLLISION_DESC& OutDesc,
	std::vector<DEBUG_BODY_COLLISION_SHAPE>&
		OutDebugShapes)
{
	if (Rig.Shapes.empty() ||
		BoneIndices.size() != Rig.Shapes.size())
	{
		return false;
	}

	size_t iAppendedShapeCount{};
	for (size_t iShape = 0;
		iShape < Rig.Shapes.size();
		++iShape)
	{
		const int32_t iBoneIndex =
			BoneIndices[iShape];
		if (iBoneIndex < 0)
			continue;

		const auto& Shape = Rig.Shapes[iShape];
		if (!Shape.bEnabled)
			continue;

		_matrix BoneMatrix{};
		if (!GetTargetBoneMatrix(
			ModelInstance,
			iBoneIndex,
			BoneMatrix))
		{
			return false;
		}

		_matrix BoneWorld{};
		if (!NvClothCapeDetail::MakeRigidMatrix(
			BoneMatrix * TargetWorld,
			BoneWorld))
		{
			return false;
		}

		_matrix ShapeWorld{};
		if (!NvClothCapeDetail::MakeRigidMatrix(
			NvClothCapeDetail::MakePoseMatrix(
				Shape.vLocalPosition,
				Shape.vLocalRotation) *
			BoneWorld,
			ShapeWorld))
		{
			return false;
		}

		const _matrix ShapeToSimulation =
			ShapeWorld *
			InverseSimulationWorld;
		DEBUG_BODY_COLLISION_SHAPE DebugShape{};
		DebugShape.eType = Shape.eType;
		XMStoreFloat4x4(
			&DebugShape.SimulationPose,
			ShapeToSimulation);

		switch (Shape.eType)
		{
		case NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE:
		{
			if (OutDesc.vecSpheres.size() + 1 > 32)
				return false;

			NVCLOTH_COLLISION_SPHERE Sphere{};
			XMStoreFloat3(
				&Sphere.vCenter,
				XMVector3TransformCoord(
					XMVectorZero(),
					ShapeToSimulation));
			Sphere.fRadius =
				std::max(
					Shape.fRadius + Shape.fMargin,
					0.001f);
			OutDesc.vecSpheres.push_back(Sphere);
			DebugShape.fRadius = Sphere.fRadius;
			break;
		}

		case NVCLOTH_COLLISION_SHAPE_TYPE::CAPSULE:
		{
			if (OutDesc.vecSpheres.size() + 2 > 32)
				return false;

			const float fRadius =
				std::max(
					Shape.fRadius + Shape.fMargin,
					0.001f);
			const float fHalfHeight =
				std::max(Shape.fHalfHeight, 0.f);
			const uint32_t iSphere0 =
				static_cast<uint32_t>(
					OutDesc.vecSpheres.size());
			NVCLOTH_COLLISION_SPHERE Sphere0{};
			XMStoreFloat3(
				&Sphere0.vCenter,
				XMVector3TransformCoord(
					XMVectorSet(
						0.f,
						-fHalfHeight,
						0.f,
						1.f),
					ShapeToSimulation));
			Sphere0.fRadius = fRadius;
			OutDesc.vecSpheres.push_back(Sphere0);

			const uint32_t iSphere1 =
				static_cast<uint32_t>(
					OutDesc.vecSpheres.size());
			NVCLOTH_COLLISION_SPHERE Sphere1{};
			XMStoreFloat3(
				&Sphere1.vCenter,
				XMVector3TransformCoord(
					XMVectorSet(
						0.f,
						fHalfHeight,
						0.f,
						1.f),
					ShapeToSimulation));
			Sphere1.fRadius = fRadius;
			OutDesc.vecSpheres.push_back(Sphere1);
			OutDesc.vecCapsules.push_back({
				iSphere0,
				iSphere1
			});

			DebugShape.fRadius = fRadius;
			DebugShape.fHalfHeight = fHalfHeight;
			break;
		}

		case NVCLOTH_COLLISION_SHAPE_TYPE::BOX:
		{
			if (OutDesc.vecPlanes.size() + 6 > 32)
				return false;

			const _float3 vHalfExtents{
				std::max(
					Shape.vHalfExtents.x + Shape.fMargin,
					0.001f),
				std::max(
					Shape.vHalfExtents.y + Shape.fMargin,
					0.001f),
				std::max(
					Shape.vHalfExtents.z + Shape.fMargin,
					0.001f)
			};
			const _vector vCenter =
				XMVector3TransformCoord(
					XMVectorZero(),
					ShapeToSimulation);
			const _vector Axes[3]{
				XMVector3Normalize(
					XMVector3TransformNormal(
						XMVectorSet(
							1.f, 0.f, 0.f, 0.f),
						ShapeToSimulation)),
				XMVector3Normalize(
					XMVector3TransformNormal(
						XMVectorSet(
							0.f, 1.f, 0.f, 0.f),
						ShapeToSimulation)),
				XMVector3Normalize(
					XMVector3TransformNormal(
						XMVectorSet(
							0.f, 0.f, 1.f, 0.f),
						ShapeToSimulation))
			};
			const float Extents[3]{
				vHalfExtents.x,
				vHalfExtents.y,
				vHalfExtents.z
			};

			uint32_t iPlaneMask{};
			for (uint32_t iAxis = 0;
				iAxis < 3;
				++iAxis)
			{
				for (const float fSign :
					{ -1.f, 1.f })
				{
					const _vector vNormal =
						Axes[iAxis] * fSign;
					const _vector vPoint =
						vCenter +
						vNormal * Extents[iAxis];

					NVCLOTH_COLLISION_PLANE Plane{};
					XMStoreFloat3(
						&Plane.vNormal,
						vNormal);
					Plane.fDistance =
						-XMVectorGetX(
							XMVector3Dot(
								vNormal,
								vPoint));

					const uint32_t iPlane =
						static_cast<uint32_t>(
							OutDesc.vecPlanes.size());
					OutDesc.vecPlanes.push_back(
						Plane);
					iPlaneMask |= 1u << iPlane;
				}
			}
			OutDesc.vecConvexes.push_back({
				iPlaneMask
			});
			DebugShape.vHalfExtents = vHalfExtents;
			break;
		}
		}

		OutDebugShapes.push_back(
			DebugShape);
		++iAppendedShapeCount;
	}

	return iAppendedShapeCount > 0;
}

void CNvClothCape::DebugDrawBodyCollisions()
{
	auto* pDebug =
		CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const auto PreviousDepthMode =
		pDebug->GetDepthMode();
	const auto PreviousColor =
		pDebug->GetColor();
	pDebug->SetDepthTest(false);
	pDebug->SetColor(
		{ 0.1f, 0.9f, 1.f, 1.f });

	const _matrix SimulationWorld =
		GetTransform().
		GetLoadedCombinedWorldMatrix();
	for (size_t iShape = 0;
		iShape < m_DebugBodyCollisionShapes.size();
		++iShape)
	{
		if (iShape == m_iBroomDebugShapeStart)
		{
			// [LSY] 빗자루 전용 충돌은 몸 충돌과 구분되는 색으로 표시한다.
			pDebug->SetColor(
				{ 1.f, 0.55f, 0.05f, 1.f });
		}

		const auto& Shape =
			m_DebugBodyCollisionShapes[iShape];
		const _matrix ShapeWorld =
			XMLoadFloat4x4(
				&Shape.SimulationPose) *
			SimulationWorld;
		switch (Shape.eType)
		{
		case NVCLOTH_COLLISION_SHAPE_TYPE::BOX:
			pDebug->AddBox(
				Shape.vHalfExtents,
				ShapeWorld);
			break;
		case NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE:
			pDebug->AddSphere(
				Shape.fRadius,
				ShapeWorld);
			break;
		case NVCLOTH_COLLISION_SHAPE_TYPE::CAPSULE:
			pDebug->AddCapsule(
				Shape.fRadius,
				Shape.fHalfHeight,
				ShapeWorld);
			break;
		}
	}

	pDebug->SetColor(PreviousColor);
	pDebug->SetDepthMode(PreviousDepthMode);
}

_bool CNvClothCape::UpdateRuntimeSimulationProfile(
	_float fTimeDelta,
	CPlayer* pPlayer)
{
	if (!m_pComNvCloth ||
		!std::isfinite(fTimeDelta) ||
		fTimeDelta < 0.f)
	{
		return false;
	}

	_float fVerticalVelocity{};
	_float fRawVerticalAcceleration{};
	if (m_bUsingBroomCollisionRig && pPlayer)
	{
		const auto* pMotor = pPlayer->GetCharacterMotor();
		if (pMotor)
		{
			fVerticalVelocity = pMotor->GetVelocity().y;
			if (m_bVerticalVelocityInitialized &&
				fTimeDelta > FLT_EPSILON)
			{
				fRawVerticalAcceleration =
					(fVerticalVelocity -
						m_fPreviousVerticalVelocity) /
					fTimeDelta;
			}
			m_fPreviousVerticalVelocity =
				fVerticalVelocity;
			m_bVerticalVelocityInitialized = true;
		}
		else
		{
			m_fPreviousVerticalVelocity = 0.f;
			m_fFilteredVerticalAcceleration = 0.f;
			m_bVerticalVelocityInitialized = false;
		}
	}
	else
	{
		m_fPreviousVerticalVelocity = 0.f;
		m_fFilteredVerticalAcceleration = 0.f;
		m_bVerticalVelocityInitialized = false;
	}

	// [LSY] Motor 속도의 한 틱 차분값은 작은 흔들림도 큰 가속도로 증폭한다.
	// 저역 통과 필터를 거쳐 급격한 수직 이동만 고부하 Solver 전환에 사용한다.
	constexpr _float VERTICAL_ACCELERATION_RESPONSE = 12.f;
	const _float fAccelerationBlend =
		1.f - std::exp(
			-VERTICAL_ACCELERATION_RESPONSE *
			fTimeDelta);
	m_fFilteredVerticalAcceleration +=
		(fRawVerticalAcceleration -
			m_fFilteredVerticalAcceleration) *
		fAccelerationBlend;
	m_fCurrentVerticalVelocity =
		fVerticalVelocity;
	const _float fAbsoluteVerticalSpeed =
		std::abs(fVerticalVelocity);
	const _float fAbsoluteVerticalAcceleration =
		std::abs(m_fFilteredVerticalAcceleration);
	// [LSY] 서로 다른 진입/해제 임계값으로 경계 부근의 60/120Hz 반복 전환을 막는다.
	const _float fEnterVerticalSpeed =
		std::max(
			m_fHighLoadEnterVerticalSpeed,
			0.f);
	const _float fExitVerticalSpeed =
		std::clamp(
			m_fHighLoadExitVerticalSpeed,
			0.f,
			fEnterVerticalSpeed);
	const _float fEnterVerticalAcceleration =
		std::max(
			m_fHighLoadEnterVerticalAcceleration,
			0.f);
	const _float fExitVerticalAcceleration =
		std::clamp(
			m_fHighLoadExitVerticalAcceleration,
			0.f,
			fEnterVerticalAcceleration);
	_bool bUseHighLoadSolver{};
	if (m_bUsingBroomCollisionRig)
	{
		if (m_bHighLoadSolverEnabled)
		{
			bUseHighLoadSolver =
				fAbsoluteVerticalSpeed >
					fExitVerticalSpeed ||
				fAbsoluteVerticalAcceleration >
					fExitVerticalAcceleration;
		}
		else
		{
			bUseHighLoadSolver =
				fAbsoluteVerticalSpeed >=
					fEnterVerticalSpeed ||
				fAbsoluteVerticalAcceleration >=
					fEnterVerticalAcceleration;
		}
	}

	_float3 vDesiredLinearInertia{ 1.f, 1.f, 1.f };
	if (m_bUsingBroomCollisionRig)
		vDesiredLinearInertia.y =
			std::clamp(m_fBroomVerticalInertia, 0.f, 1.f);

	_float fDesiredSolverFrequency =
		std::max(m_fBaseSolverFrequency, 1.f);
	if (bUseHighLoadSolver)
	{
		fDesiredSolverFrequency = std::max(
			m_fHighLoadSolverFrequency,
			fDesiredSolverFrequency);
	}

	const _bool bLinearInertiaChanged =
		!m_bRuntimeSimulationProfileInitialized ||
		std::abs(m_vAppliedLinearInertia.x -
			vDesiredLinearInertia.x) > 0.0001f ||
		std::abs(m_vAppliedLinearInertia.y -
			vDesiredLinearInertia.y) > 0.0001f ||
		std::abs(m_vAppliedLinearInertia.z -
			vDesiredLinearInertia.z) > 0.0001f;
	if (bLinearInertiaChanged &&
		!m_pComNvCloth->SetLinearInertia(
			vDesiredLinearInertia))
	{
		return false;
	}

	const _bool bSolverFrequencyChanged =
		!m_bRuntimeSimulationProfileInitialized ||
		std::abs(m_fAppliedSolverFrequency -
			fDesiredSolverFrequency) > 0.0001f;
	if (bSolverFrequencyChanged &&
		!m_pComNvCloth->SetSolverFrequency(
			fDesiredSolverFrequency))
	{
		return false;
	}

	m_vAppliedLinearInertia =
		vDesiredLinearInertia;
	m_fAppliedSolverFrequency =
		fDesiredSolverFrequency;
	m_bHighLoadSolverEnabled =
		bUseHighLoadSolver;
	m_bRuntimeSimulationProfileInitialized = true;
	return true;
}

_bool CNvClothCape::UpdateVirtualWind(
	_float fTimeDelta,
	CPlayer* pPlayer,
	_bool bSuppressed)
{
	if (!m_pComNvCloth ||
		!std::isfinite(fTimeDelta) ||
		fTimeDelta < 0.f)
	{
		return false;
	}

	_float3 vTargetWind{};
	if (!bSuppressed && pPlayer)
	{
		//vTargetWind =
		//	pPlayer->GetClothWindImpulseVelocity();
		vTargetWind = GetClothWindImpulseVelocity();
		if (m_bUseVelocityWind &&
			pPlayer->IsFlyRequested())
		{
			const auto* pMotor =
				pPlayer->GetCharacterMotor();
			if (pMotor)
			{
				const _float3& vVelocity =
					pMotor->GetVelocity();
				vTargetWind.x -=
					vVelocity.x * m_fVelocityWindScale;
				vTargetWind.y -=
					vVelocity.y * m_fVelocityWindScale *
						m_fVerticalVelocityWindScale;
				vTargetWind.z -=
					vVelocity.z * m_fVelocityWindScale;
			}
		}
	}

	_vector vTarget = XMLoadFloat3(&vTargetWind);
	const _float fBaseWindSpeed = XMVectorGetX(
		XMVector3Length(vTarget));
	const _vector vHorizontalWind = XMVectorSetY(
		vTarget,
		0.f);
	const _float fHorizontalWindSpeed = XMVectorGetX(
		XMVector3Length(vHorizontalWind));
	m_fHighSpeedWindBlend = 0.f;
	const _float fHighSpeedRange =
		m_fHighSpeedWindFull - m_fHighSpeedWindStart;
	if (!bSuppressed && fHighSpeedRange > FLT_EPSILON)
	{
		m_fHighSpeedWindBlend = std::clamp(
			(fHorizontalWindSpeed - m_fHighSpeedWindStart) /
				fHighSpeedRange,
			0.f,
			1.f);
		// [LSY] 경계에서 풍압이 갑자기 바뀌지 않도록 Smoothstep 곡선을 사용한다.
		m_fHighSpeedWindBlend =
			m_fHighSpeedWindBlend * m_fHighSpeedWindBlend *
			(3.f - 2.f * m_fHighSpeedWindBlend);
	}

	const _float fFlutterStrength =
		m_fWindFlutterStrength +
		(m_fHighSpeedFlutterStrength - m_fWindFlutterStrength) *
			m_fHighSpeedWindBlend;
	const _float fFlutterFrequency =
		m_fWindFlutterFrequency +
		(m_fHighSpeedFlutterFrequency - m_fWindFlutterFrequency) *
			m_fHighSpeedWindBlend;
	const _float fGustStrength =
		m_fWindGustStrength +
		(m_fHighSpeedGustStrength - m_fWindGustStrength) *
			m_fHighSpeedWindBlend;
	if (!bSuppressed)
	{
		m_fWindTime += fTimeDelta;
		m_fWindFlutterPhase +=
			fTimeDelta * XM_2PI * fFlutterFrequency;
		if (m_fWindTime > 1024.f)
			m_fWindTime = std::fmod(
				m_fWindTime,
				1024.f);
		if (m_fWindFlutterPhase > XM_2PI)
			m_fWindFlutterPhase = std::fmod(
				m_fWindFlutterPhase,
				XM_2PI);
	}
	else
	{
		m_fWindTime = 0.f;
		m_fWindFlutterPhase = 0.f;
	}

	if (m_bUseWindFlutter &&
		fBaseWindSpeed > FLT_EPSILON)
	{
		const _vector vWindDirection =
			XMVector3Normalize(vTarget);
		const _vector vWorldUp =
			XMVectorSet(0.f, 1.f, 0.f, 0.f);
		_vector vSideDirection =
			XMVector3Cross(vWorldUp, vWindDirection);
		if (XMVectorGetX(
			XMVector3LengthSq(vSideDirection)) <=
			FLT_EPSILON)
		{
			vSideDirection =
				XMVectorSet(1.f, 0.f, 0.f, 0.f);
		}
		else
		{
			vSideDirection =
				XMVector3Normalize(vSideDirection);
		}

		const _float fFlutterPhase =
			m_fWindFlutterPhase;
		const _float fSideWave =
			(std::sin(fFlutterPhase) +
				std::sin(
					fFlutterPhase * 1.73f +
					1.2f) * 0.45f) /
			1.45f;
		const _float fVerticalWave =
			std::sin(
				fFlutterPhase * 0.71f +
				0.45f) * 0.65f +
			std::sin(
				fFlutterPhase * 1.31f +
				2.1f) * 0.35f;

		const _float fGustPhase =
			m_fWindTime * XM_2PI *
			m_fWindGustFrequency;
		const _float fGustWave =
			std::sin(fGustPhase) * 0.65f +
			std::sin(
				fGustPhase * 2.17f +
				0.8f) * 0.35f;
		const _float fBaseGustScale = std::max(
			0.1f,
			1.f + fGustWave *
				m_fWindGustStrength);
		const _float fHorizontalGustScale = std::max(
			0.1f,
			1.f + fGustWave *
				fGustStrength);
		const _float fSideFlutterAmplitude =
			fBaseWindSpeed *
			fFlutterStrength;
		const _float fVerticalFlutterAmplitude =
			fBaseWindSpeed *
			m_fWindFlutterStrength;

		// [LSY] 서로 다른 주기의 파형을 섞어 반복이 눈에 띄는
		// 단일 사인파 대신 불규칙한 횡풍과 상하 들썩임을 만든다.
		// 고속 증폭은 수평 성분에만 적용해 수직 상승 관통이 재발하지 않게 한다.
		const _vector vVerticalTarget = XMVectorSet(
			0.f,
			XMVectorGetY(vTarget),
			0.f,
			0.f);
		const _vector vHorizontalTarget =
			vTarget - vVerticalTarget;
		vTarget =
			vHorizontalTarget * fHorizontalGustScale +
			vVerticalTarget * fBaseGustScale;
		vTarget +=
			vSideDirection *
			(fSideFlutterAmplitude * fSideWave);
		vTarget +=
			vWorldUp *
			(fVerticalFlutterAmplitude * 0.55f *
				fVerticalWave);
	}

	const _float fTargetSpeed = XMVectorGetX(
		XMVector3Length(vTarget));
	if (m_fMaxWindSpeed <= 0.f)
	{
		vTarget = XMVectorZero();
	}
	else if (
		fTargetSpeed > m_fMaxWindSpeed)
	{
		vTarget = XMVector3Normalize(vTarget) *
			m_fMaxWindSpeed;
	}

	if (bSuppressed)
	{
		m_vCurrentWindVelocity = {};
	}
	else
	{
		const _float fBlend = std::clamp(
			1.f - std::exp(
				-m_fWindResponse * fTimeDelta),
			0.f,
			1.f);
		const _vector vCurrent =
			XMLoadFloat3(&m_vCurrentWindVelocity);
		XMStoreFloat3(
			&m_vCurrentWindVelocity,
			XMVectorLerp(vCurrent, vTarget, fBlend));
	}

	const _float fCurrentSpeedSq = XMVectorGetX(
		XMVector3LengthSq(
			XMLoadFloat3(&m_vCurrentWindVelocity)));
	const _bool bWindActive =
		fCurrentSpeedSq > 0.0001f;
	if (!bWindActive)
		m_vCurrentWindVelocity = {};

	NVCLOTH_WIND_DESC WindDesc{};
	WindDesc.vVelocity = m_vCurrentWindVelocity;
	if (bWindActive)
	{
		WindDesc.fDragCoefficient =
			m_fWindDragCoefficient;
		WindDesc.fLiftCoefficient =
			m_fWindLiftCoefficient;
		WindDesc.fFluidDensity =
			m_fWindFluidDensity;
	}

	return m_pComNvCloth->SetWind(WindDesc);
}

_bool CNvClothCape::UpdateBodyCollisions()
{
	return UpdateBodyCollisions(
		m_BodyCollisionRig,
		m_CollisionRigBoneIndices);
}

_bool CNvClothCape::UpdateBodyCollisions(
	const NVCLOTH_COLLISION_RIG_DESC& BodyRig,
	const std::vector<int32_t>& BodyRigBoneIndices)
{
	if (!m_pComNvCloth)
		return false;

	auto* pPlayer =
		CGameInstance::Get().
		GetGameObjectByHandleT<CPlayer>(m_hTarget);
	if (!pPlayer)
		return false;

	auto* pModelInstance =
		pPlayer->GetComponent<CComModelInstance>(
			m_sTargetModelComponentTag);
	if (!pModelInstance ||
		!pModelInstance->GetModel())
	{
		return false;
	}

	_matrix SimulationWorld{};
	if (!NvClothCapeDetail::MakeRigidMatrix(
		GetTransform().
			GetLoadedCombinedWorldMatrix(),
		SimulationWorld))
	{
		return false;
	}

	_vector vDeterminant{};
	const _matrix InverseSimulationWorld =
		XMMatrixInverse(
			&vDeterminant,
			SimulationWorld);
	const float fDeterminant =
		XMVectorGetX(vDeterminant);
	if (!std::isfinite(fDeterminant) ||
		std::abs(fDeterminant) <= 1.e-8f)
	{
		return false;
	}

	const _matrix TargetWorld =
		pPlayer->GetTransform().
		GetLoadedCombinedWorldMatrix();

	NVCLOTH_COLLISION_DESC CollisionDesc{};
	CollisionDesc.bContinuousCollision =
		m_bContinuousBodyCollision;
	CollisionDesc.fCollisionMassScale =
		m_fCollisionMassScale;
	CollisionDesc.fFriction =
		m_fCollisionFriction;
	std::vector<DEBUG_BODY_COLLISION_SHAPE>
		DebugBodyCollisionShapes{};
	size_t iBroomDebugShapeStart =
		std::numeric_limits<size_t>::max();
	const _bool bBroomObjectCollisionRequested =
		pPlayer->IsBroomVisible() &&
		!m_BroomObjectCollisionRig.Shapes.empty();

	if (!BodyRig.Shapes.empty())
	{
		if (!AppendCollisionsFromRig(
			BodyRig,
			BodyRigBoneIndices,
			*pModelInstance,
			TargetWorld,
			InverseSimulationWorld,
			CollisionDesc,
			DebugBodyCollisionShapes))
		{
			return false;
		}
	}
	else
	{
		CollisionDesc.vecSpheres.reserve(
			m_BodyCollisionBones.size());
		for (const auto& Binding :
			m_BodyCollisionBones)
		{
			_matrix BoneMatrix{};
			if (!GetTargetBoneMatrix(
				*pModelInstance,
				Binding.iBoneIndex,
				BoneMatrix))
			{
				return false;
			}

			const _vector vWorldPosition =
				XMVector3TransformCoord(
					XMVectorZero(),
					BoneMatrix * TargetWorld);
			const _vector vLocalPosition =
				XMVector3TransformCoord(
					vWorldPosition,
					InverseSimulationWorld);

			NVCLOTH_COLLISION_SPHERE Sphere{};
			XMStoreFloat3(
				&Sphere.vCenter,
				vLocalPosition);
			Sphere.fRadius = Binding.fRadius;
			CollisionDesc.vecSpheres.push_back(
				Sphere);

			DEBUG_BODY_COLLISION_SHAPE DebugShape{};
			DebugShape.eType =
				NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE;
			DebugShape.fRadius =
				Sphere.fRadius;
			XMStoreFloat4x4(
				&DebugShape.SimulationPose,
				XMMatrixTranslationFromVector(
					vLocalPosition));
			DebugBodyCollisionShapes.push_back(
				DebugShape);
		}

		CollisionDesc.vecCapsules = {
			{ 0, 1 },
			{ 1, 2 },
			{ 1, 3 },
			{ 1, 4 }
		};
	}

	if (bBroomObjectCollisionRequested)
	{
		auto* pBroom =
			CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Broom>(
				pPlayer->GetBroomHandle());
		if (!pBroom)
			return false;

		auto* pBroomModelInstance =
			pBroom->GetComponent<CComModelInstance>(
				"ComCModelIntance");
		if (!pBroomModelInstance ||
			!pBroomModelInstance->GetModel())
		{
			return false;
		}

		if (m_BroomObjectCollisionRigBoneIndices.size() !=
			m_BroomObjectCollisionRig.Shapes.size() &&
			!ResolveCollisionRigBones(
				m_BroomObjectCollisionRig,
				*pBroomModelInstance,
				m_BroomObjectCollisionRigBoneIndices,
				"Broom Object"))
		{
			return false;
		}

		m_iResolvedBroomObjectShapeCount =
			static_cast<size_t>(std::count_if(
				m_BroomObjectCollisionRigBoneIndices.begin(),
				m_BroomObjectCollisionRigBoneIndices.end(),
				[](int32_t iBoneIndex)
				{
					return iBoneIndex >= 0;
				}));
		iBroomDebugShapeStart =
			DebugBodyCollisionShapes.size();

		// [LSY] 빗자루의 Cached Combined World는 LateUpdate에서 만들어지므로
		// FixedUpdate에서는 한 프레임 전 플레이어 위치일 수 있다. 현재 소켓,
		// 플레이어 루트, 빗자루 Local S/R/T로 이번 물리 틱의 월드를 직접 만든다.
		_matrix BroomWorld{};
		if (!pBroom->TryBuildAttachedWorldMatrix(BroomWorld))
			return false;
		if (!AppendCollisionsFromRig(
			m_BroomObjectCollisionRig,
			m_BroomObjectCollisionRigBoneIndices,
			*pBroomModelInstance,
			BroomWorld,
			InverseSimulationWorld,
			CollisionDesc,
			DebugBodyCollisionShapes))
		{
			return false;
		}
	}

	if (!m_pComNvCloth->SetCollisions(
		CollisionDesc))
	{
		if (!m_bCollisionUpdateFailureLogged)
		{
			char szLog[256]{};
			sprintf_s(
				szLog,
				"[NvClothCape] Failed to update collisions: "
				"%zu spheres, %zu capsules, %zu planes, "
				"%zu convexes. NvCloth limits may be exceeded.\n",
				CollisionDesc.vecSpheres.size(),
				CollisionDesc.vecCapsules.size(),
				CollisionDesc.vecPlanes.size(),
				CollisionDesc.vecConvexes.size());
			DEBUG_LOG(szLog);
			m_bCollisionUpdateFailureLogged = true;
		}
		return false;
	}
	m_bCollisionUpdateFailureLogged = false;
	m_DebugBodyCollisionShapes =
		std::move(DebugBodyCollisionShapes);
	m_iBroomDebugShapeStart =
		iBroomDebugShapeStart;
	m_bBroomObjectCollisionRequested =
		bBroomObjectCollisionRequested;
	m_bBroomObjectCollisionApplied =
		bBroomObjectCollisionRequested;

	m_LastBodyCollisionDesc =
		std::move(CollisionDesc);
	return true;
}

_bool CNvClothCape::ValidateAndRecoverSimulation()
{
	// [LSY] 현재 Client는 CPU NvCloth를 사용한다. DX11 Backend에서 이 검사를
	// 수행하면 GPU 동기화 비용이 생길 수 있으므로 고속 빗자루 상태의 CPU만 검사한다.
	auto* pManager = CGameInstance::Get().GetNvClothManager();
	if (!m_bUsingBroomCollisionRig)
	{
		m_iSimulationValidationTick = 0;
		m_iConsecutiveDistanceWarningSamples = 0;
		m_iLastDistanceWarningParticleCount = 0;
		m_fLastMaximumParticleDistance = 0.f;
		// [LSY] 누적 통계는 유지하되 다음 빗자루 세션의 첫 이상 상태는
		// 다시 기록할 수 있도록 세션 단위 로그 억제 상태만 해제한다.
		m_bSimulationValidationFailureLogged = false;
		m_bSimulationRecoveryLogged = false;
		m_bSimulationDistanceWarningLogged = false;
		return true;
	}
	if (!pManager ||
		pManager->GetBackend() != NVCLOTH_BACKEND::CPU)
	{
		return true;
	}

	// [LSY] 평상시에는 기존 검사 주기를 유지하고 수직 급이동 중에만 검사 간격을
	// 줄여 파티클 복사 비용과 폭주 감지 지연을 함께 관리한다.
	constexpr uint32_t NORMAL_VALIDATION_INTERVAL = 30;
	constexpr uint32_t HIGH_LOAD_VALIDATION_INTERVAL = 10;
	uint32_t iValidationInterval = NORMAL_VALIDATION_INTERVAL;
	if (m_bHighLoadSolverEnabled)
		iValidationInterval = HIGH_LOAD_VALIDATION_INTERVAL;
	++m_iSimulationValidationTick;
	if (m_iSimulationValidationTick < iValidationInterval)
		return true;
	m_iSimulationValidationTick = 0;

	const auto& TargetPositions =
		m_AnimationConstraintDesc.vecTargetPositions;
	const auto& MaxDistances =
		m_AnimationConstraintDesc.vecMaxDistances;
	if (!m_pComNvCloth ||
		TargetPositions.empty() ||
		TargetPositions.size() != MaxDistances.size())
	{
		if (!m_bSimulationValidationFailureLogged)
		{
			DEBUG_LOG(
				"[NvClothCape] Simulation validation data is unavailable.\n");
			m_bSimulationValidationFailureLogged = true;
		}
		return true;
	}
	for (size_t i = 0; i < TargetPositions.size(); ++i)
	{
		const auto& Target = TargetPositions[i];
		const _float fMaxDistance = MaxDistances[i];
		if (std::isfinite(Target.x) &&
			std::isfinite(Target.y) &&
			std::isfinite(Target.z) &&
			std::isfinite(fMaxDistance))
		{
			continue;
		}

		// [LSY] 잘못된 애니메이션 타깃으로 파티클을 리셋하면 NaN을 다시 주입한다.
		// 이 경우에는 복구 자세를 만들지 않고 호출자에게 갱신 실패를 알린다.
		if (!m_bSimulationValidationFailureLogged)
		{
			char szLog[256]{};
			sprintf_s(
				szLog,
				"[NvClothCape] Invalid animation constraint "
				"at particle %zu. Recovery was skipped.\n",
				i);
			DEBUG_LOG(szLog);
			m_bSimulationValidationFailureLogged = true;
		}
		return false;
	}

	if (!m_pComNvCloth->GetParticles(
		m_SimulationValidationParticles) ||
		m_SimulationValidationParticles.size() !=
			TargetPositions.size())
	{
		if (!m_bSimulationValidationFailureLogged)
		{
			DEBUG_LOG(
				"[NvClothCape] Failed to inspect simulation particles.\n");
			m_bSimulationValidationFailureLogged = true;
		}
		return true;
	}
	m_bSimulationValidationFailureLogged = false;

	const size_t INVALID_PARTICLE_INDEX =
		std::numeric_limits<size_t>::max();
	size_t iInvalidParticle =
		INVALID_PARTICLE_INDEX;
	size_t iDistanceWarningParticle =
		INVALID_PARTICLE_INDEX;
	size_t iDistanceWarningParticleCount{};
	_bool bHardRunaway{};
	_float fMaximumDistanceSq{};
	for (size_t i = 0;
		i < m_SimulationValidationParticles.size();
		++i)
	{
		const auto& Particle =
			m_SimulationValidationParticles[i];
		const auto& Target = TargetPositions[i];
		const _float fMaxDistance = MaxDistances[i];
		if (!std::isfinite(Particle.x) ||
			!std::isfinite(Particle.y) ||
			!std::isfinite(Particle.z))
		{
			iInvalidParticle = i;
			break;
		}

		const _float fSoftRecoveryDistance =
			std::max(2.f, std::abs(fMaxDistance) * 8.f + 2.f);
		const _float fHardRecoveryDistance =
			fSoftRecoveryDistance * 2.f;
		const _float fDeltaX = Particle.x - Target.x;
		const _float fDeltaY = Particle.y - Target.y;
		const _float fDeltaZ = Particle.z - Target.z;
		const _float fDistanceSq =
			fDeltaX * fDeltaX +
			fDeltaY * fDeltaY +
			fDeltaZ * fDeltaZ;
		if (!std::isfinite(fDistanceSq))
		{
			iInvalidParticle = i;
			break;
		}
		fMaximumDistanceSq = std::max(
			fMaximumDistanceSq,
			fDistanceSq);
		if (fDistanceSq >
			fSoftRecoveryDistance * fSoftRecoveryDistance)
		{
			if (iDistanceWarningParticle ==
				INVALID_PARTICLE_INDEX)
			{
				iDistanceWarningParticle = i;
			}
			++iDistanceWarningParticleCount;
		}
		if (fDistanceSq >
			fHardRecoveryDistance * fHardRecoveryDistance)
		{
			bHardRunaway = true;
		}
	}
	m_iLastDistanceWarningParticleCount =
		iDistanceWarningParticleCount;
	m_fLastMaximumParticleDistance =
		std::sqrt(fMaximumDistanceSq);

	if (iInvalidParticle != INVALID_PARTICLE_INDEX)
	{
		return RecoverSimulation(
			"non-finite particle data",
			iInvalidParticle,
			false);
	}

	if (iDistanceWarningParticle != INVALID_PARTICLE_INDEX)
	{
		++m_iConsecutiveDistanceWarningSamples;
		if (!m_bSimulationDistanceWarningLogged)
		{
			char szLog[256]{};
			sprintf_s(
				szLog,
				"[NvClothCape] Particle exceeded the soft animation range "
				"at index %zu. Outliers: %zu, max distance: %.3f.\n",
				iDistanceWarningParticle,
				iDistanceWarningParticleCount,
				m_fLastMaximumParticleDistance);
			DEBUG_LOG(szLog);
			++m_iSimulationDistanceWarningCount;
			m_bSimulationDistanceWarningLogged = true;
		}
	}
	else
	{
		m_iConsecutiveDistanceWarningSamples = 0;
		m_bSimulationDistanceWarningLogged = false;
		m_bSimulationRecoveryLogged = false;
	}

	const size_t iWidespreadRunawayCount =
		std::max<size_t>(
			4,
			(m_SimulationValidationParticles.size() + 19) / 20);
	const _bool bWidespreadRunaway =
		iDistanceWarningParticleCount >=
			iWidespreadRunawayCount;
	const _bool bPersistentRunaway =
		m_iConsecutiveDistanceWarningSamples >= 2;
	if (!bHardRunaway &&
		!bWidespreadRunaway &&
		!bPersistentRunaway)
	{
		return true;
	}

	return RecoverSimulation(
		"finite particle runaway",
		iDistanceWarningParticle,
		true);
}

_bool CNvClothCape::RecoverSimulation(
	const char* szReason,
	size_t iParticleIndex,
	_bool bRunaway)
{
	// [LSY] 폭주한 속도와 외력을 다음 Step에 넘기지 않고 현재 애니메이션
	// 자세에서 다시 시작한다. UpdateVirtualWind가 같은 틱에 완만하게 재적용한다.
	m_vCurrentWindVelocity = {};
	m_vClothWindImpulseVelocity = {};
	m_fClothWindImpulseRemaining = 0.f;
	m_pComNvCloth->SetWind({});
	if (!ResetSimulationToAnimationPose())
		return false;

	++m_iSimulationRecoveryCount;
	if (bRunaway)
		++m_iSimulationRunawayRecoveryCount;

	if (!m_bSimulationRecoveryLogged)
	{
		const char* szRecoveryReason = "unknown reason";
		if (szReason)
			szRecoveryReason = szReason;
		char szLog[256]{};
		sprintf_s(
			szLog,
			"[NvClothCape] Recovered unstable simulation: "
			"%s at particle %zu.\n",
			szRecoveryReason,
			iParticleIndex);
		DEBUG_LOG(szLog);
		m_bSimulationRecoveryLogged = true;
	}

	m_iConsecutiveDistanceWarningSamples = 0;
	m_bSimulationDistanceWarningLogged = false;
	return true;
}

_bool CNvClothCape::ResetSimulationToAnimationPose()
{
	if (!m_pComNvCloth ||
		m_AnimationConstraintDesc.
			vecTargetPositions.empty())
	{
		return false;
	}

	return m_pComNvCloth->ResetParticlesToPositions(
		m_AnimationConstraintDesc.vecTargetPositions);
}

_bool CNvClothCape::UpdateAttachment(
	_bool bUpdateSimulation,
	_bool bForceTeleport)
{
	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return false;

	auto* pModelInstance =
		pTarget->GetComponent<CComModelInstance>(
			m_sTargetModelComponentTag);
	if (!pModelInstance ||
		!pModelInstance->GetModel())
	{
		return false;
	}

	if (m_iAttachBoneIndex < 0 &&
		!ResolveAttachment())
	{
		return false;
	}

	const auto iBoneIndex =
		static_cast<size_t>(m_iAttachBoneIndex);
	const auto& CombinedBones =
		pModelInstance->Get_CombinedBoneMatrices();
	_matrix BoneMatrix{};
	if (iBoneIndex < CombinedBones.size())
	{
		BoneMatrix =
			XMLoadFloat4x4(
				&CombinedBones[iBoneIndex]);
	}
	else
	{
		const auto& Bones =
			pModelInstance->GetModel()->GetBones();
		if (iBoneIndex >= Bones.size() ||
			!Bones[iBoneIndex])
		{
			return false;
		}
		BoneMatrix =
			Bones[iBoneIndex]->
			Get_CombinedTransformationMatrix();
	}

	_matrix AttachmentWorld{};
	if (!NvClothCapeDetail::MakeRigidMatrix(
		BoneMatrix *
			pTarget->GetTransform().
			GetLoadedCombinedWorldMatrix(),
		AttachmentWorld))
	{
		return false;
	}

	XMStoreFloat4x4(
		&m_ParentWorld,
		AttachmentWorld);
	GetTransform().SetParentWorldMatrix(
		m_ParentWorld);
	GetTransform().Update();
	m_bAttachmentInitialized = true;

	if (!bUpdateSimulation ||
		!m_pComNvCloth)
	{
		return true;
	}

	_matrix SimulationWorld{};
	if (!NvClothCapeDetail::MakeRigidMatrix(
		GetTransform().
			GetLoadedCombinedWorldMatrix(),
		SimulationWorld))
	{
		return false;
	}

	_vector vScale{};
	_vector qRotation{};
	_vector vTranslation{};
	if (!XMMatrixDecompose(
		&vScale,
		&qRotation,
		&vTranslation,
		SimulationWorld))
	{
		return false;
	}

	_float3 vCurrentPosition{};
	_float4 vCurrentRotation{};
	XMStoreFloat3(
		&vCurrentPosition,
		vTranslation);
	XMStoreFloat4(
		&vCurrentRotation,
		XMQuaternionNormalize(qRotation));

	const float fDistance =
		XMVectorGetX(XMVector3Length(
			XMLoadFloat3(&vCurrentPosition) -
			XMLoadFloat3(
				&m_vPreviousAttachPosition)));
	_bool bAngularTeleport{};
	if (m_bSimulationTransformInitialized &&
		m_fTeleportAngleDegrees > 0.f)
	{
		const _vector qPrevious =
			XMLoadFloat4(&m_vPreviousAttachRotation);
		const _vector qCurrent =
			XMLoadFloat4(&vCurrentRotation);
		_float fQuaternionDot = std::abs(
			XMVectorGetX(
				XMVector4Dot(qPrevious, qCurrent)));
		fQuaternionDot = std::clamp(
			fQuaternionDot,
			0.f,
			1.f);
		const _float fAngularDistance =
			2.f * std::acos(fQuaternionDot);
		const _float fTeleportAngleRadians =
			m_fTeleportAngleDegrees * XM_PI / 180.f;
		bAngularTeleport =
			fAngularDistance > fTeleportAngleRadians;
	}
	const _bool bTeleport =
		bForceTeleport ||
		!m_bSimulationTransformInitialized ||
		fDistance > m_fTeleportDistance ||
		bAngularTeleport;
	if (!m_pComNvCloth->SetSimulationTransform(
		vCurrentPosition,
		vCurrentRotation,
		bTeleport))
	{
		return false;
	}

	if (!UpdateAnimationConstraints(
		*pModelInstance,
		AttachmentWorld,
		bTeleport))
	{
		return false;
	}

	m_vPreviousAttachPosition =
		vCurrentPosition;
	m_vPreviousAttachRotation =
		vCurrentRotation;
	m_bSimulationTransformInitialized = true;
	return true;
}

_bool CNvClothCape::GetValidatedRenderParticleView(
	NVCLOTH_RENDER_PARTICLE_VIEW& OutView)
{
	OutView = {};
	const _bool bValid =
		m_pComNvCloth &&
		m_pClothMesh &&
		m_pComNvCloth->GetRenderParticleView(OutView) &&
		OutView.iParticleCount ==
			m_pClothMesh->GetParticleCount();
	if (bValid)
	{
		m_bRenderParticleViewFailureLogged = false;
		return true;
	}

	if (!m_bRenderParticleViewFailureLogged)
	{
		char szLog[256]{};
		sprintf_s(
			szLog,
			"[NvClothCape] Render particle view is invalid: "
			"runtime=%u, mesh=%u.\n",
			OutView.iParticleCount,
			m_pClothMesh ? m_pClothMesh->GetParticleCount() : 0u);
		DEBUG_LOG(szLog);
		m_bRenderParticleViewFailureLogged = true;
	}
	return false;
}

HRESULT CNvClothCape::Render(
	ID3D11DeviceContext* pContext,
	const RENDER_CTX& Context)
{
	if (!pContext ||
		!m_pComCBufferPerObject ||
		!m_pComModelInstance ||
		!m_pComNvCloth ||
		!m_pClothMesh ||
		!m_pVertexShader ||
		!m_pPixelShader)
	{
		return E_FAIL;
	}

	NVCLOTH_RENDER_PARTICLE_VIEW ParticleView{};
	if (!GetValidatedRenderParticleView(ParticleView))
	{
		return E_FAIL;
	}

	CB_PER_OBJECT PerObject{};
	PerObject.matWorld =
		*GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(
		&PerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() *
			Context.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext,
		&PerObject,
		sizeof(PerObject))))
	{
		return E_FAIL;
	}

	pContext->VSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT),
		1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT),
		1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->IASetInputLayout(
		m_pVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(
		m_pVertexShader->GetVertexShader().Get(),
		nullptr,
		0);
	pContext->PSSetShader(
		m_pPixelShader->GetPixelShader().Get(),
		nullptr,
		0);
	pContext->VSSetShaderResources(
		9,
		1,
		&ParticleView.pSRV);

	// [LSY] 망토는 두께가 없는 천 메시다. 고속 이동으로 면이 뒤집혀도
	// 전체가 사라져 보이지 않도록 양면 렌더하고 기존 Rasterizer는 원복한다.
	ComPtr<ID3D11RasterizerState> pPreviousRasterizer{};
	pContext->RSGetState(
		pPreviousRasterizer.GetAddressOf());
	const auto pNoCullRasterizer =
		CGameInstance::Get().GetResourceFirst<CResRasterizerState>(
			TAG_RES_GRP_PERMANENT_STATE,
			TAG_RES_STATE_RS_SOLID_NOCULL);
	if (pNoCullRasterizer)
	{
		pContext->RSSetState(
			pNoCullRasterizer->GetRasterizerState().Get());
	}

	const auto& Sections =
		m_pClothMesh->GetSections();
	for (uint32_t i = 0;
		i < Sections.size();
		++i)
	{
		const auto& Section = Sections[i];
		if (!Section.pVIBuffer)
			continue;

		ID3D11Buffer* pVertexBuffer =
			Section.pVIBuffer->
				GetVertexBuffer().Get();
		const uint32_t iVertexStride =
			Section.pVIBuffer->
				GetVertexStride();
		const uint32_t iOffset{};
		pContext->IASetVertexBuffers(
			0,
			1,
			&pVertexBuffer,
			&iVertexStride,
			&iOffset);
		pContext->IASetIndexBuffer(
			Section.pVIBuffer->
				GetIndexBuffer().Get(),
			Section.pVIBuffer->
				GetIndexFormat(),
			0);
		pContext->IASetPrimitiveTopology(
			Section.pVIBuffer->
				GetPrimitiveType());

		m_pComModelInstance->Bind_Textures(
			pContext,
			Section.iSourceMeshIndex);
		m_pComModelInstance->Bind_Materials(
			pContext,
			{ 1.f, 1.f, 1.f },
			0.f,
			{ 1.f, 1.f, 1.f },
			0.f,
			1.f);
		pContext->DrawIndexed(
			Section.pVIBuffer->
				GetNumIndices(),
			0,
			0);
	}

	ID3D11ShaderResourceView* pNullSRV{};
	pContext->VSSetShaderResources(
		9,
		1,
		&pNullSRV);
	pContext->RSSetState(
		pPreviousRasterizer.Get());
	return S_OK;
}

HRESULT CNvClothCape::Render_Shadow(
	ID3D11DeviceContext* pContext,
	const RENDER_CTX& Context)
{
	if (!pContext ||
		!m_pComCBufferPerObject ||
		!m_pComNvCloth ||
		!m_pClothMesh ||
		!m_pShadowVertexShader ||
		!m_pPointShadowVertexShader)
	{
		return E_FAIL;
	}

	NVCLOTH_RENDER_PARTICLE_VIEW ParticleView{};
	if (!GetValidatedRenderParticleView(ParticleView))
	{
		return E_FAIL;
	}

	CB_PER_OBJECT PerObject{};
	PerObject.matWorld =
		*GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(
		&PerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() *
			Context.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext,
		&PerObject,
		sizeof(PerObject))))
	{
		return E_FAIL;
	}

	ComPtr<ID3D11InputLayout> pPreviousInputLayout{};
	ComPtr<ID3D11VertexShader> pPreviousVertexShader{};
	ComPtr<ID3D11PixelShader> pPreviousPixelShader{};
	ComPtr<ID3D11ShaderResourceView>
		pPreviousParticleSRV{};
	pContext->IAGetInputLayout(
		pPreviousInputLayout.GetAddressOf());
	pContext->VSGetShader(
		pPreviousVertexShader.GetAddressOf(),
		nullptr,
		nullptr);
	/*----------- 광윤 추가 -----------*/
	pContext->PSGetShader(pPreviousPixelShader.GetAddressOf(),nullptr,nullptr);
	/*---------------------------------*/
	/*----------- 광윤 수정 -----------*/
	//ComPtr<ID3D11GeometryShader> pPreviousGeometryShader{};
	//pContext->GSGetShader(
	//	pPreviousGeometryShader.GetAddressOf(),
	//	nullptr,
	//	nullptr);
	/*---------------------------------*/
	pContext->VSGetShaderResources(
		9,
		1,
		pPreviousParticleSRV.GetAddressOf());

	pContext->VSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT),
		1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	// Point Shadow는 LightManager의 GS가 월드 좌표를 큐브맵 6면으로 확장한다.
	// GS가 없는 Directional/Spot Shadow는 기존 SV_POSITION 출력 VS를 사용한다.
	//const auto& pShadowVertexShader =
	//	pPreviousGeometryShader ?
	//	m_pPointShadowVertexShader :
	//	m_pShadowVertexShader;
	/*----------- 광윤 추가 -----------*/	// 기존 로직 변경되어서 아래 코드로 변경
	const bool IsPointFaceShadow =
		Context.PointShadowFaceIndex >= 0;

	const auto& pShadowVertexShader =
		IsPointFaceShadow
		? m_pPointShadowVertexShader
		: m_pShadowVertexShader;
	/*---------------------------------*/
	pContext->IASetInputLayout(
		pShadowVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(
		pShadowVertexShader->GetVertexShader().Get(),
		nullptr,
		0);
	/*----------- 광윤 추가 -----------*/
	pContext->PSSetShader(nullptr, nullptr, 0);
	/*---------------------------------*/
	pContext->VSSetShaderResources(
		9,
		1,
		&ParticleView.pSRV);

	for (const auto& Section :
		m_pClothMesh->GetSections())
	{
		if (!Section.pVIBuffer)
			continue;

		ID3D11Buffer* pVertexBuffer =
			Section.pVIBuffer->
				GetVertexBuffer().Get();
		const uint32_t iVertexStride =
			Section.pVIBuffer->GetVertexStride();
		const uint32_t iOffset{};
		pContext->IASetVertexBuffers(
			0,
			1,
			&pVertexBuffer,
			&iVertexStride,
			&iOffset);
		pContext->IASetIndexBuffer(
			Section.pVIBuffer->GetIndexBuffer().Get(),
			Section.pVIBuffer->GetIndexFormat(),
			0);
		pContext->IASetPrimitiveTopology(
			Section.pVIBuffer->GetPrimitiveType());
		pContext->DrawIndexed(
			Section.pVIBuffer->GetNumIndices(),
			0,
			0);
	}

	pContext->VSSetShaderResources(
		9,
		1,
		pPreviousParticleSRV.GetAddressOf());
	pContext->IASetInputLayout(
		pPreviousInputLayout.Get());
	pContext->VSSetShader(
		pPreviousVertexShader.Get(),
		nullptr,
		0);
	/*----------- 광윤 추가 -----------*/
	pContext->PSSetShader(pPreviousPixelShader.Get(), nullptr, 0);
	/*---------------------------------*/
	return S_OK;
}

bool CNvClothCape::GetShadowBounds(BoundingBox& OutBounds) const {
	CGameObject* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hTarget);

	if (pTarget == nullptr)	return false;

	if (pTarget->GetShadowBounds(OutBounds))	{
		OutBounds.Extents.x += 0.5f;
		OutBounds.Extents.y += 0.25f;
		OutBounds.Extents.z += 1.0f;

		return true;
	}

	OutBounds.Center =pTarget->GetTransform().GetPosition();
	OutBounds.Extents = { 1.75f, 2.0f, 2.0f };

	return true;
}

UPtr<CNvClothCape> CNvClothCape::Create()
{
	auto pInstance =
		ToUPtr(new CNvClothCape{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CNvClothCape");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CNvClothCape::Clone(void* pArg)
{
	auto pInstance =
		ToUPtr(new CNvClothCape{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CNvClothCape");
		return nullptr;
	}
	return pInstance;
}
