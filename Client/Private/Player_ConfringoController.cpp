#include "pch.h"
#include "Player_ConfringoController.h"

#include "Player.h"

#include "ComSound.h"
#include "Monster.h"
#include "Player_Confringo_Bullet.h"
#include "Player_Weapon.h"

NS_BEGIN(Client)

CPlayer_ConfringoController::CPlayer_ConfringoController(CPlayer& Owner)
	: m_Owner{ Owner }
{
}

HRESULT CPlayer_ConfringoController::Initialize()
{
	return S_OK;
}

_bool CPlayer_ConfringoController::EnsureParticleCommandsLoaded()
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

_bool CPlayer_ConfringoController::TryGetWandPosition(
	_float3& OutPosition) const
{
	auto* pWeapon = CGameInstance::Get().
		GetGameObjectByHandleT<CPlayer_Weapon>(m_Owner.GetWeaponHandle());
	if (!pWeapon)
		return false;

	const _float4x4 wandWorld = pWeapon->GetSpawnWorldMatrix();
	OutPosition = { wandWorld._41, wandWorld._42, wandWorld._43 };
	return true;
}

void CPlayer_ConfringoController::UpdateGUI()
{
	if (!ImGui::CollapsingHeader("[LSY] Confringo Debug"))
		return;

	const char* pCastButtonLabel = m_bCastEffectActive
		? "Stop Cast Effect"
		: "Start Cast Effect";
	if (ImGui::Button(pCastButtonLabel))
	{
		if (m_bCastEffectActive)
			StopCastEffect();
		else
			StartCastEffect();
	}

	ImGui::DragFloat(
		"Spark Interval",
		&m_fSparkInterval,
		0.005f,
		0.02f,
		0.2f);
	ImGui::DragFloat(
		"Spark Trail Spacing",
		&m_fSparkTrailSpacing,
		0.005f,
		0.02f,
		0.5f);
}

void CPlayer_ConfringoController::StartCastEffect()
{
	if (!EnsureParticleCommandsLoaded())
		return;

	// [LSY] 상태 재진입이나 GUI 반복 입력으로 루프 파티클이 중복되지 않게
	// 기존 캐스팅 효과를 먼저 정리한다.
	StopCastEffect();

	_float3 wandPosition{};
	if (!TryGetWandPosition(wandPosition))
		return;

	_float4x4 spawnWorld{};
	XMStoreFloat4x4(&spawnWorld, XMMatrixIdentity());
	spawnWorld._41 = wandPosition.x;
	spawnWorld._42 = wandPosition.y;
	spawnWorld._43 = wandPosition.z;

	m_iFlameOwnerId =
		CGameInstance::Get().Spawn(m_FlameCommands, spawnWorld);
	if (m_iFlameOwnerId == INVALID_PARTICLE_OWNER_ID)
		return;

	CGameInstance::Get().Spawn(m_SparkCommands, spawnWorld);

	// [LSY] 완드 스윙음은 현재 연출과 맞지 않아 비활성화했다.
	// 다시 필요하면 이 지점에서 완드 위치 기반 3D 사운드를 재생한다.

	m_vPreviousWandPosition = wandPosition;
	m_SparkControlPoints.fill(wandPosition);
	m_fSparkElapsed = 0.f;
	m_bCastEffectActive = true;
}

void CPlayer_ConfringoController::StopCastEffect()
{
	if (m_iFlameOwnerId != INVALID_PARTICLE_OWNER_ID)
	{
		CGameInstance::Get().ClearParticleOwner(
			m_iFlameOwnerId);
	}

	m_iFlameOwnerId = INVALID_PARTICLE_OWNER_ID;
	m_fSparkElapsed = 0.f;
	m_bCastEffectActive = false;
}

void CPlayer_ConfringoController::Update(_float fTimeDelta)
{
	if (!m_bCastEffectActive)
		return;

	_float3 wandPosition{};
	if (!TryGetWandPosition(wandPosition))
	{
		StopCastEffect();
		return;
	}
	const _float3 delta{
		wandPosition.x - m_vPreviousWandPosition.x,
		wandPosition.y - m_vPreviousWandPosition.y,
		wandPosition.z - m_vPreviousWandPosition.z
	};

	CGameInstance::Get().TranslateOwner(
		m_iFlameOwnerId,
		delta);
	m_vPreviousWandPosition = wandPosition;

	m_fSparkElapsed += std::max(0.f, fTimeDelta);
	const _float sparkInterval = std::max(0.001f, m_fSparkInterval);
	if (m_fSparkElapsed < sparkInterval)
		return;

	m_fSparkElapsed = std::fmod(
		m_fSparkElapsed,
		sparkInterval);

	if (!EnsureParticleCommandsLoaded())
		return;

	m_SparkControlPoints[0] = m_SparkControlPoints[1];
	m_SparkControlPoints[1] = m_SparkControlPoints[2];
	m_SparkControlPoints[2] = m_SparkControlPoints[3];
	m_SparkControlPoints[3] = wandPosition;

	EmitSparkCurve();
}

