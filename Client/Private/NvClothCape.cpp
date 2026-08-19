#include "pch.h"
#include "NvClothCape.h"

#include "ComConstantBuffer.h"
#include "ComCharacterMotor.h"
#include "ComModelInstance.h"
#include "ComNvCloth.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "Player.h"
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
			pDesc->fBroomBackstopDisableRatio) ||
		pDesc->fBroomBackstopDisableRatio < 0.f ||
		pDesc->fBroomBackstopDisableRatio > 1.f ||
		pDesc->fBroomBackstopFullInfluenceRatio >=
			pDesc->fBroomBackstopDisableRatio ||
		!std::isfinite(pDesc->fSelfCollisionDistance) ||
		pDesc->fSelfCollisionDistance < 0.f ||
		!std::isfinite(pDesc->fSelfCollisionStiffness) ||
		pDesc->fSelfCollisionStiffness < 0.f ||
		pDesc->fSelfCollisionStiffness > 1.f ||
		!std::isfinite(pDesc->fVelocityWindScale) ||
		pDesc->fVelocityWindScale < 0.f ||
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
	m_fBroomBackstopDisableRatio =
		pDesc->fBroomBackstopDisableRatio;
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
		Desc.tCloth.fSolverFrequency = 60.f;
		Desc.tCloth.fStiffnessFrequency = 60.f;
		Desc.tCloth.fPhaseStiffness = 0.9f;
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
	if (!UpdateCollisionRigProfile(pPlayer))
		return;

	// [LSY] 대시 중에는 망토가 보이지 않으므로 이동 관성을 누적하지 않는다.
	// 종료 시에는 현재 애니메이션 자세로 복원한 뒤 다시 표시한다.
	if (!UpdateAttachment(
			true,
			bOwnerRenderSuppressed ||
				bSuppressionChanged))
	{
		return;
	}

	if (bSuppressionChanged &&
		!ResetSimulationToAnimationPose())
	{
		return;
	}

	if (!UpdateVirtualWind(
		fTimeDelta,
		pPlayer,
		bOwnerRenderSuppressed))
	{
		return;
	}

	if (!UpdateBodyCollisions())
		return;

	m_bOwnerRenderSuppressed =
		bOwnerRenderSuppressed;
}

void CNvClothCape::Update(_float)
{
}

