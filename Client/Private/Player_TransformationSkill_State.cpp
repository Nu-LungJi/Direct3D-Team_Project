#include "pch.h"
#include "Player_TransformationSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "Monster.h"
#include "PlayerAnimationRatioGuard.h"
#include "Player_Weapon.h"
#include "PropBarrel.h"
#include "TmbGurdian.h"

NS_USING(Client)

void CPlayer_TransformationSkill_State::Enter(CStateMachine* pStateMachine)
{
	m_hTransformationTarget = {};
	m_fTransformationElapsed = 0.f;
	m_bTransformationResolved = false;

	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !HasTarget(*pPlayer) || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}
	m_hTransformationTarget = pPlayer->GetTargetHandle();

	// [TRANSFORMATION_ENTER] 시전 시작 시 이동 입력을 잠그고 타깃 방향 회전만 허용한다.
	// 완드 발광, 캐스팅 사운드처럼 즉시 시작할 연출은 이 아래에 연결한다.
	SetSkillControl(*pPlayer, true, false, true, true);
	pPlayer->SetCurrentMoveSpeed(0.f);

	// 봄바르다와 동일한 방향별 강공격 애니메이션을 시작하는 지점이다.
	if (!PlayTargetAttack(*pPlayer, true, 0.14f))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	pPlayer->SetRootMotionTranslationActive(false);
	m_ePhase = PHASE::CAST_BEGIN;
	m_fAnimRatio = 0.f;

	auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());

	if (!pWeapon)
		return;
	const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
	_vector weaPonPos = XMVectorSet(spawnWorld._41, spawnWorld._42, spawnWorld._43, spawnWorld._44);

	m_iEffectID = CGameInstance::Get().PlayEffect("TransWand", spawnWorld, _vector{},
		[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
		{
			if (effectId != m_iEffectID)
				return;
			m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
		});
}

void CPlayer_TransformationSkill_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
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

	switch (m_ePhase)
	{
	case PHASE::CAST_BEGIN:

		if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
		{
			auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());

			if (!pWeapon)
				return;

			const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();

			_float4x4 followWorld{};
			XMStoreFloat4x4(&followWorld, XMMatrixTranslation(spawnWorld._41, spawnWorld._42, spawnWorld._43));

			CGameInstance::Get().SetEffectWorldMatrix(m_iEffectID, followWorld);
		}
		// [TRANSFORMATION_CAST_BEGIN]
		// 캐릭터가 주문을 준비하는 구간이다. 손/완드 차징 이펙트와 시전음을 연결한다.
		// 타깃 변신 판정은 아직 수행하지 않는다.
		if (m_fAnimRatio >= RELEASE_RATIO)
		{
			// [TRANSFORMATION_RELEASE_CUE]
			// 주문이 실제로 방출되는 프레임이다. 완드 발사와 타깃 피격
			// 이펙트를 시작하고, 실제 교체는 이펙트 중간 Cue까지 지연한다.

			auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());
			if (pWeapon)
			{
				const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
				_float4x4 particleWorld{};
				XMStoreFloat4x4(&particleWorld, XMMatrixTranslation(spawnWorld._41, spawnWorld._42, spawnWorld._43));
				const uint32_t iParticleOwnerID = CGameInstance::Get().Spawn("TransParticle.json", particleWorld);
				if (iParticleOwnerID == INVALID_PARTICLE_OWNER_ID)
					DEBUG_LOG("[Transformation] Failed to spawn TransParticle.json.\n");
			}

			auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(
				m_hTransformationTarget);

			if (nullptr != pTarget)
			{
				_vector vTargetPosition = pTarget->GetTransform().GetState(STATE::POSITION);

				_matrix matEffect = XMMatrixIdentity();
				matEffect.r[3] = XMVectorSetW(vTargetPosition, 1.f);

				_float4x4 effectWorld{};
				XMStoreFloat4x4(&effectWorld, matEffect);

				CGameInstance::Get().PlayEffect("Transformation", effectWorld);
			}
			m_fTransformationElapsed = 0.f;
			m_ePhase = PHASE::RELEASE;
		}
		break;

	case PHASE::RELEASE:
		// [TRANSFORMATION_RELEASE]
		// 변신 연출의 중간 Cue에서 몬스터를 완성된 오크통으로 교체한다.
		// 파괴/파편 생성은 이후 던지기 스킬의 충돌 처리에서 별도로 호출한다.
		m_fTransformationElapsed += std::max(fTimeDelta, 0.f);
		if (!m_bTransformationResolved &&
			m_fTransformationElapsed >= TRANSFORMATION_DELAY)
		{
			TransformTargetToBarrel();
		}

		if (m_bTransformationResolved && m_fAnimRatio >= RECOVERY_RATIO)
		{
			// [TRANSFORMATION_RECOVERY_CUE]
			// 주문 방출이 끝나고 후딜로 넘어가는 시점이다.
			m_ePhase = PHASE::RECOVERY;
		}
		break;

	case PHASE::RECOVERY:
		// [TRANSFORMATION_RECOVERY]
		// 후딜 구간이다. 캔슬 입력이나 다음 행동 허용이 필요하면 여기에서 처리한다.
		// 현재는 지정 비율 또는 애니메이션 종료까지 조작 잠금을 유지한다.
		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO || pAnimator->GetFinish())
		{
			// [TRANSFORMATION_EXIT_CUE] 이동 상태로 복귀해 일반 조작을 다시 허용한다.
			RequestLocomotion(pStateMachine);
		}
		break;
	}
}

