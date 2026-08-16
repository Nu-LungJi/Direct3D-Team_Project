#include "pch.h"
#include "Player_AvadaKedavraController.h"

#include "ComSound.h"
#include "Monster.h"
#include "Player.h"
#include "Player_Weapon.h"
#include "Particle.h"

NS_BEGIN(Client)

CPlayer_AvadaKedavraController::CPlayer_AvadaKedavraController(CPlayer& Owner)
	: m_Owner{ Owner }
{
}

HRESULT CPlayer_AvadaKedavraController::Initialize()
{
	return S_OK;
}

void CPlayer_AvadaKedavraController::Update(_float fTimeDelta)
{
	if (m_bCastActive)
	{
		_float4x4 wandWorld{};
		if (!TryGetWandWorld(wandWorld))
		{
			StopCastEffect();
		}
		else
		{
			if (m_iCastEffectID != INVALID_EFFECT_INSTANCE_ID)
			{
				CGameInstance::Get().SetEffectWorldMatrix(
					m_iCastEffectID,
					wandWorld);
			}
			if (m_Owner.m_pComSound)
			{
				m_Owner.m_pComSound->SetSlot3DAttributes(
					E::StringID{ "PLAYER_AVADA_KEDAVRA_CAST" },
					{ wandWorld._41, wandWorld._42, wandWorld._43 });
			}

			EmitCastTrail({
				wandWorld._41,
				wandWorld._42,
				wandWorld._43
			});
		}
	}

	if (!m_bImpactPending)
		return;

	m_fImpactDelayRemaining -= std::max(0.f, fTimeDelta);
	if (m_fImpactDelayRemaining > 0.f)
		return;

	m_bImpactPending = false;
	PlayImpactEffects(m_vPendingImpactPosition);
}

void CPlayer_AvadaKedavraController::StartCastEffect()
{
	StopCastEffect();

	_float4x4 wandWorld{};
	if (!TryGetWandWorld(wandWorld))
		return;

	if (auto* pTrail = CGameInstance::Get().GetParticle(
		"AvadaKedavra_Cast_Energy_Trail",
		"AvadaKedavra_Cast_Energy_Trail"))
	{
		pTrail->SetColor({ 0.12f, 1.f, 0.24f, 1.f });
		pTrail->SetEmissive({ 0.04f, 1.f, 0.14f, 8.f });
	}

	m_bCastActive = true;
	m_bTrailRegistrationFailureLogged = false;
	EmitCastTrail({ wandWorld._41, wandWorld._42, wandWorld._43 });

	m_iCastEffectID = CGameInstance::Get().PlayEffect(
		"AvadaKedavra_Cast",
		wandWorld,
		XMVectorZero(),
		[hOwner = m_Owner.GetHandle()](
			EFFECT_INSTANCE_ID iEffectID,
			EFFECT_FINISH_REASON)
		{
			auto* pPlayer = CGameInstance::Get().
				GetGameObjectByHandleT<CPlayer>(hOwner);
			if (!pPlayer || !pPlayer->m_pAvadaKedavraController)
				return;

			auto& controller = *pPlayer->m_pAvadaKedavraController;
			if (controller.m_iCastEffectID == iEffectID)
			{
				controller.m_iCastEffectID =
					INVALID_EFFECT_INSTANCE_ID;
			}
		});

	if (m_Owner.m_pComSound)
	{
		m_Owner.m_pComSound->PlaySlot3D(
			E::StringID{ "PLAYER_AVADA_KEDAVRA_CAST" },
			"./Resources/SampleClient/Sound/Player/SkillEffect/AvadaKedavra/AvadaKedavra_Cast.wav",
			SOUND_3D_DESC{
				.vPosition = { wandWorld._41, wandWorld._42, wandWorld._43 },
				.fMinDistance = 1.f,
				.fMaxDistance = 80.f,
				.eRolloff = SOUND_3D_ROLLOFF::LINEAR
			},
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.65f,
				.fPitch = 1.f,
				.iPriority = 88,
				.bLoop = false
			});
	}
}

void CPlayer_AvadaKedavraController::StopCastEffect()
{
	m_bCastActive = false;
	if (m_Owner.m_pComSound)
	{
		m_Owner.m_pComSound->StopSlot(
			E::StringID{ "PLAYER_AVADA_KEDAVRA_CAST" });
	}

	if (m_iCastEffectID == INVALID_EFFECT_INSTANCE_ID)
		return;

	const EFFECT_INSTANCE_ID iEffectID = m_iCastEffectID;
	m_iCastEffectID = INVALID_EFFECT_INSTANCE_ID;
	CGameInstance::Get().StopEffect(iEffectID);
}

