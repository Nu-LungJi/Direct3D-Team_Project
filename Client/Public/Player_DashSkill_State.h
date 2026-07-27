#pragma once
#include "Player_SkillStateBase.h"


NS_BEGIN(Client)


class CPlayer_DashSkill_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_DashSkill_State, CPlayer_SkillStateBase)

private:
	CPlayer_DashSkill_State() = default;
	~CPlayer_DashSkill_State() override = default;

public:
	void Enter(CStateMachine* stateMachine) override;
	void Update(CStateMachine* stateMachine, _float deltaTime) override;
	void Exit(CStateMachine* stateMachine) override;

	static SPtr<CPlayer_DashSkill_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);
private:
	enum class PHASE
	{
		CAST,
		DASH,
		RECOVERY
	};

	static constexpr _float CAST_START_RATIO = 0.f;
	static constexpr _float DASH_END_RATIO = 0.25f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.20f;
	static constexpr _float DASH_SPEED = 120.f;
	static constexpr _float DASH_DURATION = 0.25f;

	PHASE m_ePhase = PHASE::CAST;
	_float m_fAnimRatio{};
	_float m_fScaleTime{};
	_float m_fDashElapsed{};
	_float3 m_vDashDirection{};
	int32_t m_iDashAnimIndex{ -1 };
	int32_t m_iDashEndAnimIndex{ -1 };


	_float3 vNormalScale = { 1.f, 1.f, 1.f };
	_float3 vSmallScale = { 0.f, 0.f, 0.f };

	_float	m_fDistanceOffeset = 2.f;
	_float3	m_vSpwanPos{};
	_bool    m_bAnimationIndicesCached = false;
};

NS_END
