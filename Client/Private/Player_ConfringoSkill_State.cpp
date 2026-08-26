#include "pch.h"
#include "Player_ConfringoSkill_State.h"

#include "ComAnimator.h"
#include "ComSound.h"
#include "Monster.h"
#include "Player.h"
#include "PlayerAnimationRatioGuard.h"
#include "Player_Confringo_Bullet.h"
#include "Player_Weapon.h"

NS_USING(Client)

void CPlayer_ConfringoSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !HasTarget(*pPlayer) || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	SetSkillControl(*pPlayer, true, true, true);
	if (!PlayTargetAttack(*pPlayer, true, 0.14f))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	if (auto* pSound = pPlayer->GetSound())
	{
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_VOICE_CONFRINGO" },
			"./Resources/SampleClient/Sound/Player/SkillEffect/Confringo/Confringo_Voice_Male.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 80,
				.bLoop = false
			});
	}

	pPlayer->SetCurrentMoveSpeed(0.f);
	m_ePhase = PHASE::CAST;
	m_fAnimationRatio = 0.f;
	m_bCastingCueReached = false;
	m_bProjectileCueReached = false;
}

void CPlayer_ConfringoSkill_State::Update(
	CStateMachine* pStateMachine,
	_float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_fAnimationRatio = PlayerAnimationRatioGuard::Sanitize(
		pPlayer->GetAnimator()->GetPlayAnimRatio());
	pPlayer->SetCurrentMoveSpeed(0.f);

	if (!m_bCastingCueReached && m_fAnimationRatio >= CASTING_EFFECT_RATIO)
	{
		m_bCastingCueReached = true;
		StartCastEffect(*pPlayer);
	}

	if (!m_bProjectileCueReached &&
		m_fAnimationRatio >= PROJECTILE_RELEASE_RATIO)
	{
		m_bProjectileCueReached = true;
		if (!FireProjectile(*pPlayer))
			DEBUG_LOG("[Confringo] Failed to spawn projectile.\n");
	}

	switch (m_ePhase)
	{
	case PHASE::CAST:
		if (m_fAnimationRatio >= CAST_TO_RECOVERY_RATIO)
			m_ePhase = PHASE::RECOVERY;
		break;

	case PHASE::RECOVERY:
		if (m_fAnimationRatio >= RECOVERY_EXIT_RATIO ||
			pPlayer->GetAnimator()->GetFinish())
		{
			RequestLocomotion(pStateMachine);
		}
		break;
	}
}

void CPlayer_ConfringoSkill_State::LateUpdate(
	CStateMachine* pStateMachine,
	_float fTimeDelta)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		UpdateCastEffect(*pPlayer, fTimeDelta);
}

void CPlayer_ConfringoSkill_State::Exit(CStateMachine* pStateMachine)
{
	StopCastEffect();
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);

	m_ePhase = PHASE::CAST;
	m_fAnimationRatio = 0.f;
	m_bCastingCueReached = false;
	m_bProjectileCueReached = false;
}

_bool CPlayer_ConfringoSkill_State::EnsureParticleCommandsLoaded()
{
	if (m_FlameCommands.empty())
	{
		const auto* pCommands = CGameInstance::Get().FindCachedCommandQueue(
			"LSY_Confringo_CastFlame_Queue.json");
		if (pCommands)
			m_FlameCommands = *pCommands;
	}

	if (m_SparkCommands.empty())
	{
		const auto* pCommands = CGameInstance::Get().FindCachedCommandQueue(
			"LSY_Confringo_CastSparks_Queue.json");
		if (pCommands)
			m_SparkCommands = *pCommands;
	}

	return !m_FlameCommands.empty() && !m_SparkCommands.empty();
}

_bool CPlayer_ConfringoSkill_State::TryGetWandPosition(
	const CPlayer& player,
	_float3& OutPosition) const
{
	auto* pWeapon = CGameInstance::Get().
		GetGameObjectByHandleT<CPlayer_Weapon>(player.GetWeaponHandle());
	if (!pWeapon)
		return false;

	const _float4x4 wandWorld = pWeapon->GetSpawnWorldMatrix();
	OutPosition = { wandWorld._41, wandWorld._42, wandWorld._43 };
	return true;
}

