#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_BombardaSkill_State final
	: public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_BombardaSkill_State, CPlayer_SkillStateBase)

private:
	CPlayer_BombardaSkill_State() = default;
	~CPlayer_BombardaSkill_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void LateUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_BombardaSkill_State> Create();

private:
	enum class PHASE : uint8_t
	{
		CAST,
		RELEASE,
		RECOVERY
	};

	void UpdateCastEffect(CPlayer& player);
	void StartCastEffect(CPlayer& player);
	void StopCastEffect();
	_bool FireProjectile(CPlayer& player);
	void EnsureCastParticleCommandsLoaded();
	_bool TryGetWandWorld(const CPlayer& player, _float4x4& OutWorld) const;
	_bool ResolveTargetPosition(
		const CPlayer& player,
		const _float3& vStartPosition,
		_float3& OutTargetPosition) const;
	void EmitCastParticleCurve() const;
	void EmitCastEnergyTrail(CPlayer& player, const _float3& vWandPosition);

protected:
	void Free() override;

private:
	PHASE m_ePhase{ PHASE::CAST };
	_float m_fAnimRatio{};
	_bool m_bCastingEffectCueReached{};
	_bool m_bReleaseEffectCueReached{};

	_bool m_bCastActive{};
	_bool m_bTrailRegistrationFailureLogged{};
	EFFECT_INSTANCE_ID m_iCastEffectID{ INVALID_EFFECT_INSTANCE_ID };
	std::vector<SPAWN_COMMAND> m_CastParticleCommands{};
	std::array<_float3, 4> m_CastTrailControlPoints{};
	_float m_fCastParticleSpacing{ 0.12f };

	static constexpr _float CASTING_EFFECT_RATIO = 0.08f;
	static constexpr _float RELEASE_EFFECT_RATIO = 0.28f;
	static constexpr _float RELEASE_TO_RECOVERY_RATIO = 0.38f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.52f;
};

NS_END
