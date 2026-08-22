#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_AvadaKedavraSkill_State final
	: public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_AvadaKedavraSkill_State, CPlayer_SkillStateBase)

private:
	CPlayer_AvadaKedavraSkill_State() = default;
	~CPlayer_AvadaKedavraSkill_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void LateUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_AvadaKedavraSkill_State> Create();

private:
	enum class PHASE : uint8_t
	{
		CAST_BEGIN,
		RELEASE,
		RECOVERY
	};

	void CacheAnimationIndices(const CPlayer& player);
	void UpdateSpellEffects(CPlayer& player, _float fTimeDelta);
	void StartCastEffect(CPlayer& player);
	void StopCastEffect(CPlayer* pPlayer);
	void StopBeamEffect(CPlayer* pPlayer);
	_bool ReleaseSpell(CPlayer& player);
	_bool TryGetWandWorld(const CPlayer& player, _float4x4& OutWorld) const;
	_bool ResolveTargetPosition(
		const CPlayer& player,
		const _float3& vStartPosition,
		_float3& OutTargetPosition) const;
	_float3 CalculateVisualTargetPosition(
		const _float3& vStartPosition,
		const _float3& vTargetPosition) const;
	void EmitCastTrail(CPlayer& player, const _float3& vWandPosition);
	void UpdateBeamClothWind(CPlayer& player);
	void ClearBeamClothWind(CPlayer* pPlayer);
	void PlayImpactEffects(CPlayer& player, const _float3& vImpactPosition) const;
	void PlayImpactArcs(
		const _float4x4& impactWorld,
		const _float3& vImpactPosition) const;

protected:
	void Free() override;

private:
	int32_t m_iCastAnimation{ -1 };
	_bool m_bAnimationsCached{};
	PHASE m_ePhase{ PHASE::CAST_BEGIN };
	_float m_fAnimRatio{};
	_bool m_bCinematicStarted{};

	CHandle m_hOwner{};
	_bool m_bCastActive{};
	_bool m_bImpactPending{};
	_bool m_bTrailRegistrationFailureLogged{};
	_float m_fImpactDelayRemaining{};
	_float3 m_vPendingImpactPosition{};
	_float3 m_vBeamClothWindVelocity{};
	EFFECT_INSTANCE_ID m_iCastEffectID{ INVALID_EFFECT_INSTANCE_ID };
	EFFECT_INSTANCE_ID m_iBeamEffectID{ INVALID_EFFECT_INSTANCE_ID };

	static constexpr _float CAST_BLEND_DURATION = 0.16f;
	static constexpr _float RELEASE_RATIO = 0.18f;
	static constexpr _float RECOVERY_RATIO = 0.76f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.95f;
	static constexpr _float IMPACT_DELAY = 0.05f;
	static constexpr _float CLOTH_WIND_SPEED = 24.f;
	static constexpr _float CLOTH_WIND_REFRESH_DURATION = 0.12f;
};

NS_END
