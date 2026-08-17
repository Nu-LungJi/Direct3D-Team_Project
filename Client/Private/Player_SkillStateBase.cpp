#include "pch.h"
#include "Player_SkillStateBase.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "GameInstance.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
#include "SkillTarget.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)

CPlayer* CPlayer_SkillStateBase::GetPlayer(CStateMachine* pStateMachine) const
{
	return pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
}

CPlayer_StateMachine* CPlayer_SkillStateBase::GetPlayerStateMachine(CStateMachine* pStateMachine) const
{
	return Cast<CPlayer_StateMachine>(pStateMachine);
}

void CPlayer_SkillStateBase::SetSkillControl(CPlayer& player,_bool bMovementLocked,_bool bRootMotionTranslation,_bool bRootMotionRotation,_bool bClearMoveIntent) const
{
	player.SetMovementLocked(bMovementLocked);
	player.SetRootMotionTranslationActive(bRootMotionTranslation);
	player.SetRootMotionRotationActive(bRootMotionRotation);

	if (bClearMoveIntent)
	{
		if (auto* pMoveIntent = player.GetMoveIntent())
		{
			pMoveIntent->ClearMoveIntent();
			pMoveIntent->ClearFacingIntent();
		}
	}
}

void CPlayer_SkillStateBase::ResetSkillControl(CPlayer& player) const
{
	player.SetMovementLocked(false);
	player.SetRootMotionTranslationActive(false);
	player.SetRootMotionRotationActive(false);
}

_bool CPlayer_SkillStateBase::RequestLocomotion(CStateMachine* pStateMachine) const
{
	auto* pPlayerStateMachine = GetPlayerStateMachine(pStateMachine);
	return pPlayerStateMachine &&
		pPlayerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
}

_bool CPlayer_SkillStateBase::HasTarget(const CPlayer& player) const
{
	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(
		player.GetTargetHandle());
	if (!pTarget)
		return false;

	_vector vToTarget =pTarget->GetTransform().GetState(STATE::POSITION) -player.GetTransform().GetState(STATE::POSITION);
	vToTarget = XMVectorSetY(vToTarget, 0.f);

	const _float fDistanceSq = XMVectorGetX(XMVector3LengthSq(vToTarget));
	return fDistanceSq > std::numeric_limits<_float>::epsilon() && fDistanceSq <= TARGET_MAX_DISTANCE * TARGET_MAX_DISTANCE;
}

_bool CPlayer_SkillStateBase::HasValidTarget(const CPlayer& player)
{
	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(
		player.GetTargetHandle());
	if (!pTarget)
		return false;

	_vector vToTarget =
		pTarget->GetTransform().GetState(STATE::POSITION) -
		player.GetTransform().GetState(STATE::POSITION);
	vToTarget = XMVectorSetY(vToTarget, 0.f);

	const _float fDistanceSq = XMVectorGetX(XMVector3LengthSq(vToTarget));
	if (fDistanceSq <= std::numeric_limits<_float>::epsilon() ||
		fDistanceSq > TARGET_MAX_DISTANCE * TARGET_MAX_DISTANCE)
	{
		return false;
	}

	auto* pPlayerCamera = CGameInstance::Get().GetActiveCamera("PlayerCamera");
	if (!pPlayerCamera)
		return false;

	_vector vPlayerLook = XMVectorSetY(player.GetTransform().GetState(STATE::LOOK),0.f);
	_vector vCameraLook = XMVectorSetY(pPlayerCamera->GetTransform().GetState(STATE::LOOK),0.f);
	if (XMVectorGetX(XMVector3LengthSq(vPlayerLook)) <=std::numeric_limits<_float>::epsilon() ||
		XMVectorGetX(XMVector3LengthSq(vCameraLook)) <=std::numeric_limits<_float>::epsilon())
	{
		return false;
	}

	vToTarget = XMVector3Normalize(vToTarget);
	vPlayerLook = XMVector3Normalize(vPlayerLook);
	vCameraLook = XMVector3Normalize(vCameraLook);

	const _float fPlayerDot =XMVectorGetX(XMVector3Dot(vPlayerLook, vToTarget));
	const _float fCameraDot =XMVectorGetX(XMVector3Dot(vCameraLook, vToTarget));

	return fPlayerDot >= TARGET_FRONT_DOT_THRESHOLD && fCameraDot >= TARGET_FRONT_DOT_THRESHOLD;
}

_bool CPlayer_SkillStateBase::TryApplySkillToTarget(CPlayer& player,PLAYER_SKILL_TYPE eSkillType) const
{//창준 변경
	auto pMonster = CGameInstance::Get().GetGameObjectByHandle(player.GetTargetHandle());
	if (nullptr == pMonster) return false;
	auto pSkillTarget = dynamic_cast<CSkillTarget*>(pMonster);
	if (nullptr == pSkillTarget) return false;
	return pSkillTarget->Check_Table(eSkillType);
}

