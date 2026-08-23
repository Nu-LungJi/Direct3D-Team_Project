#include "pch.h"
#include "PropBarrel.h"
#include "PropBarrelDebris.h"
#include "Monster.h"

#include "ComPxConvexCollider.h"
#include "ComPxRigidBody.h"
#include "ComConstantBuffer.h"
#include "ComSound.h"
#include "ComStaticModelInstance.h"
#include "GameInstance.h"
#include "PhysXManager.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXMaterial.h"
#include "ResRasterizerState.h"
#include "Resources.h"
#include "SoundManager.h"

NS_USING(Client)

namespace
{
	// 따라다니거나 중단되어야 하는 소리는 컴포넌트 슬롯으로 관리한다.
	const StringID ANCIENT_THROW_HOLD_SOUND_SLOT{
		"PLAYER_ANCIENT_THROW_HOLD" };
	const StringID ANCIENT_THROW_FLIGHT_SOUND_SLOT{
		"PLAYER_ANCIENT_THROW_FLIGHT" };

	constexpr const char* ANCIENT_THROW_PULL_SOUND_PATH =
		"./Resources/SampleClient/Sound/Player/SkillEffect/AncientThrow/AncientThrow_Pull_Start.wav";
	constexpr const char* ANCIENT_THROW_HOLD_SOUND_PATH =
		"./Resources/SampleClient/Sound/Player/SkillEffect/AncientThrow/AncientThrow_Hold_Energy.wav";
	constexpr const char* ANCIENT_THROW_RELEASE_SOUND_PATH =
		"./Resources/SampleClient/Sound/Player/SkillEffect/AncientThrow/AncientThrow_Release.wav";
	constexpr const char* ANCIENT_THROW_FLIGHT_SOUND_PATH =
		"./Resources/SampleClient/Sound/Player/SkillEffect/AncientThrow/AncientThrow_Flight.wav";
	constexpr const char* ANCIENT_THROW_IMPACT_MAIN_SOUND_PATH =
		"./Resources/SampleClient/Sound/Player/SkillEffect/AncientThrow/AncientThrow_Impact_Main.wav";
	constexpr const char* ANCIENT_THROW_IMPACT_TAIL_SOUND_PATH =
		"./Resources/SampleClient/Sound/Player/SkillEffect/AncientThrow/AncientThrow_Impact_Tail.wav";
	constexpr const char* PROP_BARREL_WOOD_IMPACT_SOUND_PATHS[]{
		"./Resources/SampleClient/Sound/Effect/PropBarrel/PropBarrel_WoodImpact_01.wav",
		"./Resources/SampleClient/Sound/Effect/PropBarrel/PropBarrel_WoodImpact_02.wav" };
	constexpr const char* PROP_BARREL_WOOD_BREAK_MAIN_SOUND_PATH =
		"./Resources/SampleClient/Sound/Effect/PropBarrel/PropBarrel_WoodBreak_Main.wav";
	constexpr const char* PROP_BARREL_WOOD_BREAK_TAIL_SOUND_PATH =
		"./Resources/SampleClient/Sound/Effect/PropBarrel/PropBarrel_WoodBreak_Tail.wav";
	constexpr _float PROP_BARREL_COLLISION_SOUND_MIN_IMPULSE = 0.08f;
	constexpr _float PROP_BARREL_COLLISION_SOUND_COOLDOWN = 0.14f;

	SOUND_3D_DESC MakeLinearSound3DDesc(
		const _float3& vPosition,
		_float fMinDistance,
		_float fMaxDistance,
		const _float3& vVelocity = {})
	{
		return {
			.vPosition = vPosition,
			.vVelocity = vVelocity,
			.fMinDistance = fMinDistance,
			.fMaxDistance = fMaxDistance,
			.eRolloff = SOUND_3D_ROLLOFF::LINEAR
		};
	}
}

CPropBarrel::CPropBarrel() = default;

CPropBarrel::CPropBarrel(const CPropBarrel& prototype)
	: CGameObject{ prototype }
	, m_pResConvexGeometry{ prototype.m_pResConvexGeometry }
	, m_pResPhysXMaterial{ prototype.m_pResPhysXMaterial }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
	, m_sResourceGroup{ prototype.m_sResourceGroup }
{
}

HRESULT CPropBarrel::InitializePrototype(void*)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim");
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	if (!m_pResVertexShader || !m_pResPixelShader ||
		FAILED(m_pResVertexShader->Load()) || FAILED(m_pResPixelShader->Load()))
		return E_FAIL;

	m_pResConvexGeometry = CGameInstance::Get()
		.GetOrCreateResourceByPath<CResPhysXConvexGeometry>(
			"./Resources/PhysX/Cooked/SM_Prop_Barrel_Breakable_A2.pxconvex",
			[]()
			{
				return CResPhysXConvexGeometry::CreateAndLoad(
					"./Resources/PhysX/Cooked/SM_Prop_Barrel_Breakable_A2.pxconvex");
			});
	m_pResPhysXMaterial = CResPhysXMaterial::CreateAndLoad({});
	return m_pResConvexGeometry && m_pResPhysXMaterial ? S_OK : E_FAIL;
}

