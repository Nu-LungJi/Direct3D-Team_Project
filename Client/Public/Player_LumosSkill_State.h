#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

class CPlayer_LumosSkill_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_LumosSkill_State, CPlayer_SkillStateBase)

private:
	int32_t m_iCastAnimation{ -1 };
	_bool m_bAnimationCached{};
	_bool m_bToggleApplied{};
	static constexpr _float TOGGLE_RATIO = 0.2f;
	static constexpr _float EXIT_RATIO = 0.8f;
	void CacheAnimation(const CPlayer& player);

	private:
	CPlayer_LumosSkill_State() = default;
	~CPlayer_LumosSkill_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_LumosSkill_State> Create();
};

NS_END