void CPlayer_ConfringoController::EmitSparkCurve()
{
	if (m_SparkCommands.empty())
		return;

	const _vector vPoint0 =
		XMLoadFloat3(&m_SparkControlPoints[0]);
	const _vector vPoint1 =
		XMLoadFloat3(&m_SparkControlPoints[1]);
	const _vector vPoint2 =
		XMLoadFloat3(&m_SparkControlPoints[2]);
	const _vector vPoint3 =
		XMLoadFloat3(&m_SparkControlPoints[3]);
	if (XMVectorGetX(XMVector3LengthSq(vPoint2 - vPoint1)) <=
		FLT_EPSILON)
	{
		return;
	}

	// [LSY] 네 개의 완드 위치로 곡선 길이를 근사해 프레임이 달라도
	// 비슷한 간격으로 스파크가 생성되게 한다.
	constexpr uint32_t CURVE_LENGTH_SAMPLE_COUNT = 12;
	_float curveLength = 0.f;
	_vector vPreviousPoint = vPoint1;
	for (uint32_t i = 1; i <= CURVE_LENGTH_SAMPLE_COUNT; ++i)
	{
		const _float ratio = static_cast<_float>(i) /
			static_cast<_float>(CURVE_LENGTH_SAMPLE_COUNT);
		const _vector vCurvePoint = XMVectorCatmullRom(
			vPoint0,
			vPoint1,
			vPoint2,
			vPoint3,
			ratio);
		curveLength += XMVectorGetX(
			XMVector3Length(vCurvePoint - vPreviousPoint));
		vPreviousPoint = vCurvePoint;
	}

	if (curveLength <= FLT_EPSILON)
		return;

	const _float trailSpacing = std::max(
		0.001f,
		m_fSparkTrailSpacing);
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
			XMVectorCatmullRom(
				vPoint0,
				vPoint1,
				vPoint2,
				vPoint3,
				ratio));

		_float4x4 spawnWorld{};
		XMStoreFloat4x4(&spawnWorld, XMMatrixIdentity());
		spawnWorld._41 = spawnPosition.x;
		spawnWorld._42 = spawnPosition.y;
		spawnWorld._43 = spawnPosition.z;

		CGameInstance::Get().Spawn(m_SparkCommands, spawnWorld);
	}
}

_bool CPlayer_ConfringoController::FireProjectile()
{
	_float3 startPosition{};
	if (!TryGetWandPosition(startPosition))
		return false;

	_float3 targetPosition{};
	if (auto* pTarget = CGameInstance::Get().
		GetGameObjectByHandleT<CMonster>(m_Owner.m_hAutoTarget))
	{
		targetPosition = pTarget->GetHurtBoxPosition();
	}
	else
	{
		const _vector vStart = XMLoadFloat3(&startPosition);
		const _vector vLook = XMVector3Normalize(
			m_Owner.GetTransform().GetState(STATE::LOOK));
		XMStoreFloat3(&targetPosition, vStart + vLook * 30.f);
	}

	CPlayer_Confringo_Bullet::DESC Desc{};
	Desc.sObjectTag = "PlayerConfringoBullet";
	Desc.vStartPosition = startPosition;
	Desc.vEndPosition = targetPosition;
	Desc.hOwner = m_Owner.GetHandle();
	Desc.eSkillType = PLAYER_SKILL_TYPE::CONFRIGO;
	Desc.bDebugDraw = false;

	const auto hProjectile = CGameInstance::Get().AddGameObjectToLayer(
		m_Owner.m_LevelTag,
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
	if (XMVectorGetX(XMVector3LengthSq(vProjectileDirection)) <=
		FLT_EPSILON)
	{
		vProjectileDirection = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	}
	else
	{
		vProjectileDirection = XMVector3Normalize(vProjectileDirection);
	}

	// [LSY] 머즐 Queue는 로컬 -Z 방향으로 방사되므로 -Z축을
	// 투사체가 실제로 출발하는 첫 곡선 구간 방향과 일치시킨다.
	const _vector vMuzzleLook = -vProjectileDirection;
	_vector vMuzzleUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	_vector vMuzzleRight = XMVector3Cross(vMuzzleUp, vMuzzleLook);
	if (XMVectorGetX(XMVector3LengthSq(vMuzzleRight)) <= FLT_EPSILON)
	{
		vMuzzleUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		vMuzzleRight = XMVector3Cross(vMuzzleUp, vMuzzleLook);
	}

	vMuzzleRight = XMVector3Normalize(vMuzzleRight);
	vMuzzleUp = XMVector3Normalize(
		XMVector3Cross(vMuzzleLook, vMuzzleRight));

	_matrix muzzleWorld = XMMatrixIdentity();
	muzzleWorld.r[0] = XMVectorSetW(vMuzzleRight, 0.f);
	muzzleWorld.r[1] = XMVectorSetW(vMuzzleUp, 0.f);
	muzzleWorld.r[2] = XMVectorSetW(vMuzzleLook, 0.f);
	muzzleWorld.r[3] = XMVectorSetW(
		XMLoadFloat3(&startPosition),
		1.f);

	_float4x4 muzzleParticleWorld{};
	XMStoreFloat4x4(&muzzleParticleWorld, muzzleWorld);
	CGameInstance::Get().Spawn(
		"LSY_Confringo_MuzzleV9_Queue.json",
		muzzleParticleWorld);

	if (m_Owner.m_pComSound)
	{
		m_Owner.m_pComSound->Play3D(
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

UPtr<CPlayer_ConfringoController>
CPlayer_ConfringoController::Create(CPlayer& Owner)
{
	auto pInstance = ToUPtr(new CPlayer_ConfringoController{ Owner });
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create: CPlayer_ConfringoController");
		return nullptr;
	}

	return pInstance;
}

void CPlayer_ConfringoController::Free()
{
	StopCastEffect();
	CEngineBase::Free();
}

NS_END