void CPlayer_SkillStateBase::CacheDirectionalAttackAnimations(const CPlayer& player)
{
	if (m_bDirectionalAttackAnimationsCached)
		return;

	m_DirectionalLightAnimations.fill(-1);
	m_DirectionalHeavyAnimations.fill(-1);

	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::FWD)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_01_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_45)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_45_Lht_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_90)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_90_Lht_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_135)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_135_Lht_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_180)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_180_Lht_Spin_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_45)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_45_Lht_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_90)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_90_Lht_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_135)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_135_Lht_frmLft_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_180)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_180_Lht_Spin_anm.bin");

	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::FWD)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_01_Spin_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_45)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_45_Hvy_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_90)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_90_Hvy_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_135)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_135_Hvy_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_180)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_180_Hvy_SpinRht_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_45)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_45_Hvy_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_90)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_90_Hvy_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_135)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_135_Hvy_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_180)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_180_Hvy_Spin_anm.bin");

	m_bDirectionalAttackAnimationsCached = true;
}

CPlayer_SkillStateBase::ATTACK_DIRECTION
CPlayer_SkillStateBase::ResolveTargetAttackDirection(const CPlayer& player) const
{
	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(player.GetTargetHandle());
	if (!pTarget)
		return ATTACK_DIRECTION::FWD;

	_vector vPlayerLook = XMVectorSetY(player.GetTransform().GetState(STATE::LOOK), 0.f);
	_vector vPlayerRight = XMVectorSetY(player.GetTransform().GetState(STATE::RIGHT), 0.f);
	_vector vAttackDirection = XMVectorSetY(
		pTarget->GetTransform().GetState(STATE::POSITION) -
		player.GetTransform().GetState(STATE::POSITION), 0.f);

	constexpr _float EPSILON = std::numeric_limits<_float>::epsilon();
	if (XMVectorGetX(XMVector3LengthSq(vPlayerLook)) <= EPSILON ||
		XMVectorGetX(XMVector3LengthSq(vPlayerRight)) <= EPSILON ||
		XMVectorGetX(XMVector3LengthSq(vAttackDirection)) <= EPSILON)
		return ATTACK_DIRECTION::FWD;

	vPlayerLook = XMVector3Normalize(vPlayerLook);
	vPlayerRight = XMVector3Normalize(vPlayerRight);
	vAttackDirection = XMVector3Normalize(vAttackDirection);

	const _float fForward = XMVectorGetX(XMVector3Dot(vPlayerLook, vAttackDirection));
	const _float fRight = XMVectorGetX(XMVector3Dot(vPlayerRight, vAttackDirection));
	const _float fSignedAngle = XMConvertToDegrees(std::atan2(fRight, fForward));
	const _float fAbsAngle = std::abs(fSignedAngle);
	const _bool bRight = fSignedAngle >= 0.f;

	if (fAbsAngle < 22.5f) return ATTACK_DIRECTION::FWD;
	if (fAbsAngle < 67.5f) return bRight ? ATTACK_DIRECTION::RHT_45 : ATTACK_DIRECTION::LFT_45;
	if (fAbsAngle < 112.5f) return bRight ? ATTACK_DIRECTION::RHT_90 : ATTACK_DIRECTION::LFT_90;
	if (fAbsAngle < 157.5f) return bRight ? ATTACK_DIRECTION::RHT_135 : ATTACK_DIRECTION::LFT_135;
	return bRight ? ATTACK_DIRECTION::RHT_180 : ATTACK_DIRECTION::LFT_180;
}

_bool CPlayer_SkillStateBase::PlayRandomTargetAttack(CPlayer& player, _float fBlendDuration)
{
	return PlayTargetAttack(
		player, Engine::RandInt(0, 1) == 1, fBlendDuration);
}

_bool CPlayer_SkillStateBase::PlayTargetAttack(
	CPlayer& player, _bool bHeavy, _float fBlendDuration)
{
	auto* pAnimator = player.GetAnimator();
	if (!pAnimator)
		return false;

	CacheDirectionalAttackAnimations(player);
	const ATTACK_DIRECTION eDirection = ResolveTargetAttackDirection(player);
	const size_t iDirection = static_cast<size_t>(eDirection);

	int32_t iAnimation = bHeavy
		? m_DirectionalHeavyAnimations[iDirection]
		: m_DirectionalLightAnimations[iDirection];
	if (iAnimation < 0)
	{
		iAnimation = bHeavy
			? m_DirectionalLightAnimations[iDirection]
			: m_DirectionalHeavyAnimations[iDirection];
	}
	if (iAnimation < 0)
		return false;

	player.SetRootMotionTranslationActive(true);
	player.SetRootMotionRotationActive(
		bHeavy || eDirection != ATTACK_DIRECTION::FWD);
	pAnimator->Play_Anim(iAnimation, false, fBlendDuration);
	pAnimator->GetCurAnimState().fSpeed = 1.3f;
	return true;
}

int32_t CPlayer_SkillStateBase::FindAnimationIndex(const CPlayer& player,_string_view sAnimationName) const
{
	auto* pModelInstance = player.GetModelInstance();
	if (!pModelInstance || !pModelInstance->GetModel())
		return -1;

	const auto& animations = pModelInstance->GetModel()->GetAnimations();
	for (size_t i = 0; i < animations.size(); ++i)
	{
		if (animations[i] && animations[i]->GetAnimName() == sAnimationName)
		{
			return (int32_t)(i);
		}
	}

	return -1;
}