HRESULT CPropBarrel::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	m_sResourceGroup = pDesc->sResourceGroup;
	m_vModelScale = pDesc->vInitialScale;
	m_vDebrisConvexScale = pDesc->vConvexScale;
	m_fCollisionDestroyImpulse = std::max(
		pDesc->fCollisionDestroyImpulse, 0.f);
	m_fCollisionDestroyGraceTime = std::max(
		pDesc->fCollisionDestroyGraceTime, 0.f);
	m_fCollisionDestroyElapsed = 0.f;
	m_fCollisionSoundCooldown = 0.f;
	m_bDestroyFromAncientThrow = false;
	m_bHasDestroyImpactPosition = false;
	m_eState = BARREL_STATE::CREATED;
	GetTransform().SetScale(m_vModelScale);
	GetTransform().Update();

	{
		CComConstantBuffer::DESC desc{};
		desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_ConstantBuffer",
			"ComCBufferPerObject", &desc, &m_pComCBufferPerObject)))
			return E_FAIL;
	}

	{
		CComSound::DESC desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComSound,
			"Com_Sound", &desc, &m_pComSound)))
		{
			return E_FAIL;
		}
	}

	{
		CComStaticModelInstance::DESC desc{};
		desc.sGroupTag = pDesc->sResourceGroup;
		desc.sResTag = "Static_Prop_Barrel_Resource";
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_StaticModelInstance",
			"ComModelInstance", &desc, &m_pComModelInstance)))
			return E_FAIL;
	}
	if (const auto& pModel = m_pComModelInstance->GetModel();
		pModel && pModel->HasLocalBounds())
	{
		const _float3 vLocalCenter = pModel->GetLocalBounds().Center;
		m_vVisualCenterLocalOffset = {
			vLocalCenter.x * m_vModelScale.x,
			vLocalCenter.y * m_vModelScale.y,
			vLocalCenter.z * m_vModelScale.z };
		const _float3 vLocalExtents = pModel->GetLocalBounds().Extents;
		m_fAncientThrowSweepRadius = std::max(
			0.25f,
			std::min({
				std::abs(vLocalExtents.x * m_vModelScale.x),
				std::abs(vLocalExtents.y * m_vModelScale.y),
				std::abs(vLocalExtents.z * m_vModelScale.z) }));
	}

	{
		CComPxRigidBody::DESC desc{};
		desc.eType = CComPxRigidBody::TYPE::DYNAMIC;
		desc.fMass = std::max(pDesc->fMass, 0.001f);
		desc.vPosition = pDesc->vInitialPosition;
		desc.vRotation = GetTransform().GetQuaternion();
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxRigidBody",
			"ComPxRigidBody", &desc, &m_pComPxRigidBody)))
			return E_FAIL;
	}

	{
		CComPxConvexCollider::DESC desc{};
		desc.pComPxRigidBody = m_pComPxRigidBody;
		desc.pResConvex = m_pResConvexGeometry;
		desc.pResMaterial = m_pResPhysXMaterial;
		desc.vScale = {
			std::max(std::abs(pDesc->vConvexScale.x), 0.001f),
			std::max(std::abs(pDesc->vConvexScale.y), 0.001f),
			std::max(std::abs(pDesc->vConvexScale.z), 0.001f) };
		desc.tFilter = pDesc->tFilter;
		desc.vLocalOffset = {0.f, 0.f, 0.f};
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxConvexCollider",
			"ComPxConvexCollider", &desc, &m_pComPxConvexCollider)))
			return E_FAIL;
	}

	const _bool bHasInitialImpulse =
		pDesc->vInitialImpulse.x != 0.f ||
		pDesc->vInitialImpulse.y != 0.f ||
		pDesc->vInitialImpulse.z != 0.f;
	const _bool bHasInitialAngularVelocity =
		pDesc->vInitialAngularVelocityRadians.x != 0.f ||
		pDesc->vInitialAngularVelocityRadians.y != 0.f ||
		pDesc->vInitialAngularVelocityRadians.z != 0.f;

	if (!m_pComPxRigidBody->SetAngularDamping(
			std::max(pDesc->fAngularDamping, 0.f)) ||
		!m_pComPxRigidBody->SetAngularVelocity(
			pDesc->vInitialAngularVelocityRadians) ||
		(bHasInitialImpulse &&
			!m_pComPxRigidBody->AddImpulse(pDesc->vInitialImpulse)))
	{
		return E_FAIL;
	}

	if ((bHasInitialImpulse || bHasInitialAngularVelocity) &&
		!m_pComPxRigidBody->WakeUp())
	{
		return E_FAIL;
	}

	//if (!m_pComPxRigidBody->SetGravityEnabled(false) ||
	//	!m_pComPxConvexCollider->SetSimulationEnabled(false) ||
	//	!m_pComPxConvexCollider->SetQueryEnabled(false))
	//	return E_FAIL;

	return S_OK;
}

