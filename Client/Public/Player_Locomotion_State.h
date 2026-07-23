
#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_Locomotion_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Locomotion_State, CState)


	enum class MOVE_DIRECTION : uint32_t
	{
		FRONT,
		RIGHT_45,
		RIGHT_90,
		RIGHT_135,
		BACKWARD,
		LEFT_135,
		LEFT_90,
		LEFT_45,
		END,
	};

	enum class GAIT : uint32_t
	{
		WALK,
		JOG,
		SPRINT,
		END,
	};

	enum class FOOT_PHASE : uint32_t
	{
		LEFT,
		RIGHT,
		END,
	};

	enum class TURN_SIDE : uint32_t
	{
		LEFT,
		RIGHT,
		END,
	};

	enum class TRANSITION : uint32_t
	{
		NONE,
		START,
		STOP,
		GAIT_CHANGE,
		IDLE_TURN,
		PIVOT,
	};

private:
	CPlayer_Locomotion_State();
	~CPlayer_Locomotion_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

	static SPtr<CPlayer_Locomotion_State> Create();

private:
	_float CalculateSignedAngle(const CPlayer& player,const _float3& vMoveDirection) const;

	MOVE_DIRECTION ResolveDirection(_float fSignedAngle) const;
	int32_t FindAnimationIndex(const CPlayer& player, const _string_view& sAnimationName) const;
	int32_t ResolveIdleTurnAnimation(_float fSignedAngle) const;
	int32_t ResolveJogTurnAnimation(_float fSignedAngle) const;
	void BeginTurnDecision(CPlayer& player, const _float3& vTargetDirection, int32_t iIdleAnimation);
	void BeginIdleTurn(CPlayer& player, const _float3& vTargetDirection, int32_t iAnimationIndex);
	void BeginJogTurn(CPlayer& player, const _float3& vTargetDirection, int32_t iAnimationIndex);
	void BeginJogStart(CPlayer& player);
	void BeginJogStop(CPlayer& player);
	void UpdateIdleTurnRotation(CPlayer& player, _float fAnimationRatio);
	void FinishIdleTurn(CPlayer& player);

	_float m_fSignedMoveAngle{};
	MOVE_DIRECTION m_eMoveDirection{ MOVE_DIRECTION::FRONT };
	std::array<int32_t, 4> m_LeftIdleTurns{ -1, -1, -1, -1 };
	std::array<int32_t, 4> m_RightIdleTurns{ -1, -1, -1, -1 };
	std::array<int32_t, 4> m_LeftJogTurns{ -1, -1, -1, -1 };
	std::array<int32_t, 4> m_RightJogTurns{ -1, -1, -1, -1 };
	int32_t m_iIdleAnimation{ -1 };
	int32_t m_iJogStartForwardAnimation{ -1 };
	int32_t m_iJogForwardAnimation{ -1 };
	int32_t m_iJogStopForwardAnimation{ -1 };
	int32_t m_iPendingIdleTurnAnimation{ -1 };
	_bool m_bTurnPending{};
	_bool m_bIdleTurning{};
	_bool m_bJogTurning{};
	_bool m_bJogStarting{};
	_bool m_bJogStopping{};
	_bool m_bWasMoving{};
	_float m_fTurnHoldTime{};
	_float m_fJogTurnHoldThreshold{ 0.15f };
	_float3 m_vTurnTargetDirection{};
	_float4 m_qTurnStartRotation{ 0.f, 0.f, 0.f, 1.f };
	_float4 m_qTurnTargetRotation{ 0.f, 0.f, 0.f, 1.f };
	_float m_fTurnSignedAngleRadians{};
};

NS_END