void CPlayer_TransformationSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
	{
		// [TRANSFORMATION_EXIT] 남아 있는 캐스팅 연출 정리도 이 위치에 연결한다.
		ResetSkillControl(*pPlayer);
	}

	m_ePhase = PHASE::CAST_BEGIN;
	m_fAnimRatio = 0.f;
	m_fTransformationElapsed = 0.f;
	m_bTransformationResolved = false;
	m_hTransformationTarget = {};
}

void CPlayer_TransformationSkill_State::TransformTargetToBarrel()
{
	// 유효하지 않은 타깃이나 생성 실패를 매 프레임 재시도하지 않는다.
	m_bTransformationResolved = true;

	auto* pMonster = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(
		m_hTransformationTarget);
	if (!pMonster || pMonster->GetPendingDestroy())
	{
		DEBUG_LOG("[Transformation] Target is no longer transformable.\n");
		return;
	}

	CPropBarrel::DESC desc{};
	desc.sObjectTag = "Transformation_PropBarrel";
	desc.sResourceGroup = "PERMANENT";
	desc.vInitialPosition = pMonster->GetTransform().GetPosition();
	desc.vInitialRotation = pMonster->GetTransform().GetRotationEuler();

	const auto hBarrel = CGameInstance::Get().AddGameObjectToLayer(
		"PERMANENT",
		PROTO_GAMEOBJECT::Prototype_GameObject_PropBarrel,
		"PropBarrel",
		&desc);
	if (!hBarrel)
	{
		DEBUG_LOG("[Transformation] Failed to spawn PropBarrel; target was preserved.\n");
		return;
	}

	// Tomb Guardian의 무기는 별도 레이어 오브젝트이므로 본체와 함께 정리한다.
	if (auto* pTmbGurdian = Cast<CTmbGurdian>(pMonster))
	{
		if (auto* pWeapon = CGameInstance::Get().GetGameObjectByHandle(
			pTmbGurdian->GetWeaponHandle()))
		{
			pWeapon->SetPendingDestroy();
		}
	}

	// 생성 성공을 확인한 뒤에만 원본 몬스터를 지연 삭제한다.
	pMonster->SetPendingDestroy();
}

SPtr<CPlayer_TransformationSkill_State> CPlayer_TransformationSkill_State::Create()
{
	return ToSPtr(new CPlayer_TransformationSkill_State{});
}
