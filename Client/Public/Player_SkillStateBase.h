#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;
class CPlayer_StateMachine;

class CPlayer_SkillStateBase : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_SkillStateBase, CState)

protected:
	CPlayer_SkillStateBase() = default;
	~CPlayer_SkillStateBase() override = default;

protected:
	CPlayer* GetPlayer(CStateMachine* pStateMachine) const;
	CPlayer_StateMachine* GetPlayerStateMachine(CStateMachine* pStateMachine) const;

	void SetSkillControl(CPlayer& player, _bool bMovementLocked, _bool bRootMotionTranslation, _bool bRootMotionRotation, _bool bClearMoveIntent = true) const;
	void ResetSkillControl(CPlayer& player) const;
	_bool RequestLocomotion(CStateMachine* pStateMachine) const;

	int32_t FindAnimationIndex(const CPlayer& player, _string_view sAnimationName) const;
};

NS_END
