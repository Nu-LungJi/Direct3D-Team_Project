#include "pch.h"
#include "Player_Hit_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)

void CPlayer_Hit_State::Enter(CStateMachine* pStateMachine)
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

	CacheAnimationIndices(*player);

	player->SetCurrentMoveSpeed(0.f);
	player->SetMovementLocked(true);
	player->SetRootMotionTranslationActive(true);
	player->SetRootMotionRotationActive(false);
	if (auto* moveIntent = player->GetMoveIntent())
	{
		moveIntent->ClearMoveIntent();
		moveIntent->ClearFacingIntent();
	}

	int32_t iAnimation = GetHitAnimation(ResolveHitDirection(*player));
	if (iAnimation < 0)
		iAnimation = GetHitAnimation(HIT_DIRECTION::BWD);

	if (iAnimation < 0)
	{
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}

	animator->Play_Anim(iAnimation, false, HIT_BLEND_DURATION);
}

void CPlayer_Hit_State::Exit(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!player)
		return;

	player->SetMovementLocked(false);
	player->SetRootMotionTranslationActive(false);
	player->SetRootMotionRotationActive(false);
}

void CPlayer_Hit_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);

	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	auto* playerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine);
	if (!player || !playerStateMachine)
		return;

	auto* animator = player->GetAnimator();
	if (!animator || animator->GetFinish())
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
}

void CPlayer_Hit_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	m_HitAnimations.fill(-1);
	m_HitAnimations[static_cast<size_t>(HIT_DIRECTION::FWD)] =
		FindAnimationIndex(player, "AN_ElegantStudent_PrettyGirl2_Rig_ESPG2_Hu_Rct_Hit_Fwd_anm.bin");
	m_HitAnimations[static_cast<size_t>(HIT_DIRECTION::BWD)] =
		FindAnimationIndex(player, "AN_ElegantStudent_PrettyGirl2_Rig_ESPG2_Hu_Rct_Hit_Bwd_anm.bin");
	m_HitAnimations[static_cast<size_t>(HIT_DIRECTION::LFT)] =
		FindAnimationIndex(player, "AN_ElegantStudent_PrettyGirl2_Rig_ESPG2_Hu_Rct_Hit_Lft_anm.bin");
	m_HitAnimations[static_cast<size_t>(HIT_DIRECTION::RHT)] =
		FindAnimationIndex(player, "AN_ElegantStudent_PrettyGirl2_Rig_ESPG2_Hu_Rct_Hit_Rht_anm.bin");

	m_bAnimationIndicesCached = true;
}

CPlayer_Hit_State::HIT_DIRECTION
CPlayer_Hit_State::ResolveHitDirection(const CPlayer& player) const
{
	auto* attacker = CGameInstance::Get().GetGameObjectByHandle(player.GetTargetHandle());
	if (!attacker)
		return HIT_DIRECTION::BWD;

	_vector vPlayerLook = XMVectorSetY(player.GetTransform().GetState(STATE::LOOK), 0.f);
	_vector vPlayerRight = XMVectorSetY(player.GetTransform().GetState(STATE::RIGHT), 0.f);
	_vector vToAttacker = XMVectorSetY(attacker->GetTransform().GetState(STATE::POSITION) - player.GetTransform().GetState(STATE::POSITION),0.f);

	constexpr _float EPSILON = std::numeric_limits<_float>::epsilon();
	if (XMVectorGetX(XMVector3LengthSq(vPlayerLook)) <= EPSILON ||
		XMVectorGetX(XMVector3LengthSq(vPlayerRight)) <= EPSILON ||
		XMVectorGetX(XMVector3LengthSq(vToAttacker)) <= EPSILON)
	{
		return HIT_DIRECTION::BWD;
	}

	vPlayerLook = XMVector3Normalize(vPlayerLook);
	vPlayerRight = XMVector3Normalize(vPlayerRight);
	vToAttacker = XMVector3Normalize(vToAttacker);

	const _float fForward = XMVectorGetX(XMVector3Dot(vPlayerLook, vToAttacker));
	const _float fRight = XMVectorGetX(XMVector3Dot(vPlayerRight, vToAttacker));

	if (std::abs(fForward) >= std::abs(fRight))
		return fForward >= 0.f ? HIT_DIRECTION::BWD : HIT_DIRECTION::FWD;

	return fRight >= 0.f ? HIT_DIRECTION::LFT : HIT_DIRECTION::RHT;
}

int32_t CPlayer_Hit_State::GetHitAnimation(HIT_DIRECTION eDirection) const
{
	const size_t iDirection = static_cast<size_t>(eDirection);
	if (iDirection >= HIT_DIRECTION_COUNT)
		return -1;

	return m_HitAnimations[iDirection];
}

int32_t CPlayer_Hit_State::FindAnimationIndex(
	const CPlayer& player,
	_string_view sAnimationName) const
{
	auto* modelInstance = player.GetModelInstance();
	if (!modelInstance || !modelInstance->GetModel())
		return -1;

	const auto& animations = modelInstance->GetModel()->GetAnimations();
	for (size_t i = 0; i < animations.size(); ++i)
	{
		if (animations[i] &&
			animations[i]->GetAnimName() == sAnimationName)
		{
			return static_cast<int32_t>(i);
		}
	}

	return -1;
}

SPtr<CPlayer_Hit_State> CPlayer_Hit_State::Create()
{
	return ToSPtr(new CPlayer_Hit_State{});
}
