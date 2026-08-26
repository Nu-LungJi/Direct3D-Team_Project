#include "pch.h"
#include "Player_AccioSkill_State.h"

#include "Player.h"
#include "AccioBall.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"

#include "ComCharacterMoveIntent.h"

#include "Monster.h"
#include "Player_Weapon.h"

#include "ComSound.h"
NS_USING(Client)

void CPlayer_AccioSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	CacheAnimationIndices(*pPlayer);
	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(
		pPlayer->GetTargetHandle());
	if (auto* pBall = dynamic_cast<CAccioBall*>(pTarget))
	{
		if (!EnterObjectAccio(*pPlayer, *pBall))
			RequestLocomotion(pStateMachine);
		return;
	}

	if (!HasTarget(*pPlayer))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_eAccio = ACCIOSTATE::MONSTER;
	if (auto* pSound = pPlayer->GetSound())
	{
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_VOICE_ACCIO" },
			"./Resources/SampleClient/Sound/Player/Spell/Accio/Accio_Man.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 2.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			});
	}
	SetSkillControl(*pPlayer, true, true, false);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->SetCurrentSkill(PLAYER_SKILL_TYPE::ACCIO);
	//if (auto pMonster = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle()))
	//	pMonster->Check_Table(PLAYER_SKILL_TYPE::ACCIO);
	TryApplySkillToTarget(*pPlayer, PLAYER_SKILL_TYPE::ACCIO); //창준 변경
	m_ePhase = PHASE::CAST;
	m_fAnimationRatio = 0.f;
	m_bPulling = true;
}

_bool CPlayer_AccioSkill_State::EnterObjectAccio(
	CPlayer& player, CAccioBall& ball)
{
	if (ball.GetPendingDestroy() || m_AccioCast_Animation < 0 ||
		!ball.TryAcquireControl(player.GetHandle()))
	{
		return false;
	}

	// [LSY] 공을 당기는 동안에도 이동은 허용한다. 물리 힘은 공의 FixedUpdate가
	// 컨트롤러 Handle을 따라 계산하므로 상태는 소유권과 연출만 관리한다.
	SetSkillControl(player, false, false, false, false);
	player.SetCurrentSkill(PLAYER_SKILL_TYPE::ACCIO);
	m_eAccio = ACCIOSTATE::OBJECT;
	m_hObjectBall = ball.GetHandle();
	m_bObjectAnimationPlaying = false;
	m_bObjectAnimationHeld = false;
	m_bObjectAnimationReleasing = false;
	m_bObjectFacingActive = false;
	return true;
}