_bool CPropBarrel::DestroyBarrel()
{
	if (m_eState == BARREL_STATE::DESTROYED)
		return true;

	StopAncientThrowHoldSound();
	StopAncientThrowFlightSound();

	// 파괴 요청이 Update에서 처리되므로, LateUpdate를 기다리지 않고
	// 이번 프레임의 최신 PhysX 자세를 먼저 Transform에 반영한다.
	UpdatePhysicData();
	GetTransform().Update();

	const _float3 vPosition = GetTransform().GetPosition();
	const _float3 vSoundPosition = GetVisualCenterPosition();
	const _float4 vRotation = GetTransform().GetQuaternion();
	const _float3 vScale = GetTransform().GetScale();
	_float4 vDebrisRotation{};
	const _vector qAxisCorrection = XMQuaternionRotationAxis(
		XMVectorSet(1.f, 0.f, 0.f, 0.f),
		XMConvertToRadians(-90.f));
	XMStoreFloat4(
		&vDebrisRotation,
		XMQuaternionNormalize(
			XMQuaternionMultiply(
				qAxisCorrection,
				XMLoadFloat4(&vRotation))));
	std::vector<CHandle> spawnedDebrisHandles{};
	spawnedDebrisHandles.reserve(12);
	const _float3 vInheritedLinearVelocity = m_bDestroyFromAncientThrow ?
		m_vAncientThrowKinematicLinearVelocity :
		(m_pComPxRigidBody ?
			m_pComPxRigidBody->GetLinearVelocity() : _float3{});
	const _float3 vInheritedAngularVelocity = m_bDestroyFromAncientThrow ?
		m_vAncientThrowKinematicAngularVelocity :
		(m_pComPxRigidBody ?
			m_pComPxRigidBody->GetAngularVelocity() : _float3{});
	const _bool bAncientRadialExplosion =
		m_bDestroyFromAncientThrow && m_bHasDestroyImpactPosition;
	const _vector vBarrelCenter = XMLoadFloat3(&vPosition);
	const _vector vImpactPosition =
		XMLoadFloat3(&m_vDestroyImpactPosition);
	const _vector vInheritedLinear =
		XMLoadFloat3(&vInheritedLinearVelocity);
	const _vector vInheritedAngular =
		XMLoadFloat3(&vInheritedAngularVelocity);

	for (uint32_t i = 1; i <= 12; ++i)
	{
		CPropBarrelDebris::DESC desc{};
		desc.sObjectTag = "PropBarrelDebris_" + std::to_string(i);
		desc.sResourceGroup = m_sResourceGroup;
		desc.sResourceTag =
			"Static_Prop_Barrel_Debris_Resource_" + std::to_string(i);

		desc.sConvexPath = "./Resources/PhysX/Cooked/";
		if (i < 10)
			desc.sConvexPath +=
				"SM_Prop_Barrel_Breakable_A_Fragment2_0" + std::to_string(i);
		else
			desc.sConvexPath +=
				"SM_Prop_Barrel_Breakable_A_Fragment2_" + std::to_string(i);
		desc.sConvexPath += ".pxconvex";

		desc.vInitialPosition = vPosition;
		desc.vInitialScale = vScale;
		desc.vConvexScale = m_vDebrisConvexScale;
		// 파편 리소스의 X축 -90도 보정을 먼저 적용한 뒤,
		// 원본 배럴의 현재 월드 회전을 이어서 적용한다.
		desc.vInitialQuaternion = vDebrisRotation;

		const _float fAngle = XM_2PI *
			(static_cast<_float>(i - 1) / 12.f);
		const _float fSpinSign = (i % 2 == 0) ? -1.f : 1.f;
		if (bAncientRadialExplosion)
		{
			// 실제 파편별 중심점을 대신하는 가상 지점을 배럴 둘레에 잡고,
			// 충돌 지점에서 그 지점으로 향하는 방사 방향을 만든다.
			const _float fLayerHeight =
				(static_cast<int32_t>(i % 3) - 1) * 0.65f;
			const _vector vFragmentPoint =
				vBarrelCenter + XMVectorSet(
					std::cos(fAngle) * 1.15f,
					fLayerHeight,
					std::sin(fAngle) * 1.15f,
					0.f);
			_vector vRadialDirection = vFragmentPoint - vImpactPosition;
			if (XMVectorGetX(XMVector3LengthSq(vRadialDirection)) <= FLT_EPSILON)
			{
				vRadialDirection = XMVectorSet(
					std::cos(fAngle),
					0.25f,
					std::sin(fAngle),
					0.f);
			}
			vRadialDirection = XMVector3Normalize(vRadialDirection);

			const _float fRadialSpeed =
				5.f + 0.3f * static_cast<_float>(i % 4);
			const _vector vDebrisLinearVelocity =
				vInheritedLinear * ANCIENT_DEBRIS_INHERITED_VELOCITY_RATIO +
				vRadialDirection * fRadialSpeed;
			const _vector vDebrisAngularVelocity =
				vInheritedAngular * ANCIENT_DEBRIS_INHERITED_VELOCITY_RATIO +
				XMVectorSet(
					std::cos(fAngle) * 4.f,
					fSpinSign * 5.f,
					std::sin(fAngle) * 4.f,
					0.f);
			XMStoreFloat3(
				&desc.vInitialLinearVelocity,
				vDebrisLinearVelocity);
			XMStoreFloat3(
				&desc.vInitialAngularVelocityRadians,
				vDebrisAngularVelocity);
		}
		else
		{
			const _float fOutwardSpeed = 4.f +
				0.35f * static_cast<_float>(i % 4);
			const _float fUpwardSpeed = 4.5f +
				0.25f * static_cast<_float>(i % 3);
			desc.vInitialLinearVelocity = {
				vInheritedLinearVelocity.x + std::cos(fAngle) * fOutwardSpeed,
				vInheritedLinearVelocity.y + fUpwardSpeed,
				vInheritedLinearVelocity.z + std::sin(fAngle) * fOutwardSpeed };
			desc.vInitialAngularVelocityRadians = {
				vInheritedAngularVelocity.x + std::cos(fAngle) * 5.f,
				vInheritedAngularVelocity.y + fSpinSign * 6.f,
				vInheritedAngularVelocity.z + std::sin(fAngle) * 5.f };
		}

		const auto hDebris = CGameInstance::Get().AddGameObjectToLayer(
			m_sResourceGroup,
			PROTO_GAMEOBJECT::Prototype_GameObject_PropBarrelDebris,
			"PropBarrelDebris",
			&desc);
		if (!hDebris)
		{
			for (const CHandle& hSpawnedDebris : spawnedDebrisHandles)
			{
				if (auto* pDebris = CGameInstance::Get().GetGameObjectByHandle(
					hSpawnedDebris))
				{
					pDebris->SetPendingDestroy();
				}
			}
			return false;
		}

		spawnedDebrisHandles.push_back(*hDebris);
	}

	if (!m_bDestroyFromAncientThrow)
		PlayBarrelDestroySounds(vSoundPosition);

	m_eState = BARREL_STATE::DESTROYED;
	// 새로 생성된 파편은 다음 프레임부터 렌더 목록에 들어갈 수 있으므로,
	// 원본 배럴은 이번 프레임까지 렌더하고 다음 Update에서 제거한다.
	// 대기 중에는 파편과 원본 콜라이더가 겹쳐 밀어내지 않도록 충돌만 끈다.
	if (m_pComPxConvexCollider)
	{
		m_pComPxConvexCollider->SetSimulationEnabled(false);
		m_pComPxConvexCollider->SetQueryEnabled(false);
	}
	m_bDestroyOriginalNextFrame = true;
	return true;
}

