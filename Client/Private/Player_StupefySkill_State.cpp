#include "pch.h"
#include "Player_StupefySkill_State.h"
#include "Player.h"
#include "ComAnimator.h"
#include "ComSound.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_StupefySkill_State::Enter(CStateMachine* pStateMachine)
{
	EndBlur();

	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator()) { RequestLocomotion(pStateMachine); return; }
	_bool bHeavyReaction = false;
	const _bool bProtegoReaction =
		pPlayer->ConsumeProtegoReaction(m_vParryPosition, bHeavyReaction);
	m_bCounterQueued = pPlayer->ConsumeParryCounter(m_vParryPosition);
	if (!bProtegoReaction && !m_bCounterQueued) { RequestLocomotion(pStateMachine); return; }
	if (bProtegoReaction)
	{
		BeginBlur(
			bHeavyReaction ? PROTEGO_HEAVY_BLUR_INTENSITY
				: PROTEGO_LIGHT_BLUR_INTENSITY,
			bHeavyReaction ? PROTEGO_HEAVY_BLUR_DURATION
				: PROTEGO_LIGHT_BLUR_DURATION);
	}
	if (bHeavyReaction)
		pPlayer->StartProtegoRecoil(m_vParryPosition);
	CacheAnimationIndices(*pPlayer);
	SetSkillControl(*pPlayer, true, true, true);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->SetCurrentSkill(PLAYER_SKILL_TYPE::ATTACK);
	m_bSpeedRestored = false;
	m_bProjectileReleased = false;
	m_fPreviousAnimationRatio = 0.f;
	m_ePhase = PHASE::PARRY_REACTION;

	// 약한 공격은 제자리 방어, 강한 공격은 슬라이드 방어 후 반격한다.
	// 반동 애니메이션이 없는 데이터에서는 기존 반격 애니메이션으로 바로 넘어간다.
	if (!PlayParryReaction(*pPlayer, bHeavyReaction))
	{
		if (!m_bCounterQueued)
			m_bCounterQueued = pPlayer->ConsumeParryCounter(m_vParryPosition);
		if (!m_bCounterQueued)
		{
			RequestLocomotion(pStateMachine);
			return;
		}
		m_ePhase = PHASE::COUNTER_ATTACK;
		if (!PlayCounterAnimation(*pPlayer, m_vParryPosition))
			RequestLocomotion(pStateMachine);
	}
}

void CPlayer_StupefySkill_State::Update(CStateMachine* pStateMachine, _float)
{
	if (m_bBlurActive)
	{
		m_fBlurRemainUnscaled -= CGameInstance::Get().GetUnscaledDelta();
		if (m_fBlurRemainUnscaled <= 0.f)
			EndBlur();
	}

	auto* pPlayer = GetPlayer(pStateMachine);
	auto* pAnimator = pPlayer ? pPlayer->GetAnimator() : nullptr;
	if (!pPlayer || !pAnimator) { RequestLocomotion(pStateMachine); return; }
	const _float fAnimationRatio = PlayerAnimationRatioGuard::Sanitize(
		pAnimator->GetPlayAnimRatio());
	pPlayer->SetCurrentMoveSpeed(0.f);

	if (m_ePhase == PHASE::PARRY_REACTION)
	{
		if (fAnimationRatio >= REACTION_EXIT_RATIO || pAnimator->GetFinish())
		{
			if (!m_bCounterQueued)
				m_bCounterQueued = pPlayer->ConsumeParryCounter(m_vParryPosition);
			if (!m_bCounterQueued)
			{
				RequestLocomotion(pStateMachine);
				return;
			}
			m_ePhase = PHASE::COUNTER_ATTACK;
			m_fPreviousAnimationRatio = 0.f;
			m_bSpeedRestored = false;
			if (!PlayCounterAnimation(*pPlayer, m_vParryPosition))
				RequestLocomotion(pStateMachine);
		}
		return;
	}

	if (!m_bSpeedRestored && fAnimationRatio >= TURN_END_RATIO)
	{
		pAnimator->GetCurAnimState().fSpeed = ATTACK_SPEED;
		m_bSpeedRestored = true;
	}
	if (!m_bProjectileReleased &&
		PlayerAnimationRatioGuard::Crossed(
			m_fPreviousAnimationRatio,
			fAnimationRatio,
			PROJECTILE_RELEASE_RATIO))
	{
		m_bProjectileReleased = true;
		BeginBlur(
			PROJECTILE_RELEASE_BLUR_INTENSITY,
			PROJECTILE_RELEASE_BLUR_DURATION);
		// [Stupefy Effect] 완드 섬광, 순백색 코어, 옅은 청백색 리본 트레일,
		// 피격 섬광은 FireStupefyProjectile()의 데이터 이름으로 각각 연결한다.
		if (!pPlayer->FireStupefyProjectile())
			DEBUG_LOG("[Stupefy] Failed to spawn projectile.\n");
	}
	m_fPreviousAnimationRatio = fAnimationRatio;
	// 발사가 끝난 뒤 애니메이션 후반부를 기다리지 않고 다음 조작을 받는다.
	if ((m_bProjectileReleased &&
		fAnimationRatio >= RECOVERY_EXIT_RATIO) ||
		pAnimator->GetFinish())
		RequestLocomotion(pStateMachine);
}

void CPlayer_StupefySkill_State::Exit(CStateMachine* pStateMachine)
{
	EndBlur();
	if (auto* pPlayer = GetPlayer(pStateMachine)) ResetSkillControl(*pPlayer);
	m_bSpeedRestored = false;
	m_bProjectileReleased = false;
	m_bCounterQueued = false;
	m_fPreviousAnimationRatio = 0.f;
	m_vParryPosition = {};
	m_ePhase = PHASE::PARRY_REACTION;
}

