#pragma once
#include "Player_SkillStateBase.h"


NS_BEGIN(Client)


class CPlayer_RepairoSkill_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_RepairoSkill_State, CPlayer_SkillStateBase)

private:
	CPlayer_RepairoSkill_State() = default;
	~CPlayer_RepairoSkill_State() override = default;

public:
	void Enter(CStateMachine* stateMachine) override;
	void Update(CStateMachine* stateMachine, _float deltaTime) override;
	void Exit(CStateMachine* stateMachine) override;

	static SPtr<CPlayer_RepairoSkill_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);

private:
	enum class PHASE
	{
		CAST,
		REPAIRO,
		RECOVERY
	};

	_bool	m_bAnimationIndicesCached = false;
	int32_t	m_iRepairoStartAnimation = -1;
	int32_t	m_iRepairoLoopAnimation = -1;
	int32_t	m_iRepairoEndAnimation = -1;
	PHASE	m_ePhase = PHASE::CAST;
	_float	m_fAnimationRatio = 0.f;


	static constexpr _float PHASE_EXIT_RATIO = 0.98f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.3f;
	_bool trailEnd = false;
};


NS_END
