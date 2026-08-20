#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_Roll_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Roll_State, CState)

private:
	CPlayer_Roll_State() = default;
	~CPlayer_Roll_State() override = default;

public:


	void Enter(CStateMachine* pStateMachine) override;
	void Exit(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

	static SPtr<CPlayer_Roll_State> Create();

private:
	int32_t FindAnimationIndex(const CPlayer& player,_string_view sAnimationName) const;

public:
	static constexpr _float ATTACK_CANCEL_RATIO{ 0.6f };
private:
	int32_t m_iRollAnimation{ -1 };
	_float3 m_vRollDirection{};
	_float m_fRollSpeed{ 16.5f };
	_float m_fRollStopStartRatio{ 0.20f };
	_float m_fRollMoveEndRatio{ 0.45f };
	_float m_fRollMinSpeedScale{ 0.15f };
	_float m_fLocomotionCancelRatio{ 0.30f };
	_float m_fRollDirectionResponse{ 3.f };
	_float m_fPreviousAnimRatio{};
};

NS_END
