
#pragma once
#include "Player_SkillStateBase.h"


NS_BEGIN(Client)


class CPlayer_AccioSkill_State final : public CPlayer_SkillStateBase
{
public:
	enum class ACCIOSTATE {
		MONSTER,
		OBJECT,
		END
	};
public:
	DECLARE_DERIVED_TYPE(CPlayer_AccioSkill_State, CPlayer_SkillStateBase)


private:
	CPlayer_AccioSkill_State() = default;
	~CPlayer_AccioSkill_State() override = default;

public:
	void Enter(CStateMachine* stateMachine) override;
	void Update(CStateMachine* stateMachine, _float deltaTime) override;
	void Exit(CStateMachine* stateMachine) override;

	static SPtr<CPlayer_AccioSkill_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);
private:
	enum class PHASE
	{
		CAST,
		ATTACK,
		RECOVERY
	};

	_bool	m_bAnimationIndicesCached = false;

	int32_t m_AccioCast_Animation{};
	int32_t m_AccioEnd_Animation{};

	PHASE m_ePhase = PHASE::CAST;
	ACCIOSTATE m_eAccio = ACCIOSTATE::END;
	static constexpr _float CAST_START_RATIO = 0.f;
	static constexpr _float ATTACK_END_RATIO = 0.3f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.5f;
	_float	m_fAnimRatio = 0.f;

};


NS_END