void CPlayer_AccioSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;


	//m_AccioCast_Animation = FindAnimationIndex( player, "AN_ProfessorSharp_MasterRig_Hu_BM_RF_Cast_Casual_Fwd_Accio_anm.bin");
	m_AccioCast_Animation = FindAnimationIndex( player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_AccioPull_anm.bin");
	m_AccioEnd_Animation = FindAnimationIndex( player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_AccioPull_anm.bin");
	m_AttackFail_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_LF_Atk_Heavy_Fail_anm.bin");

	m_bAnimationIndicesCached =m_AccioCast_Animation >= 0 &&m_AccioEnd_Animation >= 0 &&m_AttackFail_Animation >= 0;
}

void CPlayer_AccioSkill_State::Update(CStateMachine* pStateMachine, _float deltatime)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	if (m_eAccio == ACCIOSTATE::OBJECT)
	{
		UpdateObjectAccio(pStateMachine, *pPlayer, deltatime);
		return;
	}

	pPlayer->SetCurrentMoveSpeed(0.f);

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_fAnimationRatio =PlayerAnimationRatioGuard::Sanitize(pAnimator->GetPlayAnimRatio());

	switch (m_ePhase)
	{
	case PHASE::CAST:
		if (m_fAnimationRatio >= CAST_START_RATIO)
		{
			m_ePhase = PHASE::ATTACK;
			if (!PlayRandomTargetAttack(*pPlayer))
				RequestLocomotion(pStateMachine);
			if (auto pMonster = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle())) {
				auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());

				if (!pWeapon)
					return;
				const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
				_vector weaPonPos = XMVectorSet(spawnWorld._41, spawnWorld._42, spawnWorld._43, spawnWorld._44);
				CGameInstance::Get().Set_ChromaticRingOpacity(0.2f);
				CGameInstance::Get().Render_ChromaticRing(weaPonPos, 0.5f, 100);
				// 무기 발사 위치

				//const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
				//_vector weaPonPos = XMVectorSet(spawnWorld._41, spawnWorld._42, spawnWorld._43, spawnWorld._44);
			
				_vector monstervPos = XMVectorSetW(XMLoadFloat3(&pMonster->GetHurtBoxPosition()), 1.f);
				m_iAccioEffectID = CGameInstance::Get().PlayEffect("Accio", spawnWorld, monstervPos,
					[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
					{
						if (effectId != m_iAccioEffectID)
							return;

						m_iAccioEffectID = INVALID_EFFECT_INSTANCE_ID;
					});

			}
		}
		break;

	case PHASE::ATTACK:
	{
		if (m_fAnimationRatio >= CAST_END_RATIO) {
			//if (!TryApplySkillToTarget(*pPlayer, PLAYER_SKILL_TYPE::ACCIO))
			//{
			//	m_ePhase = PHASE::ATTACK_FAILED;
			//	pAnimator->Play_Anim(m_AttackFail_Animation, false, 0.2f);
			//	break;
			//}
		
			// 끌어 오기 시작

			auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());

			if (!pWeapon)
				return;
			//const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
			//_vector weaPonPos = XMVectorSet(spawnWorld._41, spawnWorld._42, spawnWorld._43, spawnWorld._44);
			//CGameInstance::Get().Render_ChromaticRing(weaPonPos, 0.5f, 100);

			CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle())->GetSpawnWorldMatrix();

			m_ePhase = PHASE::PULL;
			pAnimator->Play_Anim(m_AccioCast_Animation, false, 0.2f);

			
		}




		break;
	}


	case PHASE::ATTACK_FAILED:
		if (pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;

	case PHASE::PULL: {
		auto* Target = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle());

		if (!Target)
		{
			RequestLocomotion(pStateMachine);
			return;
		}

		
		if (m_fAnimationRatio >= MONSTER_PULL_TIME && m_bPulling && Target->Monster_Type(MONSTER_TYPE::NORMAL))
		{
			auto* pMoveIntent = Target->GetComponent<CComCharacterMoveIntent>("ComCharacterMoveIntent");

			if (!pMoveIntent)
			{
				m_bPulling = false;
				break;
			}
	
			//_vector targetPos = XMVectorSetW(
			//	XMLoadFloat3(&Target->GetHurtBoxPosition()), 1.f);
		
			_vector targetPos = XMVectorSetW(
				XMLoadFloat3(&Target->GetTransform().GetPosition()), 1.f);
			const _vector playerPos = pPlayer->GetTransform().GetState(STATE::POSITION);

			const _vector playerLook = XMVector3Normalize(XMVectorSetY(pPlayer->GetTransform().GetState(STATE::LOOK), 0.f));

			_vector destination = playerPos + playerLook * 10.f;

			_vector offset = { 0.f,0.3f,0.f };

			targetPos = offset + targetPos;

			destination = XMVectorSetY(destination, XMVectorGetY(targetPos));

			const _float lerpRatio = 1.f - expf(-10.f * deltatime);

			const _vector nextPos = XMVectorLerp(targetPos, destination, lerpRatio);

			_float3 nextPosition{};
			XMStoreFloat3(&nextPosition, nextPos);

			pMoveIntent->RequestWarp(nextPosition);

			if (XMVectorGetX(XMVector3LengthSq(destination - targetPos)) <= 0.01f)
			{
				_float3 destinationPosition{};
				XMStoreFloat3(&destinationPosition, destination);

				pMoveIntent->RequestWarp(destinationPosition);
				m_bPulling = false;
				m_ePhase = PHASE::RECOVERY;
			}

			if (auto pMonster = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle())) {
				auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());

				if (!pWeapon)
					return;

				// 무기 발사 위치
				const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();

				_float3 spawnPos = _float3(spawnWorld._41, spawnWorld._42, spawnWorld._43);

				CGameInstance::Get().SetBeamPositionsByOwner(m_iAccioEffectID, spawnPos, pMonster->GetTransform().GetPosition());
			}

		}
		else {
			m_bPulling = false;
			m_ePhase = PHASE::RECOVERY;
		}

		if (m_fAnimationRatio >= ATTACK_END_RATIO)
			RequestLocomotion(pStateMachine);
		break;


	}

	case PHASE::RECOVERY:
		if (m_fAnimationRatio >= RECOVERY_EXIT_RATIO) {
			CGameInstance::Get().StopEffect(m_iAccioEffectID);
			RequestLocomotion(pStateMachine);
		}
		break;
	}
}

