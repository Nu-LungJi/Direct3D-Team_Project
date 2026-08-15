#include "pch.h"
#include "Player_BombardaController.h"

#include "ComSound.h"
#include "Monster.h"
#include "Player.h"
#include "Player_Bombarda_Bullet.h"
#include "Player_Weapon.h"

NS_BEGIN(Client)

CPlayer_BombardaController::CPlayer_BombardaController(CPlayer& Owner)
	: m_Owner{ Owner }
{
}

HRESULT CPlayer_BombardaController::Initialize()
{
	return S_OK;
}

void CPlayer_BombardaController::EnsureCastParticleCommandsLoaded()
{
	if (m_CastParticleCommands.empty())
	{
		// [LSY] 캐스팅 곡선 전용 큐를 한 번만 파싱하여 매 프레임 파일을 읽지 않게 한다.
		// 투사체 꼬리 큐와 분리되어 두 연출의 생성 수치를 독립적으로 조정할 수 있다.
		m_CastParticleCommands = CGameInstance::Get().Parse_Command(
			"LSY_Bombarda_Cast_Particle_Queue.json");
	}
}

void CPlayer_BombardaController::Update()
{
	if (!m_bCastActive)
		return;

	_float4x4 wandWorld{};
	if (!TryGetWandWorld(wandWorld))
	{
		StopCastEffect();
		return;
	}

	if (m_iCastEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		CGameInstance::Get().SetEffectWorldMatrix(
			m_iCastEffectID,
			wandWorld);
	}

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
	EmitCastEnergyTrail(currentPosition);
}

void CPlayer_BombardaController::StartCastEffect()
{
	StopCastEffect();

	_float4x4 wandWorld{};
	if (!TryGetWandWorld(wandWorld))
		return;

	EnsureCastParticleCommandsLoaded();
	if (auto* pTrail = CGameInstance::Get().GetParticle(
		"Bombarda_Cast_Energy_Trail",
		"Bombarda_Cast_Energy_Trail"))
	{
		pTrail->SetColor({ 0.52f, 0.72f, 1.f, 1.f });
		pTrail->SetEmissive({ 0.32f, 0.58f, 1.f, 6.f });
	}

	const _float3 wandPosition{
		wandWorld._41,
		wandWorld._42,
		wandWorld._43
	};
	m_CastTrailControlPoints.fill(wandPosition);
	m_bCastActive = true;
	m_bTrailRegistrationFailureLogged = false;
	EmitCastEnergyTrail(wandPosition);

	m_iCastEffectID = CGameInstance::Get().PlayEffect(
		"Bombarda_Cast",
		wandWorld,
		XMVectorZero(),
		[hOwner = m_Owner.GetHandle()](
			EFFECT_INSTANCE_ID iEffectID,
			EFFECT_FINISH_REASON)
		{
			auto* pPlayer = CGameInstance::Get().
				GetGameObjectByHandleT<CPlayer>(hOwner);
			if (!pPlayer || !pPlayer->m_pBombardaController)
				return;

			auto& Controller = *pPlayer->m_pBombardaController;
			if (Controller.m_iCastEffectID == iEffectID)
			{
				Controller.m_iCastEffectID =
					INVALID_EFFECT_INSTANCE_ID;
			}
		});
}

void CPlayer_BombardaController::StopCastEffect()
{
	m_bCastActive = false;

	if (m_iCastEffectID == INVALID_EFFECT_INSTANCE_ID)
		return;

	const EFFECT_INSTANCE_ID iEffectID = m_iCastEffectID;
	m_iCastEffectID = INVALID_EFFECT_INSTANCE_ID;
	CGameInstance::Get().StopEffect(iEffectID);
}

