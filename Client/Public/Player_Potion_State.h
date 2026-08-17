#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

class CPlayer_Potion_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Potion_State, CPlayer_SkillStateBase)

private:
	CPlayer_Potion_State() = default;
	~CPlayer_Potion_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_Potion_State> Create();

private:
	void CacheAnimation(const CPlayer& player);

private:
	int32_t m_iDrinkAnimation{ -1 };
	_bool m_bAnimationCached{};
};

NS_END