_bool CPropBarrel::BeginAncientThrowControl()
{
	if (!m_pComPxRigidBody || !m_pComPxConvexCollider ||
		m_eState != BARREL_STATE::CREATED || GetPendingDestroy())
	{
		return false;
	}

	if (!m_pComPxRigidBody->SetLinearVelocity({}) ||
		!m_pComPxRigidBody->SetAngularVelocity({}) ||
		!m_pComPxRigidBody->SetKinematic(true) ||
		!m_pComPxConvexCollider->SetSimulationEnabled(false) ||
		!m_pComPxConvexCollider->SetQueryEnabled(false))
	{
		CancelAncientThrowControl();
		return false;
	}

	m_bAncientThrowControlled = true;
	m_bAncientThrowFlying = false;
	m_vAncientThrowKinematicLinearVelocity = {};
	m_vAncientThrowKinematicAngularVelocity = {};
	m_bDestroyFromAncientThrow = false;
	m_bHasDestroyImpactPosition = false;
	m_bDestroyRequested = false;
	PlayAncientThrowPullSounds();
	InvalidatePhysXSyncData();
	return true;
}

_float3 CPropBarrel::GetVisualCenterPosition() const
{
	const _vector vPivotPosition =
		XMLoadFloat3(&GetTransform().GetPosition());
	const _vector qRotation = XMQuaternionNormalize(
		XMLoadFloat4(&GetTransform().GetQuaternion()));
	_float3 vCenterPosition{};
	XMStoreFloat3(
		&vCenterPosition,
		vPivotPosition + XMVector3Rotate(
			XMLoadFloat3(&m_vVisualCenterLocalOffset),
			qRotation));
	return vCenterPosition;
}

_bool CPropBarrel::SetAncientThrowVisualPose(
	const _float3& vCenterPosition,
	const _float4& vQuaternion)
{
	if (!m_bAncientThrowControlled || !m_pComPxRigidBody)
	{
		return false;
	}

	const _vector qRotation = XMQuaternionNormalize(
		XMLoadFloat4(&vQuaternion));
	const _vector vPivotPosition =
		XMLoadFloat3(&vCenterPosition) -
		XMVector3Rotate(
			XMLoadFloat3(&m_vVisualCenterLocalOffset),
			qRotation);
	_float3 vCorrectedPivotPosition{};
	_float4 vNormalizedRotation{};
	XMStoreFloat3(&vCorrectedPivotPosition, vPivotPosition);
	XMStoreFloat4(&vNormalizedRotation, qRotation);
	if (!m_pComPxRigidBody->SetPose(
		vCorrectedPivotPosition,
		vNormalizedRotation))
	{
		return false;
	}

	GetTransform().SetPosition(vCorrectedPivotPosition);
	GetTransform().SetQuaternion(vNormalizedRotation);
	GetTransform().Update();
	UpdateAncientThrowSoundPosition(vCenterPosition);
	// Update에서 지정한 보간 자세가 LateUpdate의 이전 PhysX 캐시에
	// 다시 덮이지 않도록 수동 제어 시점의 캐시를 폐기한다.
	InvalidatePhysXSyncData();
	return true;
}

