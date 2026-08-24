#include "pch.h"
#include "Player_BombardaSkill_State.h"

#include "ComAnimator.h"
#include "ComSound.h"
#include "Monster.h"
#include "Particle.h"
#include "Player.h"
#include "PlayerAnimationRatioGuard.h"
#include "Player_Bombarda_Bullet.h"
#include "Player_Weapon.h"

NS_USING(Client)

void CPlayer_BombardaSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !HasTarget(*pPlayer) || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	SetSkillControl(*pPlayer, true, false, true, true);
	pPlayer->SetCurrentMoveSpeed(0.f);

	if (!PlayTargetAttack(*pPlayer, true, 0.14f))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	if (auto* pSound = pPlayer->GetSound())
	{
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_VOICE_BOMBARDA" },
			"./Resources/SampleClient/Sound/Player/SkillEffect/Bombarda/Bombarda_Voice_Male.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 80,
				.bLoop = false
			});
	}

	pPlayer->SetRootMotionTranslationActive(false);
	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_bCastingEffectCueReached = false;
	m_bReleaseEffectCueReached = false;
}

void CPlayer_BombardaSkill_State::Update(
	CStateMachine* pStateMachine,
	_float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	auto* pAnimator = pPlayer->GetAnimator();
	m_fAnimRatio = PlayerAnimationRatioGuard::Sanitize(
		pAnimator->GetPlayAnimRatio());
	pPlayer->SetCurrentMoveSpeed(0.f);

	if (!m_bCastingEffectCueReached &&
		m_fAnimRatio >= CASTING_EFFECT_RATIO)
	{
		m_bCastingEffectCueReached = true;
		StartCastEffect(*pPlayer);
	}

	if (!m_bReleaseEffectCueReached &&
		m_fAnimRatio >= RELEASE_EFFECT_RATIO)
	{
		m_bReleaseEffectCueReached = true;
		m_ePhase = PHASE::RELEASE;
		if (!FireProjectile(*pPlayer))
			DEBUG_LOG("[Bombarda] Failed to spawn projectile.\n");
	}

	switch (m_ePhase)
	{
	case PHASE::CAST:
	{
		auto* pWeapon = CGameInstance::Get().
			GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());
		if (!pWeapon)
			return;

		_float4x4 mat;
		XMStoreFloat4x4(&mat,
		XMMatrixTranslation(pWeapon->GetSpawnWorldMatrix()._41, pWeapon->GetSpawnWorldMatrix()._42, pWeapon->GetSpawnWorldMatrix()._43));
		
		CGameInstance::Get().Spawn("Bombarda_CastEnd_Particle_Queue.json", mat);
	}
		break;

	case PHASE::RELEASE:
		if (m_fAnimRatio >= RELEASE_TO_RECOVERY_RATIO)
		{
			auto* pWeapon = CGameInstance::Get().
				GetGameObjectByHandleT<CPlayer_Weapon>(
					pPlayer->GetWeaponHandle());

			if (!pWeapon)
				return;

			const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();

			const _matrix currentWorld = XMLoadFloat4x4(&spawnWorld);

			const _vector effectPosition = currentWorld.r[3];

			CGameInstance::Get().Set_ChromaticRingOpacity(0.2f);

			CGameInstance::Get().Render_ChromaticRing(effectPosition,0.5f,100);

			m_ePhase = PHASE::RECOVERY;
		}
		break;

	case PHASE::RECOVERY:
		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO || pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;
	}
}

void CPlayer_BombardaSkill_State::LateUpdate(
	CStateMachine* pStateMachine,
	_float)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		UpdateCastEffect(*pPlayer);
}

void CPlayer_BombardaSkill_State::Exit(CStateMachine* pStateMachine)
{
	StopCastEffect();
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_bCastingEffectCueReached = false;
	m_bReleaseEffectCueReached = false;
}

void CPlayer_BombardaSkill_State::UpdateCastEffect(CPlayer& player)
{
	if (!m_bCastActive)
		return;

	_float4x4 wandWorld{};
	if (!TryGetWandWorld(player, wandWorld))
	{
		StopCastEffect();
		return;
	}

	if (m_iCastEffectID != INVALID_EFFECT_INSTANCE_ID)
		CGameInstance::Get().SetEffectWorldMatrix(m_iCastEffectID, wandWorld);

	const _float3 currentPosition{
		wandWorld._41,
		wandWorld._42,
		wandWorld._43
	};
	m_CastTrailControlPoints[0] = m_CastTrailControlPoints[1];
	m_CastTrailControlPoints[1] = m_CastTrailControlPoints[2];
	m_CastTrailControlPoints[2] = m_CastTrailControlPoints[3];
	m_CastTrailControlPoints[3] = currentPosition;
	EmitCastParticleCurve();
	EmitCastEnergyTrail(player, currentPosition);
}

