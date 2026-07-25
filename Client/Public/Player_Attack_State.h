
#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_Attack_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Attack_State, CState)


private:
	CPlayer_Attack_State() = default;
	~CPlayer_Attack_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Exit(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

	static SPtr<CPlayer_Attack_State> Create();

private:
	int32_t FindAnimationIndex(
		const CPlayer& player,
		_string_view sAnimationName) const;

private:
	static constexpr size_t FORWARD_LIGHT_ANIMATION_COUNT = 9;
	static constexpr size_t FORWARD_HVY_ANIMATION_COUNT = 10;
	static constexpr _float ATTACK_BLEND_DURATION = 0.12f;
	static constexpr _float COMBO_INPUT_START_RATIO = 0.25f;
	static constexpr _float COMBO_LINK_RATIO = 0.20f;
	static constexpr _float COMBO_INPUT_END_RATIO = 0.85f;
	static constexpr _float MOVE_CANCEL_START_RATIO = 0.35f;

	std::array<int32_t, FORWARD_LIGHT_ANIMATION_COUNT> m_ForwardLightAnimations{};
	std::array<int32_t, FORWARD_HVY_ANIMATION_COUNT > m_ForwardHvyAnimations{};
	size_t m_iCurrentForwardLightAnimation{};
	size_t m_iCurrentForwardHvyAnimation{};
	uint32_t m_iComboCount{};
	_bool m_bAttackQueued{};
};

NS_END