void CPropBarrel::CancelAncientThrowControl()
{
	StopAncientThrowHoldSound();
	StopAncientThrowFlightSound();

	if (m_pComPxRigidBody && m_pComPxConvexCollider)
	{
		m_pComPxRigidBody->SetKinematic(false);
		m_pComPxRigidBody->SetGravityEnabled(true);
		m_pComPxConvexCollider->SetSimulationEnabled(true);
		m_pComPxConvexCollider->SetQueryEnabled(true);
		m_pComPxRigidBody->SetLinearVelocity({});
		m_pComPxRigidBody->SetAngularVelocity({});
		m_pComPxRigidBody->WakeUp();
		InvalidatePhysXSyncData();
	}

	m_bAncientThrowControlled = false;
	m_bAncientThrowFlying = false;
	m_vAncientThrowKinematicLinearVelocity = {};
	m_vAncientThrowKinematicAngularVelocity = {};
}

_bool CPropBarrel::Launch(
	const _float3& vLinearVelocity,
	const _float3& vAngularVelocityRadians)
{
	if (!m_pComPxRigidBody || !m_pComPxConvexCollider ||
		m_eState != BARREL_STATE::CREATED ||
		GetPendingDestroy())
	{
		return false;
	}

	if (!m_pComPxRigidBody->SetKinematic(true) ||
		!m_pComPxConvexCollider->SetSimulationEnabled(false) ||
		!m_pComPxConvexCollider->SetQueryEnabled(false))
	{
		CancelAncientThrowControl();
		return false;
	}

	m_bAncientThrowControlled = false;
	m_bAncientThrowFlying = true;
	m_vAncientThrowKinematicLinearVelocity = vLinearVelocity;
	m_vAncientThrowKinematicAngularVelocity = vAngularVelocityRadians;
	m_bDestroyRequested = false;
	StopAncientThrowHoldSound();
	PlayAncientThrowLaunchSounds();
	// 발사 직후에도 끌어오기 전의 PhysX 캐시로 한 프레임 되돌아가지 않는다.
	InvalidatePhysXSyncData();
	return true;
}

void CPropBarrel::PlayAncientThrowPullSounds()
{
	const _float3 vPosition = GetVisualCenterPosition();
	if (auto* pSoundManager = CGameInstance::Get().GetSoundManager())
	{
		pSoundManager->Play3D(
			ANCIENT_THROW_PULL_SOUND_PATH,
			MakeLinearSound3DDesc(vPosition, 2.f, 65.f),
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.4f,
				.fPitch = 1.f,
				.iPriority = 82,
				.bLoop = false
			});
	}

	if (m_pComSound)
	{
		m_pComSound->PlaySlot3D(
			ANCIENT_THROW_HOLD_SOUND_SLOT,
			ANCIENT_THROW_HOLD_SOUND_PATH,
			MakeLinearSound3DDesc(vPosition, 2.f, 55.f),
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.16f,
				.fPitch = 1.f,
				.fFadeInDuration = 0.06f,
				.iPriority = 76,
				.bLoop = false
			});
	}
}

void CPropBarrel::PlayAncientThrowLaunchSounds()
{
	const _float3 vPosition = GetVisualCenterPosition();
	if (auto* pSoundManager = CGameInstance::Get().GetSoundManager())
	{
		pSoundManager->Play3D(
			ANCIENT_THROW_RELEASE_SOUND_PATH,
			MakeLinearSound3DDesc(
				vPosition, 2.f, 70.f,
				m_vAncientThrowKinematicLinearVelocity),
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.5f,
				.fPitch = 1.f,
				.iPriority = 86,
				.bLoop = false
			});
	}

	if (m_pComSound)
	{
		m_pComSound->PlaySlot3D(
			ANCIENT_THROW_FLIGHT_SOUND_SLOT,
			ANCIENT_THROW_FLIGHT_SOUND_PATH,
			MakeLinearSound3DDesc(
				vPosition, 2.f, 75.f,
				m_vAncientThrowKinematicLinearVelocity),
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.28f,
				.fPitch = 1.f,
				.iPriority = 80,
				.bLoop = false
			});
	}
}

void CPropBarrel::PlayAncientThrowImpactSounds(
	const _float3& vImpactPosition) const
{
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (!pSoundManager)
		return;

	const SOUND_3D_DESC sound3DDesc =
		MakeLinearSound3DDesc(vImpactPosition, 2.f, 85.f);
	// 오크통은 이 호출 직후 제거될 수 있으므로 충돌음과 잔향은
	// 오브젝트 슬롯이 아닌 SoundManager의 독립 one-shot으로 재생한다.
	pSoundManager->Play3D(
		ANCIENT_THROW_IMPACT_MAIN_SOUND_PATH,
		sound3DDesc,
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 0.58f,
			.fPitch = 1.f,
			.iPriority = 88,
			.bLoop = false
		});
	pSoundManager->Play3D(
		ANCIENT_THROW_IMPACT_TAIL_SOUND_PATH,
		sound3DDesc,
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 0.26f,
			.fPitch = 1.f,
			.iPriority = 80,
			.bLoop = false
		});
}

void CPropBarrel::PlayBarrelCollisionSound(
	const _float3& vImpactPosition,
	_float fImpulse) const
{
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (!pSoundManager)
		return;

	constexpr int32_t SOUND_VARIANT_COUNT =
		static_cast<int32_t>(std::size(PROP_BARREL_WOOD_IMPACT_SOUND_PATHS));
	const int32_t iVariant = RandInt(0, SOUND_VARIANT_COUNT - 1);
	const _float fStrength = std::clamp(
		fImpulse / std::max(m_fCollisionDestroyImpulse, 0.001f),
		0.f,
		1.f);
	pSoundManager->Play3D(
		PROP_BARREL_WOOD_IMPACT_SOUND_PATHS[iVariant],
		MakeLinearSound3DDesc(vImpactPosition, 1.5f, 45.f),
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 0.12f + fStrength * 0.2f,
			.fPitch = Randf(0.94f, 1.06f),
			.iPriority = 68,
			.bLoop = false
		});
}