void CPlayer_ConfringoSkill_State::StartCastEffect(CPlayer& player)
{
	if (!EnsureParticleCommandsLoaded())
		return;

	StopCastEffect();
	_float3 wandPosition{};
	if (!TryGetWandPosition(player, wandPosition))
		return;

	_float4x4 spawnWorld{};
	XMStoreFloat4x4(&spawnWorld, XMMatrixIdentity());
	spawnWorld._41 = wandPosition.x;
	spawnWorld._42 = wandPosition.y;
	spawnWorld._43 = wandPosition.z;

	m_iFlameOwnerId = CGameInstance::Get().Spawn(
		m_FlameCommands,
		spawnWorld);
	if (m_iFlameOwnerId == INVALID_PARTICLE_OWNER_ID)
		return;

	CGameInstance::Get().Spawn(m_SparkCommands, spawnWorld);
	m_vPreviousWandPosition = wandPosition;
	m_SparkControlPoints.fill(wandPosition);
	m_fSparkElapsed = 0.f;
	m_bCastEffectActive = true;
}

void CPlayer_ConfringoSkill_State::StopCastEffect()
{
	if (m_iFlameOwnerId != INVALID_PARTICLE_OWNER_ID)
		CGameInstance::Get().ClearParticleOwner(m_iFlameOwnerId);

	m_iFlameOwnerId = INVALID_PARTICLE_OWNER_ID;
	m_fSparkElapsed = 0.f;
	m_bCastEffectActive = false;
}

void CPlayer_ConfringoSkill_State::UpdateCastEffect(
	CPlayer& player,
	_float fTimeDelta)
{
	if (!m_bCastEffectActive)
		return;

	_float3 wandPosition{};
	if (!TryGetWandPosition(player, wandPosition))
	{
		StopCastEffect();
		return;
	}

	const _float3 delta{
		wandPosition.x - m_vPreviousWandPosition.x,
		wandPosition.y - m_vPreviousWandPosition.y,
		wandPosition.z - m_vPreviousWandPosition.z
	};
	CGameInstance::Get().TranslateOwner(m_iFlameOwnerId, delta);
	m_vPreviousWandPosition = wandPosition;

	m_fSparkElapsed += std::max(0.f, fTimeDelta);
	const _float sparkInterval = std::max(0.001f, m_fSparkInterval);
	if (m_fSparkElapsed < sparkInterval)
		return;

	m_fSparkElapsed = std::fmod(m_fSparkElapsed, sparkInterval);
	if (!EnsureParticleCommandsLoaded())
		return;

	m_SparkControlPoints[0] = m_SparkControlPoints[1];
	m_SparkControlPoints[1] = m_SparkControlPoints[2];
	m_SparkControlPoints[2] = m_SparkControlPoints[3];
	m_SparkControlPoints[3] = wandPosition;
	EmitSparkCurve();
}

void CPlayer_ConfringoSkill_State::EmitSparkCurve()
{
	if (m_SparkCommands.empty())
		return;

	const _vector vPoint0 = XMLoadFloat3(&m_SparkControlPoints[0]);
	const _vector vPoint1 = XMLoadFloat3(&m_SparkControlPoints[1]);
	const _vector vPoint2 = XMLoadFloat3(&m_SparkControlPoints[2]);
	const _vector vPoint3 = XMLoadFloat3(&m_SparkControlPoints[3]);
	if (XMVectorGetX(XMVector3LengthSq(vPoint2 - vPoint1)) <= FLT_EPSILON)
		return;

	constexpr uint32_t CURVE_LENGTH_SAMPLE_COUNT = 12;
	_float curveLength = 0.f;
	_vector vPreviousPoint = vPoint1;
	for (uint32_t i = 1; i <= CURVE_LENGTH_SAMPLE_COUNT; ++i)
	{
		const _float ratio = static_cast<_float>(i) /
			static_cast<_float>(CURVE_LENGTH_SAMPLE_COUNT);
		const _vector vCurvePoint = XMVectorCatmullRom(
			vPoint0, vPoint1, vPoint2, vPoint3, ratio);
		curveLength += XMVectorGetX(
			XMVector3Length(vCurvePoint - vPreviousPoint));
		vPreviousPoint = vCurvePoint;
	}

	if (curveLength <= FLT_EPSILON)
		return;

	const _float trailSpacing = std::max(0.001f, m_fSparkTrailSpacing);
	constexpr uint32_t MAX_TRAIL_SPAWN_PER_FRAME = 16;
	const uint32_t spawnCount = std::clamp(
		static_cast<uint32_t>(std::ceil(curveLength / trailSpacing)),
		1u,
		MAX_TRAIL_SPAWN_PER_FRAME);

	for (uint32_t i = 1; i <= spawnCount; ++i)
	{
		const _float ratio = static_cast<_float>(i) /
			static_cast<_float>(spawnCount);
		_float3 spawnPosition{};
		XMStoreFloat3(
			&spawnPosition,
			XMVectorCatmullRom(vPoint0, vPoint1, vPoint2, vPoint3, ratio));

		_float4x4 spawnWorld{};
		XMStoreFloat4x4(&spawnWorld, XMMatrixIdentity());
		spawnWorld._41 = spawnPosition.x;
		spawnWorld._42 = spawnPosition.y;
		spawnWorld._43 = spawnPosition.z;
		CGameInstance::Get().Spawn(m_SparkCommands, spawnWorld);
	}
}

