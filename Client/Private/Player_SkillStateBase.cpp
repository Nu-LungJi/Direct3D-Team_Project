#include "pch.h"
#include "Player_SkillStateBase.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "GameInstance.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
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

_bool CPlayer_SkillStateBase::HasValidTarget(const CPlayer& player) const
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