void CPropBarrel::PlayBarrelDestroySounds(
	const _float3& vPosition) const
{
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (!pSoundManager)
		return;

	const SOUND_3D_DESC sound3DDesc =
		MakeLinearSound3DDesc(vPosition, 2.f, 70.f);
	// 파편 생성과 함께 원본 오브젝트가 제거되어도 나무 파괴음의
	// main/tail이 끝까지 들리도록 독립 one-shot으로 유지한다.
	pSoundManager->Play3D(
		PROP_BARREL_WOOD_BREAK_MAIN_SOUND_PATH,
		sound3DDesc,
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 0.42f,
			.fPitch = Randf(0.96f, 1.04f),
			.iPriority = 82,
			.bLoop = false
		});
	pSoundManager->Play3D(
		PROP_BARREL_WOOD_BREAK_TAIL_SOUND_PATH,
		sound3DDesc,
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 0.3f,
			.fPitch = Randf(0.94f, 1.02f),
			.iPriority = 76,
			.bLoop = false
		});
}

void CPropBarrel::UpdateAncientThrowSoundPosition(
	const _float3& vPosition,
	const _float3& vVelocity)
{
	if (!m_pComSound)
		return;

	if (m_bAncientThrowControlled)
	{
		m_pComSound->SetSlot3DAttributes(
			ANCIENT_THROW_HOLD_SOUND_SLOT,
			vPosition,
			vVelocity);
	}
	if (m_bAncientThrowFlying)
	{
		m_pComSound->SetSlot3DAttributes(
			ANCIENT_THROW_FLIGHT_SOUND_SLOT,
			vPosition,
			vVelocity);
	}
}

void CPropBarrel::StopAncientThrowHoldSound()
{
	if (m_pComSound)
	{
		m_pComSound->FadeOutAndDetachSlot(
			ANCIENT_THROW_HOLD_SOUND_SLOT,
			0.08f);
	}
}

void CPropBarrel::StopAncientThrowFlightSound()
{
	if (m_pComSound)
		m_pComSound->StopSlot(ANCIENT_THROW_FLIGHT_SOUND_SLOT);
}

void CPropBarrel::UpdateGUI()
{
	__super::UpdateGUI();
	ImGui::Separator();
	ImGui::Text("Prop Barrel State: %s",
		m_eState == BARREL_STATE::CREATED ? "Created" : "Destroyed");
	const _bool bCanDestroy =
		m_eState == BARREL_STATE::CREATED && !GetPendingDestroy();
	if (bCanDestroy && ImGui::Button("Destroy Prop Barrel"))
		m_bDestroyRequested = true;
	else if (!bCanDestroy)
		ImGui::TextDisabled("Destroy unavailable");
}

