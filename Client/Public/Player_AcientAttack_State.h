
#pragma once
#include "Player_SkillStateBase.h"


NS_BEGIN(Client)


class CPlayer_AcientAttack_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_AcientAttack_State, CPlayer_SkillStateBase)

public:
	enum class ACIENT_SKILL {
		ACIENT_LIGHTENING,
		END
	};
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
		ATTACK,
		RECOVERY
	};

	_bool			  m_bAnimationIndicesCached = false;

	std::array<int32_t, (size_t)ETOUI(ACIENT_SKILL::END)> m_AcientCast_Animations{};
	std::array<int32_t, (size_t)ETOUI(ACIENT_SKILL::END)> m_AcientEnd_Animations{};

	PHASE m_ePhase = PHASE::CAST;
	static constexpr _float ACIENT_LIGHTENING_CAST_START_RATIO = 0.f;
	static constexpr _float ACIENT_LIGHTENING_ATTACK_DURATION = 1.f;
	static constexpr _float RECOVERY_EXIT_RATIO = 1.f;
	_float	m_fAnimRatio = 0.f;
	_float	m_fAcientElapsed = 0.f;
};


NS_END
