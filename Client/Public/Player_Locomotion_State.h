
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
	void CacheAnimationIndices(const CPlayer& player);
	int32_t FindAnimationIndex(const CPlayer& player, const _string_view& sAnimationName) const;
	int32_t SelectIdleAnimationForCurrentFeet(const CPlayer& player) const;
	void BeginJogStart(CPlayer& player);
	void BeginJogStop(CPlayer& player);

	_float m_fSignedMoveAngle{};
	MOVE_DIRECTION m_eMoveDirection{ MOVE_DIRECTION::FRONT };
	int32_t m_iIdleAnimation{ -1 };
	int32_t m_iRightFootIdleAnimation{ -1 };
	int32_t m_iWalkForwardAnimation{ -1 };
	int32_t m_iJogStartForwardAnimation{ -1 };
	int32_t m_iJogForwardAnimation{ -1 };
	int32_t m_iJogStopForwardAnimation{ -1 };
	int32_t m_iSprintForwardAnimation{ -1 };
	int32_t m_iSprintLeanLeftAnimation{ -1 };
	int32_t m_iSprintLeanRightAnimation{ -1 };
	_bool m_bAnimationIndicesCached{};
	int32_t m_iActiveMoveLoopAnimation{ -1 };
	_bool m_bJogStarting{};
	_bool m_bJogStopping{};
	_bool m_bWasMoving{};
	_float m_fSprintTurnSpeed{ 240.f };
	_float m_fSprintMoveDirectionBlend{ 0.3f };
	_float m_fFallStateVerticalSpeed{ -3.f };
	_float m_fAirborneTime{};
	_float m_fFallStateGraceTime{ 0.25f };
};

NS_END