void CPropBarrel::FixedUpdate(_float fTimeDelta)
{
	if (m_eState != BARREL_STATE::CREATED || !m_pComPxRigidBody)
		return;

	m_fCollisionDestroyElapsed += std::max(fTimeDelta, 0.f);
	if (m_bAncientThrowFlying)
	{
		const _float fStep = std::max(fTimeDelta, 0.f);
		if (fStep > 0.f)
		{
			const _float3 vCurrentPositionValue =
				m_pComPxRigidBody->GetPosition();
			const _float4 vCurrentRotationValue =
				m_pComPxRigidBody->GetRotation();
			const _vector vCurrentPosition =
				XMLoadFloat3(&vCurrentPositionValue);
			const _vector vCurrentRotation = XMQuaternionNormalize(
				XMLoadFloat4(&vCurrentRotationValue));
			const _vector vAngularVelocity =
				XMLoadFloat3(&m_vAncientThrowKinematicAngularVelocity);
			const _float fAngularSpeed = XMVectorGetX(
				XMVector3Length(vAngularVelocity));
			_vector vTargetRotation = vCurrentRotation;
			if (fAngularSpeed > FLT_EPSILON)
			{
				const _vector qDelta = XMQuaternionRotationAxis(
					vAngularVelocity / fAngularSpeed,
					fAngularSpeed * fStep);
				vTargetRotation = XMQuaternionNormalize(
					XMQuaternionMultiply(qDelta, vCurrentRotation));
			}

			const _vector vCurrentCenter = vCurrentPosition +
				XMVector3Rotate(
					XMLoadFloat3(&m_vVisualCenterLocalOffset),
					vCurrentRotation);
			// 모델 피벗은 배럴 바닥에 있으므로 피벗을 직선 이동시키면
			// 회전할 때 시각적 중심이 피벗 둘레를 돈다. 시각적 중심을
			// 직선 이동시킨 뒤 회전된 중심 오프셋만큼 피벗을 역보정한다.
			const _vector vTargetCenter = vCurrentCenter +
				XMLoadFloat3(&m_vAncientThrowKinematicLinearVelocity) * fStep;
			const _vector vTargetPosition = vTargetCenter -
				XMVector3Rotate(
					XMLoadFloat3(&m_vVisualCenterLocalOffset),
					vTargetRotation);
			const _vector vSweepDisplacement =
				vTargetCenter - vCurrentCenter;
			const _float fSweepDistance = XMVectorGetX(
				XMVector3Length(vSweepDisplacement));
			if (fSweepDistance > FLT_EPSILON)
			{
				_float3 vSweepOrigin{};
				_float3 vSweepDirection{};
				XMStoreFloat3(&vSweepOrigin, vCurrentCenter);
				XMStoreFloat3(
					&vSweepDirection,
					vSweepDisplacement / fSweepDistance);

				PX_SWEEP_DESC tSweepDesc{};
				tSweepDesc.tGeometry = {
					.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE,
					.fRadius = m_fAncientThrowSweepRadius };
				tSweepDesc.tPose.vPosition = vSweepOrigin;
				tSweepDesc.vDirection = vSweepDirection;
				tSweepDesc.fMaxDistance = fSweepDistance;
				tSweepDesc.tFilter = {
					.iQueryMask =
						ETOUI(COLLISION_LAYER::WORLD_STATIC) |
						ETOUI(COLLISION_LAYER::MOVING_PLATFORM) |
						ETOUI(COLLISION_LAYER::ENEMY_BODY) |
						ETOUI(COLLISION_LAYER::ENEMY_HURTBOX) |
						ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
					.hIgnoreGameObject = GetHandle(),
					.bQueryStatic = true,
					.bQueryDynamic = true,
					.bIncludeTrigger = false };

				PX_SWEEP_RESULT tSweepHit{};
				auto* pPhysX =
					CGameInstance::Get().GetPhysXManager();
				if (pPhysX && pPhysX->Sweep(tSweepDesc, tSweepHit) &&
					tSweepHit.bHit)
				{
					const _float fStopDistance = std::clamp(
						tSweepHit.fDistance,
						0.f,
						fSweepDistance);
					const _vector vImpactCenter = vCurrentCenter +
						XMLoadFloat3(&vSweepDirection) * fStopDistance;
					const _vector vImpactPivot = vImpactCenter -
						XMVector3Rotate(
							XMLoadFloat3(&m_vVisualCenterLocalOffset),
							vTargetRotation);
					_float3 vImpactPivotPosition{};
					_float4 vImpactRotation{};
					XMStoreFloat3(
						&vImpactPivotPosition,
						vImpactPivot);
					XMStoreFloat4(
						&vImpactRotation,
						vTargetRotation);
					m_pComPxRigidBody->SetPose(
						vImpactPivotPosition,
						vImpactRotation);
					InvalidatePhysXSyncData();

					HandleAncientThrowImpact(
						tSweepHit.pGameObject,
						tSweepHit.vHitpos,
						tSweepHit.vHitNormal);
					return;
				}
			}

			_float3 vKinematicTargetPosition{};
			_float4 vKinematicTargetRotation{};
			XMStoreFloat3(&vKinematicTargetPosition, vTargetPosition);
			XMStoreFloat4(&vKinematicTargetRotation, vTargetRotation);
			m_pComPxRigidBody->SetKinematicTarget(
				vKinematicTargetPosition,
				vKinematicTargetRotation);
			_float3 vTargetCenterPosition{};
			XMStoreFloat3(&vTargetCenterPosition, vTargetCenter);
			UpdateAncientThrowSoundPosition(
				vTargetCenterPosition,
				m_vAncientThrowKinematicLinearVelocity);
		}
	}
}

void CPropBarrel::Update(_float fTimeDelta)
{
	if (m_pComSound)
		m_pComSound->Update();
	m_fCollisionSoundCooldown = std::max(
		0.f,
		m_fCollisionSoundCooldown - std::max(fTimeDelta, 0.f));

	if (m_bDestroyOriginalNextFrame)
	{
		m_bDestroyOriginalNextFrame = false;
		SetPendingDestroy();
		return;
	}

	if (!m_bDestroyRequested)
		return;

	m_bDestroyRequested = false;
	if (!DestroyBarrel())
		DEBUG_LOG("[PropBarrel] Deferred destroy failed.\n");
}

void CPropBarrel::HandleAncientThrowImpact(
	CGameObject* pHitObject,
	const _float3& vImpactPosition,
	const _float3& vImpactNormal)
{
	if (m_eState != BARREL_STATE::CREATED || GetPendingDestroy() ||
		m_bDestroyRequested || !m_bAncientThrowFlying)
		return;

	m_bAncientThrowFlying = false;
	StopAncientThrowFlightSound();
	PlayAncientThrowImpactSounds(vImpactPosition);
	m_bDestroyFromAncientThrow = true;
	m_bHasDestroyImpactPosition = true;
	m_vDestroyImpactPosition = vImpactPosition;
	_vector vLook = XMLoadFloat3(&vImpactNormal);
	if (XMVectorGetX(XMVector3LengthSq(vLook)) <= FLT_EPSILON)
		vLook = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	else
		vLook = XMVector3Normalize(vLook);

	_vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	_vector vRight = XMVector3Cross(vUp, vLook);
	if (XMVectorGetX(XMVector3LengthSq(vRight)) <= FLT_EPSILON)
	{
		vUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		vRight = XMVector3Cross(vUp, vLook);
	}
	vRight = XMVector3Normalize(vRight);
	vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));

	_matrix matImpactWorld = XMMatrixIdentity();
	matImpactWorld.r[0] = XMVectorSetW(vRight, 0.f);
	matImpactWorld.r[1] = XMVectorSetW(vUp, 0.f);
	matImpactWorld.r[2] = XMVectorSetW(vLook, 0.f);
	matImpactWorld.r[3] = XMVectorSetW(
		XMLoadFloat3(&vImpactPosition), 1.f);
	_float4x4 impactWorld{};
	XMStoreFloat4x4(&impactWorld, matImpactWorld);
	CGameInstance::Get().Spawn(
		"LSY_AncientThrow_ImpactDust_Queue.json",
		impactWorld);
	if (auto* pMonster = dynamic_cast<CMonster*>(pHitObject))
		pMonster->Check_Table(PLAYER_SKILL_TYPE::DESTORY);
	m_bDestroyRequested = true;
}

