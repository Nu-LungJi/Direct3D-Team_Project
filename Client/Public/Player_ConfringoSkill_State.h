#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_ConfringoSkill_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_ConfringoSkill_State, CPlayer_SkillStateBase)

private:
	CPlayer_ConfringoSkill_State() = default;
	~CPlayer_ConfringoSkill_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void LateUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;
	static SPtr<CPlayer_ConfringoSkill_State> Create();

private:
	enum class PHASE : uint8_t
	{
		CAST,
		RECOVERY
	};

	_bool EnsureParticleCommandsLoaded();
	_bool TryGetWandPosition(const CPlayer& player, _float3& OutPosition) const;
	void StartCastEffect(CPlayer& player);
	void StopCastEffect();
	void UpdateCastEffect(CPlayer& player, _float fTimeDelta);
	void EmitSparkCurve();
	_bool FireProjectile(CPlayer& player);

protected:
	void Free() override;

private:
	PHASE m_ePhase{ PHASE::CAST };
	_float m_fAnimationRatio{};
	_bool m_bCastingCueReached{};
	_bool m_bProjectileCueReached{};

	std::vector<SPAWN_COMMAND> m_FlameCommands{};
	std::vector<SPAWN_COMMAND> m_SparkCommands{};
	uint32_t m_iFlameOwnerId{ INVALID_PARTICLE_OWNER_ID };
	_bool m_bCastEffectActive{};
	_float m_fSparkElapsed{};
	_float m_fSparkInterval{ 0.05f };
	_float m_fSparkTrailSpacing{ 0.08f };
	_float3 m_vPreviousWandPosition{};
	std::array<_float3, 4> m_SparkControlPoints{};

	static constexpr _float CASTING_EFFECT_RATIO = 0.08f;
	static constexpr _float PROJECTILE_RELEASE_RATIO = 0.28f;
	static constexpr _float CAST_TO_RECOVERY_RATIO = 0.32f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.45f;
};

NS_END
