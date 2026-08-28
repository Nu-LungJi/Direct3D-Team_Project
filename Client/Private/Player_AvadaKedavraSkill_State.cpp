#include "pch.h"
#include "Player_AvadaKedavraSkill_State.h"

#include "ComAnimator.h"
#include "ComSound.h"
#include "GameInstance.h"
#include "Monster.h"
#include "NvClothCape.h"
#include "Particle.h"
#include "Player.h"
#include "PlayerAnimationRatioGuard.h"
#include "Player_Weapon.h"
#include "SkillTarget.h"

NS_USING(Client)

namespace
{
	constexpr std::string_view AVADA_FACIAL_MORPH = "Avada_Facial_Peak";
	constexpr std::string_view AVADA_FACIAL_FALLBACK_MORPH = "jaw_drop";
	const StringID AVADA_FACIAL_PREVIEW_TIME_SCALE_TAG{
		"Debug_AvadaFacialPreview" };
}

void CPlayer_AvadaKedavraSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_hOwner = pPlayer->GetHandle();
	StopCastEffect(pPlayer);
	StopBeamEffect(pPlayer);
	m_bImpactPending = false;

	CacheAnimationIndices(*pPlayer);
	if (m_iCastAnimation < 0)
	{
		DEBUG_LOG("[AvadaKedavra] Cast animation was not found.\n");
		RequestLocomotion(pStateMachine);
		return;
	}

	if (auto* pTarget = CGameInstance::Get().
		GetGameObjectByHandle(pPlayer->GetTargetHandle()))
	{
		_vector vTargetPosition = pTarget->GetTransform().GetState(STATE::POSITION);
		vTargetPosition = XMVectorSetY(
			vTargetPosition,
			XMVectorGetY(pPlayer->GetTransform().GetState(STATE::POSITION)));
		pPlayer->GetTransform().LookAt(vTargetPosition);
	}

	SetSkillControl(*pPlayer, true, false, false, true);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->GetAnimator()->Play_Anim(
		m_iCastAnimation,
		false, 
		CAST_BLEND_DURATION);

	if (m_iFacialAnimation >= 0)
	{
		pPlayer->GetAnimator()->Stop_UpperAnim(0.f);
		if (pPlayer->GetAnimator()->Set_UpperBodyRootBone("face", 1))
		{
			pPlayer->GetAnimator()->Play_UpperAnim(
				m_iFacialAnimation,
				false,
				0.f);
			pPlayer->GetAnimator()->SetUpperAnimationFadeOutDuration(0.1f);
		}
		else
		{
			DEBUG_LOG("[AvadaKedavra] Facial root bone was not found.\n");
		}
	}
	else
	{
		DEBUG_LOG("[AvadaKedavra] Facial bone animation was not found.\n");
	}

	m_iFacialMorphTarget = pPlayer->GetAnimator()->FindMorphTargetIndex(
		AVADA_FACIAL_MORPH);
	if (m_iFacialMorphTarget == UINT32_MAX)
	{
		m_iFacialMorphTarget = pPlayer->GetAnimator()->FindMorphTargetIndex(
			AVADA_FACIAL_FALLBACK_MORPH);
	}
	if (m_iFacialMorphTarget != UINT32_MAX)
		pPlayer->GetAnimator()->SetMorphPreview(m_iFacialMorphTarget, 1.f);
	else
		DEBUG_LOG("[AvadaKedavra] Facial Morph Target was not found.\n");

	FCinematicPlayOptions CinematicOptions{};
	CinematicOptions.eStartMode = ECinematicStartMode::Blend;
	CinematicOptions.fStartBlendDuration = 0.45f;
	CinematicOptions.eReturnMode = ECinematicReturnMode::Blend;
	CinematicOptions.fReturnBlendDuration = 0.35f;

	const HRESULT hrCinematicResult = CGameInstance::Get().PlayCinematic(
		"AvadaKedavra",
		pPlayer->GetHandle(),
		CinematicOptions);
	m_bCinematicStarted = hrCinematicResult == S_OK;
	if (FAILED(hrCinematicResult))
		DEBUG_LOG("[AvadaKedavra] Failed to play cinematic.\n");

	if (auto* pSound = pPlayer->GetSound())
	{
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_VOICE_AVADA_KEDAVRA" },
			"./Resources/SampleClient/Sound/Player/SkillEffect/AvadaKedavra/AvadaKedavra_Voice_Male.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 90,
				.bLoop = false
			});
	}

	m_ePhase = PHASE::CAST_BEGIN;
	m_fAnimationRatio = 0.f;
	StartCastEffect(*pPlayer);
}