_bool CPlayer_AvadaKedavraController::ReleaseSpell()
{
	StopCastEffect();

	_float4x4 wandWorld{};
	if (!TryGetWandWorld(wandWorld))
		return false;

	const _float3 startPosition{
		wandWorld._41,
		wandWorld._42,
		wandWorld._43
	};

	_float3 targetPosition{};
	if (!ResolveTargetPosition(startPosition, targetPosition))
		return false;

	const _float3 effectTargetPosition = CalculateVisualTargetPosition(
		startPosition,
		targetPosition);

	CGameInstance::Get().PlayEffect(
		"AvadaKedavra_Release",
		wandWorld);
	if (m_Owner.m_pComSound)
	{
		m_Owner.m_pComSound->Play3D(
			"./Resources/SampleClient/Sound/Player/SkillEffect/AvadaKedavra/AvadaKedavra_Release.wav",
			SOUND_3D_DESC{
				.vPosition = startPosition,
				.fMinDistance = 1.f,
				.fMaxDistance = 100.f,
				.eRolloff = SOUND_3D_ROLLOFF::LINEAR
			},
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.8f,
				.fPitch = 1.f,
				.iPriority = 92,
				.bLoop = false
			});
	}
	CGameInstance::Get().PlayEffect(
		"AvadaKedavra_Beam",
		wandWorld,
		XMVectorSet(
			effectTargetPosition.x,
			effectTargetPosition.y,
			effectTargetPosition.z,
			1.f));

	// [LSY] 현재는 연출만 연결되어 있다. 실제 즉사/피해 처리는
	// m_Owner.GetTargetHandle()을 대상으로 이 지점에 연결한다.
	m_vPendingImpactPosition = effectTargetPosition;
	m_fImpactDelayRemaining = IMPACT_DELAY;
	m_bImpactPending = true;
	return true;
}

_bool CPlayer_AvadaKedavraController::TryGetWandWorld(
	_float4x4& OutWorld) const
{
	auto* pWeapon = CGameInstance::Get().
		GetGameObjectByHandleT<CPlayer_Weapon>(m_Owner.GetWeaponHandle());
	if (!pWeapon)
		return false;

	OutWorld = pWeapon->GetSpawnWorldMatrix();
	return true;
}

_float3 CPlayer_AvadaKedavraController::CalculateVisualTargetPosition(
	const _float3& vStartPosition,
	const _float3& vTargetPosition) const
{
	// [LSY] HurtBox 중심까지 굵은 Beam을 그리면 전기다발이 몸 안으로 파고들어 보인다.
	// 실제 게임플레이 타깃은 유지하고 렌더링 끝점과 피격 연출만 완드 방향으로 당긴다.
	_float3 visualTargetPosition = vTargetPosition;
	const _vector vTargetToWand =
		XMLoadFloat3(&vStartPosition) - XMLoadFloat3(&vTargetPosition);
	const _float fTargetDistance =
		XMVectorGetX(XMVector3Length(vTargetToWand));
	if (fTargetDistance <= FLT_EPSILON)
		return visualTargetPosition;

	constexpr _float BEAM_END_FORWARD_OFFSET = 4.f;
	const _float fAppliedOffset = std::min(
		BEAM_END_FORWARD_OFFSET,
		fTargetDistance * 0.5f);
	XMStoreFloat3(
		&visualTargetPosition,
		XMLoadFloat3(&vTargetPosition) +
		XMVector3Normalize(vTargetToWand) * fAppliedOffset);
	return visualTargetPosition;
}

_bool CPlayer_AvadaKedavraController::ResolveTargetPosition(
	const _float3& vStartPosition,
	_float3& OutTargetPosition) const
{
	const CHandle hTarget = m_Owner.GetTargetHandle();
	if (auto* pMonster = CGameInstance::Get().
		GetGameObjectByHandleT<CMonster>(hTarget))
	{
		OutTargetPosition = pMonster->GetHurtBoxPosition();
		return true;
	}

	if (auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(hTarget))
	{
		OutTargetPosition = pTarget->GetTransform().GetPosition();
		return true;
	}

	_vector vLook = m_Owner.GetTransform().GetState(STATE::LOOK);
	if (XMVectorGetX(XMVector3LengthSq(vLook)) <= FLT_EPSILON)
		return false;

	vLook = XMVector3Normalize(vLook);
	XMStoreFloat3(
		&OutTargetPosition,
		XMLoadFloat3(&vStartPosition) + vLook * 30.f);
	return true;
}