_bool CPlayer_ConfringoSkill_State::FireProjectile(CPlayer& player)
{
	_float3 startPosition{};
	if (!TryGetWandPosition(player, startPosition))
		return false;

	_float3 targetPosition{};
	if (auto* pTarget = CGameInstance::Get().
		GetGameObjectByHandleT<CMonster>(player.GetTargetHandle()))
	{
		targetPosition = pTarget->GetHurtBoxPosition();
	}
	else
	{
		const _vector vStart = XMLoadFloat3(&startPosition);
		const _vector vLook = XMVector3Normalize(
			player.GetTransform().GetState(STATE::LOOK));
		XMStoreFloat3(&targetPosition, vStart + vLook * 30.f);
	}

	CPlayer_Confringo_Bullet::DESC Desc{};
	Desc.sObjectTag = "PlayerConfringoBullet";
	Desc.vStartPosition = startPosition;
	Desc.vEndPosition = targetPosition;
	Desc.hOwner = player.GetHandle();
	Desc.eSkillType = PLAYER_SKILL_TYPE::CONFRIGO;
	Desc.bDebugDraw = false;

	const auto hProjectile = CGameInstance::Get().AddGameObjectToLayer(
		player.GetLevelTag(),
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerConfringoBullet,
		"PlayerConfringoBullet",
		&Desc);
	if (!hProjectile)
		return false;

	auto* pProjectile = CGameInstance::Get().
		GetGameObjectByHandleT<CPlayer_Confringo_Bullet>(*hProjectile);
	if (!pProjectile)
		return true;

	_vector vProjectileDirection =
		pProjectile->GetTransform().GetState(STATE::LOOK);
	if (XMVectorGetX(XMVector3LengthSq(vProjectileDirection)) <= FLT_EPSILON)
		vProjectileDirection = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	else
		vProjectileDirection = XMVector3Normalize(vProjectileDirection);

	const _vector vMuzzleLook = -vProjectileDirection;
	_vector vMuzzleUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	_vector vMuzzleRight = XMVector3Cross(vMuzzleUp, vMuzzleLook);
	if (XMVectorGetX(XMVector3LengthSq(vMuzzleRight)) <= FLT_EPSILON)
	{
		vMuzzleUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		vMuzzleRight = XMVector3Cross(vMuzzleUp, vMuzzleLook);
	}

	vMuzzleRight = XMVector3Normalize(vMuzzleRight);
	vMuzzleUp = XMVector3Normalize(XMVector3Cross(vMuzzleLook, vMuzzleRight));
	_matrix muzzleWorld = XMMatrixIdentity();
	muzzleWorld.r[0] = XMVectorSetW(vMuzzleRight, 0.f);
	muzzleWorld.r[1] = XMVectorSetW(vMuzzleUp, 0.f);
	muzzleWorld.r[2] = XMVectorSetW(vMuzzleLook, 0.f);
	muzzleWorld.r[3] = XMVectorSetW(XMLoadFloat3(&startPosition), 1.f);

	_float4x4 muzzleParticleWorld{};
	XMStoreFloat4x4(&muzzleParticleWorld, muzzleWorld);
	CGameInstance::Get().Spawn(
		"LSY_Confringo_MuzzleV9_Queue.json",
		muzzleParticleWorld);

	if (auto* pSound = player.GetSound())
	{
		pSound->Play3D(
			"./Resources/SampleClient/Sound/Player/SkillEffect/Confringo/Confringo_Muzzle.wav",
			SOUND_3D_DESC{
				.vPosition = startPosition,
				.fMinDistance = 1.f,
				.fMaxDistance = 60.f,
				.eRolloff = SOUND_3D_ROLLOFF::LINEAR
			},
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.55f,
				.fPitch = 1.f,
				.iPriority = 82,
				.bLoop = false
			});
	}

	return true;
}

SPtr<CPlayer_ConfringoSkill_State> CPlayer_ConfringoSkill_State::Create()
{
	return ToSPtr(new CPlayer_ConfringoSkill_State{});
}

void CPlayer_ConfringoSkill_State::Free()
{
	StopCastEffect();
	CPlayer_SkillStateBase::Free();
}
