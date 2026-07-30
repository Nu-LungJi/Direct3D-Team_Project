#pragma once
#include "Player_SkillStateBase.h"


NS_BEGIN(Client)


class CPlayer_RevelioSkill_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_RevelioSkill_State, CPlayer_SkillStateBase)

private:
	CPlayer_RevelioSkill_State() = default;
	~CPlayer_RevelioSkill_State() override = default;

public:
	void Enter(CStateMachine* stateMachine) override;
	void Update(CStateMachine* stateMachine, _float deltaTime) override;
	void Exit(CStateMachine* stateMachine) override;

	static SPtr<CPlayer_RevelioSkill_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);

private:
	enum class PHASE
	{
		CAST,
		REVEAL,
		RECOVERY
	};

	_bool	m_bAnimationIndicesCached = false;
	int32_t	m_iRevelioAnimation = -1;
	PHASE	m_ePhase = PHASE::CAST;
	_float	m_fAnimRatio = 0.f;


	static constexpr _float REVEAL_START_RATIO = 0.3f;
	static constexpr _float RECOVERY_START_RATIO = 0.7f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.95f;
};


NS_END
