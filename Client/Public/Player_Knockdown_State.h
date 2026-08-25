#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_Knockdown_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Knockdown_State, CState)

private:
	enum class DIRECTION : uint8_t { FWD, BWD, LFT, RHT, END };
	enum class SEQUENCE : uint8_t { AIRBORNE, SPLAT_HOLD, GETUP, END };

	struct ANIMATION_SET
	{
		std::array<int32_t, static_cast<size_t>(SEQUENCE::END)> iAnimations{};
	};

private:
	CPlayer_Knockdown_State() = default;
	~CPlayer_Knockdown_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Exit(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	static SPtr<CPlayer_Knockdown_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);
	DIRECTION ResolveDirection(const CPlayer& player) const;
	_bool PlaySequence(CPlayer& player, SEQUENCE eSequence);
	int32_t FindAnimationIndex(const CPlayer& player, _string_view sAnimationName) const;

private:
	static constexpr size_t DIRECTION_COUNT = static_cast<size_t>(DIRECTION::END);
	static constexpr size_t SEQUENCE_COUNT = static_cast<size_t>(SEQUENCE::END);
	static constexpr _float BLEND_DURATION = 0.1f;
	static constexpr _float LANDING_BLEND_DURATION = 0.18f;
	static constexpr _float DOWN_HOLD_DURATION = 0.75f;
	static constexpr _float INPUT_GETUP_ANIMATION_SPEED = 1.65f;
	static constexpr _float AIRBORNE_ANIMATION_SPEED = 1.0f;
	static constexpr _float LAUNCH_VERTICAL_SPEED = 13.5f;
	static constexpr _float LAUNCH_HORIZONTAL_SPEED = 13.5f;
	static constexpr _float KNOCKDOWN_GRAVITY = -20.f;
	static constexpr _float LANDING_SLIDE_DURATION = 0.45f;
	static constexpr _float LANDING_SLIDE_MIN_SPEED = 7.f;
	static constexpr _float LANDING_SLIDE_MAX_SPEED = 10.f;
	static constexpr _float AIRBORNE_ROOT_MOTION_SCALE = 0.f;

	std::array<ANIMATION_SET, DIRECTION_COUNT> m_AnimationSets{};
	DIRECTION m_eDirection{ DIRECTION::BWD };
	SEQUENCE m_eSequence{ SEQUENCE::END };
	_float m_fSequenceTime{};
	_float m_fLandingSlideTime{};
	_float3 m_vLandingSlideVelocity{};
	_float3 m_vLaunchSlideDirection{};
	_float m_fPreviousGravity{};
	_bool m_bWasAirborne{};
	_bool m_bLandingSliding{};
	_bool m_bPreviousUseGravity{};
	_bool m_bPreviousPreserveHorizontalVelocity{};
	_bool m_bMotorSettingsCaptured{};
	_bool m_bAnimationIndicesCached{};
};

NS_END
