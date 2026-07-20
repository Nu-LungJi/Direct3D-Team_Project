#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer_Locomotion_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Locomotion_State, CState)

	static constexpr int32_t INVALID_ANIMATION = -1;

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
	using DIRECTION_TABLE = std::array<int32_t, ETOUI(MOVE_DIRECTION::END)>;
	using GAIT_DIRECTION_TABLE = std::array<DIRECTION_TABLE, ETOUI(GAIT::END)>;
	using PHASE_GAIT_DIRECTION_TABLE =
		std::array<GAIT_DIRECTION_TABLE, ETOUI(FOOT_PHASE::END)>;

	void InitializeAnimationTable(class CPlayer& player);
	MOVE_DIRECTION ResolveDirection(_float fSignedAngle) const;
	_float CalculateSignedAngle(const CPlayer& player, const _float3& vWorldDirection) const;
	FOOT_PHASE ResolveFootPhase(_float fAnimationRatio) const;
	GAIT ResolveDesiredGait(const CPlayer& player) const;

	int32_t FindDirectionalAnimation(
		const GAIT_DIRECTION_TABLE& table,
		GAIT eGait,
		MOVE_DIRECTION eDirection) const;
	int32_t FindPhasedDirectionalAnimation(
		const PHASE_GAIT_DIRECTION_TABLE& table,
		FOOT_PHASE ePhase,
		GAIT eGait,
		MOVE_DIRECTION eDirection) const;

	_bool PlayTransient(
		class CComAnimator& animator,
		int32_t iAnimationIndex,
		TRANSITION eTransition,
		GAIT ePendingGait,
		_float fBlendDuration = 0.1f);
	void PlayLoop(
		CPlayer& player,
		class CComAnimator& animator,
		GAIT eGait,
		MOVE_DIRECTION eDirection);
	_bool UpdateTransient(CPlayer& player, class CComAnimator& animator);
	_bool TryStartIdleTurn(CPlayer& player, class CComAnimator& animator);

private:
	_bool m_bAnimationTableInitialized = false;
	int32_t m_iIdleAnimation = INVALID_ANIMATION;

	GAIT_DIRECTION_TABLE m_LoopAnimations{};
	PHASE_GAIT_DIRECTION_TABLE m_StartAnimations{};
	PHASE_GAIT_DIRECTION_TABLE m_StopAnimations{};
	PHASE_GAIT_DIRECTION_TABLE m_FreeTurnStartAnimations{};

	// [from gait][to gait][foot phase]
	std::array<
		std::array<
			std::array<int32_t, ETOUI(FOOT_PHASE::END)>,
			ETOUI(GAIT::END)>,
		ETOUI(GAIT::END)> m_GaitTransitions{};

	// [turn side][45, 90, 135, 180]
	std::array<std::array<int32_t, 4>, ETOUI(TURN_SIDE::END)> m_IdleTurns{};
	// [turn side][foot phase], Jog 180 pivot
	std::array<
		std::array<int32_t, ETOUI(FOOT_PHASE::END)>,
		ETOUI(TURN_SIDE::END)> m_JogPivots{};
	// [gait][turn side][foot phase], free start 180
	std::array<
		std::array<
			std::array<int32_t, ETOUI(FOOT_PHASE::END)>,
			ETOUI(TURN_SIDE::END)>,
		ETOUI(GAIT::END)> m_FreeTurnStart180{};

	GAIT m_eCurrentGait = GAIT::END;
	GAIT m_ePendingGait = GAIT::END;
	MOVE_DIRECTION m_eLastDirection = MOVE_DIRECTION::FRONT;
	TRANSITION m_eTransition = TRANSITION::NONE;
	int32_t m_iTransientAnimation = INVALID_ANIMATION;
};

NS_END