void CPlayer_BombardaSkill_State::StartCastEffect(CPlayer& player)
{
	StopCastEffect();

	_float4x4 wandWorld{};
	if (!TryGetWandWorld(player, wandWorld))
		return;

	EnsureCastParticleCommandsLoaded();
	if (auto* pTrail = CGameInstance::Get().GetParticle(
		"Bombarda_Cast_Energy_Trail",
		"Bombarda_Cast_Energy_Trail"))
	{
		pTrail->SetColor({ 1.f,0.55f,0.05f,1.f });
		pTrail->SetEmissive({ 0.03f,0.32f,1.f,30.f });
	}

	const _float3 wandPosition{
		wandWorld._41,
		wandWorld._42,
		wandWorld._43
	};
	m_CastTrailControlPoints.fill(wandPosition);
	m_bCastActive = true;
	m_bTrailRegistrationFailureLogged = false;
	EmitCastEnergyTrail(player, wandPosition);

	m_iCastEffectID = CGameInstance::Get().PlayEffect(
		"Bombarda_Cast",
		wandWorld,
		XMVectorZero(),
		[this](
			EFFECT_INSTANCE_ID iEffectID,
			EFFECT_FINISH_REASON)
		{
			if (m_iCastEffectID == iEffectID)
				m_iCastEffectID = INVALID_EFFECT_INSTANCE_ID;
		});
}

void CPlayer_BombardaSkill_State::StopCastEffect()
{
	m_bCastActive = false;
	if (m_iCastEffectID == INVALID_EFFECT_INSTANCE_ID)
		return;

	const EFFECT_INSTANCE_ID iEffectID = m_iCastEffectID;
	m_iCastEffectID = INVALID_EFFECT_INSTANCE_ID;
	CGameInstance::Get().StopEffect(iEffectID);
}

_bool CPlayer_BombardaSkill_State::FireProjectile(CPlayer& player)
{
	StopCastEffect();

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

	CPlayer_Bombarda_Bullet::DESC Desc{};
	Desc.sObjectTag = "PlayerBombardaBullet";
	Desc.vStartPosition = startPosition;
	Desc.vEndPosition = targetPosition;
	Desc.hOwner = player.GetHandle();
	Desc.bDebugDraw = false;

	const auto hProjectile = CGameInstance::Get().AddGameObjectToLayer(
		player.GetLevelTag(),
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerBombardaBullet,
		"PlayerBombardaBullet",
		&Desc);
	if (!hProjectile)
		return false;

	CGameInstance::Get().PlayEffect("Bombarda_Muzzle", wandWorld);
	if (auto* pSound = player.GetSound())
	{
		pSound->Play3D(
			"./Resources/SampleClient/Sound/Player/SkillEffect/Bombarda/Bombarda_Muzzle.wav",
			SOUND_3D_DESC{
				.vPosition = startPosition,
				.fMinDistance = 1.f,
				.fMaxDistance = 70.f,
				.eRolloff = SOUND_3D_ROLLOFF::LINEAR
			},
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.65f,
				.fPitch = 1.f,
				.iPriority = 84,
				.bLoop = false
			});
	}

	return true;
}

void CPlayer_BombardaSkill_State::EnsureCastParticleCommandsLoaded()
{
	if (m_CastParticleCommands.empty())
	{
		m_CastParticleCommands = CGameInstance::Get().Parse_Command(
			"LSY_Bombarda_Cast_Particle_Queue.json");
	}
}

_bool CPlayer_BombardaSkill_State::TryGetWandWorld(
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

_bool CPlayer_BombardaSkill_State::ResolveTargetPosition(
	const CPlayer& player,
	const _float3& vStartPosition,
	_float3& OutTargetPosition) const
{
	if (auto* pTarget = CGameInstance::Get().
		GetGameObjectByHandleT<CMonster>(player.GetTargetHandle()))
	{
		OutTargetPosition = pTarget->GetHurtBoxPosition();
		return true;
	}

	_vector vLook = player.GetTransform().GetState(STATE::LOOK);
	vLook = XMVectorSetY(vLook, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vLook)) <= FLT_EPSILON)
		return false;

	vLook = XMVector3Normalize(vLook);
	XMStoreFloat3(
		&OutTargetPosition,
		XMLoadFloat3(&vStartPosition) + vLook * 30.f);
	return true;
}

