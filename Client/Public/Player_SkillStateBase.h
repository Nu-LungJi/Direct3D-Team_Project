#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;
class CPlayer_StateMachine;

class CPlayer_SkillStateBase : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_SkillStateBase, CState)

	static _bool HasValidTarget(const CPlayer& player);

protected:
	CPlayer_SkillStateBase() = default;
	~CPlayer_SkillStateBase() override = default;

protected:
	CPlayer* GetPlayer(CStateMachine* pStateMachine) const;
	CPlayer_StateMachine* GetPlayerStateMachine(CStateMachine* pStateMachine) const;

	void SetSkillControl(CPlayer& player, _bool bMovementLocked, _bool bRootMotionTranslation, _bool bRootMotionRotation, _bool bClearMoveIntent = true) const;
	void ResetSkillControl(CPlayer& player) const;
	_bool RequestLocomotion(CStateMachine* pStateMachine) const;
	_bool HasTarget(const CPlayer& player) const;
	_bool TryApplySkillToTarget(CPlayer& player, PLAYER_SKILL_TYPE eSkillType) const;
	_bool PlayRandomTargetAttack(CPlayer& player, _float fBlendDuration = 0.24f);

	int32_t FindAnimationIndex(const CPlayer& player, _string_view sAnimationName) const;

private:
	enum class ATTACK_DIRECTION : uint8_t
	{
		FWD,
		LFT_45,
		LFT_90,
		LFT_135,
		LFT_180,
		RHT_45,
		RHT_90,
		RHT_135,
		RHT_180,
		END
	};

	void CacheDirectionalAttackAnimations(const CPlayer& player);
	ATTACK_DIRECTION ResolveTargetAttackDirection(const CPlayer& player) const;

	static constexpr _float TARGET_MAX_DISTANCE = 40.f;
	static constexpr _float TARGET_FRONT_DOT_THRESHOLD = 0.5f;
	static constexpr size_t ATTACK_DIRECTION_COUNT =
		static_cast<size_t>(ATTACK_DIRECTION::END);

	std::array<int32_t, ATTACK_DIRECTION_COUNT> m_DirectionalLightAnimations{};
	std::array<int32_t, ATTACK_DIRECTION_COUNT> m_DirectionalHeavyAnimations{};
	_bool m_bDirectionalAttackAnimationsCached = false;
};

NS_END
