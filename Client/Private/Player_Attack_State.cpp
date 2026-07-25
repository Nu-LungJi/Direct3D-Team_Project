#include "pch.h"
#include "Player_Attack_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "ComAnimator.h"
#include "ComCharacterMotor.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)


void CPlayer_Attack_State::Enter(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	auto* playerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine);
	if (!player || !playerStateMachine)
		return;



	CacheAnimationIndices(*player);

	m_iCurrentForwardLightAnimation = 0;
	m_iComboCount = 1;
	m_bAttackQueued = false;
	m_bPlayingHeavy = false;

	auto* animator = player->GetAnimator();
	if (!animator || m_ForwardLightAnimations[m_iCurrentForwardLightAnimation] < 0)
	{
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}

	player->SetCurrentMoveSpeed(0.f);
	player->SetMovementLocked(true);
	player->SetRootMotionTranslationActive(true);
	player->SetRootMotionRotationActive(false);
	if (auto* moveIntent = player->GetMoveIntent())
	{
		moveIntent->ClearMoveIntent();
		moveIntent->ClearFacingIntent();
	}

	animator->Play_Anim(m_ForwardLightAnimations[m_iCurrentForwardLightAnimation],false,ATTACK_BLEND_DURATION);
}

void CPlayer_Attack_State::Exit(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!player)
		return;

	player->SetMovementLocked(false);
	player->SetRootMotionTranslationActive(false);
	player->SetRootMotionRotationActive(false);
	m_iComboCount = 0;
	m_bAttackQueued = false;
	m_bPlayingHeavy = false;
}

void CPlayer_Attack_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	auto* playerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine);
	if (!player || !playerStateMachine)
		return;

	auto* animator = player->GetAnimator();
	if (!animator)
	{
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}

	const _float fAnimRatio = animator->GetPlayAnimRatio();

	if (!m_bPlayingHeavy &&
		fAnimRatio >= LIGHT_FORWARD_MOVE_START_RATIO &&
		fAnimRatio <= LIGHT_FORWARD_MOVE_END_RATIO)
	{
		player->ApplyAttackForwardMovement(
			LIGHT_FORWARD_MOVE_SPEED,
			fTimeDelta);
	}

	if (!m_bPlayingHeavy &&
		fAnimRatio >= MOVE_CANCEL_START_RATIO &&
		player->HasRawMoveInput()) {
		m_bAttackQueued = false;
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}


	const _bool bInComboInputWindow = fAnimRatio >= COMBO_INPUT_START_RATIO && fAnimRatio <= COMBO_INPUT_END_RATIO;

	if (!m_bPlayingHeavy &&
		bInComboInputWindow &&
		CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB))
	{
		m_bAttackQueued = true;
	}

	if (m_bAttackQueued && fAnimRatio >= COMBO_LINK_RATIO)
	{
		const size_t iNextAnimation =(m_iCurrentForwardLightAnimation + 1) %m_ForwardLightAnimations.size();

		if (m_ForwardLightAnimations[iNextAnimation] >= 0)
		{
			m_bAttackQueued = false;
			++m_iComboCount;
			if (m_iComboCount == 3) {
				m_bPlayingHeavy = true;
				player->SetRootMotionTranslationActive(true);
				player->SetRootMotionRotationActive(true);
				animator->Play_Anim(m_ForwardHvyAnimations[0], false, ATTACK_BLEND_DURATION);
				m_iComboCount = 0;
				return;
			}
			else {
				m_bPlayingHeavy = false;
				player->SetRootMotionTranslationActive(true);
				player->SetRootMotionRotationActive(false);
				m_iCurrentForwardLightAnimation = iNextAnimation;
				animator->Play_Anim(m_ForwardLightAnimations[m_iCurrentForwardLightAnimation], false, ATTACK_BLEND_DURATION);
				return;
			}
			
		}

		m_bAttackQueued = false;
	}

	if (animator->GetFinish())
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
}

void CPlayer_Attack_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	m_ForwardLightAnimations[0] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_01_anm.bin");
	m_ForwardLightAnimations[1] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_02_anm.bin");
	m_ForwardLightAnimations[2] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_03_Uppercut_anm.bin");
	m_ForwardLightAnimations[3] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_04_anm.bin");
	m_ForwardLightAnimations[4] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_05_anm.bin");
	m_ForwardLightAnimations[5] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_06_anm.bin");
	m_ForwardLightAnimations[6] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_07_anm.bin");
	m_ForwardLightAnimations[7] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_08_anm.bin");
	m_ForwardLightAnimations[8] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_09_anm.bin");

	m_ForwardHvyAnimations[0] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_01_Spin_anm.bin");
	m_ForwardHvyAnimations[1] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_02_anm.bin");
	m_ForwardHvyAnimations[2] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_03_anm.bin");
	m_ForwardHvyAnimations[3] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_04_anm.bin");
	m_ForwardHvyAnimations[4] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_05_anm.bin");
	m_ForwardHvyAnimations[5] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_06_anm.bin");
	m_ForwardHvyAnimations[6] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_08_anm.bin");
	m_ForwardHvyAnimations[7] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_09_anm.bin");
	m_ForwardHvyAnimations[8] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_frmLft_anm.bin");
	m_ForwardHvyAnimations[9] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_frmRht_anm.bin");

	m_bAnimationIndicesCached = true;
}

int32_t CPlayer_Attack_State::FindAnimationIndex(const CPlayer& player,_string_view sAnimationName) const
{
	auto* modelInstance = player.GetModelInstance();
	if (!modelInstance || !modelInstance->GetModel())
		return -1;

	const auto& animations = modelInstance->GetModel()->GetAnimations();
	for (size_t i = 0; i < animations.size(); ++i)
	{
		if (animations[i] && animations[i]->GetAnimName() == sAnimationName)
			return static_cast<int32_t>(i);
	}

	return -1;
}


SPtr<CPlayer_Attack_State> CPlayer_Attack_State::Create()
{
	return ToSPtr(new CPlayer_Attack_State{});
}
