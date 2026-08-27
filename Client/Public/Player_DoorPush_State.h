#pragma once

#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;

// [LSY] CCT가 물리 문을 미는 동안 왼손 문 밀기 동작을 표현한다.
// 문 회전은 계속 PhysX가 담당하며 이 상태는 애니메이션만 소유한다.
class CPlayer_DoorPush_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_DoorPush_State, CState)

private:
	CPlayer_DoorPush_State() = default;
	~CPlayer_DoorPush_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_DoorPush_State> Create();

private:
	void CacheAnimationIndex(const CPlayer& player);
	int32_t FindAnimationIndex(
		const CPlayer& player,
		_string_view sAnimationName) const;

private:
	int32_t m_iDoorPushAnimation{ -1 };
	_bool m_bAnimationIndexCached{};
};

NS_END
