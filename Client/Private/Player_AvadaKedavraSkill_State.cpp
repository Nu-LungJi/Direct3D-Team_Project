#include "pch.h"
#include "Player_AvadaKedavraSkill_State.h"

#include "ComSound.h"
#include "Player.h"
#include "ComAnimator.h"
#include "GameInstance.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_AvadaKedavraSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	CacheAnimationIndices(*pPlayer);
	if (m_iCastAnimation < 0)
	{
		DEBUG_LOG("[AvadaKedavra] Cast animation was not found.\n");
		RequestLocomotion(pStateMachine);
		return;
	}

	if (auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(pPlayer->GetTargetHandle()))
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
		m_iCastAnimation, false, CAST_BLEND_DURATION);

	// [LSY] 플레이어가 타깃을 바라본 직후의 Transform을 기준으로
	// TargetLocal 컷씬을 재생해 어느 방향에서 사용해도 같은 구도를 유지한다.
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
	{
		DEBUG_LOG(
			"[AvadaKedavra] Failed to play cinematic.\n");
	}

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
	m_fAnimRatio = 0.f;

	// [AVADA_CAST_BEGIN] 충전 시작
	// [LSY] 완드 끝 충전 연출은 컨트롤러가 애니메이션을 따라 갱신한다.
	pPlayer->StartAvadaKedavraCastEffect();
}

void CPlayer_AvadaKedavraSkill_State::Update(CStateMachine* pStateMachine, _float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	auto* pAnimator = pPlayer ? pPlayer->GetAnimator() : nullptr;
	if (!pPlayer || !pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	pPlayer->SetCurrentMoveSpeed(0.f);
	if (m_bCinematicStarted &&
		!CGameInstance::Get().IsCinematicPlaying())
	{
		m_bCinematicStarted = false;
	}

	m_fAnimRatio = PlayerAnimationRatioGuard::Sanitize(
		pAnimator->GetPlayAnimRatio());

	switch (m_ePhase)
	{
	case PHASE::CAST_BEGIN:
		if (m_fAnimRatio >= RELEASE_RATIO)
		{
			m_ePhase = PHASE::RELEASE;

			// [AVADA_RELEASE] 실제 마법 방출 프레임
			// [LSY] 현재 완드 위치와 타깃 위치를 확정해 빔 및 피격 연출을 시작한다.
			if (!pPlayer->ReleaseAvadaKedavraSpell())
			{
				DEBUG_LOG(
					"[AvadaKedavra] Failed to release spell effect.\n");
			}
		}
		break;

	case PHASE::RELEASE:
		if (m_fAnimRatio >= RECOVERY_RATIO)
		{
			m_ePhase = PHASE::RECOVERY;

			// [AVADA_RECOVERY] 조작 복구 가능 시점
			// 후딜 캔슬 또는 다음 상태 입력 허용 처리는 이 위치에 연결한다.
		}
		break;

	case PHASE::RECOVERY:
		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO || pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;
	}
}

void CPlayer_AvadaKedavraSkill_State::Exit(CStateMachine* pStateMachine)
{
	// [LSY] 정상 Recovery 종료라면 에셋의 Return Blend를 끝까지 사용한다.
	// 방출 전에 상태가 강제로 끊긴 경우에만 남아 있는 컷씬을 즉시 정리한다.
	const _bool bInterrupted = m_ePhase != PHASE::RECOVERY;
	if (bInterrupted && m_bCinematicStarted &&
		CGameInstance::Get().IsCinematicPlaying())
	{
		CGameInstance::Get().StopCinematic();
	}
	m_bCinematicStarted = false;

	if (auto* pPlayer = GetPlayer(pStateMachine))
	{
		pPlayer->StopAvadaKedavraCastEffect();
		ResetSkillControl(*pPlayer);
	}

	m_ePhase = PHASE::CAST_BEGIN;
	m_fAnimRatio = 0.f;
}

void CPlayer_AvadaKedavraSkill_State::CacheAnimationIndices(
	const CPlayer& player)
{
	if (m_bAnimationsCached)
		return;

	m_iCastAnimation = FindAnimationIndex(player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Finisher_03_Cast_anm.bin");
	m_bAnimationsCached = true;
}

SPtr<CPlayer_AvadaKedavraSkill_State> CPlayer_AvadaKedavraSkill_State::Create()
{
	return ToSPtr(new CPlayer_AvadaKedavraSkill_State{});
}