void CNvClothCape::LateUpdate(_float)
{
	if (!m_bRenderCape ||
		m_bOwnerRenderSuppressed)
		return;

	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hTarget);
	if (auto* pPlayer = Cast<CPlayer>(pTarget))
	{
		// [LSY] 플레이어가 대시 연출로 숨겨질 때 망토와 망토 그림자도 함께 숨긴다.
		if (pPlayer->GetRenderInfluence())
			return;
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
				"Broom Disable Ratio",
				&m_fBroomBackstopDisableRatio,
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

_bool CNvClothCape::UpdateCollisionRigProfile(
	CPlayer* pPlayer)
{
	const _bool bUseBroomRig =
		pPlayer &&
		pPlayer->IsBroomVisible() &&
		!m_BroomBodyCollisionRig.Shapes.empty();
	if (bUseBroomRig == m_bUsingBroomCollisionRig)
		return true;

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
	m_bUsingBroomCollisionRig = bUseBroomRig;
	if (bUseBroomRig)
		m_BodyCollisionRig = m_BroomBodyCollisionRig;
	else
		m_BodyCollisionRig = m_GroundBodyCollisionRig;
	m_LastBodyCollisionDesc = {};
	m_DebugBodyCollisionShapes.clear();
	const char* szProfileName = "Ground Body";
	if (bUseBroomRig)
		szProfileName = "Broom Body";
	return ResolveCollisionRigBones(
		m_BodyCollisionRig,
		*pModelInstance,
		m_CollisionRigBoneIndices,
		szProfileName);
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
							m_fBroomBackstopDisableRatio -
								m_fBroomBackstopFullInfluenceRatio,
							0.001f),
						0.f,
						1.f);
				const float fSmoothInfluence =
					fInfluenceRatio * fInfluenceRatio *
					(3.f - 2.f * fInfluenceRatio);
				fBackstopInfluence =
					1.f - fSmoothInfluence;
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
	NVCLOTH_COLLISION_DESC& OutDesc)
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

		m_DebugBodyCollisionShapes.push_back(
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
					vVelocity.y * m_fVelocityWindScale;
				vTargetWind.z -=
					vVelocity.z * m_fVelocityWindScale;
			}
		}
	}

	_vector vTarget = XMLoadFloat3(&vTargetWind);
	const _float fBaseWindSpeed = XMVectorGetX(
		XMVector3Length(vTarget));
	if (!bSuppressed)
	{
		m_fWindTime += fTimeDelta;
		if (m_fWindTime > 1024.f)
			m_fWindTime = std::fmod(
				m_fWindTime,
				1024.f);
	}
	else
	{
		m_fWindTime = 0.f;
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
			m_fWindTime * XM_2PI *
			m_fWindFlutterFrequency;
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
		const _float fGustScale = std::max(
			0.1f,
			1.f + fGustWave *
				m_fWindGustStrength);
		const _float fFlutterAmplitude =
			fBaseWindSpeed *
			m_fWindFlutterStrength;

		// [LSY] 서로 다른 주기의 파형을 섞어 반복이 눈에 띄는
		// 단일 사인파 대신 불규칙한 횡풍과 상하 들썩임을 만든다.
		vTarget *= fGustScale;
		vTarget +=
			vSideDirection *
			(fFlutterAmplitude * fSideWave);
		vTarget +=
			vWorldUp *
			(fFlutterAmplitude * 0.55f *
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
	m_DebugBodyCollisionShapes.clear();
	m_iBroomDebugShapeStart =
		std::numeric_limits<size_t>::max();
	m_bBroomObjectCollisionRequested =
		pPlayer->IsBroomVisible() &&
		!m_BroomObjectCollisionRig.Shapes.empty();
	m_bBroomObjectCollisionApplied = false;

	if (!m_BodyCollisionRig.Shapes.empty())
	{
		if (!AppendCollisionsFromRig(
			m_BodyCollisionRig,
			m_CollisionRigBoneIndices,
			*pModelInstance,
			TargetWorld,
			InverseSimulationWorld,
			CollisionDesc))
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
			m_DebugBodyCollisionShapes.push_back(
				DebugShape);
		}

		CollisionDesc.vecCapsules = {
			{ 0, 1 },
			{ 1, 2 },
			{ 1, 3 },
			{ 1, 4 }
		};
	}

	if (m_bBroomObjectCollisionRequested)
	{
		auto* pBroom =
			CGameInstance::Get().GetGameObjectByHandle(
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
		m_iBroomDebugShapeStart =
			m_DebugBodyCollisionShapes.size();

		// [LSY] 렌더링되는 빗자루와 동일한 최종 행렬을 사용한다.
		// 플레이어 소켓을 별도로 재조립하면 애니메이션 팔레트와 갱신 시점이
		// 달라져 충돌체가 빗자루와 다른 위치에 배치될 수 있다.
		const _matrix BroomWorld =
			pBroom->GetTransform().
				GetLoadedCombinedWorldMatrix();
		if (!AppendCollisionsFromRig(
			m_BroomObjectCollisionRig,
			m_BroomObjectCollisionRigBoneIndices,
			*pBroomModelInstance,
			BroomWorld,
			InverseSimulationWorld,
			CollisionDesc))
		{
			return false;
		}
	}

	if (!m_pComNvCloth->SetCollisions(
		CollisionDesc))
	{
		return false;
	}
	m_bBroomObjectCollisionApplied =
		m_bBroomObjectCollisionRequested;

	m_LastBodyCollisionDesc =
		std::move(CollisionDesc);
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
	const _bool bTeleport =
		bForceTeleport ||
		!m_bSimulationTransformInitialized ||
		fDistance > m_fTeleportDistance;
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
	m_bSimulationTransformInitialized = true;
	return true;
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
	if (!m_pComNvCloth->GetRenderParticleView(
		ParticleView) ||
		ParticleView.iParticleCount !=
			m_pClothMesh->GetParticleCount())
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
	if (!m_pComNvCloth->GetRenderParticleView(
		ParticleView) ||
		ParticleView.iParticleCount !=
			m_pClothMesh->GetParticleCount())
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