void CPlayer_AccioSkill_State::UpdateObjectAccio(
	CStateMachine* pStateMachine, CPlayer& player, _float fTimeDelta)
{
	auto* pBall = CGameInstance::Get().GetGameObjectByHandleT<CAccioBall>(
		m_hObjectBall);
	const _bool bPullRequested =
		CGameInstance::Get().MousePressing(MOUSEKEYSTATE::MB) &&
		pBall && !pBall->GetPendingDestroy() &&
		pBall->IsControlledBy(player.GetHandle());

	UpdateObjectAnimation(player, pBall, bPullRequested);
	UpdateObjectPullEffect(player, pBall, fTimeDelta, bPullRequested);
	UpdateObjectGrabEffect(pBall, fTimeDelta, bPullRequested);

	if (bPullRequested)
		return;

	ReleaseObjectControl(player);
	if (!m_bObjectAnimationPlaying &&
		m_iObjectPullEffectID == INVALID_EFFECT_INSTANCE_ID &&
		m_iObjectGrabEffectID == INVALID_EFFECT_INSTANCE_ID)
	{
		RequestLocomotion(pStateMachine);
	}
}

void CPlayer_AccioSkill_State::UpdateObjectAnimation(
	CPlayer& player, CAccioBall* pBall, _bool bPullRequested)
{
	auto* pAnimator = player.GetAnimator();
	if (!pAnimator)
	{
		m_bObjectAnimationPlaying = false;
		return;
	}

	if (!bPullRequested)
	{
		if (m_bObjectFacingActive && player.GetMoveIntent())
			player.GetMoveIntent()->ClearFacingIntent();
		m_bObjectFacingActive = false;

		if (!m_bObjectAnimationPlaying)
		{
			m_bObjectAnimationHeld = false;
			m_bObjectAnimationReleasing = false;
			return;
		}

		// [LSY] 고정했던 당김 자세부터 남은 상체 애니메이션을 이어서 재생한다.
		if (!m_bObjectAnimationReleasing)
		{
			pAnimator->SetUpperAnimationSpeed(1.f);
			m_bObjectAnimationHeld = false;
			m_bObjectAnimationReleasing = true;
		}

		if (!pAnimator->HasUpperAnimation() ||
			pAnimator->IsUpperAnimationFinished())
		{
			m_bObjectAnimationPlaying = false;
			m_bObjectAnimationReleasing = false;
		}
		return;
	}

	if (!pBall || m_AccioCast_Animation < 0)
		return;

	if (!m_bObjectFacingActive && player.GetMoveIntent())
	{
		const _float3 vBallPosition = pBall->GetTransform().GetPosition();
		const _float3 vPlayerPosition = player.GetTransform().GetPosition();
		const _float3 vFacingDirection{
			vBallPosition.x - vPlayerPosition.x,
			0.f,
			vBallPosition.z - vPlayerPosition.z
		};
		const _float fFacingLengthSq =
			vFacingDirection.x * vFacingDirection.x +
			vFacingDirection.z * vFacingDirection.z;
		if (fFacingLengthSq > std::numeric_limits<_float>::epsilon())
		{
			player.GetMoveIntent()->SetFacingIntent(
				vFacingDirection, OBJECT_FACING_TURN_SPEED);
			m_bObjectFacingActive = true;
		}
	}

	if (!m_bObjectAnimationPlaying)
	{
		m_bObjectAnimationPlaying = player.PlayUpperBodyAnimation(
			m_AccioCast_Animation, "Spine1", 1, false, 0.15f);
		if (m_bObjectAnimationPlaying)
		{
			pAnimator->SetUpperAnimationSpeed(1.f);
			pAnimator->SetUpperAnimationFadeOutDuration(0.18f);
			m_bObjectAnimationHeld = false;
			m_bObjectAnimationReleasing = false;
		}
		return;
	}

	if (!m_bObjectAnimationHeld &&
		pAnimator->GetUpperAnimRatio() >= OBJECT_PULL_HOLD_RATIO)
	{
		pAnimator->SetUpperAnimationSpeed(0.f);
		m_bObjectAnimationHeld = true;
	}
}