void CPlayer_AvadaKedavraController::EmitCastTrail(
	const _float3& vWandPosition)
{
	constexpr _float TRAIL_HALF_WIDTH = 0.12f;
	_float3 trailStart = vWandPosition;
	_float3 trailEnd = vWandPosition;
	trailStart.y += TRAIL_HALF_WIDTH;
	trailEnd.y -= TRAIL_HALF_WIDTH;

	const HRESULT hr = CGameInstance::Get().AddTrailPoint(
		"AvadaKedavra_Cast_Energy_Trail",
		"AvadaKedavra_Cast_Energy_Trail",
		m_Owner.GetHandle(),
		trailStart,
		trailEnd);

	if (FAILED(hr) && !m_bTrailRegistrationFailureLogged)
	{
		DEBUG_LOG(
			"[AvadaKedavra] Cast energy trail particle is not registered.\n");
		m_bTrailRegistrationFailureLogged = true;
	}
}

void CPlayer_AvadaKedavraController::PlayImpactEffects(
	const _float3& vImpactPosition) const
{
	_float4x4 impactWorld{};
	XMStoreFloat4x4(
		&impactWorld,
		XMMatrixTranslation(
			vImpactPosition.x,
			vImpactPosition.y,
			vImpactPosition.z));

	CGameInstance::Get().PlayEffect(
		"AvadaKedavra_Impact",
		impactWorld);
	CGameInstance::Get().PlayEffect(
		"AvadaKedavra_Residue",
		impactWorld);

	if (m_Owner.m_pComSound)
	{
		m_Owner.m_pComSound->Play3D(
			"./Resources/SampleClient/Sound/Player/SkillEffect/AvadaKedavra/AvadaKedavra_Impact.wav",
			SOUND_3D_DESC{
				.vPosition = vImpactPosition,
				.fMinDistance = 2.f,
				.fMaxDistance = 120.f,
				.eRolloff = SOUND_3D_ROLLOFF::LINEAR
			},
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.75f,
				.fPitch = 1.f,
				.iPriority = 94,
				.bLoop = false
			});
	}

	PlayImpactArcs(impactWorld, vImpactPosition);
}

void CPlayer_AvadaKedavraController::PlayImpactArcs(
	const _float4x4& impactWorld,
	const _float3& vImpactPosition) const
{
	struct ARC_DESC
	{
		std::string_view sEffectName;
		_float3 vEndOffset;
	};

	// [LSY] 같은 Beam 리소스를 시간대별 Effect Preset으로 재생해 피격 중 방전을 누적한다.
	static constexpr std::array<ARC_DESC, 12> ARC_DESCS = {
		ARC_DESC{ "AvadaKedavra_ImpactArc_Early", { 3.4f, 1.5f, 0.8f } },
		ARC_DESC{ "AvadaKedavra_ImpactArc_Early", { -3.f, 2.1f, -0.6f } },
		ARC_DESC{ "AvadaKedavra_ImpactArc_Mid", { 1.4f, -2.7f, 2.4f } },
		ARC_DESC{ "AvadaKedavra_ImpactArc_Mid", { -2.2f, -1.5f, 2.8f } },
		ARC_DESC{ "AvadaKedavra_ImpactArc_Mid", { 0.7f, 3.5f, -2.2f } },
		ARC_DESC{ "AvadaKedavra_ImpactArc_Mid", { -0.9f, 3.f, 2.5f } },
		ARC_DESC{ "AvadaKedavra_ImpactArc_Late", { 3.1f, -0.8f, -2.1f } },
		ARC_DESC{ "AvadaKedavra_ImpactArc_Late", { -3.3f, 0.6f, -2.f } },
		ARC_DESC{ "AvadaKedavra_ImpactArc_Late", { 2.f, 1.4f, 3.2f } },
		ARC_DESC{ "AvadaKedavra_ImpactArc_Late", { -2.5f, 1.1f, 3.1f } },
		ARC_DESC{ "AvadaKedavra_ImpactArc_Late", { 0.3f, -3.2f, -2.6f } },
		ARC_DESC{ "AvadaKedavra_ImpactArc_Late", { 1.2f, 2.4f, -3.f } }
	};

	const _vector vImpact = XMLoadFloat3(&vImpactPosition);
	constexpr _float IMPACT_ARC_RADIUS_SCALE = 1.f;
	for (const ARC_DESC& ArcDesc : ARC_DESCS)
	{
		const _vector vArcEnd = vImpact +
			XMLoadFloat3(&ArcDesc.vEndOffset) *
			IMPACT_ARC_RADIUS_SCALE;
		CGameInstance::Get().PlayEffect(
			std::string{ ArcDesc.sEffectName },
			impactWorld,
			XMVectorSetW(vArcEnd, 1.f));
	}
}

UPtr<CPlayer_AvadaKedavraController>
CPlayer_AvadaKedavraController::Create(CPlayer& Owner)
{
	auto pInstance = ToUPtr(new CPlayer_AvadaKedavraController{ Owner });
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create: CPlayer_AvadaKedavraController");
		return nullptr;
	}

	return pInstance;
}

void CPlayer_AvadaKedavraController::Free()
{
	StopCastEffect();
	m_bImpactPending = false;
	CEngineBase::Free();
}

NS_END