void CPlayer_AvadaKedavraSkill_State::Update(
	CStateMachine* pStateMachine,
	_float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	auto* pAnimator = pPlayer ? pPlayer->GetAnimator() : nullptr;
	if (!pPlayer || !pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	pPlayer->SetCurrentMoveSpeed(0.f);
	if (m_bCinematicStarted && !CGameInstance::Get().IsCinematicPlaying())
		m_bCinematicStarted = false;

	m_fAnimationRatio = PlayerAnimationRatioGuard::Sanitize(
		pAnimator->GetPlayAnimRatio());

	if (m_iFacialMorphTarget != UINT32_MAX)
	{
		_float fMorphWeight = 1.f;
		if (m_fAnimationRatio >= RECOVERY_RATIO)
		{
			const _float fRecoveryRatio = std::clamp(
				(m_fAnimationRatio - RECOVERY_RATIO) /
				(RECOVERY_EXIT_RATIO - RECOVERY_RATIO),
				0.f,
				1.f);
			fMorphWeight = 1.f - fRecoveryRatio;
		}

		pAnimator->SetMorphPreview(m_iFacialMorphTarget, fMorphWeight);
	}

	switch (m_ePhase)
	{
	case PHASE::CAST_BEGIN:
		if (m_fAnimationRatio >= RELEASE_RATIO)
		{
			m_ePhase = PHASE::RELEASE;
			if (!ReleaseSpell(*pPlayer))
				DEBUG_LOG("[AvadaKedavra] Failed to release spell effect.\n");
		}
		break;

	case PHASE::RELEASE:
		if (m_fAnimationRatio >= RECOVERY_RATIO)
			m_ePhase = PHASE::RECOVERY;
		break;

	case PHASE::RECOVERY:
		if (m_fAnimationRatio >= RECOVERY_EXIT_RATIO || pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;
	}
}

void CPlayer_AvadaKedavraSkill_State::LateUpdate(
	CStateMachine* pStateMachine,
	_float fTimeDelta)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		UpdateSpellEffects(*pPlayer, fTimeDelta);
}

void CPlayer_AvadaKedavraSkill_State::Exit(CStateMachine* pStateMachine)
{
	const _bool bInterrupted = m_ePhase != PHASE::RECOVERY;
	if (bInterrupted && m_bCinematicStarted &&
		CGameInstance::Get().IsCinematicPlaying())
	{
		CGameInstance::Get().StopCinematic();
	}
	m_bCinematicStarted = false;
	if (CGameInstance::Get().IsTimeScaleActive(
		AVADA_FACIAL_PREVIEW_TIME_SCALE_TAG))
	{
		CGameInstance::Get().EndTimeScale(
			AVADA_FACIAL_PREVIEW_TIME_SCALE_TAG,
			0.15f);
	}

	auto* pPlayer = GetPlayer(pStateMachine);
	StopCastEffect(pPlayer);
	StopBeamEffect(pPlayer);
	m_bImpactPending = false;
	if (pPlayer && pPlayer->GetAnimator())
	{
		pPlayer->GetAnimator()->Stop_UpperAnim(0.f);
		pPlayer->GetAnimator()->Set_UpperBodyRootBone("RightArm", 1);
		pPlayer->GetAnimator()->ClearMorphPreview();
	}
	if (pPlayer)
		ResetSkillControl(*pPlayer);

	m_iFacialMorphTarget = UINT32_MAX;
	m_ePhase = PHASE::CAST_BEGIN;
	m_fAnimationRatio = 0.f;
}

void CPlayer_AvadaKedavraSkill_State::CacheAnimationIndices(
	const CPlayer& player)
{
	if (m_bAnimationsCached)
		return;

	m_iCastAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Finisher_03_Cast_anm.bin");
	m_iFacialAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_Facial_AM_Spell_Black_Particle_Explode_anm.bin");
	m_bAnimationsCached = true;
}

void CPlayer_AvadaKedavraSkill_State::UpdateSpellEffects(
	CPlayer& player,
	_float fTimeDelta)
{
	UpdateBeamClothWind(player);

	if (m_bCastActive)
	{
		_float4x4 wandWorld{};
		if (!TryGetWandWorld(player, wandWorld))
		{
			StopCastEffect(&player);
		}
		else
		{
			if (m_iCastEffectID != INVALID_EFFECT_INSTANCE_ID)
			{
				CGameInstance::Get().SetEffectWorldMatrix(
					m_iCastEffectID,
					wandWorld);
			}

			if (auto* pSound = player.GetSound())
			{
				pSound->SetSlot3DAttributes(
					E::StringID{ "PLAYER_AVADA_KEDAVRA_CAST" },
					{ wandWorld._41, wandWorld._42, wandWorld._43 });
			}

			EmitCastTrail(
				player,
				{ wandWorld._41, wandWorld._42, wandWorld._43 });
		}
	}

	if (!m_bImpactPending)
		return;

	m_fImpactDelayRemaining -= std::max(0.f, fTimeDelta);
	if (m_fImpactDelayRemaining > 0.f)
		return;

	m_bImpactPending = false;
	PlayImpactEffects(player, m_vPendingImpactPosition);
}

void CPlayer_AvadaKedavraSkill_State::StartCastEffect(CPlayer& player)
{
	StopCastEffect(&player);

	_float4x4 wandWorld{};
	if (!TryGetWandWorld(player, wandWorld))
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
	EmitCastTrail(
		player,
		{ wandWorld._41, wandWorld._42, wandWorld._43 });

	m_iCastEffectID = CGameInstance::Get().PlayEffect(
		"AvadaKedavra_Cast",
		wandWorld,
		XMVectorZero(),
		[this](
			EFFECT_INSTANCE_ID iEffectID,
			EFFECT_FINISH_REASON)
		{
			if (m_iCastEffectID == iEffectID)
				m_iCastEffectID = INVALID_EFFECT_INSTANCE_ID;
		});

	if (auto* pSound = player.GetSound())
	{
		pSound->PlaySlot3D(
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

void CPlayer_AvadaKedavraSkill_State::StopCastEffect(CPlayer* pPlayer)
{
	m_bCastActive = false;
	if (pPlayer && pPlayer->GetSound())
	{
		pPlayer->GetSound()->StopSlot(
			E::StringID{ "PLAYER_AVADA_KEDAVRA_CAST" });
	}

	if (m_iCastEffectID == INVALID_EFFECT_INSTANCE_ID)
		return;

	const EFFECT_INSTANCE_ID iEffectID = m_iCastEffectID;
	m_iCastEffectID = INVALID_EFFECT_INSTANCE_ID;
	CGameInstance::Get().StopEffect(iEffectID);
}

void CPlayer_AvadaKedavraSkill_State::StopBeamEffect(CPlayer* pPlayer)
{
	const EFFECT_INSTANCE_ID iEffectID = m_iBeamEffectID;
	m_iBeamEffectID = INVALID_EFFECT_INSTANCE_ID;
	ClearBeamClothWind(pPlayer);
	if (iEffectID != INVALID_EFFECT_INSTANCE_ID)
		CGameInstance::Get().StopEffect(iEffectID);
}

_bool CPlayer_AvadaKedavraSkill_State::ReleaseSpell(CPlayer& player)
{
	StopCastEffect(&player);

	_float4x4 wandWorld{};
	if (!TryGetWandWorld(player, wandWorld))
		return false;

	const _float3 startPosition{
		wandWorld._41,
		wandWorld._42,
		wandWorld._43
	};

	_float3 targetPosition{};
	if (!ResolveTargetPosition(player, startPosition, targetPosition))
		return false;

	const _float3 effectTargetPosition = CalculateVisualTargetPosition(
		startPosition,
		targetPosition);
	const _vector vCastDirection =
		XMLoadFloat3(&effectTargetPosition) - XMLoadFloat3(&startPosition);
	const _float fCastDistance = XMVectorGetX(XMVector3Length(vCastDirection));
	_float3 vClothWind{};
	if (fCastDistance > FLT_EPSILON)
	{
		_vector vCapeWindDirection = XMVectorSetY(
			player.GetTransform().GetState(STATE::LOOK),
			0.f);
		if (XMVectorGetX(XMVector3LengthSq(vCapeWindDirection)) <= FLT_EPSILON)
			vCapeWindDirection = XMVectorSetY(vCastDirection, 0.f);

		if (XMVectorGetX(XMVector3LengthSq(vCapeWindDirection)) > FLT_EPSILON)
		{
			XMStoreFloat3(
				&vClothWind,
				-XMVector3Normalize(vCapeWindDirection) * CLOTH_WIND_SPEED);
		}
	}

	CGameInstance::Get().PlayEffect("AvadaKedavra_Release", wandWorld);
	if (auto* pSound = player.GetSound())
	{
		pSound->Play3D(
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

	StopBeamEffect(&player);
	const CHandle hOwner = player.GetHandle();
	m_iBeamEffectID = CGameInstance::Get().PlayEffect(
		"AvadaKedavra_Beam",
		wandWorld,
		XMVectorSet(
			effectTargetPosition.x,
			effectTargetPosition.y,
			effectTargetPosition.z,
			1.f),
		[this, hOwner](
			EFFECT_INSTANCE_ID iEffectID,
			EFFECT_FINISH_REASON)
		{
			if (m_iBeamEffectID != iEffectID)
				return;

			m_iBeamEffectID = INVALID_EFFECT_INSTANCE_ID;
			auto* pPlayer = CGameInstance::Get().
				GetGameObjectByHandleT<CPlayer>(hOwner);
			ClearBeamClothWind(pPlayer);
		});

	if (m_iBeamEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		m_vBeamClothWindVelocity = vClothWind;
		UpdateBeamClothWind(player);
	}

	if (auto* pTargetObject = CGameInstance::Get().
		GetGameObjectByHandle(player.GetTargetHandle()))
	{
		if (auto* pSkillTarget = Engine::Cast<CSkillTarget>(pTargetObject))
			pSkillTarget->Check_Table(PLAYER_SKILL_TYPE::ABRA);
	}

	m_vPendingImpactPosition = effectTargetPosition;
	m_fImpactDelayRemaining = IMPACT_DELAY;
	m_bImpactPending = true;
	return true;
}

_bool CPlayer_AvadaKedavraSkill_State::TryGetWandWorld(
	const CPlayer& player,
	_float4x4& OutWorld) const
{
	auto* pWeapon = CGameInstance::Get().
		GetGameObjectByHandleT<CPlayer_Weapon>(player.GetWeaponHandle());
	if (!pWeapon)
		return false;

	OutWorld = pWeapon->GetSpawnWorldMatrix();
	return true;
}

_bool CPlayer_AvadaKedavraSkill_State::ResolveTargetPosition(
	const CPlayer& player,
	const _float3& vStartPosition,
	_float3& OutTargetPosition) const
{
	const CHandle hTarget = player.GetTargetHandle();
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

	_vector vLook = player.GetTransform().GetState(STATE::LOOK);
	if (XMVectorGetX(XMVector3LengthSq(vLook)) <= FLT_EPSILON)
		return false;

	vLook = XMVector3Normalize(vLook);
	XMStoreFloat3(
		&OutTargetPosition,
		XMLoadFloat3(&vStartPosition) + vLook * 30.f);
	return true;
}

_float3 CPlayer_AvadaKedavraSkill_State::CalculateVisualTargetPosition(
	const _float3& vStartPosition,
	const _float3& vTargetPosition) const
{
	_float3 visualTargetPosition = vTargetPosition;
	const _vector vTargetToWand =
		XMLoadFloat3(&vStartPosition) - XMLoadFloat3(&vTargetPosition);
	const _float fTargetDistance = XMVectorGetX(XMVector3Length(vTargetToWand));
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

void CPlayer_AvadaKedavraSkill_State::EmitCastTrail(
	CPlayer& player,
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
		player.GetHandle(),
		trailStart,
		trailEnd);
	if (FAILED(hr) && !m_bTrailRegistrationFailureLogged)
	{
		DEBUG_LOG("[AvadaKedavra] Cast energy trail particle is not registered.\n");
		m_bTrailRegistrationFailureLogged = true;
	}
}

void CPlayer_AvadaKedavraSkill_State::UpdateBeamClothWind(CPlayer& player)
{
	if (m_iBeamEffectID == INVALID_EFFECT_INSTANCE_ID)
		return;

	if (auto* pCape = CGameInstance::Get().
		GetGameObjectByHandleT<CNvClothCape>(player.GetCapeHandle()))
	{
		pCape->RequestClothWindImpulse(
			m_vBeamClothWindVelocity,
			CLOTH_WIND_REFRESH_DURATION);
	}
}

void CPlayer_AvadaKedavraSkill_State::ClearBeamClothWind(CPlayer* pPlayer)
{
	m_vBeamClothWindVelocity = {};
	if (!pPlayer)
		return;

	if (auto* pCape = CGameInstance::Get().
		GetGameObjectByHandleT<CNvClothCape>(pPlayer->GetCapeHandle()))
	{
		pCape->RequestClothWindImpulse({}, 0.001f);
	}
}

void CPlayer_AvadaKedavraSkill_State::PlayImpactEffects(
	CPlayer& player,
	const _float3& vImpactPosition) const
{
	_float4x4 impactWorld{};
	XMStoreFloat4x4(
		&impactWorld,
		XMMatrixTranslation(
			vImpactPosition.x,
			vImpactPosition.y,
			vImpactPosition.z));

	CGameInstance::Get().PlayEffect("AvadaKedavra_Impact", impactWorld);
	CGameInstance::Get().PlayEffect("AvadaKedavra_Residue", impactWorld);
	if (auto* pSound = player.GetSound())
	{
		pSound->Play3D(
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

void CPlayer_AvadaKedavraSkill_State::PlayImpactArcs(
	const _float4x4& impactWorld,
	const _float3& vImpactPosition) const
{
	struct ARC_DESC
	{
		std::string_view sEffectName;
		_float3 vEndOffset;
	};

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
	for (const ARC_DESC& ArcDesc : ARC_DESCS)
	{
		const _vector vArcEnd = vImpact + XMLoadFloat3(&ArcDesc.vEndOffset);
		CGameInstance::Get().PlayEffect(
			std::string{ ArcDesc.sEffectName },
			impactWorld,
			XMVectorSetW(vArcEnd, 1.f));
	}
}

SPtr<CPlayer_AvadaKedavraSkill_State>
CPlayer_AvadaKedavraSkill_State::Create()
{
	return ToSPtr(new CPlayer_AvadaKedavraSkill_State{});
}

void CPlayer_AvadaKedavraSkill_State::Free()
{
	auto* pPlayer = CGameInstance::Get().
		GetGameObjectByHandleT<CPlayer>(m_hOwner);
	StopCastEffect(pPlayer);
	StopBeamEffect(pPlayer);
	m_bImpactPending = false;
	CPlayer_SkillStateBase::Free();
}