void CPlayer_AccioSkill_State::UpdateObjectPullEffect(
	CPlayer& player, CAccioBall* pBall, _float fTimeDelta,
	_bool bPullRequested)
{
	auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(
		player.GetWeaponHandle());
	const _bool bEffectRequested = bPullRequested && pBall && pWeapon;

	if (bEffectRequested)
	{
		const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
		const _float3 vEffectStart{
			spawnWorld._41, spawnWorld._42, spawnWorld._43
		};
		const _float3 vEffectEnd = pBall->GetTransform().GetPosition();

		if (m_iObjectPullEffectID == INVALID_EFFECT_INSTANCE_ID)
		{
			m_fObjectPullBlend = 0.f;
			m_iObjectPullEffectID = CGameInstance::Get().PlayEffect(
				"AccioBallPull", spawnWorld,
				XMVectorSetW(XMLoadFloat3(&vEffectEnd), 1.f),
				[this](EFFECT_INSTANCE_ID iEffectID, EFFECT_FINISH_REASON)
				{
					if (m_iObjectPullEffectID != iEffectID)
						return;
					m_iObjectPullEffectID = INVALID_EFFECT_INSTANCE_ID;
					m_fObjectPullBlend = 0.f;
				});
		}

		if (m_iObjectPullEffectID != INVALID_EFFECT_INSTANCE_ID)
		{
			CGameInstance::Get().SetBeamPositionsByOwner(
				m_iObjectPullEffectID, vEffectStart, vEffectEnd);
			m_fObjectPullBlend = std::min(
				1.f, m_fObjectPullBlend +
				fTimeDelta / OBJECT_PULL_FADE_IN_TIME);
		}
	}
	else if (m_iObjectPullEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		m_fObjectPullBlend = std::max(
			0.f, m_fObjectPullBlend -
			fTimeDelta / OBJECT_PULL_FADE_OUT_TIME);
	}

	if (m_iObjectPullEffectID == INVALID_EFFECT_INSTANCE_ID)
		return;

	const _float fSmoothBlend = m_fObjectPullBlend * m_fObjectPullBlend *
		(3.f - 2.f * m_fObjectPullBlend);
	CGameInstance::Get().ChangeEffectColorByOwner(
		m_iObjectPullEffectID, { 1.f, 1.f, 0.6f, fSmoothBlend });

	if (!bEffectRequested && m_fObjectPullBlend <= 0.f)
	{
		const EFFECT_INSTANCE_ID iEffectID = m_iObjectPullEffectID;
		m_iObjectPullEffectID = INVALID_EFFECT_INSTANCE_ID;
		CGameInstance::Get().StopEffect(iEffectID);
	}
}

