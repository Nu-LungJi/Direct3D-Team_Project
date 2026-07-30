#include "pch.h"
#include "Player_AcientAttack_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_AcientAttack_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	if (!HasValidTarget(*pPlayer))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	CacheAnimationIndices(*pPlayer);
	const auto iSkillIndex = ETOUI(ACIENT_SKILL::ACIENT_LIGHTENING);
	if (m_AcientCast_Animations[iSkillIndex] < 0 ||
		m_AcientEnd_Animations[iSkillIndex] < 0)
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

	SetSkillControl(*pPlayer, true, true, false);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->SetPlayerCurSKill(PLAYER_SKILL_TYPE::ACIENT_LIGHTNING);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_fAcientElapsed = 0.f;

	// 스킬 컷신 재생
	{
		FCinematicPlayOptions options{};
		options.eStartMode = ECinematicStartMode::Blend;
		options.fStartBlendDuration = 1.f;
		options.eReturnMode = ECinematicReturnMode::Blend;
		options.fReturnBlendDuration = 1.f;
		CGameInstance::Get().PlayCinematic("AcientThunderAttack", pPlayer->GetHandle(), options);
	}
}

void CPlayer_AcientAttack_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	const auto iSkillIndex = ETOUI(ACIENT_SKILL::ACIENT_LIGHTENING);
	m_AcientCast_Animations[iSkillIndex] = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_DW_Cmbt_Atk_AOE_Lightning_Cast_Start_anm.bin");
	m_AcientEnd_Animations[iSkillIndex] = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_DW_Cmbt_Atk_AOE_Lightning_Cast_End_anm.bin");

	m_bAnimationIndicesCached =
		m_AcientCast_Animations[iSkillIndex] >= 0 &&
		m_AcientEnd_Animations[iSkillIndex] >= 0;
}

void CPlayer_AcientAttack_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	pPlayer->SetCurrentMoveSpeed(0.f);

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_fAnimRatio =
		PlayerAnimationRatioGuard::Sanitize(	
			pAnimator->GetPlayAnimRatio());

	switch (m_ePhase)
	{
	case PHASE::CAST:
		if (m_fAnimRatio >= ACIENT_LIGHTENING_CAST_START_RATIO)
		{
			m_ePhase = PHASE::ATTACK;
			pAnimator->Play_Anim(
				m_AcientCast_Animations[ETOUI(ACIENT_SKILL::ACIENT_LIGHTENING)],
				false,
				0.24f);
		}
		break;

	case PHASE::ATTACK:
		m_fAcientElapsed += std::max(0.f, fTimeDelta);
		if (m_fAcientElapsed >= ACIENT_LIGHTENING_ATTACK_DURATION)
		{
			m_ePhase = PHASE::RECOVERY;
			pAnimator->Play_Anim(m_AcientEnd_Animations[ETOUI(ACIENT_SKILL::ACIENT_LIGHTENING)],false,0.25f);
			pAnimator->GetCurAnimState().fSpeed = 1.f;
		}
		break;

	case PHASE::RECOVERY:
		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO)
			RequestLocomotion(pStateMachine);
		break;
	}
}

void CPlayer_AcientAttack_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_fAcientElapsed = 0.f;
}

SPtr<CPlayer_AcientAttack_State> CPlayer_AcientAttack_State::Create()
{
	return ToSPtr(new CPlayer_AcientAttack_State{});
}