void CPlayer_StupefySkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationsCached) return;
	m_Animations[0] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Parry_Counter_Atk_Fwd_2_Spin_Rht_anm.bin");
	m_Animations[1] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Parry_Counter_Atk_Lft_90_Spin_Lft_Slam_anm.bin");
	m_Animations[2] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Parry_Counter_Atk_Rht_90_Spin_Rht_Slam_anm.bin");
	m_Animations[3] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Parry_Counter_Atk_Lft_180_Spin_Rht_Send_anm.bin");
	m_Animations[4] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Parry_Counter_Atk_Rht_180_Spin_Rht_anm.bin");
	m_iLightParryReactionAnimation = FindAnimationIndex(player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Protego_Parry_Fwd_R2L_Up_anm.bin");
	m_iHeavyParryReactionAnimation = FindAnimationIndex(player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Protego_Parry_Fwd_Heavy_Slide_anm.bin");
	m_bAnimationsCached = true;
}

_bool CPlayer_StupefySkill_State::PlayParryReaction(
	CPlayer& player, _bool bHeavyReaction)
{
	auto* pAnimator = player.GetAnimator();
	const int32_t iReactionAnimation = bHeavyReaction
		? m_iHeavyParryReactionAnimation
		: m_iLightParryReactionAnimation;
	if (!pAnimator || iReactionAnimation < 0)
		return false;

	pAnimator->Play_Anim(iReactionAnimation, false, REACTION_BLEND_DURATION);
	pAnimator->GetCurAnimState().fSpeed = REACTION_SPEED;
	return true;
}

_bool CPlayer_StupefySkill_State::PlayCounterAnimation(CPlayer& player, const _float3& vParryPosition)
{
	auto* pAnimator = player.GetAnimator();
	if (!pAnimator) return false;
	_vector vLook = XMVectorSetY(player.GetTransform().GetState(STATE::LOOK), 0.f);
	_vector vRight = XMVectorSetY(player.GetTransform().GetState(STATE::RIGHT), 0.f);
	const _vector vPlayerPosition = player.GetTransform().GetState(STATE::POSITION);
	_vector vDirection{};
	if (auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(player.GetTargetHandle()); pTarget && !pTarget->GetPendingDestroy())
		vDirection = XMVectorSetY(pTarget->GetTransform().GetState(STATE::POSITION) - vPlayerPosition, 0.f);
	else
		vDirection = XMVectorSetY(XMLoadFloat3(&vParryPosition) - vPlayerPosition, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vDirection)) <= FLT_EPSILON) vDirection = vLook;
	vLook = XMVector3Normalize(vLook);
	vRight = XMVector3Normalize(vRight);
	vDirection = XMVector3Normalize(vDirection);
	const _float fAngle = XMConvertToDegrees(std::atan2(XMVectorGetX(XMVector3Dot(vRight, vDirection)), XMVectorGetX(XMVector3Dot(vLook, vDirection))));
	const _float fAbsAngle = std::abs(fAngle);
	size_t iDirection = 0;
	if (fAbsAngle >= 135.f) iDirection = fAngle < 0.f ? 3 : 4;
	else if (fAbsAngle >= 45.f) iDirection = fAngle < 0.f ? 1 : 2;
	const int32_t iAnimation = m_Animations[iDirection];
	if (iAnimation < 0) return false;
	pAnimator->Play_Anim(iAnimation, false, BLEND_DURATION);
	pAnimator->GetCurAnimState().fSpeed = TURN_SPEED;

	TIME_SCALE_REQUEST_DESC TimeScaleDesc{};
	TimeScaleDesc.fTargetScale = 0.2f;
	TimeScaleDesc.fMaxUnscaledDuration = 0.06f;
	TimeScaleDesc.fSafetyBlendOut = 0.04f;
	TimeScaleDesc.sTag = "Combat_StupefyCounter";
	if (CGameInstance::Get().BeginTimeScale(TimeScaleDesc))
		BeginBlur(COUNTER_BLUR_INTENSITY, COUNTER_BLUR_DURATION);

	if (auto* pSound = player.GetSound())
	{
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_VOICE_STUPEFY" },
			"./Resources/SampleClient/Sound/Player/Spell/Stupefy/Stupefy_Man.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 2.f,
				.fPitch = 1.f,
				.iPriority = 80,
				.bLoop = false
			});
	}
	return true;
}

void CPlayer_StupefySkill_State::BeginBlur(
	_float fIntensity, _float fUnscaledDuration)
{
	auto& GameInstance = CGameInstance::Get();
	if (!m_bBlurActive)
		m_fPreviousBlurIntensity = GameInstance.Get_RadialBlurIntensity();

	GameInstance.Set_RadialBlurIntensity(fIntensity);
	m_fBlurRemainUnscaled = std::max(0.f, fUnscaledDuration);
	m_bBlurActive = true;
}

void CPlayer_StupefySkill_State::EndBlur()
{
	if (!m_bBlurActive)
		return;

	CGameInstance::Get().Set_RadialBlurIntensity(
		m_fPreviousBlurIntensity);
	m_fBlurRemainUnscaled = 0.f;
	m_bBlurActive = false;
}

SPtr<CPlayer_StupefySkill_State> CPlayer_StupefySkill_State::Create()
{
	return ToSPtr(new CPlayer_StupefySkill_State{});
}