void CPlayer_AccioSkill_State::UpdateObjectGrabEffect(
	CAccioBall* pBall, _float fTimeDelta, _bool bPullRequested)
{
	const _bool bEffectRequested = bPullRequested && pBall;
	_float4x4 followWorld{};
	if (bEffectRequested)
	{
		const _float3 vBallPosition = pBall->GetTransform().GetPosition();
		XMStoreFloat4x4(&followWorld, XMMatrixTranslation(
			vBallPosition.x, vBallPosition.y, vBallPosition.z));

		if (m_iObjectGrabEffectID == INVALID_EFFECT_INSTANCE_ID)
		{
			m_fObjectGrabBlend = 0.f;
			m_iObjectGrabEffectID = CGameInstance::Get().PlayEffect(
				"AccioBallGrab", followWorld, _vector{},
				[this](EFFECT_INSTANCE_ID iEffectID, EFFECT_FINISH_REASON)
				{
					if (m_iObjectGrabEffectID != iEffectID)
						return;
					m_iObjectGrabEffectID = INVALID_EFFECT_INSTANCE_ID;
					m_fObjectGrabBlend = 0.f;
				});
		}

		if (m_iObjectGrabEffectID != INVALID_EFFECT_INSTANCE_ID)
		{
			CGameInstance::Get().SetEffectWorldMatrix(
				m_iObjectGrabEffectID, followWorld);
			m_fObjectGrabBlend = std::min(
				1.f, m_fObjectGrabBlend +
				fTimeDelta / OBJECT_GRAB_FADE_IN_TIME);
		}
	}
	else if (m_iObjectGrabEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		m_fObjectGrabBlend = std::max(
			0.f, m_fObjectGrabBlend -
			fTimeDelta / OBJECT_GRAB_FADE_OUT_TIME);
	}

	if (m_iObjectGrabEffectID == INVALID_EFFECT_INSTANCE_ID)
		return;

	const _float fSmoothBlend = m_fObjectGrabBlend * m_fObjectGrabBlend *
		(3.f - 2.f * m_fObjectGrabBlend);
	CGameInstance::Get().ChangeEffectColorByOwner(
		m_iObjectGrabEffectID,
		{ 1.f, 1.f, 0.f, OBJECT_GRAB_MAX_ALPHA * fSmoothBlend });

	if (!bEffectRequested && m_fObjectGrabBlend <= 0.f)
	{
		const EFFECT_INSTANCE_ID iEffectID = m_iObjectGrabEffectID;
		m_iObjectGrabEffectID = INVALID_EFFECT_INSTANCE_ID;
		CGameInstance::Get().StopEffect(iEffectID);
	}
}

void CPlayer_AccioSkill_State::ReleaseObjectControl(CPlayer& player)
{
	if (m_hObjectBall == CHandle{})
		return;

	if (auto* pBall = CGameInstance::Get().GetGameObjectByHandleT<CAccioBall>(
		m_hObjectBall))
	{
		pBall->ReleaseControl(player.GetHandle());
	}
}

void CPlayer_AccioSkill_State::StopObjectEffects()
{
	if (m_iObjectPullEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		const EFFECT_INSTANCE_ID iEffectID = m_iObjectPullEffectID;
		m_iObjectPullEffectID = INVALID_EFFECT_INSTANCE_ID;
		CGameInstance::Get().StopEffect(iEffectID);
	}
	if (m_iObjectGrabEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		const EFFECT_INSTANCE_ID iEffectID = m_iObjectGrabEffectID;
		m_iObjectGrabEffectID = INVALID_EFFECT_INSTANCE_ID;
		CGameInstance::Get().StopEffect(iEffectID);
	}
	m_fObjectPullBlend = 0.f;
	m_fObjectGrabBlend = 0.f;
}

void CPlayer_AccioSkill_State::ResetObjectState()
{
	m_hObjectBall = CHandle{};
	m_bObjectAnimationPlaying = false;
	m_bObjectAnimationHeld = false;
	m_bObjectAnimationReleasing = false;
	m_bObjectFacingActive = false;
}

void CPlayer_AccioSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
	{
		if (m_eAccio == ACCIOSTATE::OBJECT)
		{
			ReleaseObjectControl(*pPlayer);
			if (m_bObjectFacingActive && pPlayer->GetMoveIntent())
				pPlayer->GetMoveIntent()->ClearFacingIntent();
			if (auto* pAnimator = pPlayer->GetAnimator())
			{
				pAnimator->SetUpperAnimationSpeed(1.f);
				pAnimator->Stop_UpperAnim(0.15f);
			}
		}
		ResetSkillControl(*pPlayer);
	}

	StopObjectEffects();
	ResetObjectState();
	if (m_iAccioEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		const EFFECT_INSTANCE_ID iEffectID = m_iAccioEffectID;
		m_iAccioEffectID = INVALID_EFFECT_INSTANCE_ID;
		CGameInstance::Get().StopEffect(iEffectID);
	}
	m_bPulling = false;
	m_ePhase = PHASE::CAST;
	m_eAccio = ACCIOSTATE::END;
	m_fAnimationRatio = 0.f;
}

SPtr<CPlayer_AccioSkill_State> CPlayer_AccioSkill_State::Create()
{
	return ToSPtr(new CPlayer_AccioSkill_State{});
}