void CPropBarrel::OnCollisionEnter(
	CGameObject*,
	const PX_ON_COLLISION_DATA& info)
{
	if (m_eState != BARREL_STATE::CREATED || GetPendingDestroy() ||
		m_bDestroyRequested || m_bAncientThrowControlled ||
		m_bAncientThrowFlying)
		return;

	_float fTotalImpulse = 0.f;
	for (uint32_t i = 0; i < info.iContactCount; ++i)
	{
		fTotalImpulse += XMVectorGetX(XMVector3Length(
			XMLoadFloat3(&info.Contacts[i].vImpulse)));
	}
	if (m_fCollisionSoundCooldown <= 0.f &&
		fTotalImpulse >= PROP_BARREL_COLLISION_SOUND_MIN_IMPULSE)
	{
		// 연속 바운스로 같은 프레임대에 충돌음이 겹치는 것을 막는다.
		// 생성 직후 grace time은 파괴만 유예하고 착지음은 그대로 허용한다.
		const _float3 vImpactPosition = info.iContactCount > 0 ?
			info.Contacts[0].vWorldPosition :
			GetVisualCenterPosition();
		PlayBarrelCollisionSound(vImpactPosition, fTotalImpulse);
		m_fCollisionSoundCooldown = PROP_BARREL_COLLISION_SOUND_COOLDOWN;
	}

	if (m_fCollisionDestroyElapsed < m_fCollisionDestroyGraceTime)
		return;

	if (fTotalImpulse >= m_fCollisionDestroyImpulse)
		m_bDestroyRequested = true;
}

void CPropBarrel::LateUpdate(_float fTimeDelta)
{
	UpdatePhysicData();
	GetTransform().Update();
	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return;

	if (!CGameInstance::Get().IsInstancingEnabled())
	{
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
		return;
	}

	const auto& pModel = m_pComModelInstance->GetModel();
	if (!pModel->HasLocalBounds())
		return;

	MAPMESH_INSTANCE_DATA instanceData{};
	XMStoreFloat4x4(
		&instanceData.world,
		GetTransform().GetLoadedCombinedWorldMatrix());

	BoundingBox worldBounds{};
	pModel->GetLocalBounds().Transform(
		worldBounds,
		GetTransform().GetLoadedCombinedWorldMatrix());

	MAPMESH_OCCLUSION_DATA occlusionData{};
	occlusionData.worldCenter = worldBounds.Center;
	occlusionData.worldExtents = worldBounds.Extents;

	CGameInstance::Get().PushMapObjectInstance(
		pModel, instanceData, occlusionData);
}

HRESULT CPropBarrel::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	/*if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject ||
		!m_pResVertexShader || !m_pResPixelShader)
		return E_FAIL;

	CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(
		&cbPerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext, &cbPerObject, sizeof(cbPerObject))))
		return E_FAIL;
	pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());

	pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	ComPtr<ID3D11RasterizerState> previousRasterizer{};
	pContext->RSGetState(previousRasterizer.GetAddressOf());
	const auto noCullRasterizer = CGameInstance::Get().GetResourceFirst<CResRasterizerState>(
		TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	if (noCullRasterizer)
		pContext->RSSetState(noCullRasterizer->GetRasterizerState().Get());

	const auto& pModel = m_pComModelInstance->GetModel();
	if (!m_bRenderConfirmed)
	{
		const auto renderLog = std::format(
			"[PlayerPotion] Render reached. meshes={}, scale=({}, {}, {}).\n",
			pModel->Get_NumMeshes(), m_vModelScale.x, m_vModelScale.y,
			m_vModelScale.z);
		DEBUG_LOG(renderLog.c_str());
		m_bRenderConfirmed = true;
	}
	for (uint32_t meshIndex = 0; meshIndex < pModel->Get_NumMeshes(); ++meshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[meshIndex];
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const uint32_t stride = mesh->GetVertexStride();
		const uint32_t offset{};
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		m_pComModelInstance->Bind_Textures(pContext, meshIndex);
		m_pComModelInstance->Bind_Materials(
			pContext, { 1.f, 1.f, 1.f }, 0.f,
			{ 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexed(mesh->GetNumIndices(), 0, 0);
	}

	pContext->RSSetState(previousRasterizer.Get());*/

	return S_OK;
}

UPtr<CPropBarrel> CPropBarrel::Create()
{
	auto pInstance = ToUPtr(new CPropBarrel{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CPropBarrel::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CPropBarrel{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