_bool CPlayer_BombardaController::FireProjectile()
{
	// [LSY] 발사 Cue에 도달하면 생성 성공 여부와 관계없이 캐스팅 단계는 종료한다.
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

	CPlayer_Bombarda_Bullet::DESC Desc{};
	Desc.sObjectTag = "PlayerBombardaBullet";
	Desc.vStartPosition = startPosition;
	Desc.vEndPosition = targetPosition;
	Desc.hOwner = m_Owner.GetHandle();
	Desc.bDebugDraw = false;

	const auto hProjectile = CGameInstance::Get().AddGameObjectToLayer(
		m_Owner.m_LevelTag,
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerBombardaBullet,
		"PlayerBombardaBullet",
		&Desc);
	if (!hProjectile)
		return false;

	// [LSY] 머즐과 발사체가 같은 시작 행렬을 사용해 지팡이 끝에서 자연스럽게 이어지게 한다.
	CGameInstance::Get().PlayEffect(
		"Bombarda_Muzzle",
		wandWorld);

	if (m_Owner.m_pComSound)
	{
		m_Owner.m_pComSound->Play3D(
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

_bool CPlayer_BombardaController::TryGetWandWorld(
	_float4x4& OutWorld) const
{
	auto* pWeapon = CGameInstance::Get().
		GetGameObjectByHandleT<CPlayer_Weapon>(m_Owner.GetWeaponHandle());
	if (!pWeapon)
		return false;

	OutWorld = pWeapon->GetSpawnWorldMatrix();
	return true;
}

_bool CPlayer_BombardaController::ResolveTargetPosition(
	const _float3& vStartPosition,
	_float3& OutTargetPosition) const
{
	if (auto* pTarget = CGameInstance::Get().
		GetGameObjectByHandleT<CMonster>(m_Owner.GetTargetHandle()))
	{
		OutTargetPosition = pTarget->GetHurtBoxPosition();
		return true;
	}

	_vector vLook = m_Owner.GetTransform().GetState(STATE::LOOK);
	vLook = XMVectorSetY(vLook, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vLook)) <= FLT_EPSILON)
		return false;

	vLook = XMVector3Normalize(vLook);
	XMStoreFloat3(
		&OutTargetPosition,
		XMLoadFloat3(&vStartPosition) + vLook * 30.f);
	return true;
}

void CPlayer_BombardaController::EmitCastParticleCurve() const
{
	if (m_CastParticleCommands.empty())
		return;

	const _vector vPoint0 = XMLoadFloat3(&m_CastTrailControlPoints[0]);
	const _vector vPoint1 = XMLoadFloat3(&m_CastTrailControlPoints[1]);
	const _vector vPoint2 = XMLoadFloat3(&m_CastTrailControlPoints[2]);
	const _vector vPoint3 = XMLoadFloat3(&m_CastTrailControlPoints[3]);
	if (XMVectorGetX(XMVector3LengthSq(vPoint2 - vPoint1)) <=
		FLT_EPSILON)
		return;

	constexpr uint32_t CURVE_LENGTH_SAMPLE_COUNT = 12;
	_float fCurveLength = 0.f;
	_vector vPreviousSample = vPoint1;
	for (uint32_t i = 1; i <= CURVE_LENGTH_SAMPLE_COUNT; ++i)
	{
		const _float fRatio = static_cast<_float>(i) /
			static_cast<_float>(CURVE_LENGTH_SAMPLE_COUNT);
		const _vector vCurrentSample = XMVectorCatmullRom(
			vPoint0,
			vPoint1,
			vPoint2,
			vPoint3,
			fRatio);
		fCurveLength += XMVectorGetX(
			XMVector3Length(vCurrentSample - vPreviousSample));
		vPreviousSample = vCurrentSample;
	}

	const _float fSpacing = std::max(m_fCastParticleSpacing, 0.01f);
	constexpr uint32_t MAX_TRAIL_SPAWN_PER_FRAME = 12;
	const uint32_t iSpawnCount = std::clamp(
		static_cast<uint32_t>(std::ceil(fCurveLength / fSpacing)),
		1u,
		MAX_TRAIL_SPAWN_PER_FRAME);

	for (uint32_t i = 1; i <= iSpawnCount; ++i)
	{
		const _float fRatio = static_cast<_float>(i) /
			static_cast<_float>(iSpawnCount);
		_float3 spawnPosition{};
		XMStoreFloat3(
			&spawnPosition,
			XMVectorCatmullRom(
				vPoint0,
				vPoint1,
				vPoint2,
				vPoint3,
				fRatio));

		_float4x4 spawnWorld{};
		XMStoreFloat4x4(&spawnWorld, XMMatrixIdentity());
		spawnWorld._41 = spawnPosition.x;
		spawnWorld._42 = spawnPosition.y;
		spawnWorld._43 = spawnPosition.z;
		CGameInstance::Get().Spawn(m_CastParticleCommands, spawnWorld);
	}
}

void CPlayer_BombardaController::EmitCastEnergyTrail(
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
		m_Owner.GetHandle(),
		vTrailStart,
		vTrailEnd);

	if (FAILED(hr) && !m_bTrailRegistrationFailureLogged)
	{
		DEBUG_LOG(
			"[Bombarda] Cast energy trail particle is not registered.\n");
		m_bTrailRegistrationFailureLogged = true;
	}
}

UPtr<CPlayer_BombardaController>
CPlayer_BombardaController::Create(CPlayer& Owner)
{
	auto pInstance = ToUPtr(new CPlayer_BombardaController{ Owner });
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create: CPlayer_BombardaController");
		return nullptr;
	}

	return pInstance;
}

void CPlayer_BombardaController::Free()
{
	StopCastEffect();
	CEngineBase::Free();
}

NS_END
