#pragma once
#include "Player_SkillStateBase.h"


NS_BEGIN(Client)


class CPlayer_DepulsoSkill_State final : public CPlayer_SkillStateBase
{

public:
	DECLARE_DERIVED_TYPE(CPlayer_DepulsoSkill_State, CPlayer_SkillStateBase)


private:
	CPlayer_DepulsoSkill_State() = default;
	~CPlayer_DepulsoSkill_State() override = default;

public:
	void Enter(CStateMachine* stateMachine) override;
	void Update(CStateMachine* stateMachine, _float deltaTime) override;
	void Exit(CStateMachine* stateMachine) override;

	static SPtr<CPlayer_DepulsoSkill_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);
private:
	enum class PHASE
	{
		CAST,
		ATTACK,
		PUSH,
		RECOVERY
	};

	_bool	m_bAnimationIndicesCached = false;

	int32_t m_DepulsoCast_Animation{};
	int32_t m_DepulsoEnd_Animation {};

	PHASE m_ePhase = PHASE::CAST;

	static constexpr _float CAST_START_RATIO = 0.f;
	static constexpr _float CAST_END_RATIO = 0.2f;
	static constexpr _float MONSTER_PUSH_TIME = 0.f;
	static constexpr _float ATTACK_END_RATIO = 0.15f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.2f;
	// Depulso 이동 조절값. Root Motion 대신 이 구간 동안 전방으로 이동한다.
	_float	m_fAnimRatio = 0.f;

};


NS_END
