
#pragma once
#include "Player_SkillStateBase.h"


NS_BEGIN(Client)


class CPlayer_AcientAttack_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_AcientAttack_State, CPlayer_SkillStateBase)

private:
	CPlayer_AcientAttack_State() = default;
	~CPlayer_AcientAttack_State() override = default;

public:
	void Enter(CStateMachine* stateMachine) override;
	void Update(CStateMachine* stateMachine, _float deltaTime) override;
	void Exit(CStateMachine* stateMachine) override;

	static SPtr<CPlayer_AcientAttack_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);
private:
	enum class PHASE
	{
		CAST,
		DASH,
		RECOVERY
	};

	PLAYER_SKILL_TYPE m_AcientState;

};

NS_END
