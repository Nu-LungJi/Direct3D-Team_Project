#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

// [LSY] 봄바르다 애니메이션의 단계와 Cue 발동 여부만 관리한다.
// [LSY] 실제 연출 수명과 투사체 로직은 CPlayer_BombardaController가 담당한다.
class CPlayer_BombardaSkill_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_BombardaSkill_State, CPlayer_SkillStateBase)

private:
	CPlayer_BombardaSkill_State() = default;
	~CPlayer_BombardaSkill_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_BombardaSkill_State> Create();

private:
	enum class PHASE : uint8_t
	{
		CAST,
		RELEASE,
		RECOVERY
	};

	PHASE m_ePhase{ PHASE::CAST };
	_float m_fAnimRatio{};
	_bool m_bCastingEffectCueReached{};
	_bool m_bReleaseEffectCueReached{};

	// [LSY] 애니메이션 교체 시 아래 비율만 다시 맞추면 연출 코드는 유지된다.
	static constexpr _float CASTING_EFFECT_RATIO = 0.08f;
	static constexpr _float RELEASE_EFFECT_RATIO = 0.28f;
	static constexpr _float RELEASE_TO_RECOVERY_RATIO = 0.38f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.52f;
};

NS_END
