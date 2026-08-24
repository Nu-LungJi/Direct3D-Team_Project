#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

class CPlayer_StupefySkill_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_StupefySkill_State, CPlayer_SkillStateBase)
private:
	CPlayer_StupefySkill_State() = default;
	~CPlayer_StupefySkill_State() override = default;
public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;
	static SPtr<CPlayer_StupefySkill_State> Create();
private:
	void CacheAnimationIndices(const CPlayer& player);
	_bool PlayParryReaction(CPlayer& player, _bool bHeavyReaction);
	_bool PlayCounterAnimation(CPlayer& player, const _float3& vParryPosition);
	enum class PHASE : uint8_t
	{
		PARRY_REACTION,
		COUNTER_ATTACK,
	};

	std::array<int32_t, 5> m_Animations{};
	int32_t m_iLightParryReactionAnimation{ -1 };
	int32_t m_iHeavyParryReactionAnimation{ -1 };
	_bool m_bAnimationsCached{};
	_bool m_bSpeedRestored{};
	_bool m_bProjectileReleased{};
	_bool m_bCounterQueued{};
	_float m_fPreviousAnimRatio{};
	_float3 m_vParryPosition{};
	PHASE m_ePhase{ PHASE::PARRY_REACTION };
	static constexpr _float BLEND_DURATION = 0.12f;
	static constexpr _float REACTION_BLEND_DURATION = 0.05f;
	static constexpr _float REACTION_SPEED = 1.8f;
	static constexpr _float REACTION_EXIT_RATIO = 0.25f;
	static constexpr _float TURN_SPEED = 0.65f;
	static constexpr _float TURN_END_RATIO = 0.10f;
	static constexpr _float ATTACK_SPEED = 1.9f;
	static constexpr _float PROJECTILE_RELEASE_RATIO = 0.18f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.32f;
};

NS_END
