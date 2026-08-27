#include "pch.h"
#include "Player_DoorPush_State.h"

#include "ComAnimator.h"
#include "ComModelInstance.h"
#include "Player.h"
#include "Player_StateMachine.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)

namespace
{
	constexpr _char DOOR_PUSH_LEFT_ANIMATION[] =
		"AN_ProfessorSharp_MasterRig_Hu_Door_Push_Open_Lft_anm.bin";
}

void CPlayer_DoorPush_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = pStateMachine
		? pStateMachine->GetOwner<CPlayer>()
		: nullptr;
	if (!pPlayer)
		return;

	// [LSY] 이동 의도는 유지해야 CCT가 문을 계속 밀 수 있다.
	// 애니메이션 Root Motion만 끄고 문 이동은 PhysX에 맡긴다.
	pPlayer->SetMovementLocked(false);
	pPlayer->SetRootMotionRotationActive(false);
	pPlayer->SetRootMotionTranslationActive(false);

	CacheAnimationIndex(*pPlayer);
	auto* pAnimator = pPlayer->GetAnimator();
	if (pAnimator && m_iDoorPushAnimation >= 0)
		pAnimator->Play_Anim(m_iDoorPushAnimation, false, 0.12f);
}

void CPlayer_DoorPush_State::Update(
	CStateMachine* pStateMachine,
	_float)
{
	auto* pPlayer = pStateMachine
		? pStateMachine->GetOwner<CPlayer>()
		: nullptr;
	auto* pPlayerStateMachine =
		Cast<CPlayer_StateMachine>(pStateMachine);
	if (!pPlayer || !pPlayerStateMachine)
		return;

	// [LSY] CCT Hit는 Fixed Tick마다 갱신된다. 짧은 유예시간으로 렌더 프레임과
	// Fixed Tick 사이의 간격 때문에 상태가 깜빡이는 것을 막는다.
	if (!pPlayer->HasActiveDoorPushContact() ||
		!pPlayer->HasRawMoveInput() ||
		m_iDoorPushAnimation < 0)
	{
		pPlayerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
	}
}

void CPlayer_DoorPush_State::Exit(CStateMachine* pStateMachine)
{
	auto* pPlayer = pStateMachine
		? pStateMachine->GetOwner<CPlayer>()
		: nullptr;
	if (!pPlayer)
		return;

	pPlayer->SetRootMotionRotationActive(false);
	pPlayer->SetRootMotionTranslationActive(false);
}

void CPlayer_DoorPush_State::CacheAnimationIndex(const CPlayer& player)
{
	if (m_bAnimationIndexCached)
		return;

	m_iDoorPushAnimation = FindAnimationIndex(
		player,
		DOOR_PUSH_LEFT_ANIMATION);
	m_bAnimationIndexCached = true;
}

int32_t CPlayer_DoorPush_State::FindAnimationIndex(
	const CPlayer& player,
	_string_view sAnimationName) const
{
	auto* pModelInstance = player.GetModelInstance();
	if (!pModelInstance || !pModelInstance->GetModel())
		return -1;

	const auto& animations = pModelInstance->GetModel()->GetAnimations();
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

SPtr<CPlayer_DoorPush_State> CPlayer_DoorPush_State::Create()
{
	return ToSPtr(new CPlayer_DoorPush_State{});
}
