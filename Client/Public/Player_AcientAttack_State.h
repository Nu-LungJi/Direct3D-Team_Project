
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
		ACIENT_THROW,
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
	int32_t m_iAncientThrowLeftAnimation{ -1 };
	int32_t m_iAncientThrowRightAnimation{ -1 };

	PHASE m_ePhase = PHASE::CAST;
	static constexpr _float ACIENT_LIGHTENING_CAST_START_RATIO = 0.55f;
	
	static constexpr _float ACIENT_LIGHTENING_ATTACK_DURATION = 1.1f;
	static constexpr _float ACIENT_LIGHTENING_ATTACK_STOP_DURATION = 1.5f;
	static constexpr _float ACIENT_LIGHTENING_LAST_ATTACK= 0.2f;
	static constexpr _float RECOVERY_EXIT_RATIO = 1.f;
	_bool   m_bOnceLighting = false;
	_bool   m_bOnceLastLighting = false;
	_float	m_fAnimRatio = 0.f;
	_float	m_fAcientElapsed = 0.f;
	_float	m_fSpawnDelay = 0.f;
	std::optional<CHandle> m_hThrowBarrel{};
	std::optional<CHandle> m_hThrowDestination{};
	static constexpr _float ACIENT_THROW_FACING_END_RATIO = 0.2f;
	static constexpr _float ACIENT_THROW_STATE_RELEASE_RATIO = 0.85f;
	static constexpr _float ACIENT_THROW_TURN_SPEED = 540.f;
};


NS_END