void CPlayer_BombardaSkill_State::EmitCastParticleCurve() const
{
	if (m_CastParticleCommands.empty())
		return;

	const _vector vPoint0 = XMLoadFloat3(&m_CastTrailControlPoints[0]);
	const _vector vPoint1 = XMLoadFloat3(&m_CastTrailControlPoints[1]);
	const _vector vPoint2 = XMLoadFloat3(&m_CastTrailControlPoints[2]);
	const _vector vPoint3 = XMLoadFloat3(&m_CastTrailControlPoints[3]);
	if (XMVectorGetX(XMVector3LengthSq(vPoint2 - vPoint1)) <= FLT_EPSILON)
		return;

	constexpr uint32_t CURVE_LENGTH_SAMPLE_COUNT = 12;
	_float fCurveLength = 0.f;
	_vector vPreviousSample = vPoint1;
	for (uint32_t i = 1; i <= CURVE_LENGTH_SAMPLE_COUNT; ++i)
	{
		const _float fRatio = static_cast<_float>(i) /
			static_cast<_float>(CURVE_LENGTH_SAMPLE_COUNT);
		const _vector vCurrentSample = XMVectorCatmullRom(
			vPoint0, vPoint1, vPoint2, vPoint3, fRatio);
		fCurveLength += XMVectorGetX(
			XMVector3Length(vCurrentSample - vPreviousSample));
		vPreviousSample = vCurrentSample;
	}

	const _float fSpacing = std::max(m_fCastParticleSpacing, 0.01f);
	constexpr uint32_t MAX_TRAIL_SPAWN_PER_FRAME = 12;
	//const uint32_t iSpawnCount = std::clamp(
	//	static_cast<uint32_t>(std::ceil(fCurveLength / fSpacing)),
	//	1u,
	//	MAX_TRAIL_SPAWN_PER_FRAME);

	//for (uint32_t i = 1; i <= iSpawnCount; ++i)
	//{
	//	const _float fRatio = static_cast<_float>(i) /
	//		static_cast<_float>(iSpawnCount);
	//	_float3 spawnPosition{};
	//	XMStoreFloat3(
	//		&spawnPosition,
	//		XMVectorCatmullRom(vPoint0, vPoint1, vPoint2, vPoint3, fRatio));

	//	_float4x4 spawnWorld{};
	//	XMStoreFloat4x4(&spawnWorld, XMMatrixIdentity());
	//	spawnWorld._41 = spawnPosition.x;
	//	spawnWorld._42 = spawnPosition.y;
	//	spawnWorld._43 = spawnPosition.z;
	//	CGameInstance::Get().Spawn(m_CastParticleCommands, spawnWorld);
	//}
}

void CPlayer_BombardaSkill_State::EmitCastEnergyTrail(
	CPlayer& player,
	const _float3& vWandPosition)
{
	constexpr _float TRAIL_HALF_WIDTH = 0.2f;
	_float3 vTrailStart = vWandPosition;
	_float3 vTrailEnd = vWandPosition;
	vTrailStart.y += TRAIL_HALF_WIDTH;
	vTrailEnd.y -= TRAIL_HALF_WIDTH;

	const HRESULT hr = CGameInstance::Get().AddTrailPoint(
		"Bombarda_Cast_Energy_Trail",
		"Bombarda_Cast_Energy_Trail",
		player.GetHandle(),
		vTrailStart,
		vTrailEnd);

	if (FAILED(hr) && !m_bTrailRegistrationFailureLogged)
	{
		DEBUG_LOG("[Bombarda] Cast energy trail particle is not registered.\n");
		m_bTrailRegistrationFailureLogged = true;
	}
}

SPtr<CPlayer_BombardaSkill_State> CPlayer_BombardaSkill_State::Create()
{
	return ToSPtr(new CPlayer_BombardaSkill_State{});
}

void CPlayer_BombardaSkill_State::Free()
{
	StopCastEffect();
	CPlayer_SkillStateBase::Free();
}
